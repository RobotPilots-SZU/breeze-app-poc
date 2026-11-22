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

#include "app/sensors/rc.hpp"
#include <app/b2b/module.hpp>
#include <zephyr/device.h>

namespace app
{

b2b::DownModule b2b_com{DEVICE_DT_GET(DT_NODELABEL(can2))};
sensors::RC rc{DEVICE_DT_GET(DT_NODELABEL(remote_dt7))};

static void Start()
{
	//
}

static void Connect()
{
	//
}

} // namespace app

int main()
{
	app::Connect();
	app::Start();
}
