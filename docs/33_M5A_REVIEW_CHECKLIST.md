# 33_M5A_REVIEW_CHECKLIST.md

## M5a Review Checklist

### 1. 边界

- [ ] 未修改 `src/ppg_ibi.c`
- [ ] 未修改 M3/M4 public API
- [ ] 未删除 basic / synthetic 测试
- [ ] 未实现 HRV / RMSSD
- [ ] 未抢做 M5b CSV 解析

### 2. metrics engine

- [ ] prediction / truth 事件均为固定容量数组
- [ ] event 超限返回非零并打印错误
- [ ] 匹配使用 `M5A_MATCH_WINDOW_MS = 500`
- [ ] 超出 500ms 的事件不得匹配
- [ ] 每个 truth 最多匹配一次
- [ ] extra / miss 计算正确
- [ ] 输出 truth_count / pred_count / matched_count / miss_count / extra_count
- [ ] 输出 mae / mean_error / rmse / max_abs / p95_abs

### 3. self-test

- [ ] 60 bpm 场景时长 >= 16 秒
- [ ] 75 bpm 场景时长 >= 16 秒
- [ ] self-test 调用 `ppg_ibi_process()`
- [ ] self-test 收集 prediction events
- [ ] self-test 收集 truth events
- [ ] self-test 调用 metrics engine
- [ ] 60 bpm matched_count >= 3
- [ ] 75 bpm matched_count >= 3
- [ ] mae_ms <= 120
- [ ] max_abs_error_ms <= 200
- [ ] 失败时返回非零

### 4. Makefile

- [ ] `make test` 运行 basic
- [ ] `make test` 运行 synthetic
- [ ] `make test` 运行 `ppg_ibi_eval --self-test`
- [ ] 编译参数包含 `-std=c99 -Wall -Wextra -Werror -pedantic -Iinclude`
