# 26_M4_CODEX_REPORT.md

## 结论
通过

## 完成内容
- 修复 beat 检测：局部极值检测使用 signed composite，不再对 peak 检测输入取绝对值。
- 新增 polarity 锁定：支持正峰/负峰候选，首个有效候选锁定极性，后续仅接收同极性。
- 动态阈值：candidate_amp = fabsf(prev1_signed)，peak_threshold = 0.8 * signal_abs_ema，且 low_sqi=false 才允许候选。
- 保留并满足既有约束：HOLD/严重 dropout 不跨段输出；flat LOW_SQI；M3 basic regression 保持通过；out-of-range 行为按要求区分低于最小与高于最大。
- 恢复 synthetic IBI 输出测试：60 bpm 与 75 bpm 均验证至少 3 个 valid IBI，范围和 flag/confidence 约束满足。

## 修改文件
- include/ppg_ibi.h
- src/ppg_ibi.c
- tests/test_ppg_ibi_synthetic.c
- Makefile

## 测试
- 运行命令：`make clean && make test`
- 实际终端输出：

```text
rm -rf build
mkdir -p build
cc -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude src/ppg_ibi.c tests/test_ppg_ibi_basic.c -lm -o build/test_ppg_ibi_basic
mkdir -p build
cc -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude src/ppg_ibi.c tests/test_ppg_ibi_synthetic.c -lm -o build/test_ppg_ibi_synthetic
./build/test_ppg_ibi_basic
All M3 basic tests passed.
./build/test_ppg_ibi_synthetic
Synthetic M4 tests passed.
```

## 资源评估
- context 大小估计：`ppg_ibi_context_t` 为常量规模结构体，远小于 20KB。
- 动态内存：未使用（无 malloc/free）。

## 已知限制
- M4 不是最终产品级算法。
- 未做 ECG 验证。

## 需要 Owner 决策
无
