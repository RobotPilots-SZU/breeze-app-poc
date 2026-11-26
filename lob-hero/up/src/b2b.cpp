#include <app/b2b.hpp>
#include <app/b2b/msg.hpp>
#include <app/b2b/msgconvert.hpp>
#include <app/models/priority.hpp>
#include <breeze/b2b/proto.hpp>
#include <breeze/b2b/transmit.hpp>
#include <breeze/models/utils.hpp>
#include <etl/array.h>
#include <etl/span.h>
#include <memory>
#include <new>
#include <ramenx/async.hpp>
#include <ramenx/time.hpp>

namespace
{

using namespace app::b2b;

struct CastHandler
{
	Output& Out;

	void Handle(const MotorControl& in)
	{
		if (std::ranges::find(app::models::MOTOR_IDS, in.Id) == app::models::MOTOR_IDS.end())
		{
			return;
		}
		if (auto* p = Out.Motors.At(in.Id); p != nullptr)
		{
			(*p)(FromProto(in));
		}
	}
};

struct B2BCast
{
	ramen::ThreadLift<etl::span<const std::byte>, 512> Lift{
	    ramen::DispatcherConfig{.priority = app::models::priority::B2B_CAST_TRANS, .name = "b2b_lift"}
	};
	ramen::FrameProvider<1024> Collector;
	breeze::b2b::Transmitter Transmitter;
	etl::array<std::byte, 512> TransmitterBuffer;

	CastHandler Handler;
	Input& In;
	breeze::b2b::Encoder<CastMessages> Encoder;
	breeze::b2b::Decoder<CastMessages> Decoder;

	ramen::Pushable<etl::span<const std::byte>> InHandleRx = [this](const auto data) { Decoder.Decode(data, Handler); };
	ramen::Pushable<> InCollect = [this] { Collect(); };

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	B2BCast(const device* dev, Output& out, Input& in, k_timeout_t collectPeriod)
	    : Collector({
	          .period = collectPeriod,
	          .priority = app::models::priority::B2B_CAST_COLLECT,
	          .name = "b2b_collect",
	      }),
	      Transmitter(dev, {.SelfId = UP_ADDR.B2B, .TargetId = DOWN_ADDR.B2B, .Buffer = TransmitterBuffer}),
	      Handler{out},
	      In{in}
	{
		Lift.in_wait >> Transmitter.OutReceiveWait;
		Lift.out >> InHandleRx;
		Collector.out >> InCollect;
	}

	void Start()
	{
		Lift.start();
		Collector.start();
		Transmitter.Start();
	}

	void Collect()
	{
		for (const auto id : app::models::UP_MOTORS)
		{
			if (auto* puller = In.Motors.At(id); puller != nullptr)
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
				breeze::models::MotorReport report;
				(*puller)(report);
				const auto message = ToProto(report, id);
				Transmitter.InSend(Encoder.Encode(message, TransmitterBuffer));
			}
		}
		for (const auto id : app::models::UP_IMUS)
		{
			if (auto* puller = In.Imus.At(id); puller != nullptr)
			{
				// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
				breeze::models::ImuAngularReport report;
				(*puller)(report);
				if (report != decltype(report){})
				{
					const auto message = ToProto(report, id);
					Transmitter.InSend(Encoder.Encode(message, TransmitterBuffer));
				}
			}
		}
	}
};

} // namespace

namespace app::b2b
{

// 内部实现分层. 分为B2B Cast 和 RP-Proto
struct UpModule::Impl
{
	B2BCast Cast;
	Impl(const device* dev, Output& out, Input& in, k_timeout_t collectPeriod)
	    : Cast(dev, out, in, collectPeriod)
	{}
	void Start() { Cast.Start(); }
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
UpModule::UpModule(const device* dev, k_timeout_t collectPeriod)
    : In{
          .Motors = MotorReportPullers{[]<typename TId>(TId) { return ramen::Puller<breeze::models::MotorReport>{}; }},
          .Imus = ImuReportPullers{[]<typename TId>(TId) { return ramen::Puller<breeze::models::ImuAngularReport>{}; }},
      }
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	std::construct_at(P(), dev, Out, In, collectPeriod);
}
void UpModule::Start() { P()->Start(); }

UpModule::Impl* UpModule::P() { return std::launder(reinterpret_cast<Impl*>(&_storage)); }

} // namespace app::b2b
