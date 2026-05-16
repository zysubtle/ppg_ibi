# 15_M2_STATE_AND_QUALITY_SPEC.md

版本：v0.1  
状态：待 Owner 确认后冻结

## 1. 状态机

冻结状态：

| 状态 | 说明 | 是否允许输出 IBI |
|---|---|---|
| `INIT` | 初始化完成但尚未进入采样 | 否 |
| `ACQUIRE` | 启动获取阶段，累计基础质量和 beat 历史 | 否 |
| `TRACK` | 稳定跟踪阶段 | 是 |
| `HOLD` | 外部禁止测量，例如运动中 | 否 |
| `REACQUIRE` | 运动恢复、严重丢点或质量崩溃后的重新获取 | 否 |

## 2. 状态转换

### 2.1 初始化

`init/reset` 后进入 `INIT`。收到第一帧有效输入且 `allow_measure=true` 后进入 `ACQUIRE`。

### 2.2 ACQUIRE 到 TRACK

满足以下条件后进入 `TRACK`：

1. `allow_measure=true`；
2. 已连续累计至少 5 秒有效输入；
3. 时间戳连续性未出现严重异常；
4. PPG 质量达到最低要求；
5. 已建立可用 beat 跟踪状态。

### 2.3 任意状态到 HOLD

当 `allow_measure=false` 时，立即进入 `HOLD`：

1. 输出 `valid=false`；
2. 不输出旧 IBI；
3. 清除 beat/IBI 跟踪历史；
4. 设置 `MOTION_HOLD` flag。

### 2.4 HOLD 到 REACQUIRE

当 `allow_measure` 从 false 恢复为 true 时，进入 `REACQUIRE`。

`REACQUIRE` 必须重新累计 5 秒后才允许输出 IBI。

### 2.5 严重丢点到 REACQUIRE

若时间戳间隔超过严重丢点阈值，进入 `REACQUIRE`。

默认严重丢点参考阈值：`dt_ms > 100ms`。

### 2.6 质量崩溃到 REACQUIRE

若连续质量不足持续达到实现阈值，进入 `REACQUIRE`。

M2 不冻结连续次数，M4 实现时结合 SQI 公式确定。

## 3. 丢点处理

50Hz 下理想采样间隔为 20ms。

| 情况 | 参考判定 | 处理方向 |
|---|---|---|
| 正常 | `dt_ms` 约 20ms | 正常处理 |
| 轻微丢点 | `dt_ms > 40ms` 且 `<=100ms` | 设置 `DROPOUT_LIGHT`，降低置信度，可继续跟踪 |
| 严重丢点 | `dt_ms > 100ms` | 设置 `DROPOUT_SEVERE`，进入 `REACQUIRE` |

说明：

1. M3 先实现状态转换和 flag；
2. M4 再决定轻微丢点对 IBI 输出的具体影响；
3. 若实际 timestamp jitter 较大，可在后续根据数据调整阈值。

## 4. SQI 质量评估方向

每通道 SQI 建议由以下因素组成：

1. 幅度是否足够；
2. 是否平坦或近似无信号；
3. 是否饱和或异常跳变；
4. 波形是否具备可检测脉搏形态；
5. 与最近 beat 间隔是否一致；
6. 主通道是否稳定。

M2 不冻结 SQI 公式，但冻结输出范围方向：

- SQI 建议归一化到 0.0~1.0；
- 主通道选择基于 SQI；
- 低 SQI 不输出 valid IBI。

## 5. 置信度方向

IBI 置信度建议由以下部分构成：

1. 主通道 SQI；
2. 峰值形态置信度；
3. IBI 是否在生理范围内；
4. 与最近 IBI 的一致性；
5. 是否存在轻微丢点；
6. 是否有备份通道支持。

默认有效输出门限：`confidence >= 0.6`。

M4 可根据验证数据调整该门限，但需要记录为决策变更。

## 6. warmup / reacquire 规则

以下情况必须重新累计 5 秒：

1. 算法刚初始化；
2. 从 `HOLD` 恢复；
3. 严重丢点；
4. 连续质量崩溃；
5. 主通道长期不可用且无法切换备份通道。

在 5 秒窗口内，允许算法更新滤波器、SQI 和 beat 历史，但不得输出 valid IBI。
