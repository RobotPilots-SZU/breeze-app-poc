/* SPDX-FileCopyrightText: Copyright (c) 2026 RobotPilots-SZU */
/* SPDX-License-Identifier: Apache-2.0 */

#include <array>
#include <cstdint>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_raw_can_test, LOG_LEVEL_INF);

namespace
{
constexpr std::uint32_t RequestId = 0x321U;
constexpr std::uint32_t ResponseId = 0x322U;
constexpr k_timeout_t RxTimeout = K_SECONDS(10);
CAN_MSGQ_DEFINE(RxQueue, 2);

constexpr std::array<std::uint8_t, CAN_MAX_DLC> RequestData{0xA0, 1, 2, 3, 4, 5, 6, 7};
constexpr std::array<std::uint8_t, CAN_MAX_DLC> ResponseData{0xB0, 1, 2, 3, 4, 5, 6, 7};

bool ReceiveAndCheck(std::uint32_t id, const std::array<std::uint8_t, CAN_MAX_DLC>& expected)
{
	can_frame frame{};
	const int ret = k_msgq_get(&RxQueue, &frame, RxTimeout);
	if (ret != 0) {
		LOG_ERR("RX timeout waiting for CAN ID 0x%x: %d", id, ret);
		return false;
	}

	bool valid = frame.id == id && frame.flags == 0U && frame.dlc == expected.size();
	for (std::size_t i = 0; valid && i < expected.size(); ++i) {
		valid = frame.data[i] == expected[i];
	}
	LOG_INF("RX id=0x%x dlc=%u %s", frame.id, frame.dlc, valid ? "PASS" : "FAIL");
	return valid;
}

int InstallFilter(const device* can, std::uint32_t id)
{
	const can_filter filter{.id = id, .mask = CAN_STD_ID_MASK, .flags = 0U};
	const int filterId = can_add_rx_filter_msgq(can, &RxQueue, &filter);
	if (filterId < 0) {
		LOG_ERR("Failed to install filter for 0x%x: %d", id, filterId);
	}
	return filterId;
}

bool SendFrame(const device* can, std::uint32_t id,
	       const std::array<std::uint8_t, CAN_MAX_DLC>& data)
{
	can_frame frame{.id = id, .dlc = CAN_MAX_DLC, .flags = 0U, .data = {}};
	for (std::size_t i = 0; i < data.size(); ++i) {
		frame.data[i] = data[i];
	}
	const int ret = can_send(can, &frame, K_SECONDS(2), nullptr, nullptr);
	if (ret != 0) {
		can_state state{};
		can_bus_err_cnt errors{};
		(void)can_get_state(can, &state, &errors);
		LOG_ERR("TX id=0x%x failed: %d state=%d TEC=%u REC=%u", id, ret,
			static_cast<int>(state), errors.tx_err_cnt, errors.rx_err_cnt);
		return false;
	}
	LOG_INF("TX id=0x%x PASS", id);
	return true;
}
}

int main()
{
	const device* can = DEVICE_DT_GET(DT_NODELABEL(can1));
	if (!device_is_ready(can)) {
		LOG_ERR("CAN1 device is not ready");
		return 1;
	}

#if defined(B2B_TEST_ROLE_TX)
	constexpr std::uint32_t rxId = ResponseId;
#else
	constexpr std::uint32_t rxId = RequestId;
#endif
	const int filterId = InstallFilter(can, rxId);
	if (filterId < 0) {
		return 1;
	}
	int ret = can_set_mode(can, CAN_MODE_NORMAL);
	if (ret == 0) {
		ret = can_start(can);
	}
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("Failed to start CAN1 in normal mode: %d", ret);
		can_remove_rx_filter(can, filterId);
		return 1;
	}
	LOG_INF("CAN1 normal mode ready, role=%s",
#if defined(B2B_TEST_ROLE_TX)
		"tx");
#else
		"rx");
#endif

#if defined(B2B_TEST_ROLE_TX)
	k_sleep(K_SECONDS(2));
	const bool passed = SendFrame(can, RequestId, RequestData) &&
			    ReceiveAndCheck(ResponseId, ResponseData);
#else
	const bool received = ReceiveAndCheck(RequestId, RequestData);
	const bool passed = received && SendFrame(can, ResponseId, ResponseData);
#endif
	LOG_INF("RAW CAN SUMMARY: %s", passed ? "PASS" : "FAIL");
	can_remove_rx_filter(can, filterId);
	return passed ? 0 : 1;
}
