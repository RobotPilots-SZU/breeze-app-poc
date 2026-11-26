# B2B 板上诊断

down 测试通过 `B2B_TEST_MODE` 选择入口，默认 `board` 运行双板接收测试。
内部回环使用 `CAN_MODE_LOOPBACK`，无需上板；它不验证外部收发器和板间接线。

```sh
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-raw -- -DB2B_TEST_MODE=raw_loopback
west flash -d build/b2b-down-raw -r openocd

west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-module -- -DB2B_TEST_MODE=module_loopback
west flash -d build/b2b-down-module -r openocd
```

`raw_loopback` 仅使用原始 ISO-TP API，覆盖普通/扩展寻址、单帧/多帧、
`bs=8`/`bs=0`、数据与 FC 过滤器重叠及独立 FC ID。发送使用同步 API，
4/14/32 字节报文均能放入配置的接收池，发送完成后再读取。
`isotp_recv()` 用例按已知测试长度收齐数据；`isotp_recv_net()` 用例按剩余长度
判断报文边界，并校验内容。普通/扩展寻址均使用标准 11 位 CAN ID；扩展地址字节为 0。

启动后等待两秒供主机连接 RTT，最后输出 `RAW LOOPBACK SUMMARY`。
其中两个重叠过滤器用例预期超时，它们的 `PASS (expected FC timeout)`
表示故障按预期复现，不表示该配置能正常传输。

`module_loopback` 实例化 `DownModule`，原始 ISO-TP 端点模拟上板：

1. 保留模拟上板的普通接收绑定，发送 14 字节 IMU，观察 FC 是否被重叠过滤器截走。
2. 移除模拟上板接收绑定，等待前次接收超时清理后再次发送，读取模块 IMU 缓存。
3. 重新绑定模拟上板接收端，触发模块发送 MotorControl，校验全部 4 字节。

这是故障复现测试，当前实现的前两个阶段会输出失败。

## 实板结果（2026-11-26，rm_typec / STM32F407）

原始 API 对照测试：8/8 个用例符合预期，其中 6 个成功传输，2 个复现预期超时。

| 用例 | 结果 |
| --- | --- |
| 普通寻址、4 字节单帧 | 返回 0，内容通过 |
| 扩展寻址、32 字节、bs=8 | 返回 0，接收 5 + 27 字节，内容通过 |
| 普通寻址、32 字节、bs=8 | 返回 0，接收 6 + 26 字节，内容通过 |
| 扩展寻址、32 字节、bs=0 | 返回 0，一次接收 32 字节，内容通过 |
| 扩展寻址、14 字节、反向 RX 与 FC 同 ID | 返回 -2，FC 超时 |
| 普通寻址、14 字节、反向 RX 与 FC 同 ID | 返回 -2，FC 超时 |
| 扩展寻址、14 字节、反向 RX 保留、独立 FC ID | 返回 0，接收 5（剩余 9）+ 9（剩余 0），内容通过 |
| 普通寻址、32 字节、反向 RX 保留、独立 FC ID | 返回 0，接收 6（剩余 26）+ 26（剩余 0），内容通过 |

重叠用例将数据绑定到 `0x31`、反向接收绑定到 `0x30`，发送端在 `0x30`
等待 FC。独立 FC 用例只将 FC 的发送/接收 ID 改为 `0x130`，保留两个普通
接收绑定。该对照在原始 API 层复现并消除了超时，支持过滤器冲突的诊断。
当前结果不显示 ISO-TP 基础分帧/流控功能故障；仍需另行验证真实双板链路。

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
