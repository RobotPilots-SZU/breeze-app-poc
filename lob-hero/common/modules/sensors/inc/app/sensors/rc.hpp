/**
 * @file rc.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 遥控器(DT7)设备封装
 * @version 0.1
 * @date 2026-11-15
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <breeze/models/control.hpp>
#include <breeze/models/unit.hpp>
#include <ramen.hpp>
#include <zephyr/device.h>

namespace app::sensors
{
/**
 * @brief DT7数据源
 *
 */
class RC
{
public:
	/**
	 * @brief 线程上下文驱动
	 *
	 */
	ramen::Pushable<> InTick;
	/**
	 * @brief 设备是否在线
	 *
	 */
	ramen::Pullable<bool> OutOnline;
	/**
	 * @brief 目标云台姿态
	 *
	 */
	ramen::Pusher<breeze::models::Posture> OutPosture;
	/**
	 * @brief 云台模式.
	 * @note 关控设为none, 左下右上, 波轮向下为机械模式, 向上为陀螺仪模式
	 *
	 */
	ramen::Pullable<breeze::models::GimbalControlMode> OutGimbalMode;

	RC(const device* remote);

private:
	friend struct Impl;
	struct Impl;
	Impl& _impl;
	static constexpr auto IMPL_SIZE = 1024;
	std::byte _storage[IMPL_SIZE];
};
} // namespace app::sensors
