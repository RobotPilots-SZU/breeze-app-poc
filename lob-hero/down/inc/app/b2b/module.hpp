#pragma once

#include <app/b2b/msg.hpp>
#include <app/models/device.hpp>
#include <breeze/models/device.hpp>
#include <breeze/models/utils.hpp>
#include <cstddef>
#include <ramen.hpp>
#include <zephyr/device.h>

namespace app::b2b
{

using MotorControlPushables = breeze::models::IndexedEnumArray<
    models::MotorId,
    ramen::Pushable<breeze::models::MotorControl>,
    models::UP_MOTORS,
    models::MOTOR_IDS
>;
using MotorReportPushers = breeze::models::
    IndexedEnumArray<models::MotorId, ramen::Pusher<breeze::models::MotorReport>, models::UP_MOTORS, models::MOTOR_IDS>;

using ImuReportPullables = breeze::models::IndexedEnumArray<
    models::ImuId,
    ramen::Pullable<breeze::models::ImuAngularReport>,
    models::UP_IMUS,
    models::IMU_IDS
>;

/**
 * @brief 从模块输出的端口 (来自于UP)
 *
 */
struct Output
{
	MotorReportPushers Motors;
	ImuReportPullables Imus;
};

/**
 * @brief 输入到模块的端口 (发送到UP)
 *
 */
struct Input
{
	/**
	 * @brief 电机控制
	 *
	 */
	MotorControlPushables Motors;
};

/**
 * @brief 下板的B2B通信模块.
 *
 */
class DownModule
{
public:
	Output Out;
	Input In;
	DownModule(const device* dev);
	void Start();

private:
	constexpr static std::size_t IMPL_SIZE = 4096;
	alignas(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE) std::byte _storage[IMPL_SIZE];
	struct Impl;
	Impl* P();
};
} // namespace app::b2b
