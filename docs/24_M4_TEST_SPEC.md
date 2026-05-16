# 24_M4_TEST_SPEC.md

版本：v0.1  
状态：M4 Host 测试规格

## 1. 目标

M4 测试目标是证明：

1. M3 基础状态机行为没有被破坏；
2. 算法能在简单合成 PPG 上输出真实 IBI；
3. 运动门控、warmup、严重丢点不会产生跨段错误 IBI；
4. flat/低质量信号不会输出 IBI。

## 2. 测试文件

必须保留：

```text
tests/test_ppg_ibi_basic.c
```

必须新增：

```text
tests/test_ppg_ibi_synthetic.c
```

`make test` 必须同时运行两个测试。

## 3. 合成 PPG 建议

M4 不强制使用 `math.h` 的 `sin()`，因为部分嵌入式/host 编译环境需要额外链接 `-lm`。

推荐使用三角波、抛物线脉冲或查表方式生成周期性 PPG。

例如每个周期生成一个单峰波形：

```text
phase = sample_index % samples_per_beat
x = phase / samples_per_beat
pulse = 1.0 - abs(2.0*x - 1.0)
raw = baseline + amplitude * pulse
```

四通道可以设置为相近值：

```text
ppg[ch] = raw + small_channel_offset[ch]
```

建议：

```text
baseline = 100000
amplitude = 20000
sample_rate = 50Hz
```

## 4. 必测用例

### T1：M3 basic regression

运行原 `tests/test_ppg_ibi_basic.c`。

期望：

1. 默认配置正确；
2. init/reset 状态正确；
3. warmup / HOLD / REACQUIRE / dropout 行为仍通过；
4. null 参数错误码仍通过。

注意：M4 已允许输出 IBI，因此 M3 原测试中“全流程 valid 始终 false”的断言需要限缩到 M3 场景，例如在 basic 测试中仍使用不产生 beat 的输入，或调整为只检查 warmup/HOLD/dropout 期间不输出。

### T2：60 bpm 合成 PPG 能输出 IBI

输入：

```text
采样率：50Hz
心率：60 bpm
周期：1000ms
时长：至少 15 秒
allow_measure=true
```

期望：

1. warmup 前不得输出；
2. warmup 后输出多个 valid IBI；
3. 至少 3 个 IBI 在 `900ms ~ 1100ms` 范围内；
4. valid 输出时 `confidence >= confidence_valid_min`；
5. valid 输出时 `ibi_ms > 0`。

### T3：75 bpm 合成 PPG 能输出 IBI

输入：

```text
心率：75 bpm
周期：800ms
时长：至少 15 秒
allow_measure=true
```

期望：

1. 至少 3 个 IBI 在 `720ms ~ 880ms` 范围内；
2. 不应出现大量超范围 IBI。

### T4：flat PPG 不输出 IBI

输入：

```text
ppg[ch] = 常数
allow_measure=true
时长：至少 8 秒
```

期望：

1. 不输出 valid IBI；
2. 进入 TRACK 后至少出现一次 `PPG_IBI_FLAG_LOW_SQI`。

### T5：运动门控禁止输出

输入：

1. 先输入一段正常 PPG，使算法进入 TRACK 并可输出 IBI；
2. 中间设置 `allow_measure=false` 至少 2 秒；
3. 再恢复 `allow_measure=true`。

期望：

1. `allow_measure=false` 阶段状态为 HOLD；
2. HOLD 阶段不输出 IBI；
3. 恢复后进入 REACQUIRE；
4. REACQUIRE warmup 未完成前不输出 IBI；
5. 不得跨 HOLD 生成异常大 IBI。

### T6：严重丢点触发 REACQUIRE

输入：

1. 正常输入到 TRACK；
2. 制造一次 `dt_ms > dropout_severe_ms`；
3. 继续输入正常 PPG。

期望：

1. 严重丢点帧设置 `PPG_IBI_FLAG_DROPOUT_SEVERE`；
2. 状态进入 REACQUIRE；
3. REACQUIRE warmup 未完成前不输出 IBI；
4. 不得跨 dropout 生成 IBI。

## 5. 测试输出

测试程序可以打印简短总结，例如：

```text
All M4 synthetic tests passed.
```

不要输出大量逐样本日志。

## 6. 失败处理

如果某个测试不稳定，不要删除测试来通过。

应优先检查：

1. peak threshold 是否过高或过低；
2. 第一拍是否被错误输出；
3. warmup 后是否清除了 beat 历史；
4. 合成波形是否确实有单峰；
5. refractory 是否使用 `ibi_min_ms`。
