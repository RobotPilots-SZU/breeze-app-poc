#pragma once

#include <app/models/device.hpp>
#include <breeze/models/device.hpp>
#include <breeze/models/utils.hpp>
#include <cstddef>
#include <ramen.hpp>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

namespace app::b2b
{

using MotorControlPushers = breeze::models::IndexedEnumArray<
    models::MotorId,
    ramen::Pusher<breeze::models::MotorControl>,
    models::UP_MOTORS,
    models::MOTOR_IDS
>;
using MotorReportPullers = breeze::models::
    IndexedEnumArray<models::MotorId, ramen::Puller<breeze::models::MotorReport>, models::UP_MOTORS, models::MOTOR_IDS>;
using ImuReportPullers = breeze::models::
    IndexedEnumArray<models::ImuId, ramen::Puller<breeze::models::ImuAngularReport>, models::UP_IMUS, models::IMU_IDS>;

/**
 * @brief UP板B2B模块输出的端口
 *
 */
struct Output
{
	/**
	 * @brief 来自DOWN板的电机控制
	 *
	 */
	MotorControlPushers Motors;
};

/**
 * @brief 输入到UP板B2B模块的端口
 *
 */
struct Input
{
	MotorReportPullers Motors;
	/**
	 * @brief UP板IMU姿态数据
	 *
	 */
	ImuReportPullers Imus;
};

/**
 * @brief UP板的B2B通信模块.
 *
 */
class UpModule
{
public:
	Output Out;
	Input In;
	UpModule(const device* dev, k_timeout_t collectPeriod);
	void Start();

private:
	constexpr static std::size_t IMPL_SIZE = 4608;
	alignas(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE) std::byte _storage[IMPL_SIZE];
	struct Impl;
	Impl* P();
};
} // namespace app::b2b
