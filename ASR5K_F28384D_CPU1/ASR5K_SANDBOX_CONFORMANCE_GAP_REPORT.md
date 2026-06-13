# ASR5K_SANDBOX_CONFORMANCE_GAP_REPORT

建立：2026-06-13
審查者：Sandbox 開發線（自審）
審查對象：`Sandbox_module/`、`Wave_module/`、`dds/`、`SPIC_module/`、`SPIB_module/`（現況 code）
對照基準：正式設計文件 D01–D15、R01/R02 系列（`D:\Downloads\ASR5K設計文件\`）

---

## 1. Scope

本報告**只做符合性審查（conformance review）**：

- 不修改任何正式設計文件（D01–D15）
- 不修改任何 source code
- 不重新定義架構、不裁決開放問題
- 所有結論附文件引用（[DOC: …]）或程式碼證據（[CODE: …]）
- 找不到證據之處標記 **NEEDS_VERIFICATION**，不假設已完成

---

## 2. Formal Design Source

| 文件 | 已取得 | 約束力摘要 |
|---|---|---|
| asr5k_readme.pdf / 正式交接文件 | **NEEDS_VERIFICATION**（資料夾中無此檔，僅 md 文件與 gw_job-main.zip） | 最高優先 |
| D01_DESIGN_CONTROL_ARCHITECTURE | ✅ | **CPU1=量測核心（SPIC/SPIB）、CPU2=補償核心（MCBSP/FSI）**；EPWM1 100kHz Master Trigger；SPIC jitter-free 鐵律 |
| D02_COMM_ARCHITECTURE | ✅ | SPIB/SPIA/I2CA/SCI 全掛 CPU1；CPU1 六通道 DMA 配置表；CPU2 用自己獨立的 6 通道 DMA |
| D03_MEMORY_ARCHITECTURE | ✅ | W25Q64→SDRAM→GSRAM 階層；**GSRAM 不作為 Permanent Wave Database**；現行產品 Wave Page = **0x0000–0x0012（19 頁）**；Runtime 禁 Flash |
| D04_WAVE_DATA_PIPELINE | ✅ | Wave 生命週期：Download→SDRAM→Validation→Activation→Runtime→（選擇性）Flash Save；**Download Complete = 4096 samples + Complete Command 兩者同時成立**；Output ON 禁止清單 |
| D05_EMIF1_MEMORY_MAP | ✅ | SDRAM 0x80000000 大區佈局（Wave 2MB／**Wave Checksum 2MB**／Raw／Cal 1MB／Param 1MB／OTA 4MB／Debug 2MB）；256 頁容量、現用 19 頁；Program Window 0x00200000 保留 |
| D07_DDS_RUNTIME_MANAGER | ✅ | Runtime Source=**EMIF1 SDRAM、CPU Direct Read**；`index = phase_acc >> 20`；**GSRAM 不作 Wave Cache / ActiveWaveBuffer**；v0.1 不做 DMA Preload |
| D10_WAVE_VALIDATION_POLICY | ✅ | Page 七狀態機（EMPTY…LOCKED）；**per-sample checksum word**（`(s>>8)+(s&0xFF)`）存獨立 Checksum Area；Validation 8 步驟；錯誤碼 0x0001–0x0009；Validation 不得進 100kHz ISR |
| D11_AM3352_PROTOCOL | ✅ | **Candidate A = Legacy Register-Based（主線）／Candidate B = Packet（候選）**；正式位址配置（0x0900/0x0901/0x0910–0x0912/0x0958/0x0960/0x0961/0x0970/0x3000–0x3FFF）；Packet Header 暫定 **0x55AA**、checksum 暫定 **Legacy Checksum**；Internal Command Event Layer 強制；Output Protection Rule；DMA ownership |
| R01_1/2/3（DDS） | ✅ | DDS 全功能規格與參數範圍 |
| R02_1/2/3/4（FSI） | ✅ | 16-word M+3S 採納版封包；CC_DA 全域同步（CMPC @8µs） |
| D12/D13/D14/D15、D06/D08、ASR5K_M0_COMM、M0_README | 存在、本次未逐份精讀 | 章節 10–12 僅引用其名稱與分工，內容對齊標 NEEDS_VERIFICATION |

Sandbox code 在優先序中位列**最末**；本報告一律以文件裁決偏差方向。

---

## 3. Implemented Sandbox Features 分類

| 功能 | 檔案 | 分類 |
|---|---|---|
| Wave Memory Interface／backend（GSRAM_FAKE／HOME_SRAM／PROD_SDRAM）＋memtest | Wave_module/* | (a) Product-aligned（介面）＋(b) Sandbox-only（GSRAM_FAKE、HOME_SRAM 本體） |
| DDS pointer 化（讀 active pointer、FSM 未動） | dds/* | (a) Product-aligned（符合 D07 CPU Direct Read 模型） |
| DDS 13 命令 dispatcher（含 Ramp/Delay/Phase） | Sandbox_module/sandbox_cmd.* | (a) 功能面 product-aligned（R01_1）；**(d) 命令編碼 deviation**（id 1–13 非 D11 位址 0x09xx，見 §4） |
| Output Sink（DEBUG/DACA/AD5543） | output_sink.* | (b) Sandbox-only（觀測扇出）；AD5543 路徑 (a) |
| 100kHz ISR hook（DDS_Step→GetSample→Sink） | SPIC_module/meas_dds_module.c | (a) Product-aligned（D01 2.1.1 ISR 動作） |
| AM3352 packet layer（Header/CmdID/Len/CRC16＋PingPong） | am3352_sandbox.* | **(c) Future candidate（D11 Candidate B）＋(d) 細節 deviation**（見 §4） |
| FSI 16-word M+3S frame stub | fsi_sandbox.* | (a) frame 佈局 aligned（R02_2/R02_4）；(b) fake wire／fake slaves |
| MCBSP staged→commit（CC_DA 同步） | mcbsp_sandbox.* | (a) 機制 aligned（R02_3 §6）；(b) fake plant；**(d) 跑在 CPU1**（D01 規定 CPU2） |
| CV/CC source-sink 抽象 | cv_sandbox.* / cc_sandbox.* | (b) Sandbox-only（CC 控制律=測試 pattern，正式屬 D12 範圍） |
| M0 暫存器檔＋D02_2_3 交易層（虛擬位址＋PEC） | m0_sandbox.* | (a) 交易層 aligned（D02_2_3）；**(d) register map 為 sandbox 自創**（見 §10） |
| Fake flash（NOR 語意） | flash_sandbox.* | (b) Sandbox-only placeholder（D08 未對齊） |
| SEQ stepper＋wave_seq_layout.h | seq_sandbox.*、Wave_module/wave_seq_layout.h | (c) Future candidate；**佈局出自系統圖，未經 D 系列文件背書 → NEEDS_VERIFICATION** |
| IPC heartbeat 觀察＋fake echo | ipc_sandbox.* | (b) Sandbox-only（正式 MSGRAM 交握屬 D02 4.3／M3） |
| EPWM tick 觀察器 | epwm_sandbox.* | (b) Sandbox-only（不改 timing，無風險） |

---

## 4. Protocol Boundary: Legacy vs Packet Candidate

**裁決基準：[DOC: D11 §4] 「Candidate A: Legacy Register-Based Protocol／Candidate B: Packet Protocol。最終產品只選擇一組。」**

| 檢查項 | 結果 |
|---|---|
| 正式主線現況 | **Legacy Register-Based Protocol（Candidate A）**。code evidence：[CODE: SPIB_module/spi_b_slave.c `runSPIBslave()`＋cmd_parser.h] 即 Candidate A 實作，本次 sandbox 工作**未修改**它 ✅ |
| Packet 的定位 | `am3352_sandbox` 的封包層**只屬 D11 Candidate B 的探索性模擬**，不得視為已定案 |
| **偏差 1：Header 值** | sandbox 用 `0xA55A` [CODE: am3352_sandbox.h `AM3352_PKT_HEADER 0xA55AU`]；D11 §7.2 暫定 **0x55AA**。**不符** |
| **偏差 2：Checksum 演算法** | sandbox 用 CRC-16/MODBUS [CODE: am3352_sandbox.c `Am3352Sandbox_Crc16`]；D11 §7.6 暫定 **Legacy Checksum**（公式為 Open Question #1）。**自行選用，未經文件背書** |
| **偏差 3：Command ID 空間** | D11 §7.3 規定 Packet 的 Command ID **沿用 Register Address**（0x0900/0x0958/0x3000…）；sandbox 用自編 id 1–13 [CODE: sandbox_cmd.h]。**不符** |
| Internal Command Event Layer | D11 §14 強制兩種 protocol 收斂到同一事件層——sandbox 的 `SandboxCmd_Inject()` 即此概念的對應物 ✅（語意對齊、編碼不對齊） |
| 誤導性文字檢查 | [CODE: ASR5K_SANDBOX_PHASE1_README.md §11.3] 寫「D02_2_1 封包」未標示 Candidate 地位；[CODE: am3352_sandbox.h 頭註] 稱 "the REAL contract"——**有讓讀者誤認 Packet 已是正式協定的風險**。應補 Candidate B 註記（本報告僅記錄，不改檔） |

**結論：sandbox packet layer = D11 Candidate B 方向的雛形，三處細節與 D11 v0.1 暫定值不符；Legacy（spi_b_slave）完好未動，主線未被覆蓋。**

---

## 5. SPIB / AM3352 Route Conformance

正式 route [DOC: D04 §5／D03 §6／D11 §5]：`AM3352 → SPIB RX FIFO → DMA CH3 → RxFrame Ping/Pong → Parser → EMIF1 SDRAM`

| 環節 | 現況 | 判定 |
|---|---|---|
| SPIB RX FIFO→Parser | [CODE: spi_b_slave.c] **polled FIFO**，無 DMA CH3 | 偏差（已記錄於 DMA_DEVIATION_REPORT；CH3 未被他用，可按文件補配） |
| DMA CH3/CH4 | 未配置 | 缺口（合規方向：照 D02 §5 配置） |
| RxFrame Ping/Pong | repo M2R 里程碑有契約文件（WP_3352_SPI 側）；本專案 spi_b_slave 用環形 u32RxD buffer | NEEDS_VERIFICATION（兩實作系譜需對齊） |
| Parser→EMIF1 SDRAM | [CODE: spi_b_slave.c] block 下載目標是 **GSRAM `g_u16SpiBlockRam[4095]`**，非 SDRAM | **偏差**：與 D04 §5「Parser→EMIF1 SDRAM」不符；且 buffer 為 4095 詞 ≠ 4096 [CODE: spi_slave.h `SIZE_OF_SPI_BLOCK_RAM 4095U`] → **疑似 off-by-one，NEEDS_VERIFICATION** |

**位址保留檢查**（對照 [DOC: D11 §8]）：

| D11 正式位址 | 現行 code | 判定 |
|---|---|---|
| 0x0958 WAVE_PAGE_SELECT | [CODE: cmd_id.h] `Wave_Data_Address_Page_spi_addr 0x0704` | **不一致**——legacy code 用 0x0704；D11 v0.1 規定 0x0958。NEEDS_VERIFICATION（何者為準需正式裁決；D11 引用「Existing ASR5K screenshots」，兩者可能版本錯位） |
| 0x3000–0x3FFF Wave Data Window | [CODE: cmd_id.h] `Spi_Block_Data_Base 0x3000`／`Last 0x3FFE`／`End 0x3FFF` | **部分一致**：window 吻合；但 0x3FFF 被當「End 標記」使用，而 [DOC: D10 §6] 明定「不以收到 0x3FFF 作為唯一完成條件」→ NEEDS_VERIFICATION |
| Download Complete Command | 現行以 0x3FFF end-marker＋checksum 暫存器代行 | 缺正式 Complete Command（D11 未配位址，屬 D11 開放項） |
| 0x0960 WAVE_VALIDATE | code 中**不存在** | 缺口 |
| 0x0961 WAVE_ACTIVATE | code 中**不存在** | 缺口 |
| 0x0900/0x0901 OUTPUT_ON/OFF | [CODE: cmd_id.h] 有 `Output_Waveform_Set 0x0902`，無 0x0900/0x0901 | **不一致**，NEEDS_VERIFICATION |

**Output ON/OFF 權限檢查**（[DOC: D04 §11／D10 §12／D11 §11]）：

- **sandbox dispatcher 完全未檢查 output 狀態**：[CODE: sandbox_cmd.c `sandboxCmdWaveActivate()`] 在 DDS RUNNING 時照樣 validate＋activate＋換 pointer。**直接違反「Activation 條件 = Wave Page Valid + Output OFF」**。⚠ 高優先偏差。
- legacy spi_b_slave 是否拒絕 Output ON 時的 wave 寫入：未查到對應檢查 → NEEDS_VERIFICATION。

---

## 6. Wave Pipeline Gap（對照 D04 生命週期＋D10 驗證政策）

| 生命週期階段 | Sandbox 現況 | 判定 |
|---|---|---|
| Wave Download（0x3000 window→SDRAM） | 無（GSRAM fake 由 CPU 建表；legacy block 下載進 GSRAM） | **缺** |
| Download Complete（4096+Command 雙條件） | 無 | **缺** |
| Validation | [CODE: wave_memory_backend.c `WaveMem_ValidatePage`]：加總 checksum＋非平坦檢查 | **與 D10 不符**：無 per-sample checksum word、無獨立 Checksum Area（D05 0x00200000）、無 8 步驟程序、無 Output OFF 前置、無錯誤碼 0x0001–0x0009 |
| Page State Machine | 只有 `u16PageValidMask`＋`u16Ready` | **缺** D10 §4 七狀態（EMPTY…LOCKED）；LOCKED（Output ON 時 active page 鎖定）完全未實作 |
| Activation（Valid＋Output OFF） | 有 Valid 檢查、**無 Output OFF 檢查** | **偏差**（同 §5） |
| Activation fail 保留前一 valid page | [CODE: `WaveMem_ActivatePage` 失敗回 0 不動 pointer] | ✅ 符合 D11 §10.5 |
| Runtime Read（SDRAM CPU direct） | 介面符合 D07；目前跑 GSRAM_FAKE | ✅（介面）／待 PROD 驗證 |
| Readback（SDRAM→TxPingPong→CH4） | 無 | **缺** |
| Flash Save/Load（CH5/6） | 無（fake flash 僅 NOR 語意模擬） | **缺** |

**缺少的測試項（D10 §13／D11 §18 要求、sandbox 無對應負面測試）**：
4096 samples received、Download Complete missing、Address discontinuity、Sample count error、Checksum error、**Output ON reject**、Activation fail keeps previous valid page（最後一項機制存在但無測試程序）。

另：`WAVE_PAGE_TRACK_MAX = 16` [CODE: wave_platform_config.h] **小於產品現用 19 頁**（[DOC: D03 §5 Page 0x0000–0x0012]）→ page 16–18 無法進 valid mask。**具體錯誤，必修。**

D05 位址公式以 **byte** 表述（`page_id * 8192 Bytes`、`sample_index * 2`）；C28x 為 word addressing，[CODE: wave_platform_config.h] 以 word 計（stride 4096 words = 8192 bytes，物理一致）。文件與 code 的單位語言不同步 → NEEDS_VERIFICATION（建議文件側標註 C28x word 對應，本報告不改文件）。

---

## 7. CPU1 / CPU2 Boundary

正式分工 [DOC: D01 §1.1／D02 §4]：CPU1 = SPIB/SPIA/I2CA/SCI/system state/wave management；CPU2 = MCBSP/FSI/補償迴路（且 CPU2 有獨立 6 通道 DMA）。

| Sandbox 模組 | 正式歸屬 | 移植邊界 |
|---|---|---|
| sandbox_cmd、am3352、flash、m0、seq、epwm、ipc、output_sink、Wave_module、dds | CPU1 | CPU1-only，留在 CPU1 ✅ |
| **mcbsp_sandbox** | **CPU2** | future CPU2 module（介面已自足，可整包搬） |
| **fsi_sandbox** | **CPU2** | future CPU2 module（含未來 DMA 3-stage assembly 用 **CPU2 的 DMA**，不占 CPU1 CH1–6 [DOC: D02 §5 結論 5]） |
| cv_sandbox / cc_sandbox | 跨界（CV 量測面 CPU1、補償面 CPU2） | 拆分點＝D12 控制律落地時 |

**需要的 IPC / MSGRAM 橋（[DOC: D02 §4.3]）——sandbox 目前只有 heartbeat，全部未實作：**

- CPU1→CPU2（`MSGRAM_CPU1_TO_CPU2`＋IPC interrupt）：補償增益/PID 參數、補償 Enable、Output ON/OFF 狀態、L1/L2/L3_Ref＋FuncID/Data（現由 dmaCh1ISR 寫 GSRAM shareram——形式與 MSGRAM 規範的對齊 NEEDS_VERIFICATION）、wave/system state 摘要。
- CPU2→CPU1：CV_AD 即時讀值、CC_DA 現值、FSI link 狀態/斷線錯誤碼、硬體過載警告、補償迴路 heartbeat。

---

## 8. DMA Ownership Mapping

正式表 [DOC: D02 §5／D03 §9／D04 §14／D05 §11／D11 §16]（五份文件一致）：

| Sandbox 模組 | 目前模擬方式 | 正式硬體資源 | 正式 DMA | Runtime 允許 | 現況判定 |
|---|---|---|---|---|---|
| output_sink AD5543 路徑 | ISR 寫 `g_u16TxSequenceBuf[2]` | SPIC TX | **CH1** | ✅ 常駐 | [CODE: meas_dds_module.c] 實際 TX 掛 **CH2**（trigger ADCA1）→ **編號對調 deviation，已記錄於 DMA_DEVIATION_REPORT.md，未裁決** |
| meas_dds ADC 解包 | DMA RX→`dmaCh1ISR` | SPIC RX | **CH2** | ✅ 常駐 | 實際掛 **CH1**（同上 deviation） |
| am3352_sandbox RX | 軟體直寫 RxPacketBuffer＋flag | SPIB RX FIFO | **CH3** | ✅ 常駐 | 未配置（sandbox 以 memcpy 模擬，合規方向） |
| am3352_sandbox TX | 軟體 PingPong 填包 | SPIB TX FIFO | **CH4** | ✅ 常駐 | 未配置（同上） |
| flash_sandbox | RAM 陣列模擬 | SPIA RX | **CH5** | ❌ **Boot/OTA/Maintenance only** | 未占用 ✅ 合規 |
| flash_sandbox | RAM 陣列模擬 | SPIA TX | **CH6** | ❌ **Boot/OTA/Maintenance only** | 未占用 ✅ 合規 |
| fsi_sandbox / mcbsp_sandbox | 純軟體 | FSI/MCBSP（CPU2） | **CPU2 專屬 DMA** | — | 未占 CPU1 通道 ✅ |

**CH5/CH6 不得於 normal runtime 重配**——五份文件同文重申；現況無任何 runtime 占用 ✅。
唯一活躍 deviation = CH1/CH2 編號互換（功能正確、名實不符），維持「只記錄、待裁決」。

---

## 9. Timing Evidence Required

**現況：以下全部僅有架構估算，無任何實測。**

| 項目 | 文件估算依據 | 證據等級 |
|---|---|---|
| 100kHz ISR worst-case（含 DDS hook＋sink） | D01 隱含 <10µs；R01_3 稱優化後 <2.5µs | **estimated**（R01_3 數字屬舊平台量測，本專案 formal board pending） |
| DDS_Step / DDS_GetSample 耗時 | R01_3 指令級分析 | estimated |
| EMIF1 SDRAM CPU read latency（D07 v0.2 升級判準） | D07「若不足以支援 100kHz 再演進」 | **formal board pending**（關鍵判準，無數據） |
| FSI frame build/parse | R02_1 §6：wire 1.42µs＋proc 0.5µs | estimated |
| MCBSP staged write | R02_3 §3：0.2µs | estimated |
| CMPC commit ISR（T=8µs 點的 ISR 進出） | R02_3 §6.5 預留 0.8µs 緩衝 | estimated（真 CMPC ISR 未實作） |
| SPIB background parser（每 main-loop pass 耗時） | D02_2_1「不可阻擋」原則 | estimated（`tpMainCost` 機制存在 [CODE: main.c] 但未啟用記錄） |
| M0 I2C polling slice | D02_2_3 非阻塞原則 | estimated（fake 模式每 poll 一 byte，真 I2C 未存在） |

建議證據升級路徑：CCS profiler → GPIO toggle scope（家中板可做前兩級）→ formal board。

---

## 10. M0 Functional Gap

現況：[CODE: m0_sandbox.*] 已有 (1) D02_2_3 交易層（虛擬位址＋PEC＋非阻塞 FSM）✅、(2) 32 暫存器 fake map 含 fan/protection/IPK/ADC 欄位。

| 缺口 | 說明 |
|---|---|
| **Register map 正當性** | 現行 `M0_REG_*` 佈局為 **sandbox 自創**（依產品系統圖推定），未對照 `ASR5K_M0_COMM.md`／`M0_README.md` → **NEEDS_VERIFICATION，需建立正式 M0 Virtual Register Map 文件對齊** |
| Range switch | 系統圖 GROUP_OUT 有 RANGE_HL/P_RANG_HL 位元，僅止於 fake bitmap，無切換時序/互鎖邏輯 |
| Hardware protection input 處理 | GROUP_IN 位元存在、恆 0；無 debounce/latch/回報政策 |
| Fan control 策略 | 只有 duty 暫存器；無溫度→轉速控制律（正式歸屬 M0 韌體或 C2000？NEEDS_VERIFICATION） |
| Fan tach readback | fake 直接回填 duty；無實際 tach 換算 |
| M0 fault mapping | M0 故障 → C2000 fault code 的映射不存在（屬 D13 範圍） |
| M0 timeout / PEC error policy | PEC 錯誤只計數；無 retry/斷線判定/降級行為（D02_2_3 §7.1 要求丟棄＋排程重試——未實作） |

---

## 11. Fault Policy Gap

現況：各模組有獨立 error counter（[CODE: am3352 `u32CrcErrCount`、m0 `u32PecErrCount`、fsi `u32ErrorCount`、wave `u16MemTestPass`、meas_dds `u16SpiTimeoutCnt`…]），**互不相通，無系統級 fault policy**。

缺（僅列缺口，最終政策由 **D13_FAULT_POLICY.md** 定義，本報告不代訂）：

- fault source 登錄表（模組→來源編號）
- 統一 fault code 空間（D11 §13 僅給 protocol 測試用 6 碼，明言 full policy deferred to D13）
- severity 分級、first-fault latch、clear rule（D11 0x0970 CLEAR_FAULT 已配位址、無實作）
- output shutdown rule（哪些 fault 觸發 OUTPUT_OFF）
- AM3352 readback format（fault 區塊在 status 回傳中的格式）

---

## 12. Calibration / Parameter / Flash Persistence Gap

| 項目 | 正式依據 | 現況 |
|---|---|---|
| Calibration data map | D14＋D05 Calibration Area（0x80000000+0x00800000，1MB） | **全缺** |
| Parameter map | D15＋D05 Parameter Area（+0x00900000，1MB） | **全缺** |
| Factory mode / ATE mode | D10 §15 提及 ATE readback | 全缺 |
| Flash save/load policy | D04 §13＋D08 | 全缺（fake flash 僅語意模擬） |
| Data CRC / versioning / corrupt fallback | D08（NEEDS_VERIFICATION 細節） | 全缺 |
| **Output ON 禁止 calibration/parameter update** | [DOC: D05 §10 明文] | 無任何強制機制（與 §5 Output 保護缺口同源） |

---

## 13. Test Matrix（建議）

| Test ID | 名稱 | 依據文件 | 預期行為 | 現況 | Gap / Risk |
|---|---|---|---|---|---|
| T-DDS-01 | 13 命令逐一注入（含越界參數拒絕） | R01_1／D11 §9 | 參數套用、越界 reject++ | 可執行（debugger） | 未跑；freq 單位 x100 為假設（D11 開放項） |
| T-DDS-02 | DC mode（freq=0）／AMP_RAMP_DOWN 停止 | R01_1／R01_2 | 0Hz 純 DC；停止先緩降 | 機制在 code | 未驗 |
| T-FSI-01 | 16-word M+3S 佈局欄位對位 | R02_2 §3 | word0–15 對映正確 | FAKE 可驗 | fake wire 無錯誤注入 |
| T-MCBSP-01 | staged→commit：commit 前輸出不變 | R02_3 §6 | CC_DA 只在 commit 點更新 | FAKE 可驗 | 真 CMPC ISR 未實作 |
| T-SPIB-L-01 | Legacy 0x3000 window 4096 筆下載＋checksum | D11 §6／D10 | block READY、checksum 對 | legacy code 存在 | 寫入目標是 GSRAM 非 SDRAM；4095 buffer 疑點 |
| T-SPIB-P-01 | Packet candidate：CRC 錯誤丟包不派發 | D11 §15.3 | CrcErr++、無 internal event | 可執行 | header/checksum 與 D11 暫定值不符，結果不可作選型依據 |
| T-WAVE-01 | Output ON 時 WAVE_ACTIVATE 必須 reject | D04 §11／D10 §12 | reject＋error code | **必 FAIL**（無檢查） | ⚠ 最高風險缺口 |
| T-WAVE-02 | Activation fail 保留前一 valid page | D11 §10.5 | pointer 不變 | 機制在 code | 未走負面測試 |
| T-WAVE-03 | per-sample checksum 驗證（壞 1 字必 INVALID） | D10 §9–11 | INVALID＋0x0006 | **無法執行**（政策未實作） | 缺 |
| T-M0-01 | PEC 全路徑（WHOAMI 自測＋人工破壞 byte） | D02_2_3 | OkCount++／PecErr++ | 正向自測已內建 | 缺負向注入 hook |
| T-OUT-01 | Output ON 禁止清單全項掃描（download/cal/param/flash） | D04/D05/D10/D11 | 全部 reject | **必 FAIL** | 同 T-WAVE-01 |
| T-CPU-01 | MCBSP/FSI 模組搬移 CPU2 編譯（介面不變） | D01/D02 §4 | CPU2 build pass | 未試 | 移植性主張未被證明 |
| T-TIME-01 | 100kHz ISR worst-case（GPIO toggle） | D01 | <10µs 含餘裕 | 未測 | §9 全表 |

---

## 14. Do-Not-Change Rules（重申，含證據狀態）

1. **不得把 Packet Protocol 當正式主線**——Legacy（Candidate A）= 現行主線 [DOC: D11 §4]；sandbox packet 僅 Candidate B 模擬。
2. **不得在 Output ON 修改 wave page**［D04 §11／D10 §12］——目前 sandbox **無此防護**，在補上之前任何展示都必須人工保證 Output OFF。
3. **不得在 Runtime 存取 Flash**［D03 §4／D04 §13］——現況合規（runtime 無 flash 存取路徑）。
4. **不得在 Runtime 重配 DMA CH5/CH6**［D02/D03/D04/D05/D11 五文一致］——現況合規（未占用）。
5. **不得讓 CPU2 承擔 SPIB/SPIA/I2CA/SCI**［D02 §4.2］——現況合規（CPU2 未動）。
6. **不得讓 Sandbox convenience 覆蓋正式文件**——本報告即為此檢查的執行記錄；發現的覆蓋風險＝§4 的 README/註解誤導性文字。

---

## 15. Final Verdict

### **PASS WITH DEVIATIONS**

理由：sandbox 在「資料流骨架」層面與正式架構同構（D07 runtime 模型、D04 生命週期介面、D11 事件層概念、DMA CH5/6 紀律、CPU2 不沾通訊），且未覆蓋任何正式文件與 legacy 主線 code；但存在多項**已具體化的偏差**（Output ON 保護缺失、D11 Candidate B 細節不符、page 容量 16<19、CH1/CH2 名實互換）與**整個未起步的政策層**（D10 驗證全套、D13/D14/D15）。

### Top 10 必補事項（依風險排序）

| # | 事項 | 依據 | 風險 |
|---|---|---|---|
| 1 | **Output ON 保護**：dispatcher／wave backend 加 output-state 檢查（activation/download/cal/param/flash 全清單） | D04 §11/D10 §12/D11 §11/D05 §10 | 安全規則缺失；移植後直接變成產品 bug |
| 2 | **`WAVE_PAGE_TRACK_MAX` 16 < 19**：valid mask 容量小於產品現用頁數 | D03 §5 | 具體功能錯誤 |
| 3 | **D11 位址對齊裁決**：cmd_id.h（0x0704/0x0902）vs D11（0x0958/0x0900…）的正式定案＋0x0960/0x0961 落地 | D11 §8 | 協定雙軌並存，AM3352 端無所適從 |
| 4 | **D10 驗證政策落地**：per-sample checksum＋Checksum Area＋七狀態機＋錯誤碼 | D10 全文/D05 §8 | 「不完整波形不得進 runtime」核心保證缺失 |
| 5 | **Packet candidate 對齊 D11**：header 0x55AA、checksum 待定公式、CmdID 沿用 register address；README 補 Candidate B 標註 | D11 §7 | 選型測試結果無效化風險 |
| 6 | **DMA CH1/CH2 裁決**＋CH3/CH4 按文件配置（SPIB DMA 化） | D02 §5/D11 §16 | 文件-程式長期分歧 |
| 7 | **Wave download 目標改 SDRAM**＋4095 buffer 疑點釐清 | D04 §5 | 與正式 pipeline 結構性不符 |
| 8 | **Timing 證據**：100kHz ISR worst-case＋EMIF read latency（D07 v0.2 判準） | D01/D07 | 架構升級決策無數據 |
| 9 | **M0 Virtual Register Map 正式化**（對 ASR5K_M0_COMM.md）＋PEC error policy | D02_2_3 §7 | fake 與真 M0 介面漂移 |
| 10 | **CPU1/CPU2 邊界落地**：MCBSP/FSI 搬移計畫＋MSGRAM 欄位表（§7 清單） | D01 §1.1/D02 §4 | 越晚搬，耦合越深 |

---

*本報告為唯讀審查產物。所有修正行動需另行授權，不在本報告範圍內。*
