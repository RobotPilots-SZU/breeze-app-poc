#include <app/motors/module.hpp>
#include <cassert>
#include <cmath>
#include <drivers/bldcm/bldcm.h>
#include <drivers/bldcm/bldcm_lk.h>
#include <new>

namespace app::motors
{
namespace
{
constexpr float DEGREES_PER_REVOLUTION = 360.0F;
constexpr float RPM_PER_DEGREE_PER_SECOND = 1.0F / 6.0F;
constexpr float KT_IQ_LIMIT = 1500.0F;
} // namespace

struct LkModule::Impl
{
	const device* Dev;
	const LkConfig Conf;
	breeze::models::MotorReport Report{};
	bool Online = false;

	Impl(const device* dev, LkConfig conf)
	    : Dev(dev),
	      Conf(conf)
	{}

	void Drive(const breeze::models::MotorControl& in) const
	{
		const auto [torque] = in.Torque;
		assert(std::isfinite(torque));
		const float scaled = std::clamp(torque * Conf.IqCommandPerNewtonMeter, -KT_IQ_LIMIT, KT_IQ_LIMIT);
		const auto iq = static_cast<std::int16_t>(std::lround(scaled));
		(void)single_closedloop_control(Dev, iq);
	}

	void Tick()
	{
		Online = get_motor_heartbeat_status(Dev) > 0;
		if (!Online || Conf.EncoderCounts == 0 || Conf.TransmissionRatio == 0)
		{
			Report = {};
			return;
		}
		const auto* rx = get_motor_rxdata(Dev);
		if (rx == nullptr)
		{
			Report = {};
			return;
		}
		if (motor_rx_has(rx, MOTOR_RX_VALID_ENCODER))
		{
			Report.Encoder.Encode = static_cast<std::uint16_t>(rx->encoder);
			Report.Angle = {
			    static_cast<float>(rx->encoder) * DEGREES_PER_REVOLUTION /
			    (static_cast<float>(Conf.EncoderCounts) * Conf.TransmissionRatio)
			};
		}
		if (motor_rx_has(rx, MOTOR_RX_VALID_SPEED))
		{
			Report.Speed = {static_cast<float>(rx->speed) * RPM_PER_DEGREE_PER_SECOND / Conf.TransmissionRatio};
		}
		if (motor_rx_has(rx, MOTOR_LK))
		{
			// NOLINTNEXTLINE(bugprone-signed-char-misuse)
			Report.Temperature = {static_cast<std::int16_t>(rx->specific_data.lk.temp)};
		}
	}
}; // namespace app::motors

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
LkModule::LkModule(const device* dev, LkConfig conf)
    : InControl([this](const auto& in) { P()->Drive(in); }),
      InTick([this] { P()->Tick(); }),
      OutReport([this](auto& out) { out = P()->Report; }),
      OutOnline([this](auto& out) { out = P()->Online; })
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	static_assert(alignof(Impl) <= alignof(LkModule));
	std::construct_at(P(), dev, conf);
	assert(dev != nullptr && device_is_ready(dev));
	assert(conf.EncoderCounts != 0 && conf.TransmissionRatio != 0);
	assert(std::isfinite(conf.IqCommandPerNewtonMeter) && conf.IqCommandPerNewtonMeter > 0.0F);
	const int ret = register_motor(dev);
	assert(ret == 0);
}

LkModule::~LkModule() { P()->~Impl(); }

LkModule::Impl* LkModule::P() { return std::launder(reinterpret_cast<Impl*>(_storage)); }

} // namespace app::motors
