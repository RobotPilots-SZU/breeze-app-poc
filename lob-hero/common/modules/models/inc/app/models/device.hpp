/**
 * @file device.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 设备枚举和各种专属于吊射英雄的设备建模
 * @version 0.1
 * @date 2026-11-21
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <cstdint>

namespace app::models
{

/**
 * @brief 电机ID (类型) 集合
 *
 */
enum class MotorId : std::uint8_t
{
	Unspecified,
	GimbalYaw,         ///< 云台Yaw电机, 装在底盘上
	HeadPitch,         ///< 控头的Pitch电机
	ForwardLeftWheel,  ///< 底盘左前轮毂电机
	ForwardRightWheel, ///< 底盘右前轮毂电机
	BackwardLeftWheel, ///< 底盘左后轮毂电机
	BackwardRightWheel ///< 底盘右后轮毂电机
};

constexpr std::array MOTOR_IDS = []() -> auto {
	using enum MotorId;
	return std::array{Unspecified,       GimbalYaw,         HeadPitch,         ForwardLeftWheel,
	                  ForwardRightWheel, BackwardLeftWheel, BackwardRightWheel};
}();
constexpr std::array UP_MOTORS = []() {
	using enum models::MotorId;
	return std::array{HeadPitch};
}();
constexpr std::array DOWN_MOTORS = []() {
	using enum models::MotorId;
	return std::array{GimbalYaw, ForwardLeftWheel, ForwardRightWheel, BackwardLeftWheel, BackwardRightWheel};
}();

/**
 * @brief Imu 集合
 *
 */
enum class ImuId : std::uint8_t
{
	Unspecified,
	Head,   ///< 头上imu
	Chassis ///< 底盘imu
};
constexpr std::array IMU_IDS = []() {
	using enum ImuId;
	return std::array{Unspecified, Head, Chassis};
}();
constexpr std::array UP_IMUS = []() {
	using enum ImuId;
	return std::array{Head};
}();
constexpr std::array DOWN_IMUS = []() {
	using enum ImuId;
	return std::array{Chassis};
}();

} // namespace app::models
