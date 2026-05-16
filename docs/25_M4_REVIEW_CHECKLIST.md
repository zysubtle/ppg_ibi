# 25_M4_REVIEW_CHECKLIST.md

版本：v0.1  
状态：M4 审查清单

## 1. 通过条件

M4 通过必须同时满足：

1. `make clean && make test` 通过；
2. M3 public API 未被破坏；
3. 未使用动态内存；
4. 算法核心未使用文件系统、printf、OS、线程或第三方库；
5. 状态机符合 M3/M4 要求；
6. warmup / HOLD / REACQUIRE 阶段不输出 IBI；
7. TRACK 阶段能在合成 PPG 上输出 IBI；
8. flat PPG 不输出 IBI；
9. 严重丢点不跨段输出 IBI；
10. 生成 `docs/26_M4_CODEX_REPORT.md`。

## 2. S0：必须 Owner 决策

出现以下问题标记 S0：

1. Codex 修改 public API 语义；
2. Codex 要求改变项目目标，例如开始计算 HRV；
3. Codex 要求放宽 RAM `<20KB`；
4. Codex 要求引入第三方库或动态内存；
5. Codex 需要确认新的算法策略才能继续。

## 3. S1：阻塞实现，可由 Architect 给 Codex 修复

出现以下问题标记 S1：

1. 编译失败；
2. 测试失败；
3. `make test` 未同时运行 basic 和 synthetic；
4. warmup/HOLD/REACQUIRE 阶段输出了 IBI；
5. 合成 PPG 不输出 IBI；
6. flat PPG 输出了 IBI；
7. 严重丢点后跨段输出 IBI；
8. 使用了 `malloc/free/calloc/realloc`；
9. 算法核心使用了 `printf` 或文件系统。

## 4. S2：技术风险，不阻塞当前里程碑

可标记 S2：

1. M4 算法只适合干净 PPG；
2. 阈值尚未通过 ECG 数据调参；
3. confidence 只是启发式；
4. 未实现主通道选择；
5. 未处理 timestamp wrap-around；
6. 对强噪声、强运动伪影鲁棒性不足。

## 5. S3：风格/清理问题

可标记 S3：

1. 注释不足；
2. 内部变量命名可读性一般；
3. 测试打印略多但不影响判断；
4. 未使用字段可留待 M5 清理。

## 6. Owner 不应被打扰的问题

以下问题不需要 Owner 决策：

1. 内部阈值小幅调整；
2. 合成测试容差微调；
3. 内部 helper 函数拆分；
4. Makefile 增加测试目标；
5. context 末尾新增少量内部字段。
