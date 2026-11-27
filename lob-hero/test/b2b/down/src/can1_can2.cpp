/* SPDX-FileCopyrightText: Copyright (c) 2026 RobotPilots-SZU */
/* SPDX-License-Identifier: Apache-2.0 */

#include <array>
#include <cstdint>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_can1_can2_test, LOG_LEVEL_INF);

namespace
{
constexpr std::uint32_t Can1ToCan2Id = 0x341U;
constexpr std::uint32_t Can2ToCan1Id = 0x342U;
constexpr k_timeout_t RxTimeout = K_SECONDS(3);
CAN_MSGQ_DEFINE(Can1RxQueue, 2);
CAN_MSGQ_DEFINE(Can2RxQueue, 2);

constexpr std::array<std::uint8_t, CAN_MAX_DLC> Can1ToCan2Data{0xA1, 1, 2, 3, 4, 5, 6, 7};
constexpr std::array<std::uint8_t, CAN_MAX_DLC> Can2ToCan1Data{0xA2, 1, 2, 3, 4, 5, 6, 7};

int InstallFilter(const device* can, k_msgq* queue, std::uint32_t id)
{
	const can_filter filter{.id = id, .mask = CAN_STD_ID_MASK, .flags = 0U};
	const int filterId = can_add_rx_filter_msgq(can, queue, &filter);
	if (filterId < 0) {
		LOG_ERR("CAN %s: failed to install filter 0x%x: %d", can->name, id, filterId);
	}
	return filterId;
}

bool ReceiveAndCheck(k_msgq* queue, std::uint32_t id,
		     const std::array<std::uint8_t, CAN_MAX_DLC>& expected)
{
	can_frame frame{};
	const int ret = k_msgq_get(queue, &frame, RxTimeout);
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

bool SendFrame(const device* can, std::uint32_t id,
	       const std::array<std::uint8_t, CAN_MAX_DLC>& data)
{
	can_frame frame{.id = id, .dlc = CAN_MAX_DLC, .flags = 0U, .data = {}};
	for (std::size_t i = 0; i < data.size(); ++i) {
		frame.data[i] = data[i];
	}
	const int ret = can_send(can, &frame, K_SECONDS(1), nullptr, nullptr);
	if (ret != 0) {
		can_state state{};
		can_bus_err_cnt errors{};
		const int stateRet = can_get_state(can, &state, &errors);
		if (stateRet == 0) {
			LOG_ERR("CAN %s TX 0x%x failed: %d state=%d TEC=%u REC=%u", can->name, id,
				ret, static_cast<int>(state), errors.tx_err_cnt, errors.rx_err_cnt);
		} else {
			LOG_ERR("CAN %s TX 0x%x failed: %d; state read failed: %d", can->name, id,
				ret, stateRet);
		}
		return false;
	}
	LOG_INF("CAN %s TX id=0x%x PASS", can->name, id);
	return true;
}

int StartNormalOneShot(const device* can)
{
	int ret = can_set_mode(can, CAN_MODE_NORMAL | CAN_MODE_ONE_SHOT);
	if (ret == 0) {
		ret = can_start(can);
	}
	if (ret == -EALREADY) {
		ret = 0;
	}
	if (ret != 0) {
		LOG_ERR("CAN %s failed to start in normal one-shot mode: %d", can->name, ret);
	}
	return ret;
}
}

int main()
{
	const device* can1 = DEVICE_DT_GET(DT_NODELABEL(can1));
	const device* can2 = DEVICE_DT_GET(DT_NODELABEL(can2));
	if (!device_is_ready(can1) || !device_is_ready(can2)) {
		LOG_ERR("CAN1 or CAN2 device is not ready");
		return 1;
	}

	const int can1Filter = InstallFilter(can1, &Can1RxQueue, Can2ToCan1Id);
	if (can1Filter < 0) {
		return 1;
	}
	const int can2Filter = InstallFilter(can2, &Can2RxQueue, Can1ToCan2Id);
	if (can2Filter < 0) {
		can_remove_rx_filter(can1, can1Filter);
		return 1;
	}
	int ret = StartNormalOneShot(can1);
	if (ret == 0) {
		ret = StartNormalOneShot(can2);
	}
	if (ret != 0) {
		can_remove_rx_filter(can2, can2Filter);
		can_remove_rx_filter(can1, can1Filter);
		return 1;
	}
	LOG_INF("CAN1/CAN2 ready: 1 Mbit/s expected, normal one-shot mode");
	k_sleep(K_SECONDS(2));

	const bool requestSent = SendFrame(can1, Can1ToCan2Id, Can1ToCan2Data);
	const bool requestReceived = requestSent &&
				     ReceiveAndCheck(&Can2RxQueue, Can1ToCan2Id, Can1ToCan2Data);
	const bool responseSent = requestReceived &&
				  SendFrame(can2, Can2ToCan1Id, Can2ToCan1Data);
	const bool responseReceived = responseSent &&
				      ReceiveAndCheck(&Can1RxQueue, Can2ToCan1Id, Can2ToCan1Data);
	const bool passed = requestSent && requestReceived && responseSent && responseReceived;

	LOG_INF("CAN1/CAN2 SUMMARY: %s", passed ? "PASS" : "FAIL");
	can_remove_rx_filter(can2, can2Filter);
	can_remove_rx_filter(can1, can1Filter);
	return passed ? 0 : 1;
}
