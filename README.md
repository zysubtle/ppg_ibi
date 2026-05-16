# ppg_ibi

## 当前状态

- 当前里程碑：M3 基础框架实现
- 当前阶段：Codex 按 `docs/10_CODEX_NEXT_TASK.md` 实现最小可编译算法库框架
- 当前仍不实现完整 PPG 滤波、峰值检测或真实 IBI 输出

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
│   ├── 09_CODEX_RUNBOOK.md
│   ├── 10_CODEX_NEXT_TASK.md
│   ├── 12_M2_SPEC_INDEX.md
│   ├── 13_M2_INTERFACE_SPEC.md
│   ├── 14_M2_ALGORITHM_SPEC.md
│   ├── 15_M2_STATE_AND_QUALITY_SPEC.md
│   ├── 16_M2_VALIDATION_SPEC.md
│   ├── 17_M2_RESOURCE_BUDGET_SPEC.md
│   ├── 18_M2_RISK_REGISTER.md
│   ├── 20_M3_INTERFACE_CONTRACT.md
│   └── 21_M3_REVIEW_CHECKLIST.md
├── include/
├── src/
└── tests/
```

## M3 边界

M3 只实现：

1. 公开 C API；
2. context / config / input / output 类型；
3. 初始化、reset、逐点 process；
4. 状态机基础转换；
5. `allow_measure` 门控；
6. timestamp 丢点 flag；
7. 5 秒 warmup / reacquire 计时；
8. host 单元测试。

M3 不实现真实 IBI 检测，不输出 `valid=true` 的真实 IBI。
