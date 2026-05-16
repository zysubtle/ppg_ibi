# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 M2 算法规格设计阶段。

本任务不要求 Codex 编写算法代码。

## Codex 当前可执行任务：M2 文档一致性检查

请严格执行以下任务：

1. 阅读：
   - `AGENTS.md`
   - `README.md`
   - `docs/00_PROJECT_BRIEF.md`
   - `docs/01_DECISION_LOG.md`
   - `docs/02_MILESTONE_PLAN.md`
   - `docs/12_M2_SPEC_INDEX.md`
   - `docs/13_M2_INTERFACE_SPEC.md`
   - `docs/14_M2_ALGORITHM_SPEC.md`
   - `docs/15_M2_STATE_AND_QUALITY_SPEC.md`
   - `docs/16_M2_VALIDATION_SPEC.md`
   - `docs/17_M2_RESOURCE_BUDGET_SPEC.md`
   - `docs/18_M2_RISK_REGISTER.md`
2. 检查 M2 文档是否与 Project Brief 冲突。
3. 检查是否仍未出现算法源码。
4. 不要创建 `src/*.c` 或 `include/*.h`。
5. 不要实现 C API。
6. 不要修改已确认决策。
7. 如需输出报告，请生成 `docs/20_CODEX_M2_SPEC_REVIEW.md`，内容控制在 1 页以内。

## 禁止事项

1. 禁止写算法实现代码。
2. 禁止生成完整 C API。
3. 禁止修改 Project Brief。
4. 禁止引入第三方库。
5. 禁止进入 M3 编码。

## 期望输出

生成简短检查报告，格式如下：

```text
# 20_CODEX_M2_SPEC_REVIEW.md

## 结论
通过 / 不通过

## 检查结果
- M2 文档是否齐全
- M2 文档是否与 Project Brief 冲突
- 是否存在不应出现的源码
- 是否存在明显不一致的接口描述

## 需要 Owner 决策
无 / 有：xxx
```
