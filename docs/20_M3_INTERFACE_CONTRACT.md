# 20_M3_INTERFACE_CONTRACT.md

版本：v0.1  
状态：M3 编码接口契约

## 1. 目的

本文档用于冻结 M3 最小可编译框架的 public API。

Codex 必须按本文档实现。若发现本文档与 M2 规格冲突，不得自行改动，应在报告中标记为 S0。

## 2. M3 public header

必须创建：

```text
include/ppg_ibi.h
```

必须满足：

1. C99 兼容；
2. 支持 C++ include guard；
3. 只依赖标准头文件 `stdint.h` 和 `stdbool.h`；
4. 不包含平台私有头文件；
5. 不暴露动态内存接口。

## 3. 固定宏定义

```c
#define PPG_IBI_NUM_CHANNELS (4u)

#define PPG_IBI_FLAG_NONE           (0u)
#define PPG_IBI_FLAG_WARMUP         (1u << 0)
#define PPG_IBI_FLAG_MOTION_HOLD    (1u << 1)
#define PPG_IBI_FLAG_LOW_SQI        (1u << 2)
#define PPG_IBI_FLAG_DROPOUT_LIGHT  (1u << 3)
#define PPG_IBI_FLAG_DROPOUT_SEVERE (1u << 4)
#define PPG_IBI_FLAG_IBI_OUT_OF_RANGE (1u << 5)
#define PPG_IBI_FLAG_NO_BEAT        (1u << 6)
```

## 4. 固定枚举

```c
typedef enum {
    PPG_IBI_STATUS_OK = 0,
    PPG_IBI_STATUS_NULL_ARGUMENT = 1,
    PPG_IBI_STATUS_INVALID_CONFIG = 2
} ppg_ibi_status_t;

typedef enum {
    PPG_IBI_STATE_INIT = 0,
    PPG_IBI_STATE_ACQUIRE = 1,
    PPG_IBI_STATE_TRACK = 2,
    PPG_IBI_STATE_HOLD = 3,
    PPG_IBI_STATE_REACQUIRE = 4
} ppg_ibi_state_t;
```

## 5. 固定结构体

```c
typedef struct {
    uint16_t sample_rate_hz;
    uint16_t expected_dt_ms;
    uint16_t warmup_ms;
    uint16_t dropout_light_ms;
    uint16_t dropout_severe_ms;
    uint16_t ibi_min_ms;
    uint16_t ibi_max_ms;
    float confidence_valid_min;
} ppg_ibi_config_t;

typedef struct {
    uint32_t timestamp_ms;
    int32_t ppg[PPG_IBI_NUM_CHANNELS];
    bool allow_measure;
} ppg_ibi_input_t;

typedef struct {
    bool valid;
    uint16_t ibi_ms;
    float confidence;
    ppg_ibi_state_t state;
    uint32_t flags;
} ppg_ibi_output_t;

typedef struct {
    ppg_ibi_config_t config;
    ppg_ibi_state_t state;
    bool initialized;
    bool has_last_timestamp;
    bool last_allow_measure;
    uint32_t last_timestamp_ms;
    uint32_t state_entry_timestamp_ms;
    uint32_t samples_since_state_entry;
    uint32_t process_count;
    uint32_t dropout_light_count;
    uint32_t dropout_severe_count;
} ppg_ibi_context_t;
```

说明：

1. `ppg_ibi_context_t` 在 M3 可以公开，便于调用方静态分配；
2. M4 如需增加内部字段，必须保持无动态内存；
3. context 不得包含大数组；
4. M3 context 目标大小应远小于 1KB。

## 6. 固定函数

```c
void ppg_ibi_get_default_config(ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_init(ppg_ibi_context_t *ctx,
                              const ppg_ibi_config_t *config);

ppg_ibi_status_t ppg_ibi_reset(ppg_ibi_context_t *ctx);

ppg_ibi_status_t ppg_ibi_process(ppg_ibi_context_t *ctx,
                                  const ppg_ibi_input_t *input,
                                  ppg_ibi_output_t *output);
```

## 7. 默认配置

`ppg_ibi_get_default_config()` 必须写入以下默认值：

| 字段 | 默认值 |
|---|---:|
| `sample_rate_hz` | 50 |
| `expected_dt_ms` | 20 |
| `warmup_ms` | 5000 |
| `dropout_light_ms` | 40 |
| `dropout_severe_ms` | 100 |
| `ibi_min_ms` | 300 |
| `ibi_max_ms` | 2000 |
| `confidence_valid_min` | 0.6f |

## 8. 参数校验

M3 至少校验：

1. `ctx == NULL` 时返回 `PPG_IBI_STATUS_NULL_ARGUMENT`；
2. `config == NULL` 时使用默认配置；
3. `output == NULL` 或 `input == NULL` 时返回 `PPG_IBI_STATUS_NULL_ARGUMENT`；
4. `sample_rate_hz == 0` 为非法；
5. `expected_dt_ms == 0` 为非法；
6. `warmup_ms == 0` 为非法；
7. `dropout_light_ms >= dropout_severe_ms` 为非法；
8. `ibi_min_ms >= ibi_max_ms` 为非法；
9. `confidence_valid_min < 0.0f` 或 `> 1.0f` 为非法。

## 9. M3 process 行为

M3 不实现真实 IBI 检测，因此：

1. `output->valid` 必须始终为 `false`；
2. `output->ibi_ms` 必须为 `0`；
3. `output->confidence` 必须为 `0.0f`；
4. 正常无 beat 时设置 `PPG_IBI_FLAG_NO_BEAT`。

### 9.1 初始化后

`ppg_ibi_init()` 后：

1. `ctx->state = PPG_IBI_STATE_INIT`；
2. 清除 timestamp 历史；
3. 清除计数器；
4. 不产生输出。

### 9.2 第一帧 allow_measure=true

当第一帧 `allow_measure=true`：

1. 从 `INIT` 进入 `ACQUIRE`；
2. 设置 `state_entry_timestamp_ms = input->timestamp_ms`；
3. 输出 `valid=false`；
4. flags 至少包含 `WARMUP | NO_BEAT`。

### 9.3 warmup 完成

当 `ACQUIRE` 或 `REACQUIRE` 中，当前时间距离 `state_entry_timestamp_ms >= warmup_ms`：

1. M3 可以进入 `TRACK`；
2. 输出仍然 `valid=false`；
3. flags 至少包含 `NO_BEAT`；
4. 不要求 SQI 或 beat 条件，因为 M3 只做框架。

### 9.4 allow_measure=false

任意状态下，只要 `allow_measure=false`：

1. 立即进入 `HOLD`；
2. 清除 timestamp 历史，使恢复后不因时间间隔过长误报 dropout；
3. 输出 `valid=false`；
4. flags 至少包含 `MOTION_HOLD | NO_BEAT`。

### 9.5 HOLD 恢复

当从 `HOLD` 收到 `allow_measure=true`：

1. 进入 `REACQUIRE`；
2. 重新设置 `state_entry_timestamp_ms = input->timestamp_ms`；
3. 输出 `valid=false`；
4. flags 至少包含 `WARMUP | NO_BEAT`；
5. 重新累计 5 秒后才可进入 `TRACK`。

### 9.6 轻微丢点

当存在上一时间戳，且：

```text
dt_ms > dropout_light_ms && dt_ms <= dropout_severe_ms
```

处理：

1. 设置 `PPG_IBI_FLAG_DROPOUT_LIGHT`；
2. `dropout_light_count++`；
3. 不进入 `REACQUIRE`；
4. 输出仍然 `valid=false`。

### 9.7 严重丢点

当存在上一时间戳，且：

```text
dt_ms > dropout_severe_ms
```

处理：

1. 设置 `PPG_IBI_FLAG_DROPOUT_SEVERE`；
2. `dropout_severe_count++`；
3. 进入 `REACQUIRE`；
4. 重置 `state_entry_timestamp_ms = input->timestamp_ms`；
5. flags 至少包含 `DROPOUT_SEVERE | WARMUP | NO_BEAT`；
6. 输出仍然 `valid=false`。

## 10. M3 不处理项

M3 不要求处理：

1. timestamp wrap-around；
2. 乱序、重复或长时间中断；
3. PPG 滤波；
4. SQI 公式；
5. 主通道选择；
6. 峰值检测；
7. 真实 IBI；
8. ECG 验证。
