#include <app/b2b/msg.hpp>
#include <breeze/models/device.hpp>
#include <breeze/models/utils.hpp>
#include <cstddef>
#include <ramen.hpp>
#include <zephyr/device.h>

namespace app::b2b
{

using MotorControlPushers =
    breeze::models::IndexedEnumArray<models::MotorId, ramen::Pusher<breeze::models::MotorControl>, models::MOTOR_IDS>;
using MotorReportPullers =
    breeze::models::IndexedEnumArray<models::MotorId, ramen::Puller<breeze::models::MotorReport>, models::MOTOR_IDS>;

struct Output
{
	MotorControlPushers Motors;
};

struct Input
{
	MotorReportPullers Motors;
};

/**
 * @brief 下板的b2b模块.
 *
 */
class DownModule
{
public:
	Output Out;
	Input In;
	DownModule(const device* dev);

private:
	constexpr static std::size_t IMPL_SIZE = 4096;
	alignas(std::max_align_t) std::byte _storage[IMPL_SIZE];
	struct Impl;
	Impl* P();
};
} // namespace app::b2b
