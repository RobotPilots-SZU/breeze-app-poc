# B2B 板上诊断

down 测试通过 `B2B_TEST_MODE` 选择入口，默认 `board` 运行双板接收测试。
内部回环使用 `CAN_MODE_LOOPBACK`，无需上板；它不验证外部收发器和板间接线。

```sh
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-raw -- -DB2B_TEST_MODE=raw_loopback
west flash -d build/b2b-down-raw -r openocd

west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-down-module -- -DB2B_TEST_MODE=module_loopback
west flash -d build/b2b-down-module -r openocd
```

## 双板原始 CAN 与 ISO-TP

`raw_can` 使用 0x321/0x322 两个标准 CAN ID 做 8 字节请求/响应；`raw_isotp`
使用独立数据和 FC ID 做 20 字节请求/响应，覆盖多帧、FC 和连续帧。两个模式均显式
使用 CAN normal mode。`B2B_TEST_ROLE` 选择 `tx` 或 `rx`，应先启动 rx 板，再启动 tx 板。

```sh
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-raw-can-rx -- \
  -DB2B_TEST_MODE=raw_can -DB2B_TEST_ROLE=rx
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-raw-can-tx -- \
  -DB2B_TEST_MODE=raw_can -DB2B_TEST_ROLE=tx

west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-raw-isotp-rx -- \
  -DB2B_TEST_MODE=raw_isotp -DB2B_TEST_ROLE=rx
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx -d build/b2b-raw-isotp-tx -- \
  -DB2B_TEST_MODE=raw_isotp -DB2B_TEST_ROLE=tx
```

测试时分别将 `rx` 和 `tx` 对应构建产物烧录到两块板；本轮只要求编译，不会自动烧录。
raw CAN 的发送使用同步 `can_send()`，返回 0 表示该帧被确认发送；ISO-TP 测试先发送
20 字节请求，再由接收端回复 20 字节响应。两种测试都输出 PASS/FAIL 汇总。

## 单板 CAN1 到 CAN2

`can1_can2` 在同一块 down 板上以 CAN1 发送请求、CAN2 回复，做双向 8 字节原始帧校验。
测试使用 normal one-shot 模式，缺少 ACK 时不会无限自动重发。运行前需将板上 CAN1 与
CAN2 的总线端子连接：CAN1_H 接 CAN2_H、CAN1_L 接 CAN2_L，并确认两路收发器均正常。

```sh
west build lob-hero/test/b2b/down -b rm_typec/stm32f407xx \
  -d build/b2b-can1-can2 -- -DB2B_TEST_MODE=can1_can2
```

CAN1 使用 PD0/PD1，CAN2 使用 PB5/PB6；板级 DTS 均配置为 1 Mbit/s。
此测试验证控制器到控制器的板外收发器/接线链路，不是 MCU 内部 loopback。

`raw_loopback` 仅使用原始 ISO-TP API，覆盖普通/扩展寻址、单帧/多帧、
`bs=8`/`bs=0`、数据与 FC 过滤器重叠及独立 FC ID。发送使用同步 API，
4/14/32 字节报文均能放入配置的接收池，发送完成后再读取。
`isotp_recv()` 用例按已知测试长度收齐数据；`isotp_recv_net()` 用例按剩余长度
判断报文边界，并校验内容。普通/扩展寻址均使用标准 11 位 CAN ID；扩展地址字节为 0。

启动后等待两秒供主机连接 RTT，最后输出 `RAW LOOPBACK SUMMARY`。
其中两个重叠过滤器用例预期超时，它们的 `PASS (expected FC timeout)`
表示故障按预期复现，不表示该配置能正常传输。

`module_loopback` 实例化 `DownModule`，原始 ISO-TP 端点模拟上板：

1. 保留模拟上板的普通接收绑定，使用独立 FC ID 发送 14 字节 IMU。
2. 读取模块 IMU 缓存，校验三个姿态分量及三个角速度分量。
3. 触发模块发送 MotorControl，模拟上板接收并校验全部 4 字节。

正常结果为 `MODULE LOOPBACK SUMMARY: PASS`。

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

修复前模块：保留普通接收绑定时发送返回 -2（FC 超时）；移除绑定后发送返回 0，
但缓存仍为零；MotorControl 单帧接收通过。

已修复的两项传输层问题：

- 原 `Transmitter` 的普通数据接收和发送时 FC 接收使用同一个自身 CAN ID。
  bxCAN 不保证重叠过滤器的回调都收到帧，普通接收上下文可能先收到 FC 并将其丢弃。
  模拟对端测试复现了这一条件；真实上板的传输实例也有相同绑定方式。
- `bs=8` 时 `isotp_recv()` 可能返回 First Frame 数据和后续块，
  原 `Transmitter::Receive()` 将每段直接交给要求完整长度的 Decoder。
  现保留 `bs=8`，使用 `isotp_recv_net()` 的剩余长度信息重组完整报文后再解码。

两板现在使用以下地址；上下板固件需同步更新：

| 方向 | 数据 CAN ID | FC CAN ID |
| --- | --- | --- |
| up → down | 0x31 | 0x130 |
| down → up | 0x30 | 0x131 |

修复后实板模块回环结果：

```text
Peer IMU send result: 0
DownModule IMU: yaw=10.00 roll=-2.50 pitch=3.75: PASS
DownModule MotorControl RX length=4: PASS
MODULE LOOPBACK SUMMARY: PASS
```

两端普通接收绑定始终保留；IMU 六个字段全部校验通过。
收发线程在传输实例完成启动和 ISO-TP 绑定后才启动。

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

## 双板 FC 超时诊断

```sh
west build lob-hero/test/b2b/up -d build/up -- -DEXTRA_CONF_FILE=../diagnostic.conf
west build lob-hero/test/b2b/down -d build/down -- -DB2B_TEST_MODE=board -DEXTRA_CONF_FILE=../diagnostic.conf
```

分别连接对应板子的调试器后，使用 `west flash -d build/up -r openocd` 和
`west flash -d build/down -r openocd` 烧录。收集两端启动至第一次超时的日志。
关闭诊断配置时重新构建并传入 `-DEXTRA_CONF_FILE=`。

启动时应看到 `mode=0x0`、`initial-bitrate=1000000`，以及下表地址：

| 板 | rx-data | tx-fc | tx-data | rx-fc |
| --- | --- | --- | --- | --- |
| up | 0x30 | 0x131 | 0x31 | 0x130 |
| down | 0x31 | 0x130 | 0x30 | 0x131 |

本测试明确使用 CAN1（PD0 RX、PD1 TX），不使用板级 chosen 中的 CAN2。
比特率来自设备树的 `bitrate`，不是 Kconfig 的默认比特率。

诊断配置开启 ISO-TP DEBUG 日志；发送失败后还输出 CAN state、TEC、REC。
当前 Zephyr 实现在提交 FF 后就启动 FC 超时计时，因此 FC 超时本身不能证明
首帧已被对端 ACK。

- TEC 上升表示 CAN 发送遇到错误；缺少 ACK、速率或物理链路异常均需排查。
- down 没有 `SM process FF`：尚未证明首帧到达 down 的 ISO-TP 接收上下文。
- down 出现 `SM send CTS FC frame`：已尝试回复 FC，继续检查返回链路。
- up 出现 `Got CTS`：已收到 FC，下一步检查 CF 与完整报文接收。

`SM send CTS FC frame` 同样只表示提交发送，不等于对端已收到 FC。

## 实板端到端结果（2026-11-27，rm_typec / STM32F407）

完成以下实板验证，均通过：

| 层级 | 测试 | 结果 |
| --- | --- | --- |
| 单板物理链路 | CAN1 ↔ CAN2 原始 CAN 请求/响应 | PASS |
| 双板原始 CAN | `raw_can` tx/rx，8 字节请求/响应 | PASS |
| 双板 ISO-TP | `raw_isotp` tx/rx，20 字节请求/响应、多帧及流控 | PASS |
| B2B 模块 | up ↔ down 模块通信 | PASS |

CAN1↔CAN2 首次测试曾因重新检查后发现的供电问题而失败；恢复/确认供电后，
单板双控制器测试通过。后续跨板 raw CAN、ISO-TP 和 up/down B2B 模块测试也均通过。
因此目前已验证从控制器、收发器和板间 CAN 链路，到 ISO-TP 传输及 B2B 模块通信的
完整实板路径。此前记录的 CAN1 `LEC=5` 属于供电问题排查期间的失败现象，不应单独
作为当前仍存在物理层故障的依据。
