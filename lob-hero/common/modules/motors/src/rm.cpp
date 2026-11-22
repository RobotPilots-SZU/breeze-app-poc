#include "app/motors/module.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <drivers/bldcm/bldcm.h>
#include <memory>
#include <new>

namespace app::motors
{
namespace
{

constexpr float DEGREES_PER_REVOLUTION = 360.0F;

template <RmProfile Profile>
struct Traits;

/**
 * @brief 3508电机与C620电调参数
 * @note 参数来源:
 * https://rm-static.djicdn.com/tem/17348/RoboMaster%20C620%E6%97%A0%E5%88%B7%E7%94%B5%E6%9C%BA%E8%B0%83%E9%80%9F%E5%99%A8%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%EF%BC%88%E4%B8%AD%E8%8B%B1%E6%97%A5%EF%BC%89V1.01.pdf
 * https://bbs-web-static.robomaster.com/049ed37ac59a4dd4a2ef16876a9dae761764130946066/RoboMaster%20M3508%20%E7%9B%B4%E6%B5%81%E6%97%A0%E5%88%B7%E7%94%B5%E6%9C%BA_%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%EF%BC%88%E4%B8%AD%E8%8B%B1%EF%BC%89.pdf
 */
template <>
struct Traits<RmProfile::M3508C620>
{
	static constexpr float TORQUE_TO_CURRENT = 0.02F * 1000.0F; // 0.02 Nm/mA
	static constexpr float CURRENT_TO_DEVICE = 16384.0F / 20.0F;

	static void Tick(const smotor_receive_data_t* rx, breeze::models::MotorReport& out)
	{
		if (motor_rx_has(rx, MOTOR_DJI_3508))
		{
			out.Temperature.Value = rx->specific_data.m3508.temp;
		}
	}
};

/**
 * @brief 6020云台电机参数
 * @note 参数来源:
 * https://bbs-web-static.robomaster.com/4eb9a4d2d1cc4ce682c9f2d7005e6c291760802858143/RoboMaster%20GM6020%E7%9B%B4%E6%B5%81%E6%97%A0%E5%88%B7%E7%94%B5%E6%9C%BA%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E20231013.pdf
 */
template <>
struct Traits<RmProfile::GM6020>
{
	static constexpr float TORQUE_TO_CURRENT = 741.0F; // 741 mNm/mA
	static constexpr float CURRENT_TO_DEVICE = 16384.0F / 3.0F;

	static void Tick(const smotor_receive_data_t* rx, breeze::models::MotorReport& out)
	{
		if (motor_rx_has(rx, MOTOR_DJI_6020))
		{
			out.Temperature.Value = rx->specific_data.m6020.temp;
		}
	}
};

/**
 * @brief 2006电机参数
 * @note 参数来源:
 * https://bbs-web-static.robomaster.com/9d603c401183455eb7891344d83c2a101760806490524/RM%20M2006%20P36%E7%9B%B4%E6%B5%81%E6%97%A0%E5%88%B7%E5%87%8F%E9%80%9F%E7%94%B5%E6%9C%BA%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E.pdf
 * https://bbs-web-static.robomaster.com/c4a8093edfb9455b8f6f75e5468403081760806399241/RM%20C610%E6%97%A0%E5%88%B7%E7%94%B5%E6%9C%BA%E8%B0%83%E9%80%9F%E5%99%A8%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%20%E5%8F%91%E5%B8%83%E7%89%88.pdf
 *
 * @tparam
 */
template <>
struct Traits<RmProfile::M2006C610>
{
	static constexpr float TORQUE_TO_CURRENT = 0.18; // 0.18 Nm/A
	static constexpr float CURRENT_TO_DEVICE = 10000.0f / 10.0f;

	static void Tick(const smotor_receive_data_t* rx, breeze::models::MotorReport& out) {}
};

} // namespace

template <RmProfile Profile>
struct RmModule<Profile>::Impl
{
	const device* Dev;
	// NOLINTBEGIN(readability-identifier-naming)
	const std::uint16_t EncoderCounts;
	const std::uint8_t TransmissionRatio;
	// NOLINTEND(readability-identifier-naming)
	breeze::models::MotorReport Report{};
	bool Online = false;

	explicit Impl(const device* dev, RmConfig conf)
	    : Dev(dev),
	      EncoderCounts(conf.EncoderCounts),
	      TransmissionRatio(conf.TransmissionRatio)
	{}

	void Drive(const breeze::models::MotorControl& in) const
	{
		const auto [torque] = in.Torque;
		if (!std::isfinite(torque))
		{
			return;
		}

		const float scaled = torque * Traits<Profile>::TORQUE_TO_CURRENT * Traits<Profile>::CURRENT_TO_DEVICE;
		const float bounded = std::clamp(scaled, -16384.0F, 16384.0F);
		const auto current = static_cast<std::int16_t>(std::lround(bounded));
		(void)motor_torque_control(Dev, current);
	}

	void Tick()
	{
		Online = get_motor_heartbeat_status(Dev) > 0;
		if (!Online || EncoderCounts == 0 || TransmissionRatio == 0)
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
			Report.Angle.Value = static_cast<float>(rx->encoder) * DEGREES_PER_REVOLUTION /
			                     (static_cast<float>(EncoderCounts) * TransmissionRatio);
		}
		if (motor_rx_has(rx, MOTOR_RX_VALID_SPEED))
		{
			Report.Speed.Value = static_cast<float>(rx->speed) / TransmissionRatio;
		}
		Traits<Profile>::Tick(rx, Report);
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
template <RmProfile Profile>
RmModule<Profile>::RmModule(const device* dev, RmConfig mechanics)
    : InControl([this](const auto& in) { P()->Drive(in); }),
      InTick([this] { P()->Tick(); }),
      OutReport([this](auto& out) { out = P()->Report; }),
      OutOnline([this](auto& out) { out = P()->Online; })
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	static_assert(alignof(Impl) <= alignof(RmModule));
	std::construct_at(P(), dev, mechanics);
	assert(dev != nullptr && device_is_ready(dev));
	if (dev != nullptr && device_is_ready(dev))
	{
		const int ret = register_motor(dev);
		assert(ret == 0);
	}
}

template <RmProfile Profile>
RmModule<Profile>::~RmModule()
{
	P()->~Impl();
}

template <RmProfile Profile>
typename RmModule<Profile>::Impl* RmModule<Profile>::P()
{
	return std::launder(reinterpret_cast<Impl*>(_storage));
}

template class RmModule<RmProfile::M3508C620>;
template class RmModule<RmProfile::M2006C610>;
template class RmModule<RmProfile::GM6020>;

} // namespace app::motors
