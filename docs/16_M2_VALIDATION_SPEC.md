# 16_M2_VALIDATION_SPEC.md

版本：v0.1  
状态：待 Owner 确认后冻结

## 1. 验证目标

使用 ECG 真值验证 PPG-IBI 的准确性、稳定性和可用性。

验证工具属于 host 侧，不放入 MCU 核心算法路径。

## 2. 验证输入

至少需要：

1. PPG 四通道数据；
2. PPG 时间戳；
3. ECG 真值 beat timestamp 或 ECG 推导 IBI；
4. 可选：外部 `allow_measure` 序列；
5. 可选：人工标注的无效段或运动段。

## 3. 对齐原则

验证时应明确：

1. PPG beat timestamp 的定义；
2. ECG beat timestamp 的定义；
3. IBI 是由相邻 beat timestamp 计算，还是直接使用真值 IBI；
4. 是否剔除 warmup、HOLD、REACQUIRE、低质量段；
5. 输出延迟是否只影响 beat timestamp，还是影响 IBI 数值。

## 4. 指标定义方向

M5 验证工具至少输出：

| 指标 | 说明 |
|---|---|
| `n_ref_beats` | ECG 参考 beat 数 |
| `n_ppg_ibi` | PPG 输出 IBI 数 |
| `coverage` | 可输出覆盖率 |
| `mae_ms` | PPG IBI 与 ECG IBI 的平均绝对误差 |
| `median_abs_error_ms` | 中位绝对误差 |
| `p95_abs_error_ms` | 95 分位绝对误差 |
| `miss_rate` | 漏检率 |
| `false_rate` | 误报率 |
| `valid_ratio` | 算法 valid 输出占比 |
| `ram_estimate_bytes` | MCU context 估算 RAM |

## 5. beat / IBI 匹配方向

推荐采用 ECG beat 作为基准，对 PPG beat 或 IBI 进行时间邻近匹配。

M5 可采用两级验证：

1. beat-level：PPG beat 与 ECG beat 是否对应；
2. IBI-level：匹配后的相邻 beat 间隔误差。

## 6. 验收阈值状态

M2 暂不冻结最终 Pass/Fail 阈值。

原因：

1. 数据集、佩戴条件和 PPG 质量尚未明确；
2. 运动段由外部门控，验证时需要区分 allow/disallow；
3. 第一版需要先获得基线误差分布。

M5 后建议再冻结：

1. MAE 目标；
2. P95 误差目标；
3. 漏检率目标；
4. 误报率目标；
5. 不同质量段的分层指标。

## 7. M3 / M4 / M5 分工

| 里程碑 | 验证重点 |
|---|---|
| M3 | API、状态机、丢点、运动门控单元测试 |
| M4 | 合成 PPG 和少量样本数据上的 IBI 输出逻辑 |
| M5 | ECG 真值对齐、误差统计、报告生成 |
