/**
 * @file pid.hpp
 * @author River-zzZ (silver250928@outlook.com)
 * @brief 底盘电机pid模板
 * @version 0.1
 * @date 2026-10-30
 *
 * @copyright Copyright (c) 2026 RobotPilots-SZU
 *
 */
#pragma once

namespace breeze::chassis::pid
{

/**
 * @brief pid配置
 * @note 这是一个没有完善的结构体
 *
 */
struct Config
{
	float Kp; // 比例系数
	float Ki; // 积分系数
	float Kd; // 微分系数

	float InputDeadband;      // 输入死区
	float MaxOutput;          // 最大输出
	float MaxIntegral;        // 最大积分
	float IntegralSeparation; // 积分分离值
};

} // namespace breeze::chassis::pid
