#include <cstddef>
#include <cstdint>
#include <zephyr/canbus/isotp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_down_isotp_loopback, LOG_LEVEL_INF);

namespace
{
isotp_recv_ctx RecvCtx{};
isotp_send_ctx SendCtx{};
}

int main()
{
	const auto* can = DEVICE_DT_GET(DT_NODELABEL(can1));
	if (!device_is_ready(can)) {
		LOG_ERR("CAN device is not ready");
		return 0;
	}
	int ret = can_set_mode(can, CAN_MODE_LOOPBACK);
	if (ret != 0) {
		LOG_ERR("can_set_mode failed: %d", ret);
		return 0;
	}
	ret = can_start(can);
	if (ret != 0) {
		LOG_ERR("can_start failed: %d", ret);
		return 0;
	}
	const isotp_msg_id dataAddr{.std_id = 0x31, .flags = ISOTP_MSG_EXT_ADDR};
	const isotp_msg_id fcAddr{.std_id = 0x30, .flags = ISOTP_MSG_EXT_ADDR};
	const isotp_fc_opts opts{.bs = 8, .stmin = 5};
	ret = isotp_bind(&RecvCtx, can, &dataAddr, &fcAddr, &opts, K_SECONDS(1));
	if (ret != 0) {
		LOG_ERR("isotp_bind failed: %d", ret);
		return 0;
	}
	std::uint8_t payload[32]{};
	std::uint8_t received[32]{};
	for (std::size_t i = 0; i < sizeof(payload); ++i) {
		payload[i] = static_cast<std::uint8_t>(i);
	}
	// This payload fits in the configured RX pool, so it can be drained after sending.
	ret = isotp_send(&SendCtx, can, payload, sizeof(payload), &dataAddr, &fcAddr, nullptr, nullptr);
	LOG_INF("isotp_send result: %d", ret);
	bool valid = ret == 0;
	std::size_t total = 0;
	while (valid && total < sizeof(received)) {
		ret = isotp_recv(&RecvCtx, received + total, sizeof(received) - total, K_SECONDS(2));
		LOG_INF("isotp_recv chunk: %d", ret);
		if (ret <= 0) {
			valid = false;
			break;
		}
		total += static_cast<std::size_t>(ret);
	}
	for (std::size_t i = 0; valid && i < sizeof(payload); ++i) {
		valid = received[i] == payload[i];
	}
	LOG_INF("RX result=%u, payload=%s", static_cast<unsigned int>(total), valid ? "PASS" : "FAIL");
	isotp_unbind(&RecvCtx);
	return 0;
}
