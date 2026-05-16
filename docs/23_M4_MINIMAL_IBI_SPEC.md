# 23_M4_MINIMAL_IBI_SPEC.md

版本：v0.1  
状态：M4 最小真实 IBI 算法规格

## 1. M4 定位

M4 的目标是从 M3 的“只跑状态机、不输出 IBI”升级为“在简单、干净 PPG 上能输出逐搏 IBI”。

M4 不是最终产品级算法，不要求达到 ECG 验证后的精度指标。

## 2. 输入、输出与约束

沿用 M3 public API：

- 输入：`timestamp_ms`、4 路 `int32_t ppg[4]`、`allow_measure`；
- 输出：`valid`、`ibi_ms`、`confidence`、`state`、`flags`；
- 采样率：默认 50Hz；
- 时间戳：默认 ms，单调递增；
- 动态内存：禁止；
- RAM：算法总 RAM 预算 `<20KB`。

M4 不依赖 ADC 满量程或位宽；使用动态 DC 去除和相对阈值。

## 3. 状态机与输出原则

1. `ACQUIRE` / `REACQUIRE` / `HOLD` 阶段不得输出 IBI；
2. 只有 `TRACK` 阶段允许输出 IBI；
3. 检测到第一拍时只记录 `last_beat_timestamp_ms`，不输出；
4. 从第二个合法心搏起输出 IBI；
5. 输出有效 IBI 的那一帧：
   - `output.valid = true`；
   - `output.ibi_ms = 当前 beat timestamp - 上一 beat timestamp`；
   - `output.confidence` 在 `[confidence_valid_min, 1.0]` 范围内；
   - flags 不包含 `PPG_IBI_FLAG_NO_BEAT`。
6. 未输出新 IBI 的帧：
   - `output.valid = false`；
   - `output.ibi_ms = 0`；
   - `output.confidence = 0.0f`；
   - flags 包含 `PPG_IBI_FLAG_NO_BEAT`。

## 4. 推荐内部状态

允许在 `ppg_ibi_context_t` 末尾新增字段，例如：

```c
float dc[PPG_IBI_NUM_CHANNELS];
float smooth[PPG_IBI_NUM_CHANNELS];
float composite_prev2;
float composite_prev1;
uint32_t timestamp_prev2_ms;
uint32_t timestamp_prev1_ms;
bool has_composite_prev1;
bool has_composite_prev2;
float abs_ema;
int8_t peak_polarity;
bool has_last_beat;
uint32_t last_beat_timestamp_ms;
uint32_t beat_count;
uint32_t valid_ibi_count;
```

说明：

1. `peak_polarity` 可取 `0` 未锁定、`+1` 正峰、`-1` 负峰；
2. context 可以新增字段，但不得删除 M3 已有字段；
3. 不要加入大窗口数组；M4 推荐 streaming 实现。

## 5. 预处理

M4 推荐使用轻量 streaming 预处理，不使用大窗口缓存。

对每个通道：

```text
raw_f = (float)input->ppg[ch]
dc[ch] = dc[ch] + alpha_dc * (raw_f - dc[ch])
ac = raw_f - dc[ch]
smooth[ch] = smooth[ch] + alpha_smooth * (ac - smooth[ch])
```

推荐默认内部常量：

```text
alpha_dc = 0.01f
alpha_smooth = 0.25f
```

初始化处理：

1. reset/init 后第一帧可用 raw 初始化 `dc[ch]`，避免首帧巨大冲击；
2. `HOLD` 恢复、严重丢点进入 `REACQUIRE` 时，应清除 beat 历史；是否清除滤波状态由实现决定，但不得跨段输出 IBI。

## 6. 四通道合成

M4 暂不要求复杂主通道选择。

推荐 composite：

```text
composite = mean(smooth[0], smooth[1], smooth[2], smooth[3])
```

这样可以使用 4 路同步 PPG，并降低简单随机噪声。

## 7. 动态质量估计

维护 composite 绝对值的 EMA：

```text
abs_ema = abs_ema + alpha_abs * (fabsf(composite) - abs_ema)
```

推荐：

```text
alpha_abs = 0.02f
```

低质量判断：

```text
low_sqi = abs_ema < 1.0f
```

说明：

1. 由于 M4 不依赖 ADC 位宽，`1.0f` 只是防止 flat/全零信号误检的内部保护；
2. 若 `low_sqi=true`，不得输出 IBI，并设置 `PPG_IBI_FLAG_LOW_SQI`；
3. M4 不要求输出 SQI 数值。

## 8. 心搏候选检测

M4 推荐使用局部极值检测。

当已有 `prev2`、`prev1` 和当前 `composite` 时：

正峰候选：

```text
prev1 > prev2 && prev1 >= composite
```

负峰候选：

```text
prev1 < prev2 && prev1 <= composite
```

候选幅度：

```text
candidate_amp = fabsf(prev1)
```

动态阈值：

```text
peak_threshold = 0.8f * abs_ema
```

候选必须满足：

```text
candidate_amp >= peak_threshold
low_sqi == false
```

## 9. 极性锁定

为了兼容 PPG 正/反极性，M4 应支持极性锁定：

1. `peak_polarity == 0` 时，第一次通过阈值的局部极值决定极性；
2. 正峰锁定为 `+1`；
3. 负峰锁定为 `-1`；
4. 锁定后只接受同极性的候选；
5. `init/reset`、严重丢点、HOLD 恢复进入 REACQUIRE 时，可以清除 `peak_polarity`。

## 10. refractory 与 IBI 合法性

使用 `ibi_min_ms` 和 `ibi_max_ms`：

1. 候选 beat 与上一 beat 间隔 `< ibi_min_ms`：认为过近，忽略，不输出，可设置 `IBI_OUT_OF_RANGE`；
2. 候选 beat 与上一 beat 间隔 `> ibi_max_ms`：认为过远，不输出，可更新上一 beat 或重建 beat 历史；必须设置 `IBI_OUT_OF_RANGE`；
3. 合法 IBI：

```text
ibi_min_ms <= ibi_ms <= ibi_max_ms
```

默认范围：300ms 到 2000ms。

## 11. confidence

M4 confidence 使用简单规则即可：

```text
amp_ratio = candidate_amp / (peak_threshold + 1e-6f)
confidence = 0.6f + 0.2f * clamp(amp_ratio - 1.0f, 0.0f, 1.0f)
```

然后限制：

```text
confidence = clamp(confidence, confidence_valid_min, 1.0f)
```

说明：

1. M4 只要求 confidence 可解释、范围正确；
2. M5 再做真实 SQI 和 ECG 对齐验证。

## 12. 丢点与运动门控

沿用 M3：

1. `allow_measure=false`：立即进入 `HOLD`，不输出 IBI；
2. 从 `HOLD` 恢复：进入 `REACQUIRE`，重新 warmup；
3. 轻微丢点：只设置 flag，不进入 `REACQUIRE`；
4. 严重丢点：进入 `REACQUIRE`，清除 beat 历史，不允许跨丢点输出 IBI。

## 13. M4 不处理项

M4 不要求：

1. ECG 真值验证；
2. HRV / RMSSD；
3. 复杂 SQI；
4. 主通道选择输出；
5. 运动状态分类；
6. timestamp wrap-around；
7. 乱序、重复输入；
8. 复杂滤波器设计；
9. 医疗级准确率证明。
