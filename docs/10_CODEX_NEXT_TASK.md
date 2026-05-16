# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 M3：基础框架实现。

本任务要求 Codex 编写最小 C99 算法库框架和基础 host 单元测试。

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

如果发现文档冲突，不要自行修改 Owner 已确认决策；请在报告中标记 S0。

## 1. M3 目标

实现 `ppg_ibi` 基础框架，不实现完整 IBI 检测算法。

M3 只做：

1. 目录结构；
2. 公开头文件；
3. context / config / input / output 类型；
4. 初始化和 reset；
5. process 单样本入口；
6. 状态机基础转换；
7. `allow_measure` 门控；
8. timestamp 丢点 flag；
9. warmup 计时；
10. host 单元测试。

## 2. 必须新增或更新的文件

请新增或更新以下文件：

```text
include/ppg_ibi.h
src/ppg_ibi.c
tests/test_ppg_ibi_basic.c
Makefile
docs/22_M3_CODEX_REPORT.md
```

不得删除 M0/M1/M2 文档。

## 3. public API 要求

严格按照：

```text
docs/20_M3_INTERFACE_CONTRACT.md
```

实现 public API。

不得自行改名、删字段、改字段含义或改变默认值。

## 4. 算法核心实现要求

### 4.1 必须满足

1. C99；
2. 不使用 `malloc/free/calloc/realloc`；
3. 算法核心不使用文件系统；
4. 算法核心不使用 `printf`；
5. 算法核心不依赖 OS、线程或第三方库；
6. 所有运行期状态都在 `ppg_ibi_context_t` 中；
7. `config == NULL` 时使用默认配置；
8. `process` 每次只处理一个 4 路同步 PPG 样本；
9. M3 不输出真实 IBI：`output.valid` 必须始终为 `false`。

### 4.2 状态机行为

必须实现以下最小状态行为：

1. `init/reset` 后状态为 `INIT`；
2. 第一帧 `allow_measure=true` 时进入 `ACQUIRE`；
3. `ACQUIRE` 连续 warmup 满 `warmup_ms` 后进入 `TRACK`；
4. 任意状态下 `allow_measure=false` 立即进入 `HOLD`；
5. `HOLD` 收到 `allow_measure=true` 后进入 `REACQUIRE`；
6. `REACQUIRE` 连续 warmup 满 `warmup_ms` 后进入 `TRACK`；
7. 严重丢点进入 `REACQUIRE`；
8. 轻微丢点只设置 flag，不进入 `REACQUIRE`。

### 4.3 flags 行为

至少满足：

1. warmup 阶段包含 `PPG_IBI_FLAG_WARMUP`；
2. 运动暂停包含 `PPG_IBI_FLAG_MOTION_HOLD`；
3. 无 beat 输出包含 `PPG_IBI_FLAG_NO_BEAT`；
4. 轻微丢点包含 `PPG_IBI_FLAG_DROPOUT_LIGHT`；
5. 严重丢点包含 `PPG_IBI_FLAG_DROPOUT_SEVERE`。

## 5. Host 测试要求

请实现 `tests/test_ppg_ibi_basic.c`。

测试可使用 `assert` 或自定义最小测试宏。测试代码可以使用 `stdio` 打印，但算法核心不可以。

至少覆盖：

1. 默认配置值正确；
2. `init` 后为 `INIT`；
3. `reset` 后回到 `INIT`；
4. 第一帧允许测量后进入 `ACQUIRE`；
5. warmup 满 5 秒后进入 `TRACK`，但仍无 IBI 输出；
6. `allow_measure=false` 后立即进入 `HOLD`；
7. 从 `HOLD` 恢复后进入 `REACQUIRE`；
8. `REACQUIRE` warmup 满 5 秒后进入 `TRACK`；
9. 轻微丢点设置 `DROPOUT_LIGHT` 且不进入 `REACQUIRE`；
10. 严重丢点设置 `DROPOUT_SEVERE` 且进入 `REACQUIRE`；
11. M3 全流程 `output.valid` 始终为 `false`；
12. null 参数返回预期错误码。

## 6. 编译与测试要求

请提供 `Makefile`，至少支持：

```bash
make test
make clean
```

`make test` 应执行类似如下编译并运行测试：

```bash
cc -std=c99 -Wall -Wextra -Werror -pedantic -Iinclude \
  src/ppg_ibi.c tests/test_ppg_ibi_basic.c \
  -o build/test_ppg_ibi_basic
./build/test_ppg_ibi_basic
```

如果环境没有 `cc`，请在报告中说明实际使用的编译命令。

## 7. 禁止事项

M3 禁止：

1. 实现完整 PPG 滤波；
2. 实现峰值检测；
3. 输出真实 IBI；
4. 实现 ECG 验证工具；
5. 实现复杂 SQI；
6. 实现 HRV / RMSSD；
7. 引入第三方库；
8. 使用动态内存；
9. 改变 M2 已确认 API 语义；
10. 要求 Owner 做新的算法决策。

## 8. 输出报告

请生成：

```text
docs/22_M3_CODEX_REPORT.md
```

报告控制在 1 页以内，包含：

```text
# 22_M3_CODEX_REPORT.md

## 结论
通过 / 不通过

## 完成内容
- xxx

## 修改文件
- xxx

## 测试
- 运行命令
- 结果

## 已知限制
- M3 不实现真实 IBI

## 需要 Owner 决策
无 / 有：xxx
```

## 9. 完成后不要继续进入 M4

完成 M3 后停止。

不要实现 M4 内容。
