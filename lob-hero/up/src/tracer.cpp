#include <app/tracer.hpp>
#include <breeze/models/device.hpp>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(TRACER);

using namespace breeze::models;

namespace app
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdouble-promotion"
// NOLINTBEGIN(readability-simplify-boolean-expr)
void Tracer::OnTick() const
{
	if (auto ret = *InImu; ret != decltype(ret){})
	{
		LOG_INF(
		    "Imu value:"
		    "{"
		    "\tvalue of: yaw=%f, roll=%f, pitch=%f"
		    "\trate of: yaw=%f, roll=%f, pitch=%f"
		    "}",
		    ret.Yaw.Value, ret.Roll.Value, ret.Pitch.Value,            // 角度
		    ret.YawRate.Value, ret.RollRate.Value, ret.PitchRate.Value // 角速度
		);
	}
}
#pragma clang diagnostic pop
// NOLINTEND(readability-simplify-boolean-expr)
} // namespace app
