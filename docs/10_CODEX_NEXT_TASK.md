# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 **M5a：验证核心重拆分任务**。

背景：M5 原任务经过两轮修复仍未通过评审。根据 OAR-M 规则，本阶段不继续做小修，而是把 M5 拆小。  
M5a 只建立 **metrics + real self-test 验证核心**。CSV 文件读取、output_prefix 文件输出、复杂非法行处理放到 M5b。

请从当前 `dev` 分支开始，不要基于未合并的 PR #5 / #6 / #7 继续小修。

---

## 0. Codex 必须先阅读

请先阅读：

1. `AGENTS.md`
2. `README.md`
3. `docs/00_PROJECT_BRIEF.md`
4. `docs/01_DECISION_LOG.md`
5. `docs/02_MILESTONE_PLAN.md`
6. `docs/09_CODEX_RUNBOOK.md`
7. `docs/13_M2_INTERFACE_SPEC.md`
8. `docs/14_M2_ALGORITHM_SPEC.md`
9. `docs/20_M3_INTERFACE_CONTRACT.md`
10. `docs/23_M4_MINIMAL_IBI_SPEC.md`
11. `docs/24_M4_TEST_SPEC.md`
12. `docs/26_M4_CODEX_REPORT.md`
13. `docs/27_M5_DATA_CONTRACT.md`
14. `docs/28_M5_VALIDATION_TOOL_SPEC.md`
15. `docs/32_M5A_SPLIT_SPEC.md`
16. `docs/33_M5A_REVIEW_CHECKLIST.md`

如果发现文档冲突，不要修改 Owner 已确认决策；在 `docs/34_M5A_CODEX_REPORT.md` 中标记 S0。

---

## 1. M5a 目标

实现一个最小 host 验证核心，证明：

1. `ppg_ibi_process()` 可以在 host 工具中被逐样本调用；
2. prediction events 与 truth events 可以被收集；
3. metrics engine 正确实现 **500ms 时间窗匹配**；
4. self-test 真实生成 60 bpm / 75 bpm synthetic PPG + ECG truth；
5. self-test 至少 16 秒；
6. self-test 输出实际指标并根据阈值返回 0 / 非 0；
7. M3/M4 既有测试继续通过。

---

## 2. 必须新增或更新文件

```text
tools/ppg_ibi_eval.c
Makefile
docs/34_M5A_CODEX_REPORT.md
```

可以新增辅助测试文件，但不是必须。

不要删除已有文档、已有测试、已有源码。

---

## 3. M5a 工具 CLI 范围

M5a 只强制支持：

```bash
build/ppg_ibi_eval --self-test
```

对于：

```bash
build/ppg_ibi_eval input.csv
build/ppg_ibi_eval input.csv output_prefix
```

M5a 可以暂时打印：

```text
CSV evaluation is not implemented in M5a; see M5b.
```

并返回非零。

CSV 读取和 output_prefix 输出放到 M5b，不要在 M5a 抢做。

---

## 4. metrics engine 必须满足

实现固定容量事件数组，不使用动态内存。

推荐常量：

```c
#define M5A_MAX_EVENTS 10000u
#define M5A_MATCH_WINDOW_MS 500u
```

事件结构至少包含：

```c
typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
    float confidence;
    uint32_t flags;
    ppg_ibi_state_t state;
} m5a_pred_event_t;

typedef struct {
    uint32_t timestamp_ms;
    uint16_t ibi_ms;
} m5a_truth_event_t;
```

metrics 至少包含：

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

匹配规则必须严格满足：

1. 按时间顺序处理 prediction event；
2. 为每个 prediction 寻找尚未使用的 truth event；
3. 必须满足 `abs(pred.timestamp_ms - truth.timestamp_ms) <= 500ms`；
4. 若多个 truth 满足，选择时间差最小者；
5. 每个 truth 最多匹配一次；
6. 未匹配 prediction 计为 extra；
7. 未匹配 truth 计为 miss。

事件数超过固定容量时，必须打印清晰错误并返回非零，不得静默截断。

---

## 5. self-test 必须满足

`--self-test` 内部至少执行两个场景：

### S1：60 bpm

```text
采样率：50Hz
时长：至少 16 秒
目标 ECG IBI：1000ms
```

通过条件：

```text
matched_count >= 3
mae_ms <= 120
max_abs_error_ms <= 200
```

### S2：75 bpm

```text
采样率：50Hz
时长：至少 16 秒
目标 ECG IBI：800ms
```

通过条件：

```text
matched_count >= 3
mae_ms <= 120
max_abs_error_ms <= 200
```

self-test 必须：

1. 生成 synthetic PPG；
2. 生成 ECG truth events；
3. 调用 `ppg_ibi_process()`；
4. 收集 prediction events；
5. 调用 metrics engine；
6. 打印实际指标；
7. 任一场景失败则返回非零。

输出示例：

```text
M5a self-test 60bpm: truth=15 pred=10 matched=9 mae_ms=20.0 max_abs_error_ms=40.0 pass
M5a self-test 75bpm: truth=19 pred=12 matched=11 mae_ms=20.0 max_abs_error_ms=40.0 pass
M5a self-test passed.
```

---

## 6. Makefile 要求

`make test` 必须：

1. 编译并运行 `build/test_ppg_ibi_basic`；
2. 编译并运行 `build/test_ppg_ibi_synthetic`；
3. 编译 `build/ppg_ibi_eval`；
4. 运行：

```bash
./build/ppg_ibi_eval --self-test
```

编译参数保持：

```bash
-std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
```

如使用 `sinf/fabsf/sqrtf`，允许链接 `-lm`。

---

## 7. 禁止事项

M5a 禁止：

1. 修改 `src/ppg_ibi.c` 算法核心；
2. 修改 M3/M4 public API；
3. 删除 M3/M4 测试；
4. 使用动态内存；
5. 计算 HRV / RMSSD；
6. 抢做 M5b 的完整 CSV 解析和 output_prefix 输出；
7. 继续进入 M5b 或 M6。

---

## 8. 输出报告

请生成：

```text
docs/34_M5A_CODEX_REPORT.md
```

内容控制在 1 页以内，包含：

```text
# 34_M5A_CODEX_REPORT.md

## 结论
通过 / 不通过

## 完成内容
- xxx

## 修改文件
- xxx

## 测试
- 运行命令
- 实际终端输出

## self-test 指标
- 60 bpm: truth=, pred=, matched=, mae_ms=, max_abs_error_ms=
- 75 bpm: truth=, pred=, matched=, mae_ms=, max_abs_error_ms=

## 资源评估
- 是否修改算法核心
- 是否使用动态内存
- event buffer 上限

## 已知限制
- M5a 不实现 CSV 输入评估；M5b 再实现

## 需要 Owner 决策
无 / 有：xxx
```

---

## 9. 完成后停止

完成 M5a 后停止。不要实现 M5b。
