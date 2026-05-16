# 19_M3_CODEX_TASK_DRAFT.md

状态：草案，Owner 确认 M2 后才可启用。

## M3 目标

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

## M3 明确不做

1. 不做完整 PPG 滤波；
2. 不做峰值检测；
3. 不输出真实 IBI；
4. 不实现 ECG 验证工具；
5. 不做复杂 SQI；
6. 不做 HRV。

## M3 验收

Codex 输出后，Architect 只审查：

1. 是否符合 M2 接口规格；
2. 是否没有动态内存；
3. 是否状态机行为正确；
4. 是否有基础单元测试；
5. 是否没有提前实现复杂算法导致不可控。

## M3 预期文件方向

实际文件名由 Codex 在 M3 任务中确定，但建议：

1. `include/ppg_ibi.h`
2. `src/ppg_ibi.c`
3. `tests/test_ppg_ibi_basic.c`
4. `docs/20_M3_CODEX_REPORT.md`

## Codex 默认指令方向

Owner 后续只需给 Codex：

> 请读取 `docs/10_CODEX_NEXT_TASK.md`，并严格执行。

正式任务将在 Owner 确认 M2 后写入 `docs/10_CODEX_NEXT_TASK.md`。
