# 12_M2_SPEC_INDEX.md

版本：v0.1  
状态：待 Owner 确认后冻结

## M2 结论

本项目在 M2 阶段冻结为：

> 一个面向 Apollo3.5 MCU 的纯 C 嵌入式 PPG-IBI 算法库。算法逐点接收 4 路同步绿光 PPG，结合外部 `allow_measure` 标志，在允许测量且信号质量合格时逐搏输出 IBI，不计算 HRV。

## M2 规格文档

| 文件 | 用途 |
|---|---|
| `docs/13_M2_INTERFACE_SPEC.md` | API、输入、输出、配置、调试输出规格 |
| `docs/14_M2_ALGORITHM_SPEC.md` | 算法处理流程、多通道策略、IBI 检测原则 |
| `docs/15_M2_STATE_AND_QUALITY_SPEC.md` | 状态机、运动门控、丢点处理、质量门控 |
| `docs/16_M2_VALIDATION_SPEC.md` | ECG 真值验证方法和指标定义 |
| `docs/17_M2_RESOURCE_BUDGET_SPEC.md` | RAM / CPU / Flash 预算和实现约束 |
| `docs/18_M2_RISK_REGISTER.md` | 当前风险清单与后续处理方式 |
| `docs/19_M3_CODEX_TASK_DRAFT.md` | M3 第一轮 Codex 任务草案，待 Owner 确认 M2 后启用 |

## M2 冻结项

1. 接口采用调用方持有 context 的纯 C 库风格。
2. 每次输入一个时间点的 4 路 PPG 样本。
3. 输出采用“每点检查，逐搏 valid”的模式。
4. `allow_measure=false` 时立即停止输出并进入 HOLD。
5. 从运动恢复或严重丢点恢复后，重新进入 REACQUIRE，并等待 5 秒再允许输出。
6. 多通道策略采用主通道选择为主，备份通道辅助，不在 M4 强制实现复杂融合。
7. IBI 单位为 ms，默认有效范围 300~2000ms。
8. 置信度为 0.0~1.0 的连续值。
9. M3 不实现完整 IBI 算法，只实现框架、接口、状态和基础测试。

## Owner 只需确认的问题

是否接受以上 M2 冻结项，并允许进入 M3。

如不接受，请只指出需要修改的编号。
