/**
 * @file basis.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 板通协议相关的基本类型定义. 用于拆分表现层和传输层
 * @version 0.1
 * @date 2026-11-17
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include <cstdint>
#include <etl/span.h>

namespace app::b2b
{

/**
 * @brief 传输数据描述.
 *
 */
struct TransferInfo
{
	/**
	 * @brief CAN Id
	 *
	 */
	std::uint16_t Id;
	/**
	 * @brief 表现层数据.
	 *
	 */
	etl::span<const std::byte> Data;
};

} // namespace app::b2b
