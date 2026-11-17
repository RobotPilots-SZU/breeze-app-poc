/**
 * @file imu.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief Imu (BMI088)设备封装
 * @version 0.1
 * @date 2026-11-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <array>
#include <breeze/models/device.hpp>
#include <cstddef>
#include <cstdint>
#include <ramen.hpp>
#include <zephyr/device.h>

namespace app::sensors
{
class Imu
{
public:
	struct Param
	{
		/// 传感器坐标到机体坐标的旋转矩阵，按行存储；两颗传感器使用相同轴定义。
		/// 机体坐标为右手系，水平静止时加速度沿 +Z，X/Y/Z 对应 Roll/Pitch/Yaw。
		std::array<std::array<float, 3>, 3> SensorToBody{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
		/// Mahony 比例反馈增益，单位 s^-1；0 表示仅积分陀螺仪。
		float GravityGain = 2.0F;
	};

	/**
	 * @brief ZYX 欧拉角，单位 °；Yaw/Roll 在 [-180, 180]，Pitch 在 [-90, 90]。
	 * Yaw 为 Start 后的相对航向，六轴 IMU 无绝对航向观测，长期会漂移。
	 * Rate 为去零偏的机体轴角速度，单位 °/s（与旧实现一致，并非欧拉角导数）。
	 * 读取时计算角度，不触发采样。
	 */
	ramen::Pullable<breeze::models::ImuAngularReport> OutAngular;

	/**
	 * @brief Start 后由调用方在线程上下文周期触发采样与解算。
	 * Start、InTick、OutAngular 由调用方串行访问；设备须已就绪。
	 * 相邻调用间隔须小于 32 位硬件周期计数器的回绕周期。
	 */
	ramen::Pushable<> InTick;

	Imu(const device* accel, const device* gyro, const Param& param);
	~Imu();
	Imu(const Imu&) = delete;
	Imu& operator=(const Imu&) = delete;
	Imu(Imu&&) = delete;
	Imu& operator=(Imu&&) = delete;

	/**
	 * @brief 模块启动方法(初始化)
	 * @par 内部细节
	 * 保持机体静止，阻塞采样 sampleCount 次（每次间隔 2 ms），估计陀螺零偏，
	 *
	 * @param sampleCount 采样次数
	 */
	void Start(std::uint32_t sampleCount = 500);

private:
	static constexpr int IMPL_SIZE = 128;
	alignas(std::max_align_t) std::byte _storage[IMPL_SIZE];
	struct Impl;
	Impl& _impl;
};
} // namespace app::sensors
