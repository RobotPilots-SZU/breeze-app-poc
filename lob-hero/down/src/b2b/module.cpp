#include <app/b2b/module.hpp>
#include <app/b2b/msg.hpp>
#include <app/b2b/msgconvert.hpp>
#include <app/models/device.hpp>
#include <app/models/priority.hpp>
#include <breeze/b2b/proto.hpp>
#include <breeze/b2b/transmit.hpp>
#include <breeze/models/device.hpp>
#include <breeze/models/utils.hpp>
#include <etl/array.h>
#include <etl/span.h>
#include <memory>
#include <new>
#include <ramen.hpp>
#include <ramenx/async.hpp>

using namespace app::models;

namespace
{

using namespace app::b2b;

struct CastHandler
{
	Output& Out;
	breeze::models::IndexedEnumArray<ImuId, breeze::models::ImuAngularReport, UP_IMUS, IMU_IDS> ImuCache;

	void Handle(const MotorReport& in)
	{
		if (auto* p = Out.Motors.At(in.Id); p != nullptr)
		{
			(*p)(FromProto(in));
		}
	}
	void Handle(const ImuReport& in)
	{
		if (auto* p = ImuCache.At(in.Id); p != nullptr)
		{
			*p = FromProto(in);
		}
	}
};

/**
 * @brief B2B Cast 协议实现
 *
 */
struct B2BCast
{

	ramen::ThreadLift<etl::span<const std::byte>, 512> Lift{
	    ramen::DispatcherConfig{.priority = priority::B2B_CAST_TRANS, .name = "b2b_lift"}
	};
	breeze::b2b::Transmitter Transmitter;
	etl::array<std::byte, 512> TransmitterBuffer;

	CastHandler Handler;
	breeze::b2b::Encoder<CastMessages> Encoder;
	breeze::b2b::Decoder<CastMessages> Decoder;

	ramen::Pushable<etl::span<const std::byte>> InHandleRx = [this](const auto data) { Decoder.Decode(data, Handler); };

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	B2BCast(const device* dev, Output& out)
	    : Transmitter(dev, {.SelfId = DOWN_ADDR.B2B, .TargetId = UP_ADDR.B2B, .Buffer = TransmitterBuffer}),
	      Handler{out}
	{
		Lift.in_wait >> Transmitter.OutReceiveWait;
		Lift.out >> InHandleRx;
	}

	void Send(MotorId id, const breeze::models::MotorControl& control)
	{
		const auto message = ToProto(control, id);
		Transmitter.InSend(Encoder.Encode(message, TransmitterBuffer));
	}

	void Start()
	{
		Lift.start();
		Transmitter.Start();
	}
};

} // namespace

namespace app::b2b
{

struct DownModule::Impl
{
	B2BCast Cast;
	Impl(const device* dev, Output& out)
	    : Cast(dev, out)
	{}
	void Start() { Cast.Start(); }
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
DownModule::DownModule(const device* dev)
    : Out{.Imus = ImuReportPullables{[this]<typename TId>(TId) {
	          return ramen::Pullable<breeze::models::ImuAngularReport>{[this](auto& out) {
		          out = P()->Cast.Handler.ImuCache[TId::value];
	          }};
          }}},
      In{.Motors = MotorControlPushables{[this]<typename TId>(TId) {
	         return ramen::Pushable<breeze::models::MotorControl>{[this](const auto& control) {
		         P()->Cast.Send(TId::value, control);
	         }};
         }}}
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	std::construct_at(P(), dev, Out);
}

DownModule::Impl* DownModule::P() { return std::launder(reinterpret_cast<Impl*>(&_storage)); }

void DownModule::Start() { P()->Start(); }

} // namespace app::b2b
