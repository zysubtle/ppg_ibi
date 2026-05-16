# 10_CODEX_NEXT_TASK.md

## 当前任务状态

当前处于 M1 项目包阶段。

本任务不要求 Codex 编写算法代码。

## Codex 当前可执行任务：文档完整性检查

请严格执行以下任务：

1. 阅读 `AGENTS.md`、`README.md`、`docs/00_PROJECT_BRIEF.md`、`docs/01_DECISION_LOG.md`、`docs/02_MILESTONE_PLAN.md`。
2. 检查以下文件是否存在：
   - `AGENTS.md`
   - `README.md`
   - `docs/00_PROJECT_BRIEF.md`
   - `docs/01_DECISION_LOG.md`
   - `docs/02_MILESTONE_PLAN.md`
   - `docs/03_INTERFACE_DRAFT_PENDING.md`
   - `docs/04_OPEN_QUESTIONS.md`
   - `docs/05_VALIDATION_PLAN_DRAFT.md`
   - `docs/06_RESOURCE_BUDGET_DRAFT.md`
   - `docs/07_REVIEW_PROTOCOL.md`
   - `docs/08_OWNER_HANDOFF.md`
   - `docs/09_CODEX_RUNBOOK.md`
   - `docs/10_CODEX_NEXT_TASK.md`
3. 确认当前没有算法实现代码。
4. 不要创建 `src/*.c` 或 `include/*.h`。
5. 不要修改 Project Brief 和 Decision Log。
6. 如需输出报告，请生成 `docs/11_CODEX_REPORT.md`，内容控制在 1 页以内。

## 禁止事项

1. 禁止写算法实现代码。
2. 禁止生成完整 C API。
3. 禁止修改已确认决策。
4. 禁止引入第三方库。

## 期望输出

生成简短检查报告，格式如下：

```text
# 11_CODEX_REPORT.md

## 结论
通过 / 不通过

## 检查结果
- 必要文件是否齐全
- 是否存在不应出现的源码
- 是否发现与 Project Brief 冲突的内容

## 需要 Owner 决策
无 / 有：xxx
```
