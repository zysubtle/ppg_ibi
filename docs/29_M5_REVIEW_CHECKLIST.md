# 29_M5_REVIEW_CHECKLIST.md

版本：v0.1  
状态：M5 审查清单

## 1. API 与边界

- [ ] 未删除或重命名 M3/M4 public API
- [ ] 未改变默认配置值
- [ ] 未改变状态枚举数值
- [ ] 未改变 flags 数值
- [ ] 未实现 HRV / RMSSD
- [ ] 未删除 M3/M4 测试

## 2. 构建与测试

- [ ] `make clean && make test` 通过
- [ ] M4 basic test 仍运行
- [ ] M4 synthetic test 仍运行
- [ ] `tools/ppg_ibi_eval.c` 被编译
- [ ] `build/ppg_ibi_eval --self-test` 被执行
- [ ] `docs/31_M5_CODEX_REPORT.md` 包含实际终端输出

## 3. 数据契约

- [ ] 支持 normalized CSV header
- [ ] 字段为 `timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms`
- [ ] `allow_measure` 被传入算法
- [ ] `ecg_ibi_ms == 0` 不生成 truth event
- [ ] `ecg_ibi_ms > 0` 生成 truth event
- [ ] 非法行能被识别并报告

## 4. 验证工具

- [ ] 支持 `--self-test`
- [ ] 支持 `input.csv`
- [ ] 支持 `input.csv output_prefix`
- [ ] 能记录 PPG prediction events
- [ ] 能记录 ECG truth events
- [ ] 能进行时间窗匹配
- [ ] 能输出基础指标
- [ ] matched 为 0 时不会崩溃

## 5. 指标

- [ ] 输出 `truth_count`
- [ ] 输出 `pred_count`
- [ ] 输出 `matched_count`
- [ ] 输出 `miss_count`
- [ ] 输出 `extra_count`
- [ ] 输出 `mae_ms`
- [ ] 输出 `mean_error_ms`
- [ ] 输出 `rmse_ms`
- [ ] 输出 `max_abs_error_ms`
- [ ] 输出 `p95_abs_error_ms`

## 6. self-test

- [ ] 60 bpm synthetic 场景通过
- [ ] 75 bpm synthetic 场景通过
- [ ] 每个场景 `matched_count >= 3`
- [ ] 每个场景 `mae_ms <= 120`
- [ ] 每个场景 `max_abs_error_ms <= 200`
- [ ] self-test 成功时返回 0
- [ ] self-test 失败时返回非零

## 7. 资源与实现约束

- [ ] 算法核心未使用动态内存
- [ ] 算法核心未使用文件系统
- [ ] 算法核心未使用 `printf`
- [ ] host 工具不依赖第三方库
- [ ] 若使用数学函数，Makefile 正确链接 `-lm`
- [ ] 未引入大窗口缓存到 `ppg_ibi_context_t`

## 8. M5 通过条件

M5 通过需要同时满足：

1. 现有 M4 测试不退化；
2. 新增 eval 工具可编译；
3. `--self-test` 通过；
4. normalized CSV 路径可运行；
5. 指标字段齐全；
6. 报告完整；
7. 无 S0/S1 阻塞问题。
