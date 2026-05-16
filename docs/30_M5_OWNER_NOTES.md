# 30_M5_OWNER_NOTES.md

版本：v0.1  
状态：给 Owner 的 M5 说明

## M5 做什么

M5 只做验证工具与数据契约：

1. 建立 normalized CSV 输入格式；
2. 实现 host 侧 `ppg_ibi_eval`；
3. 输出 PPG IBI vs ECG IBI 的基础误差指标；
4. 用 synthetic self-test 证明工具可运行。

## M5 不做什么

1. 不优化 PPG-IBI 算法；
2. 不接入真实私有 ECG 文件格式；
3. 不计算 HRV / RMSSD；
4. 不做医学级验证结论。

## 为什么这样拆

真实 ECG 数据格式尚未冻结。先实现 normalized CSV，可以让后续真实数据只需要先转换为统一格式，再进入同一个验证工具，避免每种设备格式都影响算法主工程。

## 后续 M6 可能做什么

M6 可基于真实数据结果做：

1. 真实 ECG/PPG 数据适配；
2. ECG/PPG 时间偏移估计；
3. 阈值调参；
4. 漏检/误检分层分析；
5. 输出 per-file 验证报告。
