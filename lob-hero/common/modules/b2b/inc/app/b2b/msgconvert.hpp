#pragma once
#include "msg.hpp"
#include <app/models/device.hpp>
#include <breeze/models/device.hpp>
#include <cmath>
#include <cstdint>
#include <etl/tuple.h>

namespace app::b2b
{
constexpr breeze::models::MotorControl FromProto(MotorControl in) { return {.Torque = {in.Torque / 32768.0f}}; }
constexpr MotorControl ToProto(breeze::models::MotorControl in, models::MotorId id)
{
	return {.Id = id, .Torque = static_cast<int16_t>(std::round(in.Torque.Value * 32768))};
}

constexpr breeze::models::MotorReport FromProto(MotorReport in)
{
	return {
	    .Encoder = {.Encode = in.Encode},
	    .Angle = {in.Angle / 32768.0f},
	    .Speed = {in.Speed / 32768.0f},
	    .Temperature = {static_cast<int16_t>(in.Temp)},
	};
}

constexpr MotorReport ToProto(breeze::models::MotorReport in, models::MotorId id)
{
	return {
	    .Id = id,
	    .Encode = in.Encoder.Encode,
	    .Angle = static_cast<uint16_t>(std::round(in.Angle.Value * 32768)),
	    .Speed = static_cast<uint16_t>(std::round(in.Speed.Value * 32768)),
	    .Temp = static_cast<uint16_t>(in.Temperature.Value),
	};
}
} // namespace app::b2b
