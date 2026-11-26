/**
 * @file motor.hpp
 * @author River-zzZ (silver250928@outlook.com)
 * @brief 底盘模块电机相关配置定义
 * @version 0.1.0
 * @date 2026-11-24
 *
 * @copyright Copyright (c) 2026 RobotPilots-SZU
 *
 */
#pragma once

#include "chassis_pid.hpp"
#include <cstdint>

namespace breeze::chassis::motor
{

/**
 * @brief 电机转动正方向
 *
 */
enum class Direction : std::int8_t
{
	Unknown = 0,

	Clockwise = -1,       // 顺时针
	CounterClockwise = 1, // 逆时针
};

/**
 * @brief 电机作用类型
 *
 */
class Role
{
public:
	/**
	 * @brief 轮毂电机
	 *
	 */
	struct Wheel
	{
		breeze::chassis::motor::Direction Direction; // 电机转动正方向
		breeze::chassis::pid::Config MecPid;         // 机械模式pid参数
		breeze::chassis::pid::Config ImuPid;         // 陀螺仪模式pid参数
	};

	/**
	 * @brief 舵向电机
	 *
	 */
	struct Steering
	{
		breeze::chassis::motor::Direction Direction; // 电机转动正方向
		// 速度环
		breeze::chassis::pid::Config SpeedMecPid; // 速度环机械模式pid参数
		breeze::chassis::pid::Config SpeedImuPid; // 速度环陀螺仪模式pid参数
		// 角度环
		breeze::chassis::pid::Config AngleMecPid; // 角度环机械模式pid参数
		breeze::chassis::pid::Config AngleImuPid; // 角度环陀螺仪模式pid参数
	};

	/**
	 * @brief 关节电机
	 *
	 */
	struct Joint
	{
		breeze::chassis::motor::Direction Direction; // 电机转动正方向
		// 速度环
		breeze::chassis::pid::Config SpeedMecPid; // 速度环机械模式pid参数
		breeze::chassis::pid::Config SpeedImuPid; // 速度环陀螺仪模式pid参数
		// 角度环
		breeze::chassis::pid::Config AngleMecPid; // 角度环机械模式pid参数
		breeze::chassis::pid::Config AngleImuPid; // 角度环陀螺仪模式pid参数
	};
};


} // namespace breeze::chassis::motor
