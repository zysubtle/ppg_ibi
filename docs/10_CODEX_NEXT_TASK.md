# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 M4：最小真实 IBI 算法实现。

M3 已完成并合并。M4 的目标是在不改变 public API 语义的前提下，让算法在简单、干净的 PPG 输入上能够产生逐搏 IBI 输出。

M4 仍不是最终产品级算法；本阶段不做 ECG 验证工具、不做复杂 SQI、不做 HRV/RMSSD。

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
12. `docs/21_M3_REVIEW_CHECKLIST.md`
13. `docs/22_M3_CODEX_REPORT.md`
14. `docs/23_M4_MINIMAL_IBI_SPEC.md`
15. `docs/24_M4_TEST_SPEC.md`
16. `docs/25_M4_REVIEW_CHECKLIST.md`

如果发现文档冲突，不要自行修改 Owner 已确认决策；请在 `docs/26_M4_CODEX_REPORT.md` 中标记 S0。

## 1. M4 目标

实现最小真实 IBI 输出能力：

1. 保持 M3 public API 不变；
2. 在 `allow_measure=true` 且状态进入 `TRACK` 后，根据 PPG 检测心搏；
3. 每检测到一个有效新心搏，并且与上一有效心搏形成合法间隔时，输出一次 IBI；
4. 非输出时 `output.valid=false`；
5. 运动暂停、恢复、warmup、丢点行为继续符合 M3；
6. 新增 host 合成 PPG 测试，验证算法能输出约 1000ms / 800ms 级别的 IBI。

## 2. 必须新增或更新的文件

请新增或更新以下文件：

```text
include/ppg_ibi.h
src/ppg_ibi.c
tests/test_ppg_ibi_basic.c
tests/test_ppg_ibi_synthetic.c
Makefile
docs/26_M4_CODEX_REPORT.md
```

允许更新 `docs/22_M3_CODEX_REPORT.md` 以外的文档吗？
- 不允许修改 Owner 已确认的 M0/M1/M2/M3 决策文档。
- 可以新增 M4 报告。
- 不得删除任何已有文档。

## 3. API 与兼容性要求

### 3.1 必须保持

1. 不得删除或重命名 M3 public API；
2. 不得改变 `ppg_ibi_config_t`、`ppg_ibi_input_t`、`ppg_ibi_output_t` 已有字段含义；
3. 不得改变默认配置值；
4. 不得改变状态枚举数值；
5. 不得改变 flags 数值；
6. 不得引入动态内存接口；
7. `ppg_ibi_process()` 仍然每次只处理一个 4 路同步 PPG 样本。

### 3.2 允许

1. 允许在 `ppg_ibi_context_t` 末尾新增内部状态字段；
2. 允许新增内部 `static` 函数；
3. 允许新增内部宏；
4. 允许新增测试文件。

## 4. 算法实现要求

请严格参考：

```text
docs/23_M4_MINIMAL_IBI_SPEC.md
```

M4 推荐实现为：

1. 四通道 PPG 分别做轻量 DC 跟踪；
2. 对 DC 去除后的信号做轻量平滑；
3. 将 4 路处理后的信号合成为一个 composite PPG；
4. 使用局部极值 + 动态阈值 + refractory 的方式检测心搏；
5. 支持正向/反向 PPG 极性锁定；
6. 第一拍只用于建立 `last_beat_timestamp`，第二拍开始才能输出 IBI；
7. 输出 IBI 时设置 `valid=true`、`ibi_ms>0`、`confidence>0`；
8. 没有新 IBI 时设置 `valid=false`、`ibi_ms=0`、`confidence=0.0f`。

## 5. 状态机要求

必须继续满足 M3 状态机要求：

1. `init/reset` 后状态为 `INIT`；
2. 第一帧 `allow_measure=true` 时进入 `ACQUIRE`；
3. `ACQUIRE` warmup 满 `warmup_ms` 后进入 `TRACK`；
4. 任意状态下 `allow_measure=false` 立即进入 `HOLD`；
5. `HOLD` 收到 `allow_measure=true` 后进入 `REACQUIRE`；
6. `REACQUIRE` warmup 满 `warmup_ms` 后进入 `TRACK`；
7. 严重丢点进入 `REACQUIRE`；
8. 轻微丢点只设置 flag，不进入 `REACQUIRE`。

新增 M4 要求：

1. `HOLD`、`ACQUIRE`、`REACQUIRE` 阶段不得输出 IBI；
2. 从 `HOLD` 恢复或严重丢点进入 `REACQUIRE` 时，必须清除 beat 历史，避免跨段生成错误 IBI；
3. 进入 `TRACK` 后可以开始检测 beat；
4. 第一个 beat 不输出 IBI；第二个合法 beat 起才输出。

## 6. flags 行为要求

至少满足：

1. warmup 阶段包含 `PPG_IBI_FLAG_WARMUP`；
2. 运动暂停包含 `PPG_IBI_FLAG_MOTION_HOLD`；
3. 没有新 IBI 输出时包含 `PPG_IBI_FLAG_NO_BEAT`；
4. 输出有效 IBI 时不应设置 `PPG_IBI_FLAG_NO_BEAT`；
5. 轻微丢点包含 `PPG_IBI_FLAG_DROPOUT_LIGHT`；
6. 严重丢点包含 `PPG_IBI_FLAG_DROPOUT_SEVERE`；
7. 信号质量不足时包含 `PPG_IBI_FLAG_LOW_SQI`；
8. 候选 IBI 超出 `[ibi_min_ms, ibi_max_ms]` 时包含 `PPG_IBI_FLAG_IBI_OUT_OF_RANGE`，且不得输出 valid IBI。

## 7. Host 测试要求

请严格参考：

```text
docs/24_M4_TEST_SPEC.md
```

至少包含：

1. 保留并通过 M3 基础测试；
2. 新增 `tests/test_ppg_ibi_synthetic.c`；
3. 合成 60 bpm 左右 PPG，期望输出多个约 1000ms IBI；
4. 合成 75 bpm 左右 PPG，期望输出多个约 800ms IBI；
5. flat PPG 不应输出 IBI，并应出现 LOW_SQI；
6. `allow_measure=false` 阶段不得输出 IBI；
7. 从 HOLD 恢复后必须重新 warmup，不能跨 HOLD 生成 IBI；
8. 严重丢点后必须重新 REACQUIRE，不能跨 dropout 生成 IBI；
9. 全部测试通过 `make test`。

## 8. 编译与测试要求

`Makefile` 至少支持：

```bash
make test
make clean
```

`make test` 应至少编译并运行：

```text
build/test_ppg_ibi_basic
build/test_ppg_ibi_synthetic
```

编译参数保持严格：

```bash
-std=c99 -Wall -Wextra -Werror -pedantic -Iinclude
```

算法核心不得使用 `printf`、文件系统、OS、线程、第三方库。
测试文件可以使用 `stdio`。

## 9. 禁止事项

M4 禁止：

1. 计算 HRV / RMSSD；
2. 实现 ECG 验证工具；
3. 引入第三方库；
4. 使用动态内存；
5. 改变 public API 已有字段语义；
6. 要求 Owner 做新的算法决策；
7. 为了通过测试写死固定时间点输出；
8. 实现复杂窗口缓存导致 RAM 超过 20KB；
9. 修改 M0/M1/M2/M3 已确认文档；
10. 继续进入 M5。

## 10. 输出报告

请生成：

```text
docs/26_M4_CODEX_REPORT.md
```

报告控制在 1 页以内，包含：

```text
# 26_M4_CODEX_REPORT.md

## 结论
通过 / 不通过

## 完成内容
- xxx

## 修改文件
- xxx

## 测试
- 运行命令
- 结果

## 资源评估
- context 大小估计
- 是否使用动态内存

## 已知限制
- M4 不是最终产品级算法
- 未做 ECG 验证

## 需要 Owner 决策
无 / 有：xxx
```

## 11. 完成后停止

完成 M4 后停止。

不要实现 M5 内容。
