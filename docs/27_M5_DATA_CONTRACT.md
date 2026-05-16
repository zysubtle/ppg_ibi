# 27_M5_DATA_CONTRACT.md

版本：v0.1  
状态：M5 normalized ECG validation 数据契约

## 1. 目的

M5 需要一个最小、明确、可复现的数据格式，用于将 PPG 输入与 ECG 真值 IBI 放在同一条时间轴上，驱动 host 侧验证工具。

真实项目中的 ECG 文件格式可能来自不同设备或软件，M5 不做猜测式适配。M5 只实现本 normalized CSV；真实数据格式适配放到后续里程碑或单独任务。

---

## 2. CSV 文件格式

M5 默认输入文件：

```csv
timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms
```

字段说明：

| 字段 | 类型 | 必填 | 说明 |
|---|---:|---:|---|
| `timestamp_ms` | uint32 | 是 | 当前 PPG 采样点时间戳，单位 ms |
| `ppg0` | int32 | 是 | 第 0 路 PPG |
| `ppg1` | int32 | 是 | 第 1 路 PPG |
| `ppg2` | int32 | 是 | 第 2 路 PPG |
| `ppg3` | int32 | 是 | 第 3 路 PPG |
| `allow_measure` | int | 是 | 1 表示允许测量，0 表示运动/禁止测量 |
| `ecg_ibi_ms` | uint16/uint32 | 是 | ECG truth IBI，单位 ms；0 表示本行无 ECG IBI truth |

---

## 3. 时间轴约定

1. 一行对应一个 PPG 采样点。
2. 默认采样率为 50Hz，即相邻样本约 20ms。
3. `timestamp_ms` 应单调递增。
4. M5 不处理 timestamp wrap-around。
5. M5 不处理乱序和重复行。
6. 若存在轻微丢点，仍交由算法内部现有 timestamp 逻辑处理。

---

## 4. ECG truth 表达

`ecg_ibi_ms > 0` 表示该行是一个 ECG truth event。

含义：

```text
ecg_ibi_ms = 当前 ECG R peak 与上一个 ECG R peak 的间隔
```

注意：

1. 第一拍 ECG R peak 通常没有 IBI，因此可填 0。
2. ECG IBI 不等同于 HRV/RMSSD；M5 只做逐搏 IBI 误差比较。
3. 若真实 ECG 数据只有 R peak timestamp，可先由外部脚本转换为本 normalized CSV。
4. 若 ECG 和 PPG 存在固定生理延迟，M5 通过 match window 做宽松匹配；M5 不估计 PTT/PAT。

---

## 5. allow_measure

`allow_measure` 直接传入 `ppg_ibi_input_t.allow_measure`。

约定：

```text
1 = 允许算法测量
0 = 当前运动或外部质量门控禁止测量
```

当 `allow_measure=0` 时，算法应进入 HOLD，验证工具仍记录状态和 flags，但不应产生 valid IBI。

---

## 6. 示例

```csv
timestamp_ms,ppg0,ppg1,ppg2,ppg3,allow_measure,ecg_ibi_ms
0,100000,100005,99998,100003,1,0
20,100100,100105,100098,100103,1,0
...
1000,101000,101005,100998,101003,1,1000
...
2000,101000,101005,100998,101003,1,1000
```

---

## 7. M5 不处理项

M5 不处理：

1. 多文件批量评估；
2. ECG R peak 自动检测；
3. ECG/PPG 自动时间对齐；
4. PTT/PAT 估计；
5. 真实设备私有 CSV 字段自动识别；
6. HRV/RMSSD；
7. 医疗级统计报告。
