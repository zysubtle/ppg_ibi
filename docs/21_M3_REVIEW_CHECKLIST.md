# 21_M3_REVIEW_CHECKLIST.md

版本：v0.1  
状态：M3 审查清单

## 1. 通过条件

M3 Codex 输出满足以下条件时，Architect 可判定通过：

1. 文件存在：
   - `include/ppg_ibi.h`
   - `src/ppg_ibi.c`
   - `tests/test_ppg_ibi_basic.c`
   - `Makefile` 或等价 host 编译说明
   - `docs/22_M3_CODEX_REPORT.md`
2. public API 与 `docs/20_M3_INTERFACE_CONTRACT.md` 一致；
3. 编译使用 C99，且 `-Wall -Wextra -Werror` 无错误；
4. host 单元测试通过；
5. 算法核心不使用动态内存；
6. `allow_measure=false` 能立即进入 HOLD；
7. HOLD 恢复后进入 REACQUIRE，并重新 warmup；
8. 轻微丢点只打 flag，不进入 REACQUIRE；
9. 严重丢点进入 REACQUIRE；
10. M3 不输出真实 IBI，即 `valid` 始终为 false。

## 2. 阻塞问题 S1 示例

以下问题可由 Architect 给 Codex 修复任务，不一定需要 Owner 决策：

1. API 名称拼写错误；
2. 缺少基础测试；
3. 状态转换测试不完整；
4. 丢点 flag 行为与任务不一致；
5. Makefile 不可运行；
6. 报告缺失。

## 3. Owner 决策 S0 示例

以下问题需要 Owner 决策：

1. Codex 认为必须改变 public API；
2. Codex 认为必须改变 `allow_measure` 语义；
3. Codex 认为必须改变 5 秒 warmup 规则；
4. Codex 认为必须引入动态内存；
5. Codex 认为 RAM 预算 `<20KB` 与后续算法不可兼容。

## 4. M3 不应因以下问题打断 Owner

以下属于 S2/S3，记录即可：

1. 注释风格可以更好；
2. 后续需要更完整 debug 字段；
3. SQI 尚未实现；
4. IBI 尚未输出；
5. ECG 验证尚未实现。
