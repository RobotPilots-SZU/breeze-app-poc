#include "app/b2b/module.hpp"
#include "breeze/b2b/proto.hpp"
#include <app/b2b/msg.hpp>
#include <app/b2b/msgconvert.hpp>
#include <app/models/priority.hpp>
#include <breeze/b2b/transmit.hpp>
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
	using MotorReportLatches =
	    breeze::models::IndexedEnumArray<MotorId, ramen::Latch<breeze::models::MotorReport>, MOTOR_IDS>;
	MotorReportLatches MotorReports;

	void Handle(const MotorControl& in) { Out.Motors[in.Id](FromProto(in)); }

	void Handle(const MotorReport& in) { MotorReports[in.Id].in(FromProto(in)); }

	CastHandler(Input& in, Output& out)
	    : Out(out)
	{
		for (const auto id : MOTOR_IDS)
		{
			in.Motors[id] >> MotorReports[id].out;
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
	B2BCast(const device* dev, Input& in, Output& out)
	    : Transmitter(dev, {.SelfId = DOWN_ADDR.B2B, .TargetId = UP_ADDR.B2B, .Buffer = TransmitterBuffer}),
	      Handler(in, out)
	{
		Lift.in_wait >> Transmitter.OutReceiveWait;
		Lift.out >> InHandleRx;
	}
};

} // namespace

namespace app::b2b
{

struct DownModule::Impl
{
	B2BCast Cast;
	Impl(const device* dev, Input& in, Output& out)
	    : Cast(dev, in, out)
	{}
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
DownModule::DownModule(const device* dev)
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	std::construct_at(P(), dev, In, Out);
}

DownModule::Impl* DownModule::P() { return std::launder(reinterpret_cast<Impl*>(&_storage)); }

} // namespace app::b2b
