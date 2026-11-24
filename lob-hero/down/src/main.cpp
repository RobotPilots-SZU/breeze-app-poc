/**
 * @file main.cpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief Lob Hero Down Board 的应用层主入口
 * @version 0.1
 * @date 2026-11-21
 *
 * @copyright Copyright (c) 2026
 *
 */

#include <app/b2b/module.hpp>
#include <app/models/device.hpp>
#include <app/models/priority.hpp>
#include <app/motors/module.hpp>
#include <app/sensors/rc.hpp>
#include <breeze/gimbal/yaw1_pitch1_pid.hpp>
#include <ramenx/time.hpp>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

namespace app
{

ramen::FrameProvider<512> controlFrame{
    {.period = K_MSEC(1), .priority = models::priority::CONTROL_TICK, .name = "control_tick"}
};
b2b::DownModule b2bCom{DEVICE_DT_GET(DT_NODELABEL(can1))};
sensors::RC rc{DEVICE_DT_GET(DT_NODELABEL(remote_dt7))};
breeze::gimbal::Yaw1Pitch1PidModule gimbal{{}};
motors::LkModule gimbalYawMotor{
    DEVICE_DT_GET(DT_NODELABEL(gimbal_yaw)),
    {.EncoderCounts = DT_PROP(DT_NODELABEL(gimbal_yaw), motor_encoder),
	 .TransmissionRatio = DT_PROP(DT_NODELABEL(gimbal_yaw), motor_transmission_ratio),
	 .IqCommandPerNewtonMeter = 1.4f}
};

static void Start()
{
	controlFrame.start();
	gimbal.Start();
	//
}

static void Connect()
{
	// Tick wire
	controlFrame.out >> rc.InTick;
	controlFrame.out >> gimbalYawMotor.InTick;

	// Data wire

	gimbal.InHeadImu >> b2bCom.Out.Imus[models::ImuId::Head];
	gimbal.OutPitchMotor >> b2bCom.In.Motors[models::MotorId::HeadPitch];
	gimbal.OutYawMotor >> gimbalYawMotor.InControl;

	rc.OutPosture >> gimbal.InMachTarget;
	//
}

} // namespace app

int main()
{
	app::Connect();
	app::Start();
}
