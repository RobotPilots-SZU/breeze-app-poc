# B2B 板上诊断

down 测试通过 `B2B_TEST_MODE` 选择入口，默认 `board` 运行双板接收测试。
内部回环使用 `CAN_MODE_LOOPBACK`，无需上板；它不验证外部收发器和板间接线。

```sh
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-raw -- -DB2B_TEST_MODE=raw_loopback
west flash -d build/b2b-down-raw -r openocd

west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-module -- -DB2B_TEST_MODE=module_loopback
west flash -d build/b2b-down-module -r openocd
```

`raw_loopback` 使用原始 ISO-TP API 发送 32 字节递增数据，保留传输封装相同的
扩展寻址、`bs=8` 和 `stmin=5`，循环读取分段并校验全部字节。

`module_loopback` 实例化 `DownModule`，原始 ISO-TP 端点模拟上板：

1. 保留模拟上板的普通接收绑定，发送 14 字节 IMU，观察 FC 是否被重叠过滤器截走。
2. 移除模拟上板接收绑定，等待前次接收超时清理后再次发送，读取模块 IMU 缓存。
3. 重新绑定模拟上板接收端，触发模块发送 MotorControl，校验全部 4 字节。

这是故障复现测试，当前实现的前两个阶段会输出失败。

## 实板结果（2026-11-26，rm_typec / STM32F407）

原始 API：发送返回 0；接收分两次返回 5、27 字节；32 字节内容校验通过。

模块：保留普通接收绑定时发送返回 -2（FC 超时）；移除绑定后发送返回 0，
但缓存仍为零；MotorControl 单帧接收通过。

对应两项传输层问题：

- `Transmitter` 的普通数据接收和发送时 FC 接收使用同一个自身 CAN ID。
  bxCAN 不保证重叠过滤器的回调都收到帧，普通接收上下文可能先收到 FC 并将其丢弃。
  模拟对端测试复现了这一条件；真实上板的传输实例也有相同绑定方式。
- `bs=8` 时 `isotp_recv()` 可能返回 First Frame 数据和后续块，
  `Transmitter::Receive()` 当前将每段直接交给要求完整长度的 Decoder。
  需要使用 `isotp_recv_net()` 的剩余长度信息重组完整报文，或使用 `bs=0`
  并确保接收池足以容纳允许的最大报文。

后续修复 FC 应采用互不重叠的数据/FC CAN ID，或统一接收过滤器后按 PCI 分发。
不能单靠增加超时时间解决。两板的地址方案需要同步修改。

已有 `breeze-modules/test/src/b2b/test.cpp` 覆盖编解码，未覆盖真实 CAN/ISO-TP 传输。

## 无交互终端的 RTT

`west rtt` 的终端客户端依赖 TTY。可直接启动 OpenOCD RTT 服务：

```sh
openocd -f externals/breeze/boards/dji/rm_typec/support/openocd.cfg \
  -c init -c 'reset run' \
  -c 'rtt setup 0x20000000 0x20000 {SEGGER RTT}' \
  -c 'rtt start' -c 'rtt server start 5555 0'
```

然后连接 TCP 5555 读取日志。RTT 控制块搜索 ID 应为 `SEGGER RTT`，不是 `0`。
烧录前结束该 OpenOCD 实例，避免多进程同时占用 J-Link。
