/**
 * @file module.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 电机设备模块
 * @version 0.1
 * @date 2026-11-22
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <breeze/models/control.hpp>
#include <breeze/models/device.hpp>
#include <cstddef>
#include <cstdint>
#include <ramen.hpp>
#include <zephyr/device.h>

namespace app::motors
{

/**
 * @brief RM电机档案预设
 *
 */
enum class RmProfile : std::uint8_t
{
	M3508C620,
	M2006C610,
	GM6020
};

/**
 * @brief RM电机配置
 *
 */
struct RmConfig
{
	std::uint16_t EncoderCounts;    ///< 一圈对应编码器的值
	std::uint8_t TransmissionRatio; ///< 电流(A)转换为驱动值的系数
};

/** @brief LK/KT机械参数与Nm到闭环Iq命令LSB的换算系数。 */
struct LkConfig
{
	std::uint16_t EncoderCounts;
	std::uint8_t TransmissionRatio;
	float IqCommandPerNewtonMeter;
};

/**
 * @brief Rm电机模块. 无标定等等逻辑.
 *
 */
template <RmProfile Profile>
class RmModule
{
public:
	ramen::Pushable<breeze::models::MotorControl> InControl;

	ramen::Pushable<> InTick;
	ramen::Pullable<breeze::models::MotorReport> OutReport;
	ramen::Pullable<bool> OutOnline;
	RmModule(const device* dev, RmConfig mechanics);
	~RmModule();

	RmModule(const RmModule&) = delete;
	RmModule& operator=(const RmModule&) = delete;
	RmModule(RmModule&&) = delete;
	RmModule& operator=(RmModule&&) = delete;

private:
	struct Impl;
	static constexpr std::size_t IMPL_SIZE = 1024;
	alignas(std::max_align_t) std::byte _storage[IMPL_SIZE];
	Impl* P();
};

extern template class RmModule<RmProfile::M3508C620>;
extern template class RmModule<RmProfile::M2006C610>;
extern template class RmModule<RmProfile::GM6020>;

/** @brief 达妙电机模块. */
class DmModule
{
public:
	ramen::Pushable<breeze::models::MotorControl> InControl;
	ramen::Pushable<> InTick;
	ramen::Pullable<breeze::models::MotorReport> OutReport;
	ramen::Pullable<bool> OutOnline;
	DmModule(const device* dev);

	~DmModule();
	DmModule(const DmModule&) = delete;
	DmModule& operator=(const DmModule&) = delete;
	DmModule(DmModule&&) = delete;
	DmModule& operator=(DmModule&&) = delete;

private:
	struct Impl;
	static constexpr std::size_t IMPL_SIZE = 512;
	alignas(std::max_align_t) std::byte _storage[IMPL_SIZE];
	Impl* P();
};

/** @brief LK/KT单电机闭环扭矩模块。 */
class LkModule
{
public:
	ramen::Pushable<breeze::models::MotorControl> InControl;
	ramen::Pushable<> InTick;
	ramen::Pullable<breeze::models::MotorReport> OutReport;
	ramen::Pullable<bool> OutOnline;
	LkModule(const device* dev, LkConfig conf);

	~LkModule();
	LkModule(const LkModule&) = delete;
	LkModule& operator=(const LkModule&) = delete;
	LkModule(LkModule&&) = delete;
	LkModule& operator=(LkModule&&) = delete;

private:
	struct Impl;
	static constexpr std::size_t IMPL_SIZE = 512;
	alignas(std::max_align_t) std::byte _storage[IMPL_SIZE];
	Impl* P();
};

} // namespace app::motors
