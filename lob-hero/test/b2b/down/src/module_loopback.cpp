#include <app/b2b/module.hpp>
#include <app/b2b/msg.hpp>
#include <breeze/b2b/proto.hpp>
#include <breeze/models/device.hpp>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <etl/array.h>
#include <zephyr/canbus/isotp.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(b2b_down_module_loopback, LOG_LEVEL_INF);

namespace
{

constexpr std::uint16_t UP_ID = 0x30;
constexpr std::uint16_t DOWN_ID = 0x31;
constexpr std::size_t PEER_RX_SIZE = 32;

const device* Can = DEVICE_DT_GET(DT_NODELABEL(can1));
isotp_recv_ctx PeerRecvCtx{};
isotp_send_ctx PeerSendCtx{};
K_SEM_DEFINE(PeerReceiverReady, 0, 1);
K_SEM_DEFINE(PeerReceiveDone, 0, 1);
K_THREAD_STACK_DEFINE(PeerReceiverStack, 2048);
k_thread PeerReceiverThread{};
etl::array<std::uint8_t, PEER_RX_SIZE> PeerRxData{};
int PeerRxLength = ISOTP_RECV_TIMEOUT;

const isotp_msg_id PeerRxAddr{
	.std_id = UP_ID,
	.flags = ISOTP_MSG_EXT_ADDR,
};
const isotp_msg_id PeerTxAddr{
	.std_id = DOWN_ID,
	.flags = ISOTP_MSG_EXT_ADDR,
};
const isotp_fc_opts FcOpts{
	.bs = 8,
	.stmin = 5,
};

void PeerReceiver(void*, void*, void*)
{
	k_sem_give(&PeerReceiverReady);
	std::size_t total = 0;
	while (total < PeerRxData.size()) {
		const auto received = isotp_recv(&PeerRecvCtx, PeerRxData.data() + total,
						 PeerRxData.size() - total, K_SECONDS(5));
		if (received <= 0) {
			PeerRxLength = received;
			k_sem_give(&PeerReceiveDone);
			return;
		}
		total += static_cast<std::size_t>(received);
		if (total >= sizeof(std::uint8_t) + sizeof(app::b2b::MotorControl)) {
			break;
		}
	}
	PeerRxLength = static_cast<int>(total);
	k_sem_give(&PeerReceiveDone);
}

} // namespace

int main()
{
	if (!device_is_ready(Can)) {
		LOG_ERR("CAN device is not ready");
		return 0;
	}
	int ret = can_set_mode(Can, CAN_MODE_LOOPBACK);
	if (ret != 0) {
		LOG_ERR("can_set_mode(loopback) failed: %d", ret);
		return 0;
	}
	ret = can_start(Can);
	if (ret != 0 && ret != -EALREADY) {
		LOG_ERR("can_start failed: %d", ret);
		return 0;
	}

	ret = isotp_bind(&PeerRecvCtx, Can, &PeerRxAddr, &PeerTxAddr, &FcOpts, K_SECONDS(1));
	if (ret != 0) {
		LOG_ERR("Peer isotp_bind failed: %d", ret);
		return 0;
	}

	static app::b2b::DownModule b2b{Can};
	b2b.Start();

	breeze::b2b::Encoder<app::b2b::CastMessages> encoder;
	etl::array<std::byte, 32> txBuffer{};
	const app::b2b::ImuReport imuReport{
		.Id = app::models::ImuId::Head,
		.Yaw = 1000,
		.Roll = -250,
		.Pitch = 375,
	};
	const auto imuPayload = encoder.Encode(imuReport, txBuffer);
	ret = isotp_send(&PeerSendCtx, Can,
			 reinterpret_cast<const std::uint8_t*>(imuPayload.data()), imuPayload.size(),
			 &PeerTxAddr, &PeerRxAddr, nullptr, nullptr);
	LOG_INF("Peer IMU send result: %d", ret);
	isotp_unbind(&PeerRecvCtx);
	k_sleep(K_MSEC(100));
	ret = isotp_send(&PeerSendCtx, Can,
			 reinterpret_cast<const std::uint8_t*>(imuPayload.data()), imuPayload.size(),
			 &PeerTxAddr, &PeerRxAddr, nullptr, nullptr);
	LOG_INF("Peer IMU send without overlapping RX filter: %d", ret);
	k_sleep(K_MSEC(100));

	breeze::models::ImuAngularReport receivedImu{};
	b2b.Out.Imus[app::models::ImuId::Head](receivedImu);
	const bool imuPass = receivedImu.Yaw.Value == 10.0F &&
			     receivedImu.Roll.Value == -2.5F &&
			     receivedImu.Pitch.Value == 3.75F;
	LOG_INF("DownModule IMU: yaw=%.2f roll=%.2f pitch=%.2f: %s",
		static_cast<double>(receivedImu.Yaw.Value), static_cast<double>(receivedImu.Roll.Value),
		static_cast<double>(receivedImu.Pitch.Value),
		imuPass ? "PASS" : "FAIL");

	ret = isotp_bind(&PeerRecvCtx, Can, &PeerRxAddr, &PeerTxAddr, &FcOpts, K_SECONDS(1));
	if (ret != 0) {
		LOG_ERR("Peer rebind failed: %d", ret);
		return 0;
	}
	k_thread_create(&PeerReceiverThread, PeerReceiverStack, K_THREAD_STACK_SIZEOF(PeerReceiverStack),
			PeerReceiver, nullptr, nullptr, nullptr, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);
	(void)k_sem_take(&PeerReceiverReady, K_SECONDS(1));
	const breeze::models::MotorControl control{.Torque = {0.5F}};
	b2b.In.Motors[app::models::MotorId::HeadPitch](control);
	ret = k_sem_take(&PeerReceiveDone, K_SECONDS(6));
	const bool motorPass = ret == 0 && PeerRxLength == 4 &&
			PeerRxData[0] == app::b2b::MotorControl::ID &&
			PeerRxData[1] == static_cast<std::uint8_t>(app::models::MotorId::HeadPitch) &&
			PeerRxData[2] == 0x00 && PeerRxData[3] == 0x40;
	LOG_INF("DownModule MotorControl RX length=%d: %s", PeerRxLength,
		motorPass ? "PASS" : "FAIL");

	k_sleep(K_SECONDS(1));
	isotp_unbind(&PeerRecvCtx);
	return 0;
}
