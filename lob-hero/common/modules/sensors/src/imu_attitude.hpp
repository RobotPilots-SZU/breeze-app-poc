#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace app::sensors::detail
{
using Vector3 = std::array<float, 3>;

/// 六轴四元数互补滤波，四元数将机体系旋转到世界系；唯一状态为单位四元数。
class ImuAttitude
{
public:
	void Reset(const Vector3& acceleration)
	{
		const auto [x, y, z] = acceleration;
		const float halfRoll = 0.5F * std::atan2(y, z);
		const float halfPitch = 0.5F * std::atan2(-x, std::hypot(y, z));
		const float cr = std::cos(halfRoll), sr = std::sin(halfRoll);
		const float cp = std::cos(halfPitch), sp = std::sin(halfPitch);
		_quaternion = {cr * cp, sr * cp, cr * sp, -sr * sp};
	}

	/// acceleration 非零，gyro 单位 rad/s，deltaTime 单位 s。
	void Update(const Vector3& acceleration, const Vector3& gyro, float deltaTime, float gravityGain)
	{
		const auto [w, x, y, z] = _quaternion;
		const auto [ax, ay, az] = acceleration;
		const float norm = std::sqrt(ax * ax + ay * ay + az * az);
		// 世界系 +Z 在机体系的方向，测量方向叉乘预测方向得到倾角误差。
		const float vx = 2.0F * (x * z - w * y);
		const float vy = 2.0F * (w * x + y * z);
		const float vz = 1.0F - 2.0F * (x * x + y * y);
		const float gx = gyro[0] + gravityGain * (ay * vz - az * vy) / norm;
		const float gy = gyro[1] + gravityGain * (az * vx - ax * vz) / norm;
		const float gz = gyro[2] + gravityGain * (ax * vy - ay * vx) / norm;
		const float halfDt = 0.5F * deltaTime;
		_quaternion = {
		    w + (-x * gx - y * gy - z * gz) * halfDt,
		    x + (w * gx + y * gz - z * gy) * halfDt,
		    y + (w * gy - x * gz + z * gx) * halfDt,
		    z + (w * gz + x * gy - y * gx) * halfDt,
		};
		float squaredNorm = 0.0F;
		for (const float component : _quaternion)
		{
			squaredNorm += component * component;
		}
		const float inverseNorm = 1.0F / std::sqrt(squaredNorm);
		for (float& component : _quaternion)
		{
			component *= inverseNorm;
		}
	}

	/// ZYX 欧拉角 {Roll, Pitch, Yaw}，单位 rad。
	Vector3 Euler() const
	{
		const auto [w, x, y, z] = _quaternion;
		return {
		    std::atan2(2.0F * (w * x + y * z), 1.0F - 2.0F * (x * x + y * y)),
		    std::asin(std::clamp(2.0F * (w * y - z * x), -1.0F, 1.0F)),
		    std::atan2(2.0F * (w * z + x * y), 1.0F - 2.0F * (y * y + z * z)),
		};
	}

private:
	std::array<float, 4> _quaternion{1, 0, 0, 0};
};
} // namespace app::sensors::detail
