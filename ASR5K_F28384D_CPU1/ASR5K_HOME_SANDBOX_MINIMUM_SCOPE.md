# ASR5K_HOME_SANDBOX_MINIMUM_SCOPE

建立：2026-06-13
性質：範圍定義文件。**不修改 code／README／D 系列文件，不做 build／refactor，不自行決定正式產品架構。**
上游：ASR5K_SANDBOX_CONFORMANCE_GAP_REPORT.md（PASS WITH DEVIATIONS）、ASR5K_DOCUMENT_CONFLICT_DECISION_REGISTER.md（DCR-01–08）、ASR5K_SANDBOX_PHASE_A_FIX_PLAN.md（hold 中）。
Implementation hold 持續有效。

---

## 1. Goal

Home Sandbox 的目的是在**家中唯一一塊 F28388D 開發板**上，建立一個**可移植的 firmware skeleton**，而非完成整台 ASR5K。

- **不是**完成整台 ASR5K（家中無 AM3352／SDRAM／M0／類比鏈／power stage）。
- **是**建立 common source firmware 骨架，使其在邏輯與資料流層面成立、可被觀察驗證。
- 核心目標：**common source 可移植到正式控制卡**——未來上正式 ASR5K 控制卡時，只替換 board layer / real hardware backend，即可進入上機測試，common layer 不重寫。
- 衡量成功的不是「跑了多少真硬體」，而是「未來替換 backend 時 common layer 改動量趨近於零」。

---

## 2. Formal Product Boundary

下列硬體**只存在於正式 ASR5K 控制卡**，家中 F28388D 開發板無法做真硬體驗證：

| 硬體 | 正式介面 | 家中板狀態 |
|---|---|---|
| AM3352（系統主控板） | SPIB（C2000 為 Slave） | 無 |
| W25Q64（8MB SPI NOR Flash） | SPIA（C2000 為 Master） | 無 |
| EMIF1 SDRAM（IS42S16160J，32MB @0x80000000） | EMIF1 CS0 | 無（家中板僅 IS61LV51216 async SRAM @0x00300000） |
| M0G3519（管理控制 MCU） | I2CA（C2000 為 Master） | 無（且無 I2C pull-up） |
| AD5543（DDS DAC） | SPIC | 無 |
| LTC2353（量測 ADC） | SPIC | 無 |
| AD7915（補償 ADC） | MCBSPA | 無 |
| AD5543（CC_DA 補償 DAC） | MCBSPA | 無 |
| FSI external chain（LVDS/RJ45 daisy-chain） | FSITXA/FSIRXA | 無對接機 |
| Power stage（PFC/DCDC/DCAC + 保護） | EPWM/CMPSS/GPIO | 無 |

**結論**：凡涉及上述硬體之「時序驗證、訊號完整性、真實電氣行為」一律無法在家中板完成，只能做軟體層資料流模擬。

---

## 3. Home F28388D Allowed Scope

家中板**只允許完成**以下項目（軟體層模擬／可移植骨架）：

| # | 項目 | 對應現況模組 |
|---|---|---|
| 1 | DDS runtime simulation | `dds/`（FSM 未動，指標化讀取） |
| 2 | Wave fake memory backend | `Wave_module/`（GSRAM_FAKE backend） |
| 3 | 4096-sample wave page read/write | `wave_memory_backend.c`（page write ptr／memtest） |
| 4 | fake wave activation | `WaveMem_ValidatePage`／`ActivatePage`（pointer 發布） |
| 5 | FSI 16-word frame format simulation | `fsi_sandbox`（R02_2 M+3S 佈局） |
| 6 | MCBSP staged→commit data model | `mcbsp_sandbox`（R02_3 模型） |
| 7 | fake AD5543 output sink | `output_sink`（AD5543 sink → TxBuf[2]） |
| 8 | fake LTC2353 / AD7915 sample source | `cv_sandbox`（DEBUG_FAKE／LTC2353 placeholder／MCBSP_AD7915） |
| 9 | M0 I2C transaction sandbox | `m0_sandbox`（D02_2_3 虛擬位址＋PEC FSM） |
| 10 | fake M0 register/status model | `m0_sandbox`（32 暫存器，**sandbox-only**，見 DCR-05） |
| 11 | fake Flash NOR semantic | `flash_sandbox`（erase=0xFFFF、write 只清位） |
| 12 | AM3352 fake command injection | `am3352_sandbox`（封包注入→dispatcher） |
| 13 | AM3352 fake status readback | `am3352_sandbox`（TX ping-pong 狀態包） |
| 14 | CPU1-only sandbox manager | `sandbox_manager`（統一 init/poll） |
| 15 | debug counters / watch variables | `g_sSandboxOverview` 及各模組 struct |
| 16 | （選用）CPU2 heartbeat / IPC ping-pong | `ipc_sandbox`（heartbeat 觀察＋fake echo） |

**範圍紀律**：超出此清單者，本階段一律不做（見 §4）。

---

## 4. Explicitly Out of Scope

以下項目**現在明確不做**（家中板無對應硬體或屬後續里程碑）：

- real EMIF1 SDRAM validation（memtest／refresh 保持測試 → 需正式控制卡）
- real W25Q64 flash driver（SPIA 指令時序 → 需真 flash）
- real AM3352 SPIB hardware integration（真 SPI slave + DMA CH3/CH4）
- real M0G3519 integration（真 I2CA + pull-up）
- real AD5543 / LTC2353 / AD7915 timing validation（類比鏈時序）
- real FSI daisy-chain（token passing／skew training／DMA 3-stage assembly）
- DMA runtime integration（SPIB DMA 化、FSI/MCBSP CPU2 DMA）
- CPU2 migration（FSI/MCBSP 搬遷 → 見 DCR-08）
- Flash partition / OTA（D08 空殼 → 見 DCR-04）
- Fault policy implementation（D13 空殼 → 見 DCR-04）
- Calibration / Parameter system（D14/D15）
- Power stage control（PFC/DCDC/DCAC）
- Protection trip integration（CMPSS/TZ）

---

## 5. Common Layer vs Board Layer

### 目標分層

**Common Layer（可移植，硬體無關）：**
- DDS（`dds/`）
- Wave Manager（`wave_memory_backend`）
- Wave Page API（`wave_memory_if.h`）
- Sandbox Command Dispatcher（`sandbox_cmd`）
- FSI Frame Formatter（`fsi_sandbox` 的 frame pack/unpack 部分）
- MCBSP Data Model（`mcbsp_sandbox` 的 staged/commit/plant 部分）
- M0 Transaction Layer（`m0_sandbox` 的虛擬位址＋PEC FSM）
- Flash Service Interface（`flash_sandbox` 的 read/write/erase API）
- External IC Interface（各 sink/source 抽象介面）
- Status / Diagnostic Interface（`sandbox_manager` overview）

**Board Layer（硬體相依，集中差異）：**
- `Board_F28388D_Home`（家中板：GSRAM_FAKE／EMIF1 async SRAM、fake IC backend、`pinmux.syscfg`、`emif1_home_sram.c`）
- `Board_ASR5K_Product`（正式卡：EMIF1 SDRAM、real IC backend、產品 pinmux、DMA 配置）

### 分層規則

1. Common layer **不得**直接依賴 GPIO number、pinmux、syscfg、DMA channel、真 IC 暫存器。
2. 硬體差異**必須集中在 board layer**。
3. fake backend 與 real backend **必須共用同一 API**（一個 config 點切換，如 `WAVE_BACKEND_SELECT`）。

### ⚠ 現況落差（NEEDS_VERIFICATION，誠實聲明）

**目前 code 尚未在結構上落實 Common/Board 分層**——本節定義的是「目標架構」，不是「已完成狀態」。已知違反規則 1 的具體點：

| 模組 | 現況硬體耦合 | 移植時須處理 |
|---|---|---|
| `output_sink.h` | include `meas_dds_module.h`、直接呼叫 `DAC_setShadowValue(DACA_BASE,…)`、寫 `g_u16TxSequenceBuf[2]` | INTERNAL_DAC／AD5543 sink 需抽到 board layer adapter |
| `cv_sandbox.c` | include `meas_dds_module.h`、呼叫 `ADC_readResult(ADCARESULT_BASE,…)` | INTERNAL_ADC source 需抽到 board layer |
| `cc_sandbox.c` | include `driverlib.h`、呼叫 `DAC_setShadowValue(DACB_BASE,…)` | INTERNAL_DAC CC sink 需抽到 board layer |
| `epwm_sandbox.c`／`ipc_sandbox.c` | include `shareram.h`、用 `U32_UPCNTS`／`sAccessCPU1` | 時基/共享 RAM 介面需 board 化 |

此落差列為移植前必須收斂項，**現階段不修（implementation hold）**，僅登記。Common layer「零硬體依賴」的達成度見 §8 DoD 對應條目（目前 UNMET）。

---

## 6. External IC Sandbox Boundary

| IC / Interface | Home Sandbox Implementation | Product Backend | Current Status | Migration Requirement |
|---|---|---|---|---|
| **AM3352 / SPIB** | `am3352_sandbox`：封包注入＋CRC16＋TX ping-pong；命令經 `SandboxCmd_Inject` 進 dispatcher | 真 SPIB Slave + DMA CH3/CH4 + legacy parser（`spi_b_slave.c`） | fake 注入/讀回可運作；**封包層僅 D11 Candidate B**（非正式主線，見 DCR-06） | 接真 SPIB transport；正式協定（Legacy vs Packet）由 protocol owner 裁決後對齊 |
| **W25Q64 / SPIA** | `flash_sandbox`：RAM 陣列＋NOR 語意（erase/write/read）＋計數器 | 真 W25Q64 + SPIA Master + DMA CH5/CH6（boot/OTA only） | fake backend 可運作 | 換 SPIA driver；分區/OTA 待 D08（空殼，DCR-04） |
| **M0G3519 / I2CA** | `m0_sandbox`：D02_2_3 交易層（10-bit addr＋6-bit len＋SMBus PEC）＋32 暫存器 fake map | 真 M0 + I2CA Master + pull-up | 交易層可運作；**register map 為 sandbox-only**（DCR-05） | 換 I2CA 真交易；建立正式 M0 Virtual Register Map 後對齊 |
| **AD5543 / SPIC DDS DAC** | `output_sink` AD5543 sink：寫 `g_u16TxSequenceBuf[2]`（不動 SPIC/DMA 時序） | 真 AD5543 + SPIC TX（DMA CH1） | TX word 寫入可運作；無真 DAC 輸出驗證 | 接真 AD5543；示波器驗 1kHz 波形 |
| **LTC2353 / SPIC ADC** | `cv_sandbox` `CV_SOURCE_SPIC_LTC2353` placeholder（讀 `g_sMeasDds.i16AdcCh0Raw`） | 真 LTC2353 + SPIC RX（DMA CH2） | 欄位已接、無真資料 | 接真 LTC2353；驗 100kHz 同步取樣 |
| **AD7915 / MCBSP ADC** | `mcbsp_sandbox` `ReadCvAd()` fake plant（一階遲滯模型） | 真 AD7915 + MCBSPA RX | fake plant 可運作 | 換 MCBSP 真 frame；CPU2 化（DCR-08） |
| **AD5543 / MCBSP CC_DA** | `mcbsp_sandbox` `WriteCcDa()`＋staged commit（R02_3） | 真 AD5543 + MCBSPA TX + EPWM CMPC ISR | staged-commit 模型可運作 | 換 MCBSP 真 frame；接真 CMPC 同步點；CPU2 化 |
| **FSI external module chain** | `fsi_sandbox`：16-word M+3S frame pack/unpack＋fake loopback | 真 FSITXA/FSIRXA + LVDS + daisy-chain | frame 格式可驗；fake wire 無錯誤注入 | 接真 FSI；token passing／skew training／DMA 3-stage；CPU2 化 |

---

## 7. Minimum Data Flow To Complete At Home

家中板最小可完成資料流（全部軟體層，無真硬體）：

```
[AM3352 fake command]
   am3352_sandbox 封包注入（Header+CmdID+Len+CRC16）
        │  Am3352Sandbox_Poll() 驗證 → SandboxCmd_Inject()
        ▼
[Sandbox Command Dispatcher]  SandboxCmd_Poll()
        │  WAVE_ACTIVATE / DDS_FREQ / AMP / OFFSET …
        ▼
[Wave fake memory]  Wave_module（GSRAM_FAKE）
        │  page0 4096 samples → Validate → Activate → active pointer
        ▼
[DDS runtime]  dds/（100kHz hook: DDS_Step → DDS_GetSample）
        │  讀 active wave pointer，相位累加查表
        ▼
[fake DAC output]  output_sink（AD5543 sink → TxBuf[2] / DEBUG / DACA）
        ▼
[fake ADC input]  cv_sandbox（DEBUG_FAKE / LTC2353 placeholder / AD7915）
        ▼
[MCBSP staged commit]  mcbsp_sandbox（WriteCcDa staged → CommitCcDa 同步點）
        │  plant 模型：CV_AD 追 CC_DA
        ▼
[FSI 16-word frame]  fsi_sandbox（M_Ref+CV_AD+S1/S2/S3 → pack → loopback → unpack）
        ▼
[fake M0 status]  m0_sandbox（虛擬位址+PEC 交易 → 暫存器讀回）
        ▼
[AM3352 fake status readback]  am3352_sandbox（TX ping-pong 狀態包：tick/state/dac/ready）
```

統籌：`sandbox_manager`（CPU1-only）一次 init、每圈 poll，依 R02_3 因果排序串接 CV→CC→FSI→commit。

---

## 8. Definition of Done For Home Sandbox

| # | 完成條件 | 現況判定 |
|---|---|---|
| 1 | common source builds clean | **NEEDS_VERIFICATION**（尚未 build；CCS 需先 F5 Refresh 收編 `Sandbox_module/`） |
| 2 | fake wave page 可容 4096 samples | 機制就緒（`WAVE_PAGE_LEN_WORDS=4096`、RAMGS4 section）；待 build/run 確認 |
| 3 | DDS 可讀 active wave pointer | 機制就緒（`getDDS()` 讀 `hal->pu16WaveTable`）；待 run 確認 |
| 4 | 100kHz runtime hook 可產生 samples | 機制就緒（`dmaCh1ISR` 內 Step→GetSample→Sink）；待 run 確認 |
| 5 | FSI 16-word frame pack/unpack 驗證 | 機制就緒（fsi_sandbox）；待 run 確認 |
| 6 | fake IC layer init/poll 全連接 | 就緒（`Sandbox_InitAll`/`PollAll` 涵蓋 13 模組）；待 run 確認 |
| 7 | fake AM3352 可注入命令 | 就緒（封包注入路徑）；待 run 確認 |
| 8 | fake AM3352 可讀狀態 | 就緒（TX ping-pong）；待 run 確認 |
| 9 | M0 交易層有 PEC pass/fail 計數 | 就緒（`g_sM0Xfer.u32OkCount`/`u32PecErrCount`）；待 run 確認 |
| 10 | fake Flash 有 read/write/erase 計數 | 就緒（`flash_sandbox` 計數欄位）；待 run 確認 |
| 11 | 各主要模組暴露 debug counters | 就緒（`g_sSandboxOverview` 彙整） |
| 12 | **common layer 無真硬體依賴** | **UNMET**——output_sink／cv_sandbox／cc_sandbox 仍直接呼叫 driverlib（見 §5 落差表），移植前須收斂 |

**DoD 整體狀態**：機制層面 #2–#11 就緒待上機確認；#1（build）與 #12（common 零硬體依賴）為兩個必須關閉的缺口，後者屬架構收斂、前者屬驗證動作，皆在 hold 解除後處理。

---

## 9. Not Product Complete Criteria

**明確聲明：即使 Home Sandbox 完成，也不代表正式 ASR5K 完成。**

以下項目**仍需正式控制卡**，Home Sandbox 完成度不可外推為產品完成度：

- SDRAM memtest（真 IS42S16160J + refresh）
- Flash partition validation（真 W25Q64 + D08 分區）
- AM3352 hardware SPIB（真 SPI slave + DMA）
- M0 real I2C（真 M0 + pull-up + 正式 register map）
- ADC/DAC waveform validation（真 AD5543/LTC2353/AD7915 時序）
- FSI daisy-chain（真 LVDS 多節點 + skew training）
- CPU2 runtime（FSI/MCBSP 在 CPU2 + IPC）
- DMA integration（CH1–CH6 真配置 + ownership 裁決）
- protection and calibration（CMPSS/TZ + D14/D15）

---

## 10. Migration Strategy To ASR5K Control Card

未來移植順序（每步只替換 backend，common layer 不重寫；前一步驗證通過才進下一步）：

1. **replace fake WaveMem with EMIF1 SDRAM backend**——`WAVE_BACKEND_SELECT` 切 PROD（先 HOME SRAM 過渡驗可移植性，再 PROD SDRAM）。
2. **replace fake AM3352 with SPIB real backend**——接 legacy SPIB parser（正式主線）；封包協定待 owner 裁決（DCR-06）。
3. **replace fake Flash with W25Q64 SPIA backend**——`flash_sandbox` REAL mode 接 SPIA；分區待 D08。
4. **replace fake M0 with I2CA real backend**——`m0_sandbox` REAL mode 接 I2CA；正式 register map 先行（DCR-05）。
5. **replace fake ADC/DAC with SPIC/MCBSP real backend**——output_sink/cv_sandbox/mcbsp_sandbox 接真類比鏈。
6. **replace fake FSI with real FSI network**——`fsi_sandbox` DAISY mode + token passing + skew training。
7. **migrate FSI/MCBSP modules to CPU2**——依 DCR-08，整包搬至 CPU2 專案 + MSGRAM 橋。
8. **enable DMA only after data format and ownership are verified**——資料格式與 ownership（含 CH1/CH2 deviation 裁決）確認後才開 DMA；CH5/CH6 僅 boot/OTA。

前置條件：移植第 1 步前須先關閉 §5 的 common layer 硬體依賴落差（否則 backend 替換點不乾淨）。

---

## 11. Guardrails

必須重申（違反任一項即破壞可移植性或覆蓋正式設計）：

1. **不得**把 sandbox fake IC 當正式 IC driver。
2. **不得**把 Packet Candidate 當正式 AM3352 protocol（Legacy 為現行主線，DCR-06）。
3. **不得**修改 D01–D11。
4. **不得**在 Runtime 使用 Flash（D03 §4／D04 §13）。
5. **不得**重配 DMA CH5/CH6 到 runtime（D02/D03/D04/D05/D11 五文一致）。
6. **不得**把 CPU1 sandbox deviation 當正式 CPU ownership（DCR-08，正式 FSI/MCBSP 屬 CPU2）。
7. **不得**讓 SEQ 使用 D05 Wave Checksum Area（DCR-01）。
8. **不得**引用未裁決的 M0 fake register map 作為正式協定（DCR-05）。

---

## 12. Final Recommendation

- **Home F28388D 先完成 10% firmware skeleton**——以「資料流貫通＋可移植骨架」為目標，不以硬體覆蓋率為目標。
- **不追求整台 ASR5K 完成**——§9 的產品完成項全部需要正式控制卡，無法在家中板達成。
- **以可移植 common layer 為主**——優先收斂 §5 的硬體依賴落差，讓 common/board 分層在 code 結構上真正成立；這是「未來只換 backend」承諾的前提。
- **等正式控制卡到手後再逐一替換 backend**——依 §10 順序，每步獨立驗證，DMA 最後才開。

短期可立即進行（不需硬體、不違反 hold 以外）：在解除 implementation hold 後，先處理 Phase A 三項低風險小修（Output ON protection／TRACK_MAX／DDS_STATE_ERROR export），再進行 §5 common layer 解耦。

---

*本文件為唯讀範圍定義產物。任何 code 變更、backend 替換、build 驗證需另行授權。*
