#include <cstddef>
#include <cstdint>
#include <zephyr/canbus/isotp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_down_isotp_loopback, LOG_LEVEL_INF);

namespace
{
isotp_recv_ctx RecvCtx{};
isotp_recv_ctx ReverseRecvCtx{};
isotp_send_ctx SendCtx{};

bool RunCase(const device* can, const char* name, std::size_t length, std::uint8_t flags,
             std::uint8_t blockSize, bool reverseBound, bool separateFc, bool useNet)
{
	RecvCtx = {};
	ReverseRecvCtx = {};
	SendCtx = {};
	const isotp_msg_id dataAddr{.std_id = 0x31, .flags = flags};
	const isotp_msg_id reverseAddr{.std_id = 0x30, .flags = flags};
	isotp_msg_id fcAddr = reverseAddr;
	if (separateFc) {
		fcAddr.std_id = 0x130;
	}
	const isotp_fc_opts opts{.bs = blockSize, .stmin = 5};
	int ret = isotp_bind(&RecvCtx, can, &dataAddr, &fcAddr, &opts, K_SECONDS(1));
	if (ret != 0) {
		LOG_ERR("%s: bind=%d", name, ret);
		return false;
	}
	if (reverseBound) {
		ret = isotp_bind(&ReverseRecvCtx, can, &reverseAddr, &dataAddr, &opts, K_SECONDS(1));
		if (ret != 0) {
			LOG_ERR("%s: reverse bind=%d", name, ret);
			isotp_unbind(&RecvCtx);
			return false;
		}
	}
	std::uint8_t payload[32]{};
	std::uint8_t received[32]{};
	for (std::size_t i = 0; i < length; ++i) {
		payload[i] = static_cast<std::uint8_t>(i + 0x40);
	}
	// All test messages fit in the RX pool and can be drained after synchronous send.
	ret = isotp_send(&SendCtx, can, payload, length, &dataAddr, &fcAddr, nullptr, nullptr);
	const int sendResult = ret;
	bool valid = ret == 0;
	std::size_t total = 0;
	while (valid && total < length) {
		if (useNet) {
			net_buf* buffer = nullptr;
			ret = isotp_recv_net(&RecvCtx, &buffer, K_SECONDS(2));
			if (ret < 0) {
				valid = false;
				break;
			}
			const auto chunk = net_buf_frags_len(buffer);
			const auto copied = net_buf_linearize(received + total, length - total, buffer, 0, chunk);
			LOG_INF("%s: chunk=%u remaining=%d", name, static_cast<unsigned int>(chunk), ret);
			net_buf_unref(buffer);
			total += copied;
			valid = copied == chunk && total <= length && static_cast<std::size_t>(ret) == length - total;
			if (ret == 0) {
				break;
			}
		} else {
			ret = isotp_recv(&RecvCtx, received + total, length - total, K_SECONDS(2));
			LOG_INF("%s: recv chunk=%d", name, ret);
			if (ret <= 0) {
				valid = false;
				break;
			}
			total += static_cast<std::size_t>(ret);
		}
	}
	valid = valid && total == length;
	for (std::size_t i = 0; valid && i < length; ++i) {
		valid = received[i] == payload[i];
	}
	const bool expectTimeout = reverseBound && !separateFc && length > 7;
	const bool passed = expectTimeout ? sendResult == ISOTP_N_TIMEOUT_BS : valid;
	LOG_INF("%s: send=%d bytes=%u/%u %s%s", name, sendResult,
		static_cast<unsigned int>(total), static_cast<unsigned int>(length),
		passed ? "PASS" : "FAIL", expectTimeout ? " (expected FC timeout)" : "");
	// Allow timeout work to finish before reusing the contexts in the next case.
	k_sleep(K_MSEC(100));
	if (reverseBound) {
		isotp_unbind(&ReverseRecvCtx);
	}
	isotp_unbind(&RecvCtx);
	return passed;
}
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
	// Delay lets the host attach RTT before diagnostics fill its ring buffer.
	k_sleep(K_SECONDS(2));
	unsigned int passed = 0;
	passed += RunCase(can, "normal-single", 4, 0, 8, false, false, false);
	passed += RunCase(can, "extended-multi", 32, ISOTP_MSG_EXT_ADDR, 8, false, false, false);
	passed += RunCase(can, "normal-multi", 32, 0, 8, false, false, false);
	passed += RunCase(can, "extended-bs0", 32, ISOTP_MSG_EXT_ADDR, 0, false, false, false);
	passed += RunCase(can, "overlap-extended", 14, ISOTP_MSG_EXT_ADDR, 8, true, false, false);
	passed += RunCase(can, "overlap-normal", 14, 0, 8, true, false, false);
	passed += RunCase(can, "separate-fc-extended", 14, ISOTP_MSG_EXT_ADDR, 8, true, true, true);
	passed += RunCase(can, "separate-fc-normal", 32, 0, 8, true, true, true);
	LOG_INF("RAW LOOPBACK SUMMARY: %u/8 cases matched expectations", passed);
	return 0;
}
