#include "app/models/device.hpp"
#include "breeze/models/device.hpp"
#include "zephyr/sleep.h"
#include <app/b2b.hpp>
#include <etl/array.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_up_test);

using Imu = breeze::models::ImuAngularReport;

static app::b2b::UpModule b2b{DEVICE_DT_GET(DT_NODELABEL(can1)), K_MSEC(10)};
static Imu imuValue;
static ramen::Pullable<Imu> outImu = [](auto& out) { out = imuValue; };

int main()
{
	// clang-format off
	constexpr auto IMU_SEQ = etl::array{
	    Imu{.Pitch = {1}},
	    Imu{.Roll = {1}},
	    Imu{.Yaw = {1}}
	};
	b2b.In.Imus[app::models::ImuId::Head] >> outImu;
	// clang-format on
	b2b.Start();

	for (;;)
	{
		for (const auto& imu : IMU_SEQ)
		{
			::imuValue = imu;
			k_sleep(K_MSEC(10));
		}
	}
	return 0;
}
