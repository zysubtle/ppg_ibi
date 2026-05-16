# 02_MILESTONE_PLAN.md

版本：v0.1

## 里程碑总览

| 里程碑 | 名称 | 目标 | Owner 是否需要深读 |
|---|---|---|---|
| M0 | 项目启动问诊 | 明确目标、边界、约束 | 否，只需回答问题 |
| M1 | 项目包生成 | 建立仓库文档、协作规则、后续入口 | 否，只需确认是否进入 M2 |
| M2 | 算法规格设计 | 冻结 API、状态机、处理流程、验证方案 | 只需确认决策卡 |
| M3 | 基础框架实现 | Codex 实现初始化、状态、输入输出、基础测试 | 否，只看审查决策卡 |
| M4 | IBI 算法核心实现 | Codex 实现 PPG 预处理、质量评估、峰值/脉搏检测、IBI 输出 | 否，只看审查决策卡 |
| M5 | ECG 验证与指标输出 | Codex 实现 host 验证工具、误差统计、漏检/误报统计 | 否，只看审查决策卡 |
| M6 | 收敛与冻结 | 参数整理、风险归类、发布候选版本 | 只需确认是否冻结 |

## M0 状态

已完成。

## M1 状态

当前正在完成。

M1 交付物：

1. `README.md`
2. `AGENTS.md`
3. `docs/00_PROJECT_BRIEF.md`
4. `docs/01_DECISION_LOG.md`
5. `docs/02_MILESTONE_PLAN.md`
6. `docs/03_INTERFACE_DRAFT_PENDING.md`
7. `docs/04_OPEN_QUESTIONS.md`
8. `docs/05_VALIDATION_PLAN_DRAFT.md`
9. `docs/06_RESOURCE_BUDGET_DRAFT.md`
10. `docs/07_REVIEW_PROTOCOL.md`
11. `docs/08_OWNER_HANDOFF.md`
12. `docs/09_CODEX_RUNBOOK.md`
13. `docs/10_CODEX_NEXT_TASK.md`

## M2 目标

M2 不写代码。

M2 需要冻结：

1. 输入 API；
2. 输出 API；
3. 状态机；
4. 外部运动标志处理策略；
5. 丢点处理策略；
6. IBI 候选检测主流程；
7. 多通道 PPG 选择 / 融合策略；
8. RAM 预算草案；
9. ECG 验证方法草案；
10. 第一轮 Codex 编码任务边界。

## Codex 修复规则

每个里程碑最多两轮 Codex 修复。

如果两轮后仍未通过，必须重新拆分任务，而不是继续在同一任务上小修。
