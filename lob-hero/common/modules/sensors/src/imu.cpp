#include "app/sensors/imu.hpp"
#include "imu_attitude.hpp"
#include <new>
#include <numbers>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

namespace app::sensors
{

struct Imu::Impl
{
	const device* Accel{};
	const device* Gyro{};
	Param Config;
	detail::ImuAttitude Attitude;
	detail::Vector3 GyroBias{};
	detail::Vector3 AngularVelocity{};
	std::uint32_t LastCycle{};

	detail::Vector3 Read(const device* dev, sensor_channel channel) const
	{
		sensor_value values[3];
		sensor_sample_fetch(dev);
		sensor_channel_get(dev, channel, values);
		const detail::Vector3 raw{
		    sensor_value_to_float(&values[0]),
		    sensor_value_to_float(&values[1]),
		    sensor_value_to_float(&values[2]),
		};
		detail::Vector3 body{};
		for (std::size_t axis = 0; axis < body.size(); ++axis)
		{
			const auto& row = Config.SensorToBody[axis];
			body[axis] = row[0] * raw[0] + row[1] * raw[1] + row[2] * raw[2];
		}
		return body;
	}

	void Start(std::uint32_t sampleCount)
	{
		detail::Vector3 accelerationSum{};
		GyroBias = {};
		for (std::uint32_t sample = 0; sample < sampleCount; ++sample)
		{
			// BMI088 默认加速度采样率 800 Hz，避免连续读取同一份样本。
			k_sleep(K_MSEC(2));
			const auto acceleration = Read(Accel, SENSOR_CHAN_ACCEL_XYZ);
			const auto gyro = Read(Gyro, SENSOR_CHAN_GYRO_XYZ);
			for (std::size_t axis = 0; axis < GyroBias.size(); ++axis)
			{
				accelerationSum[axis] += acceleration[axis];
				GyroBias[axis] += gyro[axis];
			}
		}
		for (float& bias : GyroBias)
		{
			bias /= static_cast<float>(sampleCount);
		}
		// 求倾角仅依赖重力方向，无需求加速度均值。
		Attitude.Reset(accelerationSum);
		AngularVelocity = {};
		LastCycle = k_cycle_get_32();
	}

	void Update()
	{
		const auto acceleration = Read(Accel, SENSOR_CHAN_ACCEL_XYZ);
		const auto gyro = Read(Gyro, SENSOR_CHAN_GYRO_XYZ);
		const auto now = k_cycle_get_32();
		const float deltaTime = static_cast<float>(k_cyc_to_ns_floor64(now - LastCycle)) * 1.0e-9F;
		LastCycle = now;
		for (std::size_t axis = 0; axis < AngularVelocity.size(); ++axis)
		{
			AngularVelocity[axis] = gyro[axis] - GyroBias[axis];
		}
		Attitude.Update(acceleration, AngularVelocity, deltaTime, Config.GravityGain);
	}

	breeze::models::ImuAngularReport Report() const
	{
		constexpr float TO_DEGREES = 180.0F / std::numbers::pi_v<float>;
		const auto [roll, pitch, yaw] = Attitude.Euler();
		return {
		    .Yaw = {yaw * TO_DEGREES},
		    .Roll = {roll * TO_DEGREES},
		    .Pitch = {pitch * TO_DEGREES},
		    .YawRate = {AngularVelocity[2] * TO_DEGREES},
		    .RollRate = {AngularVelocity[0] * TO_DEGREES},
		    .PitchRate = {AngularVelocity[1] * TO_DEGREES},
		};
	}
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Imu::Imu(const device* accel, const device* gyro, const Param& param)
    : OutAngular([this](auto& out) { out = _impl.Report(); }),
      InTick([this]() { _impl.Update(); }),
      _impl(*new (_storage) Impl{.Accel = accel, .Gyro = gyro, .Config = param})
{
	static_assert(sizeof(Impl) <= IMPL_SIZE);
	static_assert(alignof(Impl) <= alignof(Imu));
}

void Imu::Start(std::uint32_t sampleCount) { _impl.Start(sampleCount); }

Imu::~Imu() { _impl.~Impl(); }
} // namespace app::sensors
