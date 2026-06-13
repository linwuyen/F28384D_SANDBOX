# Wave Memory Backend — DDS Runtime External Wave Source

Created: 2026-06-11

## 架構

```
DDS Runtime (dds/)                ← 只看 active wave pointer，無任何位址
   ↓ wave_memory_if.h
Wave Memory Backend (Wave_module/)← page address / valid / active / pointer
   ↓ wave_platform_config.h       ← 唯一允許出現實體位址的檔案
Memory Backend 實體：
   - GSRAM Fake Page              (RAMGS4, debug 用)
   - Home Board EMIF1 SRAM        (IS61LV51216, CS3n, 0x00300000, 512K words)
   - Product Board EMIF1 SDRAM    (IS42S16160J, CS0, 0x80000000, 16M words)
```

切換平台只改 [wave_platform_config.h](wave_platform_config.h) 的一行：

```c
#define WAVE_BACKEND_SELECT  WAVE_BACKEND_HOME_EMIF1_SRAM
```

DDS Runtime（`dds/`）、SPIC driver、AD5543 輸出路徑完全不需修改。

## 100kHz 資料流

```
EPWM1(MEAS_CNV) 100kHz → ADCA SOC0 → ADCA INT1 → DMA CH2
→ SPIC 送 3 words（[2] = 上一週期的 DDS DAC code）
→ RX 滿 3 → DMA CH1 → dmaCh1ISR():
     DDS_Step();                              // FSM + 相位累加
     g_u16TxSequenceBuf[2] = DDS_GetSample(); // 經 active pointer 查表
```

固定 1 sample（10µs）pipeline 延遲。

## 開機順序（main.c）

1. `Board_init()`（SysConfig：產品 SDRAM 腳位組，不動）
2. `WaveMem_Init()` — HOME backend 時呼叫 `Emif1HomeSram_Init()`
3. `WaveMem_MemTest()` — 破壞性測試（data bus walk / address bus / page0 patterns）
4. `DDS_Init(100000, 100000, 65535, 32768)` — 100kHz、1kHz、滿幅、中點
5. main loop：`DDS_Poll()` 每圈建 1 點 sine → 4096 圈建完
   → `WaveMem_ValidatePage(0)` → `WaveMem_ActivatePage(0)` → attach pointer
   → `DDS_IsInitComplete()` 成立後自動 `DDS_Start()`

## 家中板 EMIF1 腳位（依開發板手冊接線確認）

家中板 IS61LV51216 接線特性（手冊確認）：
- SRAM A0 → **EM1BA1**（C2000 16-bit async 標準行為：word address LSB
  由 EM1BA1 輸出；GPIO21 已由 Board_init 設定）
- SRAM A1–A18 → EM1A0–A13、A15–A17、A14（亂序但一對一，軟體不需還原；
  2 冪次 address bus memory test 仍有效）
- /UB、/LB 接地 → DQM 不使用
- 實際用到的 EMIF 訊號：BA1 + A0–A17（**A18/A19 未使用**）

HOME build 額外設定的腳位（[emif1_home_sram.c](emif1_home_sram.c)，
GPIO 解碼證據見 [emif1_home_sram_pin_audit.md](emif1_home_sram_pin_audit.md) 第 6 節）：

| 訊號 | GPIO | 備註 |
|---|---|---|
| EM1CS3n | **29（預設）/ 19** | 原理圖無法解碼；memtest 第一步全面亂碼時改 `HOME_SRAM_CS3N_PINCFG=GPIO_19_EMIF1_CS3N` |
| EM1OEn  | **32（預設）/ 37** | 原理圖無法解碼；寫入正常但讀回全 0xFFFF 時改 `HOME_SRAM_OEN_PINCFG=GPIO_37_EMIF1_OEN` |
| EM1A1–A4 | **39 / 40 / 41 / 44** | 家中板用替代腳（≠syscfg 的 36/37/38/39），原理圖解碼確認 |
| EM1A13–A17 | 86–90 | 元件唯一腳；產品板 VMONI_LO / IMONI_LO / RSVD_SYNCIN 被覆蓋 |
| EM1D0–D4 | **85 / 83 / 82 / 81 / 80** | 家中板用替代腳（≠syscfg 的 55–59），原理圖解碼確認 |
| EM1D7 | **77** | 替代腳（≠syscfg 的 62） |
| （釋放） | 36/37/38/55–59/62 | syscfg 舊腳改回 GPIO，避免資料腳雙重 mux 造成讀取未定義 |

A0(GPIO35,推論)/A5–A12/BA1(GPIO21,推論)/D5–D6/D8–D15/WEn 沿用 syscfg。
HOME build 覆蓋後 0x80000000 SDRAM 視窗失效——**家中板勿觸發
HwVerification 的 SDRAM 測試（`g_hwTest.stSdram.u16Ctrl` 保持 0）**。

## Watch 變數

| 變數 | 成功條件 |
|---|---|
| `g_sWaveMem.u16Backend` | 1 (HOME_EMIF1_SRAM) |
| `g_sWaveMem.u32BaseWordAddr` | 0x00300000 |
| `g_sWaveMem.u16MemTestPass` | 1 |
| `g_sWaveMem.u16ActivePageId` | 0 |
| `g_sWaveMem.pu16ActiveWave` | 非 NULL (0x00300000) |
| `g_sWaveMem.u16Ready` | 1 |
| `sDDS.fgState` | DDS_STATE_RUNNING (0x08) |
| `sDDS.u32PhaseAccumulator` | 持續遞增循環 |
| `sDDS.u16RtIndex` | 0–4095 週期變化 |
| `g_sMeasDds.u16IsrCounter` | 持續遞增 |
| `g_sMeasDds.u16DacOut` | 32768±32767 正弦變化 |
| `g_u16TxSequenceBuf[2]` | 同上 |

## 最小驗證步驟

0. （除錯備援）`WAVE_BACKEND_SELECT = WAVE_BACKEND_GSRAM_FAKE` 先驗證
   interface→DDS→SPIC→AD5543 全鏈路，排除 EMIF 因素。
1. `WAVE_BACKEND_SELECT = WAVE_BACKEND_HOME_EMIF1_SRAM`，build / 燒錄。
2. 斷點停 memtest 後：`u16MemTestPass == 1`（fail 時看
   `u32MemTestFailAddr / u16MemTestExpect / u16MemTestRead`，
   address-bus fail 通常是 A13–A18 接線或 mux 問題）。
3. Memory Browser 看 0x00300000：sine pattern（Q15 bipolar）。
4. 全速跑，核對上表 watch。
5. 示波器量 AD5543 輸出：1kHz sine。
6. 改頻測試：`DDS_SetFrequency(200000)` → clamp 至 1000Hz 上限；
   `DDS_SetFrequency(50000)` → 500Hz。

## 邊界與注意事項

- **C28x word addressing**：所有 size/offset 以 16-bit word 為單位，
  `sizeof(uint16_t)==1`。1MB SRAM = 0x80000 words，CS3 視窗剛好對齊。
- 本驗證涵蓋 external memory runtime wave source，**不涵蓋 SDRAM
  refresh / timing**；產品板 SDRAM 初始化是獨立的未來任務。
- EMIF async timing 目前為保守值（~110ns/access），跑通後可向
  IS61LV51216-10 規格收斂（見 emif1_home_sram.c 常數）。
- SRAM 掉電即失；每次開機由 DDS_Poll 重建 sine（本里程碑預期行為）。
- M5（SPIB Wave Download）未來只需在下載完成後呼叫
  `WaveMem_ValidatePage / WaveMem_ActivatePage`，DDS Runtime 不變。
