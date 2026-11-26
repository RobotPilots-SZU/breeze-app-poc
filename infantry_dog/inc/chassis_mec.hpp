/**
 * @file chassis_mec.hpp
 * @author River-zzZ (silver250928@outlook.com)
 * @brief 全向轮底盘模块相关配置定义
 * @version 0.1.0
 * @date 2026-11-24
 *
 * @copyright Copyright (c) 2026 RobotPilots-SZU
 *
 */
#pragma once

#include "chassis.hpp"
#include "motor.hpp"
#include <array>
#include <cstdint>

namespace breeze::chassis::mec
{

/**
 * @brief 全向轮底盘电机位置
 *
 */
enum class MotorPosition : std::uint8_t
{
	Unknown = 0,

	LeftFront,   // 左前
	LeftBehind,  // 左后
	RightFront,  // 右前
	RightBehind, // 右后
};

/**
 * @brief 全向轮底盘的电机信息
 *
 */
struct MotorConfig
{
	MotorPosition Position;                    // 位置
	breeze::chassis::motor::Role::Wheel Motor; // 电机信息
};

/**
 * @brief 电机力矩
 *
 */
struct MotorTorque
{
	float Torque;
};

/**
 * @brief 底盘电机输出
 *
 */
struct MotorOutput
{
	MotorPosition Position;
	MotorTorque Torque;
};

/**
 * @brief 传入电机配置信息
 *
 * @param motor 电机配置信息
 */
void SetMotorConfig(std::array<MotorConfig, 4> motor);

/**
 * @brief 全向轮解算
 *
 * @param target 目标值
 */
std::array<MotorOutput, 4> Solve(breeze::chassis::Control::Target& target);

} // namespace breeze::chassis::mec
