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
enum MotorId : std::uint8_t
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
} // namespace app::models
