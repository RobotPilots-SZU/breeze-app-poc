#include "app/motors/module.hpp"
#include <cassert>
#include <cmath>
#include <drivers/bldcm/bldcm.h>
#include <drivers/bldcm/bldcm_dm.h>
#include <new>

namespace app::motors
{
namespace
{
constexpr float RAD_TO_DEG = 57.29577951308232F;
constexpr float RAD_PER_SEC_TO_RPM = 9.54929658551372F;
} // namespace

struct DmModule::Impl
{
	const device* Dev;
	breeze::models::MotorReport Report{};
	bool Online = false;

	explicit Impl(const device* dev)
	    : Dev(dev)
	{}

	void Drive(const breeze::models::MotorControl& in) const
	{
		const auto [torque] = in.Torque;
		(void)dm_mit_control(Dev, 0.0F, 0.0F, 0.0F, 0.0F, torque);
	}

	void Tick()
	{
		Online = get_motor_heartbeat_status(Dev) > 0;
		if (!Online)
		{
			Report = {};
			return;
		}
		const auto* rx = get_motor_rxdata(Dev);
		if (rx == nullptr || !motor_rx_has(rx, MOTOR_DM))
		{
			Report = {};
			return;
		}

		Report.Encoder.Encode = static_cast<std::uint16_t>(rx->encoder);
		Report.Angle = {rx->specific_data.dm.pos_real * RAD_TO_DEG};
		Report.Speed = {rx->specific_data.dm.vel_real * RAD_PER_SEC_TO_RPM};
		Report.Temperature = {rx->specific_data.dm.motor_temp};
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
DmModule::DmModule(const device* dev)
    : InControl([this](const auto& in) { P()->Drive(in); }),
      InTick([this] { P()->Tick(); }),
      OutReport([this](auto& out) { out = P()->Report; }),
      OutOnline([this](auto& out) { out = P()->Online; })
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	static_assert(alignof(Impl) <= alignof(DmModule));

	assert(dev != nullptr && device_is_ready(dev));

	std::construct_at(P(), dev);

	const int ret = register_motor(dev);
	assert(ret == 0);
}

DmModule::~DmModule() { P()->~Impl(); }

DmModule::Impl* DmModule::P() { return std::launder(reinterpret_cast<Impl*>(_storage)); }

} // namespace app::motors
