/**
 * @file msg.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 板间通信协议数据
 * @note 该层不应该直接使用Model的结构化数据, 而是使用pack以及定点转换等等手段来减少传输大小
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
	std::uint16_t B2B;            ///< 板通CAN地址
	std::uint16_t B2BFc;          ///< 本板发送B2B数据时接收FC的CAN地址
	std::uint16_t Communicator;   ///< Communicator (RP-Proto) CAN地址
	std::uint16_t CommunicatorFc; ///< Communicator (RP-Proto) 的FC CAN地址
};

constexpr Address UP_ADDR{.B2B = 0x30, .B2BFc = 0x130, .Communicator = 0x40};
constexpr Address DOWN_ADDR{.B2B = 0x31, .B2BFc = 0x131, .Communicator = 0x41};

#pragma pack(push, 1)
struct MotorControl
{
	static constexpr std::uint8_t ID = 0x10;
	static constexpr float SCALE = 32768.0F;
	models::MotorId Id;
	std::int16_t Torque; ///< 扭矩乘以SCALE
};

struct MotorReport
{
	static constexpr std::uint8_t ID = 0x11;
	static constexpr float SCALE = 32768.0F;
	models::MotorId Id;
	std::uint16_t Encode; ///< 编码值
	std::uint16_t Angle;  ///< 角度(Degree)值乘以SCALE
	std::uint16_t Speed;  ///< 转速(Rpm)值乘以SCALE
	std::uint16_t Temp;   ///< 温度(摄氏度)
};

struct ImuReport
{
	static constexpr std::uint8_t ID = 0x12;
	static constexpr float SCALE = 100.0F; ///< 定点数缩放比例: 1 LSB = 0.01 Degree
	models::ImuId Id;
	std::int16_t Yaw;       ///< 偏航角(Degree)乘以SCALE
	std::int16_t Roll;      ///< 横滚角(Degree)乘以SCALE
	std::int16_t Pitch;     ///< 俯仰角(Degree)乘以SCALE
	std::int16_t YawRate;   ///< 偏航角速度(Degree)乘以SCALE
	std::int16_t RollRate;  ///< 横滚角速度(Degree)乘以SCALE
	std::int16_t PitchRate; ///< 俯仰角速度(Degree)乘以SCALE
};

#pragma pack(pop)

using CastMessages = breeze::b2b::AvailableMessages<MotorControl, MotorReport, ImuReport>;

} // namespace app::b2b
