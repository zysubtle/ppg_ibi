# AGENTS.md

本文件适用于整个 `ppg_ibi` 仓库。

## 角色分工

- Owner：只负责目标、约束、关键决策和阶段确认。
- Architect / Reviewer：负责规格整理、任务拆分、Codex 任务文件、源码/测试审查和风险归类。
- Runner：Codex 按任务文件编码、运行测试并报告结果。
- Milestone：所有工作按里程碑推进，避免陷入反复小修循环。

## 当前项目硬约束

1. 项目名称：`ppg_ibi`。
2. 目标平台：Apollo3.5 MCU。
3. 算法目标：基于 4 路严格同步绿光 PPG 计算逐搏 IBI。
4. 输入采样率：PPG 50Hz。
5. 输入数据类型：`int32_t`。
6. 允许使用 `float`。
7. 禁止使用动态内存，包括 `malloc/free`。
8. RAM 预算：小于 20KB。
9. 当前项目只计算 IBI，不计算 HRV / RMSSD。
10. 外部运动标志用于控制是否暂停 IBI 计算。
11. 被运动打断后重新获取，不保持旧 IBI 输出。
12. 允许前 5 秒不输出。

## Codex 工作规则

Codex 在执行任何任务前必须先阅读：

1. `AGENTS.md`
2. `README.md`
3. `docs/00_PROJECT_BRIEF.md`
4. `docs/01_DECISION_LOG.md`
5. `docs/02_MILESTONE_PLAN.md`
6. `docs/09_CODEX_RUNBOOK.md`
7. `docs/10_CODEX_NEXT_TASK.md`

如果 `docs/10_CODEX_NEXT_TASK.md` 没有明确要求写代码，Codex 不得生成算法代码。

## 实现阶段的通用约束

后续进入编码阶段时，默认遵守以下约束：

1. 使用 C99 或可兼容嵌入式 C 的实现风格。
2. 算法核心不得依赖文件系统、标准输出、操作系统线程或动态内存。
3. 所有运行期状态必须由 context / state 结构体承载。
4. 初始化、复位、逐点输入、输出查询应有清晰 API。
5. 内部参数应集中定义，避免散落 magic number。
6. Host 测试代码可以使用更宽松的资源，但算法核心不可以。
7. 所有新增实现必须配套最小测试。
8. 不得改变已冻结的 Owner 决策；如需改变，必须标记为 S0 并请求 Owner 决策。

## 问题分级

- S0：必须 Owner 决策，例如 API 改变、算法策略改变、资源约束冲突。
- S1：阻塞实现，但 Architect 可给 Codex 修复任务。
- S2：技术风险，记录但不阻塞当前里程碑。
- S3：风格、注释、清理问题，不打扰 Owner。

## 里程碑修复规则

每个里程碑最多两轮 Codex 修复。

如果两轮仍未通过，Architect 必须重新拆分任务，而不是继续让 Owner 搬运小修 prompt。
