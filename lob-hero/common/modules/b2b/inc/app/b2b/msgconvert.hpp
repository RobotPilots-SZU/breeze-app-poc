#pragma once
#include "msg.hpp"
#include <algorithm>
#include <app/models/device.hpp>
#include <breeze/models/device.hpp>
#include <cmath>
#include <cstdint>
#include <etl/tuple.h>
#include <limits>

namespace app::b2b
{
template <typename T>
constexpr T ToFixed(float value, float scale)
{
	static_assert(std::numeric_limits<T>::is_integer);
	const auto scaled = std::round(value * scale);
	const auto bounded = std::clamp(
	    scaled, static_cast<float>(std::numeric_limits<T>::min()), static_cast<float>(std::numeric_limits<T>::max())
	);
	return static_cast<T>(bounded);
}

template <typename T>
constexpr float FromFixed(T value, float scale)
{
	static_assert(std::numeric_limits<T>::is_integer);
	return static_cast<float>(value) / scale;
}

constexpr breeze::models::MotorControl FromProto(const MotorControl& in)
{
	return {.Torque = {FromFixed(in.Torque, MotorControl::SCALE)}};
}

constexpr MotorControl ToProto(const breeze::models::MotorControl& in, models::MotorId id)
{
	return {.Id = id, .Torque = ToFixed<std::int16_t>(in.Torque.Value, MotorControl::SCALE)};
}

constexpr breeze::models::MotorReport FromProto(const MotorReport& in)
{
	return {
	    .Encoder = {.Encode = in.Encode},
	    .Angle = {FromFixed(in.Angle, MotorReport::SCALE)},
	    .Speed = {FromFixed(in.Speed, MotorReport::SCALE)},
	    .Temperature = {static_cast<int16_t>(in.Temp)},
	};
}

constexpr MotorReport ToProto(const breeze::models::MotorReport& in, models::MotorId id)
{
	return {
	    .Id = id,
	    .Encode = in.Encoder.Encode,
	    .Angle = ToFixed<std::uint16_t>(in.Angle.Value, MotorReport::SCALE),
	    .Speed = ToFixed<std::uint16_t>(in.Speed.Value, MotorReport::SCALE),
	    .Temp = static_cast<std::uint16_t>(in.Temperature.Value),
	};
}

constexpr breeze::models::ImuAngularReport FromProto(const ImuReport& in)
{
	return {
	    .Yaw = {FromFixed(in.Yaw, ImuReport::SCALE)},
	    .Roll = {FromFixed(in.Roll, ImuReport::SCALE)},
	    .Pitch = {FromFixed(in.Pitch, ImuReport::SCALE)},
	    .YawRate = {FromFixed(in.YawRate, ImuReport::SCALE)},
	    .RollRate = {FromFixed(in.RollRate, ImuReport::SCALE)},
	    .PitchRate = {FromFixed(in.PitchRate, ImuReport::SCALE)},
	};
}

constexpr ImuReport ToProto(const breeze::models::ImuAngularReport& in, models::ImuId id)
{
	return {
	    .Id = id,
	    .Yaw = ToFixed<std::int16_t>(in.Yaw.Value, ImuReport::SCALE),
	    .Roll = ToFixed<std::int16_t>(in.Roll.Value, ImuReport::SCALE),
	    .Pitch = ToFixed<std::int16_t>(in.Pitch.Value, ImuReport::SCALE),
	    .YawRate = ToFixed<std::int16_t>(in.YawRate.Value, ImuReport::SCALE),
	    .RollRate = ToFixed<std::int16_t>(in.RollRate.Value, ImuReport::SCALE),
	    .PitchRate = ToFixed<std::int16_t>(in.PitchRate.Value, ImuReport::SCALE),
	};
}

} // namespace app::b2b
