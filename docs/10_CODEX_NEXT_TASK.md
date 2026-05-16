# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 **M5：ECG 真值验证工具与数据契约**。

M4 已完成并合并。M5 的目标不是继续优化 PPG-IBI 算法本身，而是建立一个可复现的 host 侧验证入口，用于后续把真实 ECG 真值数据接入，并输出基础误差指标。

M5 仍不是最终医学/产品级验证；本阶段只建立验证框架、统一 CSV 数据契约、生成基础指标，并用内部 synthetic self-test 证明工具可运行。

---

## 0. Codex 必须先阅读

请先阅读以下文件：

1. `AGENTS.md`
2. `README.md`
3. `docs/00_PROJECT_BRIEF.md`
4. `docs/01_DECISION_LOG.md`
5. `docs/02_MILESTONE_PLAN.md`
6. `docs/09_CODEX_RUNBOOK.md`
7. `docs/13_M2_INTERFACE_SPEC.md`
8. `docs/14_M2_ALGORITHM_SPEC.md`
9. `docs/15_M2_STATE_AND_QUALITY_SPEC.md`
10. `docs/17_M2_RESOURCE_BUDGET_SPEC.md`
11. `docs/20_M3_INTERFACE_CONTRACT.md`
12. `docs/22_M3_CODEX_REPORT.md`
13. `docs/23_M4_MINIMAL_IBI_SPEC.md`
14. `docs/24_M4_TEST_SPEC.md`
15. `docs/26_M4_CODEX_REPORT.md`
16. `docs/27_M5_DATA_CONTRACT.md`
17. `docs/28_M5_VALIDATION_TOOL_SPEC.md`
18. `docs/29_M5_REVIEW_CHECKLIST.md`

如果发现文档冲突，不要自行修改 Owner 已确认决策；请在 `docs/31_M5_CODEX_REPORT.md` 中标记 S0。

---

## 1. M5 目标

实现一个 host 侧 ECG 真值验证入口：

1. 保持 M4 public API 不变；
2. 保持 M4 basic 和 synthetic 测试继续通过；
3. 新增一个 normalized CSV 评估工具；
4. 支持读取 PPG + allow_measure + ECG IBI truth；
5. 调用 `ppg_ibi_process()`，收集 PPG IBI 输出；
6. 与 ECG truth 做基础匹配；
7. 输出基础指标；
8. 提供 `--self-test`，在无真实数据时也能验证工具链；
9. 生成 M5 报告。

---

## 2. 必须新增或更新的文件

请新增或更新：

```text
tools/ppg_ibi_eval.c
Makefile
docs/31_M5_CODEX_REPORT.md
```

允许新增辅助文件，例如：

```text
tools/README.md
tests/test_ppg_ibi_eval_*.c
tests/fixtures/*.csv
```

但不要删除已有文档、已有测试、已有源码。

---

## 3. 必须保持不变

1. 不得删除或重命名 M3/M4 public API；
2. 不得改变 `ppg_ibi_config_t`、`ppg_ibi_input_t`、`ppg_ibi_output_t` 已有字段含义；
3. 不得改变状态枚举数值；
4. 不得改变 flags 数值；
5. 不得改变默认配置值；
6. 不得实现 HRV / RMSSD；
7. 不得引入第三方库；
8. 算法核心不得使用文件系统、`printf`、OS、线程或动态内存；
9. 除非为修复编译/测试失败，不要修改 `src/ppg_ibi.c` 的 M4 算法主逻辑。

---

## 4. M5 数据契约

请严格参考：

```text
docs/27_M5_DATA_CONTRACT.md
```

M5 默认 normalized CSV 格式为：

```csv
timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms
```

说明：

1. 一行对应一个 PPG 采样点；
2. `timestamp_ms` 为 `uint32_t` 毫秒时间戳；
3. `ppg0..ppg3` 为 4 路同步 PPG `int32_t`；
4. `allow_measure` 为 `0/1`；
5. `ecg_ibi_ms` 为 ECG truth IBI，单位 ms；
6. `ecg_ibi_ms == 0` 表示该行没有 ECG IBI truth；
7. `ecg_ibi_ms > 0` 表示该时间点存在一个 ECG IBI truth event；
8. 真实数据格式适配不在 M5 范围内，M5 只实现 normalized CSV。

---

## 5. M5 验证工具要求

请严格参考：

```text
docs/28_M5_VALIDATION_TOOL_SPEC.md
```

至少实现：

```bash
build/ppg_ibi_eval --self-test
build/ppg_ibi_eval input.csv
build/ppg_ibi_eval input.csv output_prefix
```

`--self-test` 必须在没有真实 CSV 的情况下，内部生成 60 bpm 和 75 bpm synthetic PPG + ECG truth，并验证：

1. 工具能调用 `ppg_ibi_process()`；
2. 能收集 PPG IBI 输出；
3. 能收集 ECG IBI truth；
4. 能完成匹配；
5. 匹配数量至少为 3；
6. MAE 在合理范围内，例如 `<= 120ms`；
7. 程序返回 0。

---

## 6. 指标要求

M5 至少输出以下指标：

```text
truth_count
pred_count
matched_count
miss_count
extra_count
mae_ms
mean_error_ms
rmse_ms
max_abs_error_ms
p95_abs_error_ms
```

定义见 `docs/28_M5_VALIDATION_TOOL_SPEC.md`。

---

## 7. Makefile 要求

更新 `Makefile`，至少支持：

```bash
make test
make clean
```

`make test` 必须：

1. 编译并运行已有 M4 basic test；
2. 编译并运行已有 M4 synthetic test；
3. 编译 `tools/ppg_ibi_eval.c`；
4. 运行：

```bash
./build/ppg_ibi_eval --self-test
```

编译参数继续保持严格：

```bash
-std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
```

如使用 `sinf/fabsf/sqrtf` 等数学函数，允许链接 `-lm`。

---

## 8. 输出文件行为

当运行：

```bash
build/ppg_ibi_eval input.csv output_prefix
```

建议输出：

```text
output_prefix_predictions.csv
output_prefix_metrics.csv
```

其中：

`output_prefix_predictions.csv` 至少包含：

```csv
timestamp_ms,ibi_ms,confidence,state,flags
```

`output_prefix_metrics.csv` 至少包含一行 summary 指标。

如果仅传入 `input.csv`，可以只向 stdout 输出 metrics，不强制写文件。

---

## 9. 禁止事项

M5 禁止：

1. 计算 HRV / RMSSD；
2. 对真实 ECG 文件格式做猜测式适配；
3. 把某个测试数据写死为固定输出；
4. 删除 M3/M4 测试；
5. 为了通过 self-test 而削弱 M4 synthetic IBI 测试；
6. 继续进入 M6；
7. 要求 Owner 做新的算法决策。

---

## 10. 输出报告

请生成：

```text
docs/31_M5_CODEX_REPORT.md
```

报告控制在 1 页以内，包含：

```text
# 31_M5_CODEX_REPORT.md

## 结论
通过 / 不通过

## 完成内容
- xxx

## 修改文件
- xxx

## 测试
- 运行命令
- 实际终端输出

## 指标 self-test 结果
- 60 bpm: xxx
- 75 bpm: xxx

## 资源评估
- 是否修改算法核心
- 是否使用动态内存
- context 是否变化

## 已知限制
- M5 只支持 normalized CSV
- 未接入真实 ECG 数据格式

## 需要 Owner 决策
无 / 有：xxx
```

---

## 11. 完成后停止

完成 M5 后停止。

不要实现 M6 内容。
