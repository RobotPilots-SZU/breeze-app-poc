#include <app/sensors/rc.hpp>
#include <drivers/remote/rc_common.h>
#include <drivers/remote/remote.h>
#include <ramenx/async.hpp>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <optional>

namespace app::sensors
{
namespace
{
// TODO: 放进 KConfig
constexpr size_t FRAME_SIZE = 18;
constexpr size_t RX_CHUNK_SIZE = 36;
constexpr size_t RX_CHUNK_COUNT = 8;
constexpr uint32_t FRAME_GAP_MS = 5;

struct RxChunk
{
	uint32_t Sequence;
	uint8_t Length;
	bool GapBefore;
	bool Reset;
	uint8_t Data[RX_CHUNK_SIZE];
};

rc_sensor_t* GetSensor(const device* remote)
{
	assert(remote != nullptr && device_is_ready(remote));
	auto* sensor = remote_get_sensor(remote);
	assert(sensor != nullptr);
	return sensor;
}

const rc_parser_ops_t* GetParser(const device* remote)
{
	auto* parser = remote_get_parser_ops(remote);
	assert(parser != nullptr && parser->parse != nullptr && parser->frame_size == FRAME_SIZE);
	return parser;
}

} // namespace

struct RC::Impl
{
	using RxMessageQueue = ramen::MsgQueue<RxChunk, RX_CHUNK_COUNT, ramen::OverflowPolicy::DropOldest>;
	static constexpr float MAX_STICK_ANGLE = 90.0F;

	rc_sensor_t* Sensor;

	const rc_parser_ops_t* Parser;

	RxMessageQueue RxQueue;

	uint32_t NextSequence = 0;
	uint32_t ExpectedSequence = 0;

	uint32_t LastRxTime = k_uptime_get_32();

	uint8_t Frame[FRAME_SIZE]{};
	size_t FrameIndex = 0;

	static void OnRx(const device* remote, remote_rx_event_t event, const uint8_t* data, size_t len, void* userData)
	{
		ARG_UNUSED(remote);
		auto& self = *static_cast<Impl*>(userData);

		if (event == REMOTE_RX_RESET)
		{
			self.RxQueue.in(
			    RxChunk{
			        .Sequence = self.NextSequence++,
			        .Reset = true,
			    }
			);
			return;
		}
		if (len == 0)
		{
			return;
		}
		assert(event == REMOTE_RX_DATA && data != nullptr && len <= RX_CHUNK_SIZE);

		const uint32_t now = k_uptime_get_32();
		RxChunk chunk{
		    .Sequence = self.NextSequence++,
		    .Length = static_cast<uint8_t>(len),
		    .GapBefore = (now - self.LastRxTime) >= FRAME_GAP_MS,
		};
		self.LastRxTime = now;
		std::memcpy(chunk.Data, data, len);
		self.RxQueue.in(chunk);
	}

	void HandleFrame()
	{
		if (Parser->frame_check != nullptr && Parser->frame_check(Frame, Parser->frame_size) != RC_FRAME_OK)
		{
			return;
		}

		Parser->parse(Frame, Sensor->info);
		rc_sensor_check(Sensor);
		rc_interrupt_update(Sensor);
		Sensor->is_online = true;
	}

	/**
	 * @brief 根据右摇杆值获取姿态指令.
	 * @note 目前采用位移控制, 未进行积分
	 */
	breeze::models::Posture Posture() const
	{
		constexpr float CHANNEL_SCALE = MAX_STICK_ANGLE / 660.0F;
		const auto& info = *Sensor->info;
		return {
		    .Pitch = {static_cast<float>(info.ch1) * CHANNEL_SCALE},
		    .Roll = {0.0F},
		    .Yaw = {static_cast<float>(-info.ch0) * CHANNEL_SCALE},
		};
	}

	void DriverTick()
	{
		std::optional<RxChunk> next;
		RxQueue.out(next);
		while (next.has_value())
		{
			const RxChunk& chunk = *next;
			if (chunk.Sequence != ExpectedSequence || chunk.GapBefore || chunk.Reset)
			{
				FrameIndex = 0;
			}
			ExpectedSequence = chunk.Sequence + 1;
			if (!chunk.Reset)
			{
				for (size_t i = 0; i < chunk.Length; ++i)
				{
					Frame[FrameIndex++] = chunk.Data[i];
					if (FrameIndex == Parser->frame_size)
					{
						HandleFrame();
						FrameIndex = 0;
					}
				}
			}
			RxQueue.out(next);
		}

		if (Sensor->is_online)
		{
			rc_keyboard_update(Sensor->info);
		}
	}
	void Tick(RC& rc)
	{
		DriverTick();
		if (Sensor->is_online)
		{
			rc.OutPosture(Posture());
		}
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
RC::RC(const device* remote)
    : InTick([this]() { _impl.Tick(*this); }),
      OutOnline([this](auto& out) { out = _impl.Sensor->is_online; }),
      _impl(*new (_storage) Impl{
          .Sensor = GetSensor(remote),
          .Parser = GetParser(remote),
      })
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	const int result = remote_set_rx_cb(remote, Impl::OnRx, &_impl);
	assert(result == 0);
}

} // namespace app::sensors
