# 32_M5A_SPLIT_SPEC.md

版本：v0.1  
状态：M5 拆分后的 M5a 验证核心规格

## 1. 拆分原因

M5 原目标包含三类能力：

1. metrics engine；
2. synthetic self-test；
3. normalized CSV 读取与输出文件。

PR #5 / #6 / #7 在修复过程中出现了 self-test 被削弱、匹配窗口丢失、事件超限静默截断等问题。  
因此 M5a 只冻结验证核心：**metrics engine + real self-test**。

## 2. M5a 交付边界

M5a 必须交付：

1. `tools/ppg_ibi_eval.c`
2. `build/ppg_ibi_eval --self-test`
3. `make test` 集成
4. 真实 60/75 bpm synthetic 验证
5. 500ms 时间窗匹配
6. 超过 event buffer 时返回非零

M5a 不交付：

1. `input.csv` 评估；
2. `output_prefix_predictions.csv`；
3. `output_prefix_metrics.csv`；
4. CSV 非法行诊断。

这些放到 M5b。

## 3. 匹配算法

必须以 prediction 为主循环：

```text
for each prediction in time order:
    find nearest unused truth within 500ms
    if found:
        match
    else:
        extra++
miss = truth_count - matched_count
```

不能把距离超过 500ms 的 truth/prediction 强行匹配。

## 4. self-test 数据

60 bpm：

```text
period_ms = 1000
duration_ms >= 16000
```

75 bpm：

```text
period_ms = 800
duration_ms >= 16000
```

PPG 可以使用 sine、triangle 或 pulse-like waveform。  
ECG truth event 的 `ibi_ms` 应分别为 1000 / 800。

## 5. 验收条件

M5a 通过条件：

1. `make clean && make test` 成功；
2. `--self-test` 真实调用算法；
3. 60 bpm / 75 bpm 均有 `matched_count >= 3`；
4. 60 bpm / 75 bpm 均满足 `mae_ms <= 120`；
5. 60 bpm / 75 bpm 均满足 `max_abs_error_ms <= 200`；
6. `compute_metrics()` 中存在 500ms match window；
7. event 超限不静默截断；
8. 未修改算法核心。
