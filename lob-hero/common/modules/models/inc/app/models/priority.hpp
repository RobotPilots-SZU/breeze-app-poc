/**
 * @file priority.hpp
 * @author vix_hentx (w1084349470@outlook.com)
 * @brief 定义线程相关的优先级, 防止后续硬编码
 * @version 0.1
 * @date 2026-11-19
 *
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace app::models::priority
{
/**
 * @brief 高速板通
 *
 */
constexpr int B2B_CAST_TRANS = 10;

/**
 * @brief 高速板通的数据收集线程
 *
 */
constexpr int B2B_CAST_COLLECT = 11;
/**
 * @brief RP-Proto 板通
 *
 */
constexpr int B2B_COM_TRANS = 11;
} // namespace app::models::priority
