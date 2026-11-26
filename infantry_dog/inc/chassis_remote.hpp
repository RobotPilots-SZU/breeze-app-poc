/**
 * @file chassis_remote.hpp
 * @author River-zzZ (silver250928@outlook.com)
 * @brief 读取遥控器数据并转换
 * @version 0.2.0
 * @date 2026-11-27
 *
 * @copyright Copyright (c) 2026 RobotPilots-SZU
 *
 */
#pragma once

#include "chassis.hpp"
#include <drivers/remote/remote.h>

namespace breeze::chassis::remote
{

/**
 * @brief 每收一帧回调一次，运行在遥控器接收线程上下文
 *
 */
using FrameCallback = void (*)(const rc_sensor_t* sensor, void* userData);

/**
 * @brief 注册遥控器并开始接收
 *
 * @param callback 每帧解析完后调用，可为空（只想轮询 sensor->info 时）
 * @param userData 透传给 callback
 * @return sensor 指针；失败返回 nullptr
 */
const rc_sensor_t* Init(FrameCallback callback = nullptr, void* userData = nullptr);

/**
 * @brief 遥控器通道转底盘目标速度
 *
 * @note ch0（右摇杆左右）转 Vx，ch1（右摇杆前后）转 Vy
 *
 * @param info 遥控器数据
 * @return 底盘目标速度
 */
Control::Target TargetFrom(const rc_sensor_info_t& info);

} // namespace breeze::chassis::remote
