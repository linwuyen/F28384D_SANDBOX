# ASR5K_SANDBOX_PHASE1 — Home Board Runtime Sandbox

建立：2026-06-12
目標：F28388D 家中板**零外部硬體**可獨立運作的 ASR5K runtime 框架。
第一個可觀察成果：`GSRAM_FAKE → DDS Runtime → DEBUG_VAR / INTERNAL_DAC / AD5543 output`。

---

## 1. Sandbox 架構圖

```
                       main loop (poll)                    100kHz domain (HW)
┌──────────────────────────────────────────┐   ┌──────────────────────────────────┐
│ DDS_Poll() ── 建表(1點/pass,4096次)        │   │ EPWM1(MEAS_CNV) 100kHz SOCA      │
│      │                                   │   │   └→ ADCA SOC0 → ADCA1 INT       │
│      ▼                                   │   │        └→ DMA CH2: TxBuf→SPIC TX │
│ WaveMem page0 ──Validate──Activate──┐    │   │ SPIC 3×16bit @10MHz              │
│                                     │    │   │   └→ RX FIFO=3 → DMA CH1         │
│ SandboxCmd_Poll() ◄── 命令信箱        │    │   │        └→ dmaCh1ISR:             │
│   OUTPUT_ON/OFF FREQ AMP            │    │   │  ① ADC 解包 / M_Ref / Heartbeat   │
│   OFFSET WAVE_ACTIVATE              │    │   │  ② DDS_Step()（FSM+相位）          │
│ HwVerification / SPIB / Modbus      │    │   │  ③ DDS_GetSample()               │
└─────────────────────────────────────│────┘   │       │（讀 active wave pointer）  │
                                      │        │  ④ OutputSink_Write(code)        │
        Wave Memory Interface         ▼        │       ├ DEBUG_VAR  → watch 變數   │
   ┌────────────────────────────────────┐      │       ├ INTERNAL_DAC → DACA 引腳  │
   │ backend（wave_platform_config.h 選） │      │       └ AD5543 → TxBuf[2]（下週期）│
   │  GSRAM_FAKE(RAMGS4) ←本階段預設       │      └──────────────────────────────────┘
   │  HOME_EMIF1_SRAM(0x00300000)        │
   │  PROD_EMIF1_SDRAM(0x80000000)       │       IPC: u32HeartBeat_CPU1（ISR 內遞增）
   └────────────────────────────────────┘            u32HeartBeat_CPU2（CPU2 側）
```

核心原則：DDS core 只看 active wave pointer（不知道 backend）；
sink 只收 16-bit code（不知道 DDS）；換板/換硬體只動 backend 與 sink。

## 2. 模組資料流

1. `WaveMem_Init()` → 選 backend、（必要時）EMIF init → `WaveMem_MemTest()`
2. `DDS_Init(100kHz, 1kHz, full amp, mid offset)` → 進 INIT_TABLE
3. main loop `DDS_Poll()`：每 pass 寫 1 點 sine 進 page0（共 4096 pass）
4. 建完 → `WaveMem_ValidatePage(0)`（checksum＋非平坦檢查）→
   `WaveMem_ActivatePage(0)`（先發布 pointer、後立 ready）→ DDS attach
5. `DDS_IsInitComplete()` 成立 → main loop 自動 `DDS_Start()` → FSM:
   STOPPED → DELAY_ON → RUNNING
6. 100kHz ISR：`DDS_Step()` 相位累加 → `DDS_GetSample()` 查表縮放 →
   `OutputSink_Write()` 依 mask 扇出

## 3. Backend / Sink 切換方式

**Wave backend**（編譯期，[Wave_module/wave_platform_config.h](Wave_module/wave_platform_config.h)）：
```c
#define WAVE_BACKEND_SELECT  WAVE_BACKEND_GSRAM_FAKE        /* 本階段預設 */
/* WAVE_BACKEND_HOME_EMIF1_SRAM＝家中板外部 SRAM；
   WAVE_BACKEND_PROD_EMIF1_SDRAM＝產品板（DDS/sink/SPIC 全部不用改） */
```

**Output sink**（執行期，debugger 直接改，不用重建）：
```
g_sOutputSink.u16SinkMask
  bit0 (0x1) DEBUG_VAR_SINK     更新 u16LastCode
  bit1 (0x2) INTERNAL_DAC_SINK  DACA 引腳輸出（12-bit, code>>4）
  bit2 (0x4) AD5543_SINK        寫 g_u16TxSequenceBuf[2]（SPIC 下週期送出）
預設 = 0x3（DEBUG_VAR + INTERNAL_DAC）；全開 = 0x7
```
INTERNAL_DAC bit 設起時，HwVerification 的 2.5ms DACA loopback 寫入自動讓位
（[HwVerification.c](HwVerification.c) 內有閘）；清掉 bit1 即恢復舊 loopback 行為。

## 4. Command Injector 使用方式

執行期（CCS Expressions 視窗，免重建）依序寫入：
```
g_sSandboxCmd.u32Param   = <參數>
g_sSandboxCmd.u16Cmd     = <命令碼>
g_sSandboxCmd.u16Pending = 1        ← 最後寫，main loop 下一圈執行
```

| 命令 | 碼 | 參數 | 範例 |
|---|---|---|---|
| OUTPUT_ON | 1 | — | 啟動輸出（DELAY_ON→RUNNING） |
| OUTPUT_OFF | 2 | — | 停止（DELAY_OFF→…→STOPPED） |
| DDS_FREQ | 3 | 頻率×100 | 50000 = 500Hz；0 = DC mode（範圍 1.00–1000.00Hz） |
| DDS_AMP | 4 | 0–65535 | 32768 = 半幅 |
| DDS_OFFSET | 5 | 0–65535 | 32768 = mid-scale |
| WAVE_ACTIVATE | 6 | page id | GSRAM_FAKE 只有 page 0 |

結果回讀：`u16LastResult`（1=接受）、`u32DispatchCount`、`u32RejectCount`。
程式內注入（未來 SPIB 映射用同一入口）：`SandboxCmd_Inject(cmd, param);`

## 5. Build / Run 步驟

1. CCS 開 `ASR5K_F28384D_CPU1`，組態 FLASH，Build（CPU2 專案不需重建）。
2. 燒錄、跑起來。GSRAM_FAKE 不需要任何外部接線。
3. 約 4096 個 main-loop pass 後（毫秒級）DDS 自動進 RUNNING。
4. 示波器接 **DACA 引腳（ADCINA0/DACA_OUT）**：1kHz 正弦、中心約 VREF/2。
5. Expressions 加入第 6 節 watch 清單逐項核對。
6. 注入 `DDS_FREQ=50000` → 波形變 500Hz，即 injector 驗收完成。

## 6. Watch 變數

| 變數 | 期望 |
|---|---|
| `g_sWaveMem.u16Backend` | 0（GSRAM_FAKE） |
| `g_sWaveMem.u16MemTestPass / u16MemTestDone` | 1 / 1 |
| `g_sWaveMem.u16PageValidMask` | bit0=1 |
| `g_sWaveMem.u16ActivePageId / u16Ready` | 0 / 1 |
| `g_sWaveMem.pu16ActiveWave` | 非 NULL（RAMGS4 位址 0x011xxx） |
| `sDDS.fgState` | 0x08（RUNNING） |
| `sDDS.bInitComplete / bTableReady` | 1 / 1 |
| `sDDS.u16RtIndex` | 0–4095 循環 |
| `sDDS.u32PhaseAccumulator` | 持續遞增繞回 |
| `g_sMeasDds.u16IsrCounter` | 每秒 +100000（wrap 正常） |
| `g_sMeasDds.u16DacOut` | 0–65535 間擺動 |
| `g_sOutputSink.u32WriteCount` | 與 IsrCounter 同速遞增（hook 在跑） |
| `g_sOutputSink.u16LastCode` | 持續變化 |
| `g_u16TxSequenceBuf[2]` | mask bit2 開啟後持續變化 |
| `g_sSandboxCmd.u32DispatchCount` | 每注入一次 +1 |
| `sAccessCPU1.u32HeartBeat_CPU1` | 100kHz 遞增 |
| `sReadCPU2.u32HeartBeat_CPU2` | CPU2 韌體有跑則遞增 |

## 7. Breakpoint 建議

| 位置 | 用途 |
|---|---|
| [dds.c](dds/dds.c) `initDDS` 內 `bComplete` 成立處 | 看 Validate/Activate/attach 一次完成 |
| [sandbox_cmd.c](Sandbox_module/sandbox_cmd.c) `SandboxCmd_Poll` switch | 驗證命令分派 |
| `dmaCh1ISR`（[meas_dds_module.c](SPIC_module/meas_dds_module.c)）| 單步看 Step→GetSample→Sink（注意 100kHz，下了就停） |
| `WaveMem_ActivatePage` | 看 pointer 發布順序 |

## 8. Scope 量測點

| 點 | 期望 |
|---|---|
| DACA 引腳（DAC_A_OUT / ADCINA0 ball U1） | 1kHz sine，0–3V 滿幅、中心 ~1.5V |
| GPIO145（EPWM1A MEAS_CNV） | 100kHz 脈波 |
| GPIO100/102（SPIC PICO/CLK） | 每 10µs 一組 3×16bit burst |
| GPIO103（SPIC PTE/CS） | 100kHz frame 包絡 |

## 9. 已完成項目

- GSRAM_FAKE wave source（RAMGS4 專屬 section、memtest、page 管理）
- Wave Validation / Activation / DDS attach（pointer 先行、ready 後立）
- DDS RUNNING 全鏈（6-state FSM 未重構，僅資料源指標化）
- Output Sink 三型（DEBUG_VAR / INTERNAL_DAC / AD5543），執行期 mask 切換
- AD5543 sink 僅寫 TxBuf[2]，SPIC/DMA 設定零變動
- Software Command Injector（6 命令＋信箱＋計數器）
- DACA 雙寫者衝突閘（sink 優先，HwVerification 讓位）
- IPC heartbeat 保留（CPU1 ISR 內、CPU2 既有韌體）
- [DMA_DEVIATION_REPORT.md](DMA_DEVIATION_REPORT.md)（CH1/CH2 與正式定義互換，僅記錄不改）
- HOME_EMIF1_SRAM / PROD_EMIF1_SDRAM backend 與腳位審計（前置任務完成，待硬體）

## 10. 未完成項目（本階段刻意不做＋待驗）

- 待上機驗證：本 README 第 5–8 節全部（桌面無法執行）
- SCI/Modbus debug readout：未接（watch 清單已備，mb_slave 既有可後續映射）
- SPIB DMA 化（CH3/CH4）、FSI loopback code、MCBSP stub、I2CA fake M0、
  SPIA fake flash（Phase 4–9）
- System State Machine / Fault / Calibration / Power Stage / 真 AM3352 /
  真 M0G3519 / Flash OTA（本階段明確排除）

## 11.（更新 2026-06-12）全 C28x 模組 Sandbox 接線

Phase 1 之後追加：所有 ASR5K 用到的 C28x 模組已在軟體層接通，統一由
[sandbox_manager.c](Sandbox_module/sandbox_manager.c) 的
`Sandbox_InitAll()` / `Sandbox_PollAll()` 驅動（main.c 各一行）。
**單點觀察表：`g_sSandboxOverview`**——一個 watch expression 看全部資料流。

| 模組 | 檔案 | 預設 mode | fake→real 切換欄位 |
|---|---|---|---|
| EPWM tick | epwm_sandbox | 觀察既有 100kHz 鏈（不改 timing） | —（真硬體） |
| CV source | cv_sandbox | DEBUG_FAKE（三角波） | `g_sCvSandbox.u16Source`：1=INTERNAL_ADC、2=SPIC_LTC2353 |
| CC sink | cc_sandbox | **MCBSP stub（預設，電流環模型開機即活）**＋CV 導出測試 pattern | `g_sCcSandbox.u16Sink`：0=DEBUG、1=DACB（示波器量測時） |
| FSI | fsi_sandbox | FAKE frame loopback＋CRC 自檢 | `u16Mode`：1=HW_LOOP、2=DAISY（placeholder） |
| MCBSP | mcbsp_sandbox | FAKE word loopback | `u16Mode`：1=DLB、2=EXTERNAL（placeholder） |
| AM3352 注入器 | am3352_sandbox | 信箱→`SandboxCmd_Inject` | 未來 SPIB bridge 走同一入口 |
| SPIA flash | flash_sandbox | FAKE（NOR 語意 RAM 模擬＋自測） | `u16Mode`：1=REAL（placeholder） |
| I2CA M0 | m0_sandbox | FAKE 暫存器檔（reg0=0x3519） | `u16Mode`：1=REAL（placeholder） |
| IPC | ipc_sandbox | CPU2 靜默時 fake echo（不寫 CPU2 區） | 自動偵測 `u16Cpu2Alive` |

追加 watch（在第 6 節之上）：`g_sSandboxOverview`（全表）、
`g_sCvSandbox.i16Cv`（±16000 三角）、`g_sCcSandbox.u16LastCode`、
`g_sFsiSandbox.u32TxCount/u32ErrorCount`（Err 恆 0）、
`g_sMcbspSandbox.u32RxCount`、`g_sAm3352Sandbox.u32ForwardCount`、
`g_sFlashSandbox.u32WriteCount`、`g_sM0Sandbox.au16Reg[0]`(=0x3519)、
`g_sIpcSandbox.u16EchoIsFake`。

AM3352 注入示範：CCS Expressions 對任意 .c 檔加 breakpoint 後用
`Am3352Sandbox_InjectCommand(3, 50000)`（GEL/Expression 呼叫），或直接
手填封包到 `g_sAm3352Sandbox.au16RxPacket[]`（Header 0xA55A＋CRC16）再設
`u16RxComplete=1` ⇒ DDS 變 500Hz（走 D02_2_1 封包驗證→dispatcher→DDS
全鏈，`u32PktOkCount` 與 `u32CmdDispatch` 各 +1，TX ping-pong 更新狀態包）。

### 11.1 依產品系統圖升級的模擬精度（2026-06-12）

| 系統圖區塊 | Sandbox 對應 | 模擬內容 |
|---|---|---|
| 電流環（McBSPA＋AD5543 CC_DA＋AD7915 CV_AD） | mcbsp_sandbox | 全雙工模型：`WriteCcDa()`/`ReadCvAd()`，fake plant＝CV 以一階遲滯追 CC（看得出因果） |
| 並聯訊息同步（FSI） | fsi_sandbox | ~~14-word 自訂 frame~~ → **已依 R02_2 採納版改為 16-word M+3S 標準佈局**（見 11.3），CV_AD 廣播字由 `FsiSandbox_GetRxCvAd()` 供 CC 迴路 |
| M0G3519（風扇×4、DAC7612 Ipeak、14 條 GROUP_OUT、8 條保護輸入、ILA/ILB/TEMP） | m0_sandbox | **32 暫存器全圖譜**（named defines `M0_REG_*`/`M0_GO_*`/`M0_GI_*`）；fake plant：風扇回授追 duty、電流三角波、保護恆 clear（可由 debugger 注入故障位）、RO 暫存器拒寫 |
| 波形資料（WAVE 256組＋SEQ 35×32bit×1000步） | [wave_seq_layout.h](Wave_module/wave_seq_layout.h)＋seq_sandbox | 產品記憶體佈局常數＋`ST_SEQ_STEP`(35×32bit)；fake 4-step 序列步進器，`u16FireActivate=1` 可實跑 SEQ→WAVE_ACTIVATE→DDS 鏈 |
| 電流環 CV 第二路（AD7915） | cv_sandbox | 新增 `CV_SOURCE_MCBSP_AD7915`(=3) |

模擬迴路串接順序（manager 內）：CC 寫 CC_DA → MCBSP plant 更新 CV_AD →
FSI frame 攜帶 CV 樣本並回傳 share% → CV source 可讀 plant。

### 11.2 系統圖與韌體現況的矛盾記錄（不改 code，待裁決）

1. 圖上 SDRAM 標 **EMIF2**——與已裁決結論（EMIF1 CS0 @0x80000000，
   2026-06-12 使用者確認）矛盾，圖面待更新。
2. 圖上 **AM3352=SPIA(GPIO16-19)、External Flash=SPIB(GPIO60/61/65/66)**，
   與韌體 syscfg 相反（AM3352=SPIB GPIO63-66、Flash=SPIA GPIO16-19）。
   Sandbox 命令層與 transport 無關（injector→dispatcher 不經 SPI），
   故無論裁決為何，sandbox 不需改；正式 SPIB/SPIA 角色需釐清後修
   syscfg 或圖面。

### 11.3 設計文件補完（2026-06-13，依 R01/R02/D02 系列文件）

| 文件 | 補完內容 |
|---|---|
| R01_1（DDS 全功能） | 命令集從 6 個擴到 **13 個**：新增 AMP/OFFSET/FREQ_RAMP（param=up_ms<<16\|down_ms，0=停用）、DELAY_ON/OFF（param=ms）、PHASE_ON/OFF（param=度×10，0xFFFFFFFF=立即）。dds_api 本身已具備全部能力（含 DC mode，R01_2 指出的缺陷在這份 code 已修復），本次只是把命令層接滿 |
| R02_2/R02_4（FSI 標準封包） | fsi_sandbox 改為**採納版 16-word M+3S 佈局**：[0-6] Master（L1/L2/L3_Ref、CV_AD、ID_Func、Data_H/L——直接取自 shareram M_Ref 區塊，100kHz ISR 本來就在維護）＋[7-15] S1/S2/S3 各 3W（Vout/Iout/Status，fake slave 填充）。增設 R02_1 的 NodeId/NodeCount/EnSlave/DlyTap 占位欄（token passing 與 skew training 屬實機 bring-up） |
| R02_3（CC_DA 全系統同步） | mcbsp_sandbox 增 **staged→commit** 機制：`WriteCcDa()` 只暫存，`CommitCcDa()`（=產品的 EPWM CMPC @T=8.0µs ISR）才落地；manager 在 FSI poll 之後呼叫 commit，模擬全節點同步輸出。`u16SyncMode=0` 可旁路 |
| D02_2_1（SPIB 封包） | am3352_sandbox 升級為**封包層**：Header 0xA55A／CmdID／Len／Payload／CRC-16(MODBUS)，RX Packet Buffer＋complete flag（=DMA CH3 行為）、TX Ping-Pong 狀態包（=DMA CH4 來源）。Header/Len/CRC 錯誤各自計數。DMA CH3/4 維持未配置（正式 ownership 保留） |
| D02_2_3（I2C M0 協定） | m0_sandbox 增**交易層**：10-bit 位址＋6-bit 長度虛擬位址、SMBus PEC（文件附表 CRC-8 poly 0x07）、PEC 涵蓋 Slave Address、非阻塞逐-byte 狀態機（每 poll 一 byte，絕不卡 main loop）。每 4096 polls 自動經完整協定路徑回讀 WHOAMI 自證。越界讀回 0xFF＋合法 PEC（照文件） |
| D01/D02 §4（CPU 分工） | **記錄性 deviation**：D01 把 MCBSP/FSI 劃給 CPU2；sandbox 全部在 CPU1 跑（家中板便利性）。移植產品時 MCBSP/FSI 模組搬到 CPU2 專案，介面不變 |

新增 watch：`g_sM0Xfer.{u16State,u16Result,u32OkCount,u32PecErrCount}`、
`g_sAm3352Sandbox.{u32PktOkCount,u32CrcErrCount,au16TxPing/Pong}`、
`g_sMcbspSandbox.{u16CcDaStaged,u32CommitCount}`、
`g_sFsiSandbox.{u16NodeId,u16NodeCount,au16RxFrame}`。

## 12. 下一階段建議

1. **上機驗收本階段**（半天）：watch 全綠＋DACA 出波＋injector 改頻。
2. **HOME_EMIF1_SRAM 切換驗證**：改一行 backend define 重建，跑 memtest
   定案 CS3n/OEn 等 6 支腳（對照 [emif1_home_sram_pin_audit.md](Wave_module/emif1_home_sram_pin_audit.md)）。
   這同時就是「換 backend 不改 core」的可移植性實證。
3. **Phase 5 提前到 Phase 3 之前**：SPIB loopback 注入器已有雛形
   （HwVerification_SPIB_RunLoopbackTest），把收到的命令映射到
   `SandboxCmd_Inject()` 即完成 AM3352-free 命令鏈，工作量最小、價值最高。
4. DMA ownership 裁決（見 deviation report 第 6 節建議選項 A）。
