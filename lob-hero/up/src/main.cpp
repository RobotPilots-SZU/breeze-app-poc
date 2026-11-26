#include <app/b2b.hpp>
#include <app/models/priority.hpp>
#include <app/sensors/imu.hpp>
#include <app/tracer.hpp>
#include <ramenx/time.hpp>

namespace
{
using namespace app;

ramen::FrameProvider<1024> traceFrame{
    {.period = K_MSEC(1000), .priority = models::priority::TRACE_TICK, .name = "trace_tick"}
};
ramen::FrameProvider<2048> sensorFrame{
    {.period = K_MSEC(1), .priority = models::priority::SENSOR_TICK, .name = "sensor_tick"}
};

/**
 * @brief 云台imu模块
 * @note 目前测试发现水平静置时, roll约为~179,
 * pitch向上为正, 水平为0
 * yaw 在上电位置为0, 左转为正
 */
sensors::Imu headImu{
    DEVICE_DT_GET(DT_NODELABEL(bmi08x_accel)),
    DEVICE_DT_GET(DT_NODELABEL(bmi08x_gyro)),
    // 默认参数
    {}
};

b2b::UpModule b2bCom{DEVICE_DT_GET(DT_NODELABEL(can1)), K_MSEC(10)};

Tracer tracer{};

} // namespace

namespace app
{

void Start()
{
	traceFrame.start();
	sensorFrame.start();

	headImu.Start();
	b2bCom.Start();
}
void Connect()
{
	sensorFrame.out >> headImu.InTick;
	traceFrame.out >> tracer.InTick;

	tracer.InImu >> headImu.OutAngular;
}
} // namespace app

int main()
{
	app::Connect();
	app::Start();
}
