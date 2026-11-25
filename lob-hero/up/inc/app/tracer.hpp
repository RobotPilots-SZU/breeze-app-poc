/**
 * @file tracer.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 用来调试的工具, 接入数据流进行trace
 * @version 0.1
 * @date 2026-11-25
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <breeze/models/control.hpp>
#include <breeze/models/device.hpp>
#include <ramen.hpp>

namespace app
{
struct Tracer
{
	ramen::Pushable<> InTick = [this]() { OnTick(); };
	ramen::Puller<breeze::models::ImuAngularReport> InImu;

private:
	void OnTick() const;
};

} // namespace app
