#include "ramen.hpp"
#include <app/b2b/module.hpp>
#include <app/models/device.hpp>
#include <breeze/models/device.hpp>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_down_test);

using namespace breeze::models;

static app::b2b::DownModule b2b{DEVICE_DT_GET(DT_NODELABEL(can1))};
static ramen::Puller<ImuAngularReport> inImu;
/**
 * @brief 来自上板的imu值, 监视该变量即可.
 *
 */
static ImuAngularReport imu{};

int main()
{
	inImu >> b2b.Out.Imus[app::models::ImuId::Head];
	b2b.Start();

	while (true)
	{
		imu = *inImu;
		LOG_INF("Imu: yaw=%.2f, pitch=%.2f, roll=%.2f", imu.Yaw.Value, imu.Pitch.Value, imu.Roll.Value);
		k_sleep(K_MSEC(100));
	}
}
