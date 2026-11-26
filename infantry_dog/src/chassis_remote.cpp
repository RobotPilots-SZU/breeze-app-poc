#include "chassis_remote.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(chassis_remote, LOG_LEVEL_INF);

namespace breeze::chassis::remote
{
namespace
{
constexpr std::size_t FRAME_SIZE = 18;       // DT7/DBUS 一帧 18 字节
constexpr std::uint32_t FRAME_GAP_MS = 5;    // 帧间空闲超过 5ms 即认为新帧开始
constexpr std::size_t FRAME_QUEUE_DEPTH = 4; // 14ms 一帧，4 个槽够线程从容处理
constexpr std::size_t THREAD_STACK_SIZE = 1024;
constexpr std::uint8_t THREAD_PRIO = 7;
constexpr float CHANNEL_MAX = 660.0F; // 通道满量程 ±660
constexpr float MAX_VX_SPEED = 40.0F;
constexpr float MAX_VY_SPEED = 40.0F;

const struct device* remoteDevice;
const rc_sensor_t* sensorPointer;
const rc_parser_ops_t* parserOps;
FrameCallback frameCallback;
void* frameCallbackData;

// 拼帧状态，仅中断使用
std::array<std::uint8_t, FRAME_SIZE> rxFrame{};
std::size_t rxFrameLength;
std::uint32_t lastRxTimestamp;

// 拼好的整帧交给线程解析
K_MSGQ_DEFINE(rxFrameQueue, FRAME_SIZE, FRAME_QUEUE_DEPTH, 1);
K_THREAD_STACK_DEFINE(rxThreadStack, THREAD_STACK_SIZE);
struct k_thread rxThread;

/**
 * @brief 取 DT7 设备
 *
 * @note devicetree 里没有 remote_dt7 节点时（例如 native_sim）返回空
 *
 * @return 设备指针；没有节点返回 nullptr
 */
const struct device* ResolveDevice()
{
#if DT_NODE_EXISTS(DT_NODELABEL(remote_dt7))
	return DEVICE_DT_GET(DT_NODELABEL(remote_dt7));
#else
	return nullptr;
#endif
}

/**
 * @brief UART 字节回调（中断上下文）：拼满一帧就丢给线程
 *
 */
void RxCallback(const struct device* /*dev*/,
                remote_rx_event_t event,
                const std::uint8_t* data,
                std::size_t length,
                void* /*userData*/
)
{
	if (event == REMOTE_RX_RESET)
	{
		rxFrameLength = 0;
		return;
	}
	if (event != REMOTE_RX_DATA)
	{
		return;
	}

	// DT7 无帧头无 CRC，只能靠帧间隙重新对齐
	const std::uint32_t now = k_uptime_get_32();
	if (now - lastRxTimestamp >= FRAME_GAP_MS)
	{
		rxFrameLength = 0;
	}
	lastRxTimestamp = now;

	for (std::size_t i = 0; i < length; ++i)
	{
		rxFrame[rxFrameLength] = data[i];
		++rxFrameLength;
		if (rxFrameLength == rxFrame.size())
		{
			rxFrameLength = 0;
			k_msgq_put(&rxFrameQueue, rxFrame.data(), K_NO_WAIT);
		}
	}
}

/**
 * @brief 解析线程：把整帧交给驱动解析器，然后回调使用方
 *
 */
void RxThreadEntry(void* /*p1*/, void* /*p2*/, void* /*p3*/)
{
	std::array<std::uint8_t, FRAME_SIZE> frame{};

	for (;;)
	{
		k_msgq_get(&rxFrameQueue, frame.data(), K_FOREVER);

		// 驱动的解析器直接写 sensorPointer->info
		parserOps->parse(frame.data(), sensorPointer->info);

		if (frameCallback != nullptr)
		{
			frameCallback(sensorPointer, frameCallbackData);
		}
	}
}

} // namespace

const rc_sensor_t* Init(FrameCallback callback, void* userData)
{
	if (remoteDevice != nullptr)
	{
		return sensorPointer;
	}

	const struct device* device = ResolveDevice();
	if (device == nullptr || !device_is_ready(device))
	{
		LOG_ERR("remote device not ready");
		return nullptr;
	}

	rc_sensor_t* sensor = remote_get_sensor(device);
	const rc_parser_ops_t* ops = remote_get_parser_ops(device);
	if (sensor == nullptr || ops == nullptr || ops->parse == nullptr)
	{
		LOG_ERR("device has no sensor / parser ops");
		return nullptr;
	}

	sensorPointer = sensor;
	parserOps = ops;
	frameCallback = callback;
	frameCallbackData = userData;
	lastRxTimestamp = k_uptime_get_32();

	k_thread_create(&rxThread, rxThreadStack, K_THREAD_STACK_SIZEOF(rxThreadStack), RxThreadEntry, nullptr, nullptr,
	                nullptr, THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(&rxThread, "chassis_remote");

	remote_set_rx_cb(device, RxCallback, nullptr);
	remoteDevice = device;

	LOG_INF("remote registered");
	return sensorPointer;
}

Control::Target TargetFrom(const rc_sensor_info_t& info)
{
	Control::Target target{};
	target.Vx = static_cast<float>(info.ch0) / CHANNEL_MAX * MAX_VX_SPEED;
	target.Vy = static_cast<float>(info.ch1) / CHANNEL_MAX * MAX_VY_SPEED;
	return target;
}

} // namespace breeze::chassis::remote
