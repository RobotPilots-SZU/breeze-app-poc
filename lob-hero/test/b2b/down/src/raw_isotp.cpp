/* SPDX-FileCopyrightText: Copyright (c) 2026 RobotPilots-SZU */
/* SPDX-License-Identifier: Apache-2.0 */

#include <array>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <zephyr/canbus/isotp.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>

LOG_MODULE_REGISTER(b2b_raw_isotp_test, LOG_LEVEL_INF);

namespace
{
constexpr std::uint32_t RequestDataId = 0x321U;
constexpr std::uint32_t RequestFcId = 0x322U;
constexpr std::uint32_t ResponseDataId = 0x323U;
constexpr std::uint32_t ResponseFcId = 0x324U;
constexpr std::size_t PayloadSize = 20U;
constexpr k_timeout_t RxTimeout = K_SECONDS(10);

#if defined(B2B_TEST_ROLE_TX)
isotp_recv_ctx ResponseRecvCtx{};
#else
isotp_recv_ctx RequestRecvCtx{};
#endif
isotp_send_ctx SendCtx{};

std::array<std::uint8_t, PayloadSize> MakePayload(std::uint8_t seed)
{
	std::array<std::uint8_t, PayloadSize> payload{};
	for (std::size_t i = 0; i < payload.size(); ++i) {
		payload[i] = static_cast<std::uint8_t>(seed + i);
	}
	return payload;
}

bool ReceiveAndCheck(isotp_recv_ctx* ctx,
		     const std::array<std::uint8_t, PayloadSize>& expected)
{
	std::array<std::uint8_t, PayloadSize> received{};
	std::size_t total = 0U;
	int remaining = 0;
	do {
		net_buf* buffer = nullptr;
		remaining = isotp_recv_net(ctx, &buffer, RxTimeout);
		if (remaining < 0) {
			LOG_ERR("ISO-TP receive failed: %d", remaining);
			return false;
		}
		const std::size_t chunk = net_buf_frags_len(buffer);
		if (chunk > received.size() - total) {
			net_buf_unref(buffer);
			LOG_ERR("ISO-TP receive exceeded test payload size");
			return false;
		}
		const std::size_t copied = net_buf_linearize(received.data() + total,
							     received.size() - total, buffer, 0, chunk);
		net_buf_unref(buffer);
		if (copied != chunk) {
			LOG_ERR("ISO-TP receive copy mismatch: %u/%u",
				static_cast<unsigned int>(copied), static_cast<unsigned int>(chunk));
			return false;
		}
		total += copied;
		if (static_cast<std::size_t>(remaining) != received.size() - total) {
			LOG_ERR("ISO-TP remaining length mismatch: remaining=%d total=%u",
				remaining, static_cast<unsigned int>(total));
			return false;
		}
	} while (remaining != 0);

	const bool passed = total == expected.size() && received == expected;
	LOG_INF("ISO-TP RX bytes=%u %s", static_cast<unsigned int>(total),
		passed ? "PASS" : "FAIL");
	return passed;
}

int Bind(isotp_recv_ctx* ctx, const device* can, std::uint16_t rxId, std::uint16_t fcId)
{
	isotp_msg_id rxAddr{};
	isotp_msg_id fcAddr{};
	rxAddr.std_id = rxId;
	fcAddr.std_id = fcId;
	const isotp_fc_opts options{.bs = 8U, .stmin = 5U};
	return isotp_bind(ctx, can, &rxAddr, &fcAddr, &options, K_SECONDS(1));
}

int Send(const device* can, const std::array<std::uint8_t, PayloadSize>& payload,
	 std::uint16_t dataId, std::uint16_t fcId)
{
	isotp_msg_id txAddr{};
	isotp_msg_id rxAddr{};
	txAddr.std_id = dataId;
	rxAddr.std_id = fcId;
	return isotp_send(&SendCtx, can, payload.data(), payload.size(), &txAddr, &rxAddr,
		   nullptr, nullptr);
}
}

int main()
{
	const device* can = DEVICE_DT_GET(DT_NODELABEL(can1));
	if (!device_is_ready(can)) {
		LOG_ERR("CAN1 device is not ready");
		return 1;
	}
	int ret = can_set_mode(can, CAN_MODE_NORMAL);
	if (ret == 0) {
		ret = can_start(can);
	}
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start CAN1 in normal mode: %d", ret);
		return 1;
	}

#if defined(B2B_TEST_ROLE_TX)
	ret = Bind(&ResponseRecvCtx, can, ResponseDataId, ResponseFcId);
#else
	ret = Bind(&RequestRecvCtx, can, RequestDataId, RequestFcId);
#endif
	if (ret != 0) {
		LOG_ERR("ISO-TP bind failed: %d", ret);
		return 1;
	}
	LOG_INF("CAN1 normal mode ready, role=%s, payload=%u bytes",
#if defined(B2B_TEST_ROLE_TX)
		"tx",
#else
		"rx",
#endif
		static_cast<unsigned int>(PayloadSize));

#if defined(B2B_TEST_ROLE_TX)
	k_sleep(K_SECONDS(2));
	const auto request = MakePayload(0x40U);
	ret = Send(can, request, RequestDataId, RequestFcId);
	const bool sent = ret == ISOTP_N_OK;
	if (!sent) {
		LOG_ERR("ISO-TP request send failed: %d", ret);
	}
	const bool received = sent && ReceiveAndCheck(&ResponseRecvCtx, MakePayload(0x60U));
	const bool passed = sent && received;
	isotp_unbind(&ResponseRecvCtx);
#else
	const bool received = ReceiveAndCheck(&RequestRecvCtx, MakePayload(0x40U));
	bool passed = received;
	if (received) {
		ret = Send(can, MakePayload(0x60U), ResponseDataId, ResponseFcId);
		passed = ret == ISOTP_N_OK;
		if (!passed) {
			LOG_ERR("ISO-TP response send failed: %d", ret);
		}
	}
	isotp_unbind(&RequestRecvCtx);
#endif
	LOG_INF("RAW ISO-TP SUMMARY: %s", passed ? "PASS" : "FAIL");
	return passed ? 0 : 1;
}
