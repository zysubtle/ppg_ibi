# 09_CODEX_RUNBOOK.md

## 1. Codex 每次任务前必须读取

1. `AGENTS.md`
2. `README.md`
3. `docs/00_PROJECT_BRIEF.md`
4. `docs/01_DECISION_LOG.md`
5. `docs/02_MILESTONE_PLAN.md`
6. `docs/10_CODEX_NEXT_TASK.md`

## 2. 不得越权

如果 `docs/10_CODEX_NEXT_TASK.md` 没有明确要求写代码，Codex 不得写算法代码。

如果任务只要求文档检查，Codex 不得创建 `src/*.c` 或 `include/*.h`。

如果任务要求编码，Codex 必须遵守：

1. 禁止动态内存；
2. 算法核心不得依赖文件系统；
3. 算法核心不得依赖标准输出；
4. Host 测试代码与算法核心分离；
5. 所有 public API 必须有测试覆盖；
6. 不得改变已冻结的 Owner 决策。

## 3. Codex 输出报告要求

Codex 完成任务后，应生成或输出简短报告，包含：

1. 本次完成了什么；
2. 修改了哪些文件；
3. 运行了哪些测试；
4. 测试结果；
5. 是否存在阻塞问题；
6. 是否需要 Owner 决策。

## 4. 风险标记

遇到以下情况必须标记风险：

- 需要改变 API；
- 需要改变算法策略；
- RAM 可能超过 20KB；
- 需要动态内存；
- ECG 验证数据格式不明确；
- 与 Project Brief 冲突。
