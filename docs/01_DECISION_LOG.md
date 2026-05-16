# 01_DECISION_LOG.md

版本：v0.3  
状态：M2 已确认，M3 启动

## 已确认决策：M0 / M1

| 编号 | 决策 |
|---|---|
| D1 | 项目名称为 `ppg_ibi` |
| D2 | 算法只计算 IBI，不计算 HRV / RMSSD |
| D3 | 输入为 4 路严格同步绿光 PPG，50Hz，`int32_t` |
| D4 | 算法逐点输入，逐搏输出 |
| D5 | 外部运动标志用于暂停 IBI 计算 |
| D6 | 运动打断后重新获取，不保持旧 IBI 输出 |
| D7 | 前 5 秒允许不输出 |
| D8 | 目标平台 Apollo3.5，允许 float，禁止动态内存 |
| D9 | RAM 预算 `< 20KB` |
| D10 | 从空仓库开始 |

## 已确认决策：M2

| 编号 | 决策 | 说明 |
|---|---|---|
| D11 | 时间戳字段采用 `uint32_t timestamp_ms` | 单位 ms；单调递增；50Hz 理想间隔 20ms |
| D12 | 外部运动标志命名为 `allow_measure` | true 表示允许 IBI 计算；false 表示立即禁止输出 |
| D13 | 算法采用调用方持有 context 的纯 C 库风格 | 不使用全局可变状态；不使用动态内存 |
| D14 | 每输入一个 PPG 四通道样本，返回一次 output 结构 | 只有有新 IBI 时 `valid=true` |
| D15 | IBI 单位为 ms，字段为 `ibi_ms` | MCU 输出不计算 HRV |
| D16 | 置信度为 `float confidence`，范围 0.0~1.0 | 后续 M4 冻结具体公式 |
| D17 | 状态机冻结为 `INIT / ACQUIRE / TRACK / HOLD / REACQUIRE` | 用于输出和调试 |
| D18 | `allow_measure=false` 时进入 HOLD，输出 invalid，并清除 beat/IBI 跟踪历史 | 过滤器是否保留由实现细节决定，但不得输出旧 IBI |
| D19 | 从 HOLD 恢复后进入 REACQUIRE，重新累计 5 秒后才允许输出 IBI | 避免运动后伪峰 |
| D20 | 严重丢点触发 REACQUIRE；轻微丢点降低质量与置信度 | 具体阈值见 M2 状态与质量规格 |
| D21 | 多通道策略采用“主通道选择为主，备份通道辅助” | M4 不强制做复杂融合 |
| D22 | IBI 有效范围默认 300~2000ms | 覆盖约 30~200 bpm；异常值不输出 valid |
| D23 | M3 只实现框架、状态、接口和基础测试，不实现完整峰值检测算法 | 避免一次 Codex 任务过大 |

## M3 实现期约束

| 编号 | 决策 | 说明 |
|---|---|---|
| D24 | M3 public API 以 `docs/20_M3_INTERFACE_CONTRACT.md` 为准 | Codex 不得自行改名或改变字段语义 |
| D25 | M3 `process` 不得输出真实 IBI | `valid` 在 M3 中应始终为 false；M4 再实现真实 IBI 输出 |
| D26 | M3 可在 warmup 满 5 秒后进入 TRACK | 这是框架占位行为；M4 再加入 SQI 与 beat 条件 |
