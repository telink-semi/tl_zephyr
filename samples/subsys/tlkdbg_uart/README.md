# tlkdbg UART VCD/LOG demo

把 usbtraceplus 的设备侧 VCD + LOG 输出移植到 Zephyr，通过 UART 发送。
组件与 Zephyr console/log 子系统**互斥**：启用 `CONFIG_TLKDBG` 时，console 和
LOG 必须关闭，因为它们会争用同一个 UART。

## 组成

- `zephyr/subsys/tlkdbg/`：组件本体
  - `tlkdbg_log.c`：LOG 编码（type 4/5/6）与 transport registry
  - `tlkdbg_tr_uart.c`：UART 帧、CRC16、VCD 批量、TXC 记账
  - `tlkdbg_uart_ll_tl323x.c`：UART 时钟/引脚/波特率初始化（调用 Telink HAL）
  - `tlkdbg_poll.c`：轮询线程（可选）
- 本 demo：每秒约 500 组 VCD 边沿 + 10 条 LOG/s

## 关键约束

1. **tick 源**：`stimer_get_tick()`（24 MHz）。上位机 `mhz` 必须配 **24**，
   与 Zephyr 的 32 kHz 系统时钟无关。
2. **无中断**：UART TX 和 DMA 都是轮询，不注册任何 UART/DMA 中断向量；
   `uart_set_irq_mask(..., 0)`，DMA 不开 IRQ。轮询线程只用内核已有的 tick 睡眠。
3. **波特率与 pclk**：
   `baud = pclk / ((div+1)*(bwpc+1))`，`bwpc >= 3`，所以 `baud <= pclk/8`。
   - 默认板级 CCLK=48 MHz → pclk=12 MHz，3 Mbps 实际会变成 2.4 Mbps。
   - 本 demo 的 overlay 把 CCLK 提到 **96 MHz**（pclk=48 MHz），3 Mbps / 6 Mbps
     都能精确分频。
   - 换芯片/换时钟时用 `CONFIG_TLKDBG_UART_BAUD` 调整。
4. **串口占用**：demo 把 `uart0` 置为 disabled，TX 引脚 PB2 由 tlkdbg 自己 mux。

## 编译与烧录

```bash
west build -p always -b tl3238x zephyr/samples/subsys/tlkdbg_uart \
    -d build/tl3238x/tlkdbg_uart
```

需要本地（CH340）验证时，可临时降速：

```bash
west build -p always -b tl3238x zephyr/samples/subsys/tlkdbg_uart \
    -d build/tl3238x/tlkdbg_uart_921k -DCONFIG_TLKDBG_UART_BAUD=921600
```

## 验证（无桥板）

1. 用 uarttool 以对应波特率、raw 分帧抓取：
   ```bash
   node "$UART" open --port log921k --framing raw --encoding latin1
   node "$UART" capture --duration 3000 --out logs/tlkdbg.ndjson
   ```
2. 拆帧并校验 CRC：
   ```bash
   python zephyr/subsys/tlkdbg/tools/tlkdbg_uart_split.py \
       logs/tlkdbg.ndjson --vcd-out logs/tlkdbg.vcd.raw \
       --log-out logs/tlkdbg.log.txt
   ```
3. 用 usbtraceplus 解码 VCD：
   ```bash
   python <usbtraceplus>/resources/usbtraceplus_cli.py decode \
       -r logs/tlkdbg.vcd.raw -o logs/tlkdbg.vcd \
       -d .vscode/tlkdbg_demo.def.h -m 24 --workspace .
   ```

## 实测结果（TL3238x，CH340）

| 波特率 | VCD 帧 | VCD 字 | CRC 错误 | 设备端 TXC drop |
|---|---|---|---|---|
| 921600 | 253 | 7560 | **0** | 0 |
| 2 Mbps | 165 | 4930 | 4 | 0 |
| 3 Mbps | 148 | 4436 | 21 | 0 |

921600 下链路完全干净，证明帧格式、CRC、LOG 编码正确。2M/3M 的 CRC 错误来自
CH340 采集适配器（其上限约 2 Mbps），设备端 `vcd_drop`/`log_drop` 始终为 0。
TL751X 桥板 3 Mbps 已复测零丢（见下）；6 Mbps 目前受桥端分频限制。

## 桥板实测（TL751X，3 Mbps）

桥板固件为 0x150（UART 桥），先用 `bridge` 命令把桥端波特率设成与设备一致：

```bash
usbtraceplus bridge --set-baud 3000000   # 实测 3000000，div=1 bwpc=10
usbtraceplus bridge --get
```

`.vscode/usbtraceplus.json` 关键项：

```json
{
  "mhz": 24,
  "version": "0x150",
  "interface": "0x1",
  "log_interface": "0x0",
  "def_file": ".vscode/tlkdbg_demo.def.h",
  "bin_file": "Y:/zephyrSDK_v4.1/build/tl3238x/tlkdbg_uart/zephyr/zephyr.bin"
}
```

实测：

| 场景 | 结果 |
|---|---|
| `capture vcd --seconds 5` | 13,022 字发出 / 13,120 收到，**lost=0**，EP8 full_hits=0 |
| `capture vcd --seconds 20` | 50,732 字发出 / 50,832 收到，**lost=0**，peak=31 words/ms |
| `capture log --seconds 5` | 89 条；`FCNT` 41 条单调递增；桥自检 `STAT/STA2/BID/BAUD/TXC` 正常 |
| `analyze vcd` | `b_task_test` 2613 个脉冲（5–9 µs），`s_counter`/`w_FCNT` 单调 |
| `analyze log` | `label_table.status=no-table` 属正常（type 5 动态标签） |

`analyze log` 的 48 行 “unparsed” 是 `STAT/STA2` 多行十六进制记录的续行，不是丢记录。

## 静态标签（type 4）

demo 默认开启 `CONFIG_TLKDBG_LABEL_TABLE=y`。构建时：

1. 扫描 `CONFIG_TLKDBG_LABEL_ROOTS`（相对应用目录，默认 `"."`）里的
   `LOG_STR_DATA(tag, "text", data, n)`；
2. 生成 `build/.../generated/tlkdbg/tlkdbg_label_hashes.h`；
3. 生成 `build/.../zephyr/zephyr.labels.json`（与 `zephyr.bin` 同目录）。

设备只发 2 字节 hash；上位机按 `bin_file` 同目录自动加载
`zephyr.labels.json`，并通过每 500 ms 的 type 6 心跳校验 seed/table_hash。
实测 `label_table.status=ok`（seed=0, table_hash=0x5c8cda）。

注意：扫描范围是应用源码目录；库/subsys 里的静态标签需要把路径加进
`CONFIG_TLKDBG_LABEL_ROOTS` 或移到应用侧。

## PM / 深睡

`CONFIG_TLKDBG_PM`（默认 y，依赖 `CONFIG_PM`）注册 PM notifier：进入低功耗前
先发 TXC 记账并 `tlkdbg_tr_uart_drain()`，保证环里的帧不会在睡眠瞬间被截断。

- **suspend-to-idle**：组件状态保留，24 MHz stimer 连续；下次 `tlkdbg_uart_ms()`
  的差值覆盖睡眠时长，时间轴正确。
- **deep-retention**：唤醒走复位流程，`.bss` 重新初始化，`tlkdbg_init()` 再次执行；
  上位机把它当作新的设备段（TXC ledger 的 `resets/segments` 已处理）。
- PM 版本已编译链接验证（`tlkdbg_pm_state_entry` + `pm_notifier_register`）；
  实际睡眠/唤醒的 tick 连续性需要在低功耗应用里复测。

### 6 Mbps

设备端在 CCLK=96 MHz（pclk=48 MHz）下可以精确产生 6 Mbps，但当前 TL751X 桥
`bridge --set-baud 6000000` 返回 `status=2: unreachable`（桥的 pclk 分不出该频率），
所以 6 Mbps 需要桥端先解决时钟/分频。设备端无需改动，改
`CONFIG_TLKDBG_UART_BAUD=6000000` 即可。
