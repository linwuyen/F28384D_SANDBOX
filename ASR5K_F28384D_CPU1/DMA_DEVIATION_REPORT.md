# DMA_DEVIATION_REPORT — CH1/CH2 編號與正式架構相反

建立：2026-06-12（ASR5K_SANDBOX_PHASE1）
性質：**Code Deviation 記錄。本報告不修改任何正式架構文件（D01–D11），
也不改動任何 DMA channel assignment。**

---

## 1. 正式架構定義

| Channel | 正式用途 |
|---|---|
| CH1 | SPIC **TX** / DDS Output |
| CH2 | SPIC **RX** / ADC Capture |
| CH3 | SPIB RX |
| CH4 | SPIB TX |
| CH5 | SPIA RX（Boot / OTA / Maintenance only） |
| CH6 | SPIA TX（Boot / OTA / Maintenance only） |

## 2. 目前程式碼現況

來源：[SPIC_module/meas_dds_module.c](SPIC_module/meas_dds_module.c)
`initMeasDds()`（commit `1fc3e73` 引入之 non-blocking DMA 實作）

| Channel | 程式現況 | 觸發源 | 方向 |
|---|---|---|---|
| CH1 | SPIC **RX**（RX FIFO → `g_u16SpiRxBuf`）＋掛 `dmaCh1ISR` | `DMA_TRIGGER_SPICRX` | 周邊→記憶體 |
| CH2 | SPIC **TX**（`g_u16TxSequenceBuf` → TX FIFO） | `DMA_TRIGGER_ADCA1` | 記憶體→周邊 |
| CH3–CH6 | 未配置（SPIB/SPIA 目前為 polled FIFO） | — | — |

**Deviation：CH1 與 CH2 的 TX/RX 角色與正式架構互換。**
CH5/CH6 無 runtime 占用，符合「Boot/OTA/Maintenance only」約束。

## 3. 為什麼現況能動

DMA channel 編號本身沒有功能語義——任何 channel 都能服務任何觸發源與
周邊。資料流的正確性由「觸發源 → 搬運方向 → 完成中斷」決定：

```
EPWM1 SOCA(100kHz) → ADCA1 INT ──觸發──> CH2：3 words → SPIC TX FIFO
SPIC 移位 3×16bit @10MHz
SPIC RX FIFO 滿 3 ──觸發──> CH1：RX FIFO → g_u16SpiRxBuf → dmaCh1ISR
```

這條鏈與編號無關，唯一受編號影響的是 PIE 中斷向量：完成中斷掛在
CH1（`INT_DMA_CH1`）。功能正確、時序正確，只有「名字」與文件相反。

## 4. 改回正式架構的成本

| 項目 | 內容 |
|---|---|
| 程式修改 | `initMeasDds()` 內 CH1/CH2 的 base 對調（約 10 行）；ISR 改掛 `INT_DMA_CH2`；`DMA_clearTriggerFlag` 的 base 對調；ISR 改名或保留 |
| 重新驗證 | 整條 100kHz SPIC 鏈需全部重測（ISR counter、M_Ref 寫入、heartbeat、DDS 輸出、AD5543 timing）——這是主要成本 |
| 風險 | 對調過程若漏改任一 base，故障型態是「靜默資料錯位」而非編譯錯誤 |
| 預估工作量 | 修改 30 分鐘；完整回歸驗證一個上機 session |

## 5. 保留現況的風險

1. **文件—程式不一致**：後續開發者按正式文件操作 CH1（以為是 TX）會
   誤改 RX 設定。此為主要風險，屬文件性風險而非功能風險。
2. CH3/CH4（SPIB DMA 化）未來實作時若照文件配，與現況無衝突（3/4 未被
   占用），不受本 deviation 影響。
3. 無任何 runtime 功能風險：觸發源、FIFO 水位、完成中斷均與編號無關。

## 6. 建議裁決

**選項 A（建議）：修訂正式文件，CH1=SPIC RX、CH2=SPIC TX。**
理由：對調編號零功能收益；現況已通過建置與（即將）上機驗證；回歸成本
全部花在「讓名字對齊」上不划算。文件修訂屬 D 級文件變更流程，由架構
負責人裁決後執行。

選項 B：照正式架構對調程式。適用時機：若正式產品的除錯工具鏈、
量產測試腳本已寫死「CH1=TX」語義，則應改程式。

**在裁決前，程式維持現況，本報告為唯一事實記錄。**

## 7. 受影響檔案

- [SPIC_module/meas_dds_module.c](SPIC_module/meas_dds_module.c)（CH1/CH2 配置與 ISR）
- [SPIC_module/meas_dds_module.h](SPIC_module/meas_dds_module.h)（`dmaCh1ISR` 宣告）
- 正式架構文件中 DMA ownership 章節（僅在裁決選 A 時修訂）

## 8. 對 DDS / SPIC / ADC timing 的影響

**零。** DDS hook 掛在 RX 完成 ISR 尾端（無論該 ISR 叫 CH1 或 CH2），
sample 在 N 週期計算、N+1 週期由 TX channel 送出，固定 1 個 sample
（10µs）pipeline 延遲。編號裁決（A 或 B）皆不改變此時序。
