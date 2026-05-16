# 28_M5_VALIDATION_TOOL_SPEC.md

版本：v0.1  
状态：M5 ECG validation tool 规格

## 1. 工具目标

实现一个 host 侧命令行工具：

```text
tools/ppg_ibi_eval.c
```

该工具用于：

1. 读取 normalized CSV；
2. 逐行调用 `ppg_ibi_process()`；
3. 收集算法输出的 PPG IBI；
4. 收集 ECG truth IBI；
5. 做基础匹配；
6. 输出基础误差指标。

---

## 2. 命令行接口

至少支持：

```bash
build/ppg_ibi_eval --self-test
build/ppg_ibi_eval input.csv
build/ppg_ibi_eval input.csv output_prefix
```

行为：

| 命令 | 行为 |
|---|---|
| `--self-test` | 内部生成 synthetic PPG + ECG truth，运行自检 |
| `input.csv` | 读取 normalized CSV，向 stdout 输出 metrics |
| `input.csv output_prefix` | 同时输出 predictions 和 metrics CSV |

---

## 3. 内部数据结构建议

可以使用固定大小静态数组，不使用动态内存。例如：

```c
#define M5_MAX_EVENTS 10000u
```

建议事件结构：

```c
typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
    float confidence;
    uint32_t flags;
} m5_pred_event_t;

typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
} m5_truth_event_t;
```

说明：

1. host 工具使用静态数组即可；
2. 不要求嵌入式部署；
3. 算法核心仍然不得使用动态内存；
4. 若事件超过上限，应返回非零状态并打印清晰错误。

---

## 4. CSV 解析要求

至少满足：

1. 识别第一行 header；
2. 支持字段顺序固定；
3. 跳过空行；
4. 对非法行计数；
5. 非法行过多或关键字段解析失败时返回非零；
6. 不需要复杂 CSV 引号处理；
7. 不需要自动识别其他真实设备格式。

固定 header：

```csv
timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms
```

---

## 5. 事件收集

对每一行 CSV：

1. 构造 `ppg_ibi_input_t`；
2. 调用 `ppg_ibi_process()`；
3. 若 `output.valid == true`，记录 prediction event：
   - `timestamp_ms = 当前输入 timestamp_ms`
   - `ibi_ms = output.ibi_ms`
   - `confidence = output.confidence`
   - `flags = output.flags`
4. 若 `ecg_ibi_ms > 0`，记录 truth event。

---

## 6. 匹配规则

M5 使用简单时间窗匹配，不做复杂对齐。

默认：

```text
M5_MATCH_WINDOW_MS = 500
```

匹配规则：

1. 按时间顺序处理 prediction event；
2. 为每个 prediction 寻找尚未使用的 truth event；
3. 要求 `abs(pred.timestamp_ms - truth.timestamp_ms) <= M5_MATCH_WINDOW_MS`；
4. 若多个 truth 满足，选择时间差最小的；
5. 每个 truth 最多匹配一次；
6. 无匹配 prediction 计为 extra；
7. 未被匹配 truth 计为 miss。

说明：

1. PPG 峰相对 ECG R peak 存在生理延迟，M5 使用宽松窗口；
2. M5 不估计 PTT/PAT；
3. 后续 M6 可以扩展为 offset search 或 cross-correlation 对齐。

---

## 7. 指标定义

对每个 matched pair：

```text
error_ms = pred_ibi_ms - truth_ibi_ms
abs_error_ms = abs(error_ms)
```

至少输出：

| 指标 | 定义 |
|---|---|
| `truth_count` | truth event 数量 |
| `pred_count` | prediction event 数量 |
| `matched_count` | 成功匹配数量 |
| `miss_count` | `truth_count - matched_count` |
| `extra_count` | `pred_count - matched_count` |
| `mae_ms` | 平均绝对误差 |
| `mean_error_ms` | 平均误差，保留正负号 |
| `rmse_ms` | 均方根误差 |
| `max_abs_error_ms` | 最大绝对误差 |
| `p95_abs_error_ms` | 绝对误差 95 分位数 |

若 `matched_count == 0`：

1. 指标应显示为 0 或 `nan`，但程序必须清晰打印；
2. 对 `--self-test` 必须判定失败并返回非零。

---

## 8. self-test 要求

`--self-test` 内部至少执行两个场景：

### S1：60 bpm

```text
采样率：50Hz
时长：至少 16 秒
目标 ECG IBI：1000ms
```

期望：

1. `matched_count >= 3`
2. `mae_ms <= 120`
3. `max_abs_error_ms <= 200`

### S2：75 bpm

```text
采样率：50Hz
时长：至少 16 秒
目标 ECG IBI：800ms
```

期望：

1. `matched_count >= 3`
2. `mae_ms <= 120`
3. `max_abs_error_ms <= 200`

self-test 应打印简短结果，例如：

```text
M5 self-test 60bpm: matched=8 mae_ms=20.0 pass
M5 self-test 75bpm: matched=9 mae_ms=20.0 pass
M5 self-test passed.
```

---

## 9. predictions CSV

当传入 `output_prefix` 时，建议生成：

```text
output_prefix_predictions.csv
```

至少包含：

```csv
timestamp_ms,ibi_ms,confidence,state,flags
```

说明：

1. 只记录 `output.valid=true` 的事件；
2. `state` 可输出枚举整数；
3. `flags` 可输出十进制或十六进制。

---

## 10. metrics CSV

当传入 `output_prefix` 时，建议生成：

```text
output_prefix_metrics.csv
```

至少包含：

```csv
truth_count,pred_count,matched_count,miss_count,extra_count,mae_ms,mean_error_ms,rmse_ms,max_abs_error_ms,p95_abs_error_ms
```

---

## 11. M5 不要求

M5 不要求：

1. ECG R peak 检测；
2. 自动生成 HRV；
3. 多文件批量汇总；
4. 自动搜索 ECG/PPG 时间偏移；
5. 图形输出；
6. Python 工具；
7. 医疗级验证报告。
