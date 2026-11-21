/**
 * @file msg.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 板间通信协议数据
 * @version 0.1
 * @date 2026-11-16
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <app/models/device.hpp>
#include <breeze/b2b/proto.hpp>
#include <cstdint>

namespace app::b2b
{

/**
 * @brief ISO-TP 使用的CAN地址
 *
 */
struct Address
{
	std::uint8_t B2B;          ///< 板通CAN地址
	std::uint8_t Communicator; ///< Communicator (RP-Proto) CAN地址
};

constexpr Address UP_ADDR{.B2B = 0x30, .Communicator = 0x40};
constexpr Address DOWN_ADDR{.B2B = 0x31, .Communicator = 0x41};

#pragma pack(push, 1)
struct MotorControl
{
	static constexpr std::uint8_t ID = 0x10;
	models::MotorId Id;
	std::int16_t Torque; ///< 扭矩除以32768
};

struct MotorReport
{
	static constexpr std::uint8_t ID = 0x11;
	models::MotorId Id;
	std::uint16_t Encode; ///< 编码值
	std::uint16_t Angle;  ///< 角度(Degree)值除以32768
	std::uint16_t Speed;  ///< 转速(Rpm)值除以32768
	std::uint16_t Temp;   ///< 温度(摄氏度)
};

#pragma pack(pop)

using CastMessages = breeze::b2b::AvailableMessages<MotorControl, MotorReport>;

} // namespace app::b2b
