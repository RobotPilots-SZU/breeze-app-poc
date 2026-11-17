#include "imu_attitude.hpp"
#include <app/sensors/imu.hpp>
#include <numbers>
#include <zephyr/drivers/sensor.h>
#include <zephyr/ztest.h>

namespace
{
using app::sensors::detail::ImuAttitude;
using app::sensors::detail::Vector3;
constexpr float PI = std::numbers::pi_v<float>;

struct SensorData
{
	sensor_channel Channel;
	Vector3 Value;
	int FetchCount = 0;
};

SensorData accelData{SENSOR_CHAN_ACCEL_XYZ, {0, 0, 9.81F}};
SensorData gyroData{SENSOR_CHAN_GYRO_XYZ, {0, 0, 0}};

int Fetch(const device* dev, sensor_channel)
{
	++static_cast<SensorData*>(dev->data)->FetchCount;
	return 0;
}

int Get(const device* dev, sensor_channel channel, sensor_value* values)
{
	const auto& data = *static_cast<SensorData*>(dev->data);
	zassert_equal(channel, data.Channel);
	for (std::size_t axis = 0; axis < data.Value.size(); ++axis)
	{
		sensor_value_from_float(&values[axis], data.Value[axis]);
	}
	return 0;
}

DEVICE_API(sensor, API) = {.sample_fetch = Fetch, .channel_get = Get};
DEVICE_DEFINE(test_accel, "test_accel", nullptr, nullptr, &accelData, nullptr, POST_KERNEL, 90, &API);
DEVICE_DEFINE(test_gyro, "test_gyro", nullptr, nullptr, &gyroData, nullptr, POST_KERNEL, 90, &API);

void ExpectEuler(const ImuAttitude& attitude, const Vector3& expected, float tolerance = 0.0001F)
{
	const auto angles = attitude.Euler();
	for (std::size_t axis = 0; axis < angles.size(); ++axis)
	{
		zassert_within(angles[axis], expected[axis], tolerance, "axis %zu", axis);
	}
}

ZTEST(imu, initializes_compound_tilt_without_yaw)
{
	ImuAttitude attitude;
	// Roll = 30°, Pitch = -45°：单位重力在机体系的投影。
	const Vector3 gravity{0.70710678F, 0.35355339F, 0.61237244F};
	attitude.Reset(gravity);
	ExpectEuler(attitude, {PI / 6, -PI / 4, 0});
	for (int i = 0; i < 1000; ++i)
	{
		attitude.Update(gravity, {0, 0, 0}, 0.001F, 2.0F);
	}
	ExpectEuler(attitude, {PI / 6, -PI / 4, 0});
}

ZTEST(imu, integrates_each_axis_with_variable_intervals)
{
	for (std::size_t axis = 0; axis < 3; ++axis)
	{
		ImuAttitude attitude;
		Vector3 rate{};
		rate[axis] = PI / 4;
		for (int i = 0; i < 1000; ++i)
		{
			attitude.Update({0, 0, 9.81F}, rate, i % 2 == 0 ? 0.0005F : 0.0015F, 0);
		}
		ExpectEuler(attitude, rate);
	}
}

ZTEST(imu, gravity_corrects_tilt_but_does_not_observe_yaw)
{
	ImuAttitude attitude;
	attitude.Reset({0, 0, 9.81F});
	for (int i = 0; i < 1000; ++i)
	{
		attitude.Update({0, 0, 9.81F}, {0, 0, PI / 2}, 0.001F, 2.0F);
	}
	for (int i = 0; i < 5000; ++i)
	{
		attitude.Update({0, 4.905F, 8.495709F}, {0, 0, 0}, 0.001F, 2.0F);
	}
	ExpectEuler(attitude, {PI / 6, 0, PI / 2});
}

ZTEST(imu, yaw_wrap_and_vertical_pitch_remain_finite)
{
	ImuAttitude attitude;
	for (int i = 0; i < 2000; ++i)
	{
		attitude.Update({0, 0, 9.81F}, {0, 0, PI}, 0.001F, 0);
	}
	ExpectEuler(attitude, {0, 0, 0});
	attitude.Reset({-9.81F, 0, 0});
	for (int i = 0; i < 1000; ++i)
	{
		attitude.Update({-9.81F, 0, 0}, {0, 0, 0}, 0.001F, 2.0F);
	}
	const auto angles = attitude.Euler();
	for (float angle : angles)
	{
		zassert_true(std::isfinite(angle));
	}
	zassert_within(angles[1], PI / 2, 0.001F);
}

ZTEST(imu, device_flow_calibrates_transforms_and_reports_units)
{
	// 传感器 Y -> 机体 Z，传感器 Z -> 机体 -Y。
	app::sensors::Imu imu(
	    DEVICE_GET(test_accel), DEVICE_GET(test_gyro),
	    {
	        .SensorToBody = {{{1, 0, 0}, {0, 0, -1}, {0, 1, 0}}},
	        .GravityGain = 0,
	    }
	);
	accelData.Value = {0, 9.81F, 0};
	gyroData.Value = {0.01F, -0.02F, 0.03F};
	accelData.FetchCount = gyroData.FetchCount = 0;
	imu.Start(4);
	zassert_equal(accelData.FetchCount, 4);
	zassert_equal(gyroData.FetchCount, 4);

	k_sleep(K_MSEC(10));
	imu.InTick();
	const auto previousCycle = k_cycle_get_32();
	breeze::models::ImuAngularReport report;
	imu.OutAngular(report);
	zassert_within(report.YawRate.Value, 0, 0.001F);
	zassert_within(report.RollRate.Value, 0, 0.001F);
	zassert_within(report.PitchRate.Value, 0, 0.001F);

	gyroData.Value = {0.01F + PI / 6, -0.02F + PI / 2, 0.03F + PI / 4};
	k_sleep(K_MSEC(10));
	imu.InTick();
	const float elapsed = static_cast<float>(k_cyc_to_ns_floor64(k_cycle_get_32() - previousCycle)) * 1.0e-9F;
	imu.OutAngular(report);
	zassert_within(report.RollRate.Value, 30, 0.001F);
	zassert_within(report.PitchRate.Value, -45, 0.001F);
	zassert_within(report.YawRate.Value, 90, 0.001F);
	zassert_within(report.Yaw.Value, 90.0F * elapsed, 0.05F);
	const float yaw = report.Yaw.Value;
	imu.OutAngular(report);
	zassert_equal(report.Yaw.Value, yaw);
	zassert_equal(accelData.FetchCount, 6);
	zassert_equal(gyroData.FetchCount, 6);

	// 显式重新校准应清除前一次姿态、零偏与角速度。
	gyroData.Value = {-0.04F, 0.05F, -0.06F};
	imu.Start(2);
	k_sleep(K_MSEC(10));
	imu.InTick();
	imu.OutAngular(report);
	zassert_within(report.Yaw.Value, 0, 0.001F);
	zassert_within(report.YawRate.Value, 0, 0.001F);
	zassert_within(report.RollRate.Value, 0, 0.001F);
	zassert_within(report.PitchRate.Value, 0, 0.001F);
}
} // namespace

ZTEST_SUITE(imu, nullptr, nullptr, nullptr, nullptr, nullptr);
