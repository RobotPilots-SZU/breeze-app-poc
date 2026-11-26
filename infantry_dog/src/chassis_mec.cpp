#include "chassis_mec.hpp"

namespace breeze::chassis::mec
{
namespace
{
// 电机信息存储
std::array<MotorConfig, 4> InternalMotorConfig{};

} // namespace
/**
 * @brief 限速
 *
 */
// static void SpeedLimit(breeze::chassis::Control::Target& target, float maxspeed)
// {
// 	float vx = target.Vx;
// 	float vy = target.Vy;
// 	float cycle = target.Cycle;
// }

/**
 * @brief 全向轮解算
 *
 * @param target 目标值
 */
std::array<MotorOutput, 4> Solve(breeze::chassis::Control::Target& target)
{
	// 输出信息
	std::array<MotorOutput, 4> output{};

	float vx = target.Vx;
	float vy = target.Vy;
	float cycle = target.Cycle;

	output = std::array<MotorOutput, 4>{{
	    {
	        MotorPosition::LeftFront,
	        MotorTorque{-vx + vy - cycle},
	    },
	    {
	        MotorPosition::LeftBehind,
	        MotorTorque{vx + vy - cycle},
	    },
	    {
	        MotorPosition::RightBehind,
	        MotorTorque{-vx + vy + cycle},
	    },
	    {
	        MotorPosition::RightFront,
	        MotorTorque{vx + vy + cycle},
	    },
	}};

	return output;
}

// 貌似是滚木，先放着吧
// 全向轮底盘各种电机数量
breeze::chassis::Config::MotorNumber Omni{
    .Wheel = 4,
    .Steering = 0,
    .Joint = 0,
};

// 设置电机信息
void SetMotorConfig(std::array<MotorConfig, 4> motor)
{
	// 存入InternalMotorConfig
	InternalMotorConfig = motor;
}

} // namespace breeze::chassis::mec
