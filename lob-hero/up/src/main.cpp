#include <app/models/priority.hpp>
#include <app/sensors/imu.hpp>
#include <app/tracer.hpp>
#include <ramenx/time.hpp>

namespace
{
using namespace app;

ramen::FrameProvider<128> traceFrame{
    {.period = K_MSEC(1000), .priority = models::priority::TRACE_TICK, .name = "trace_tick"}
};
ramen::FrameProvider<512> sensorFrame{
    {.period = K_MSEC(1), .priority = models::priority::SENSOR_TICK, .name = "sensor_tick"}
};

sensors::Imu headImu{
    DEVICE_DT_GET(DT_NODELABEL(bmi08x_accel)),
    DEVICE_DT_GET(DT_NODELABEL(bmi08x_gyro)),
    // 默认参数
    {}
};

Tracer tracer{};

} // namespace

namespace app
{

void Start()
{
	traceFrame.start();

	headImu.Start();
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
