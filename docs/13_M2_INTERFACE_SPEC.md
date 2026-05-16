# 13_M2_INTERFACE_SPEC.md

版本：v0.1  
状态：待 Owner 确认后冻结

## 1. 接口风格

算法库采用纯 C 嵌入式模块风格：

1. 调用方持有算法 context；
2. 算法内部不使用 `malloc/free`；
3. 不依赖操作系统；
4. 不依赖第三方库；
5. 允许使用 `float`；
6. 每次处理一个 PPG 采样点，即同一时间戳下的 4 路 PPG 值。

## 2. 输入规格

### 2.1 PPG 输入

| 字段 | 类型 | 说明 |
|---|---|---|
| `timestamp_ms` | `uint32_t` | 时间戳，单位 ms，单调递增 |
| `ppg[4]` | `int32_t[4]` | 4 路严格同步绿光 PPG 原始值 |
| `allow_measure` | `bool` / 等价布尔类型 | true 表示允许测量，false 表示运动或外部禁止测量 |

### 2.2 采样约束

| 项目 | 值 |
|---|---|
| PPG 采样率 | 50Hz |
| 理想采样周期 | 20ms |
| 通道数 | 4 |
| 数据异常 | 可能丢点；不考虑乱序、重复或长时间中断 |

## 3. 输出规格

每输入一个样本后，算法返回一次 output 状态。只有当前样本触发了新的有效 IBI 时，`valid=true`。

| 字段 | 类型方向 | 说明 |
|---|---|---|
| `valid` | 布尔 | 当前是否产生新的有效 IBI |
| `ibi_ms` | 无符号整数，建议 16 bit 足够 | IBI 数值，单位 ms，仅当 `valid=true` 有意义 |
| `confidence` | `float` | 0.0~1.0，表示当前 IBI 置信度 |
| `state` | enum | 当前算法状态 |
| `flags` | bitmask | 质量、运动、丢点、暖机等状态标志 |

## 4. 状态枚举

状态冻结为：

1. `INIT`：初始化后尚未开始有效采样；
2. `ACQUIRE`：启动获取阶段，允许累计信号质量，但不输出 IBI；
3. `TRACK`：稳定跟踪阶段，允许逐搏输出；
4. `HOLD`：外部禁止测量或运动打断，禁止输出；
5. `REACQUIRE`：从运动、严重丢点或质量崩溃后重新获取。

## 5. 标志位方向

建议至少支持以下 flags，具体位值由 M3 实现时定义：

| flag | 说明 |
|---|---|
| `WARMUP` | 仍在前 5 秒暖机 / 重新获取阶段 |
| `MOTION_HOLD` | `allow_measure=false` 导致暂停 |
| `LOW_SQI` | 当前 PPG 质量不足 |
| `DROPOUT_LIGHT` | 检测到轻微丢点 |
| `DROPOUT_SEVERE` | 检测到严重丢点，触发 REACQUIRE |
| `IBI_OUT_OF_RANGE` | IBI 候选超出 300~2000ms |
| `NO_BEAT` | 当前点没有新心搏输出 |

## 6. 配置参数方向

默认配置由算法提供，调用方可以传入配置覆盖。M3 可先实现默认配置，不要求完整 runtime 参数覆盖。

关键参数：

| 参数 | 默认值方向 | 说明 |
|---|---|---|
| `sample_rate_hz` | 50 | 固定 50Hz |
| `warmup_ms` | 5000 | 启动或重新获取等待时间 |
| `ibi_min_ms` | 300 | 最小有效 IBI |
| `ibi_max_ms` | 2000 | 最大有效 IBI |
| `confidence_valid_min` | 0.6 | 低于此值不输出 valid |
| `expected_dt_ms` | 20 | 期望采样间隔 |
| `dropout_light_ms` | 40 | 轻微丢点参考阈值 |
| `dropout_severe_ms` | 100 | 严重丢点参考阈值 |

## 7. 调试输出方向

M3 不强制实现完整调试输出。M4/M5 建议加入可选 debug 结构，用于 host 验证：

1. 当前主通道编号；
2. 4 路 SQI；
3. 当前峰值候选状态；
4. 丢点计数；
5. 最近一次 beat timestamp；
6. 最近一次 IBI 候选；
7. 当前状态和 flags。
