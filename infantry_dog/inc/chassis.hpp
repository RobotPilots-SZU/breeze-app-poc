/**
 * @file chassis.hpp
 * @author River-zzZ (silver250928@outlook.com)
 * @brief 底盘模块相关配置定义
 * @version 0.1.0
 * @date 2026-11-24
 *
 * @copyright Copyright (c) 2026 RobotPilots-SZU
 *
 */
#pragma once

#include <cstdint>

namespace breeze::chassis
{

class Config
{
public:
	/**
	 * @brief 底盘各种电机数量
	 *
	 */
	struct MotorNumber
	{
		std::uint8_t Wheel;
		std::uint8_t Steering;
		std::uint8_t Joint;
	};

	/**
	 * @brief 小陀螺动作信息结构体
	 *
	 */
	struct Cycle
	{
		float Target;   // 小陀螺目标速度 m/s（逆时针为正）
		float MaxSpeed; // 小陀螺最大速度
	};
};



class Control
{
public:
	/**
	 * @brief 底盘控制模式
	 *
	 */
	enum class Mode : std::uint8_t
	{
		Uknown = 0,

		Sleep,      // 卸力
		Mechanical, // 机械模式
		Gyro,       // 陀螺仪模式
		Cycle,      // 小陀螺
	};

	/**
	 * @brief 底盘运动目标
	 *
	 */
	struct Target
	{
		float Vx;    // x方向速度（向右为正）
		float Vy;    // y方向速度（向前为正）
		float Cycle; // 小陀螺旋转速度（逆时针为正）
	};
};



class Output
{
	/**
	 * @brief 当前速度
	 *
	 */
	struct CurrentSpeed
	{
		float Vx;    // X方向
		float Vy;    // Y方向
		float Cycle; // 小陀螺旋转速度
	};
};


} // namespace breeze::chassis
