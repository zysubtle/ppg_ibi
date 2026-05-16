# ppg_ibi

## 当前状态

- 当前里程碑：M1 项目包初始化
- 当前阶段：只建立项目治理文档与后续协作入口
- 当前不包含算法代码
- 当前不冻结完整算法规格

## 项目目标

在 Apollo3.5 MCU 上，根据 4 路严格同步的绿光 PPG 信号，实时计算逐搏 IBI。

本项目只负责输出 IBI，不计算 HRV / RMSSD / 应用层健康指标。

## Owner 默认工作方式

Owner 只需要做阶段确认和关键决策，不需要反复阅读长 prompt、长测试或长源码。

每次审查默认使用“决策卡”：

- 结论
- 阻塞问题
- 非阻塞问题
- 是否需要 Owner 决策
- 下一步

## Codex 默认入口

进入 Codex 任务时，Owner 给 Codex 的默认指令为：

> 请读取 docs/10_CODEX_NEXT_TASK.md，并严格执行。

## 目录说明

```text
.
├── AGENTS.md
├── README.md
├── docs/
│   ├── 00_PROJECT_BRIEF.md
│   ├── 01_DECISION_LOG.md
│   ├── 02_MILESTONE_PLAN.md
│   ├── 03_INTERFACE_DRAFT_PENDING.md
│   ├── 04_OPEN_QUESTIONS.md
│   ├── 05_VALIDATION_PLAN_DRAFT.md
│   ├── 06_RESOURCE_BUDGET_DRAFT.md
│   ├── 07_REVIEW_PROTOCOL.md
│   ├── 08_OWNER_HANDOFF.md
│   ├── 09_CODEX_RUNBOOK.md
│   └── 10_CODEX_NEXT_TASK.md
├── include/
├── src/
└── tests/
```

`include/`、`src/`、`tests/` 当前只保留空目录占位，不包含算法实现。

## 下一阶段

M1 完成后，进入 M2：算法规格设计。

M2 目标不是写代码，而是冻结：

1. 输入 / 输出 API；
2. 状态机；
3. 外部运动标志处理策略；
4. 丢点处理策略；
5. IBI 检测算法主流程；
6. RAM 预算和验证方案。
