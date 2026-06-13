# ASR5K_HOME_SANDBOX_PHASE_A_TEST_PROCEDURE

建立：2026-06-14
性質：測試程序定義文件。**本文件不修改 code／README／D 系列文件，不做 build／refactor，不新增測試程式，不自行決定正式產品架構。**
上游：ASR5K_HOME_SANDBOX_MINIMUM_SCOPE.md、ASR5K_SANDBOX_PHASE_A_FIX_PLAN.md（hold 中）、ASR5K_DOCUMENT_CONFLICT_DECISION_REGISTER.md（DCR-01–08）。

> **狀態聲明**：Implementation hold 仍然有效。本文件**定義**測試如何執行與 PASS/FAIL 判準，供未來解除 hold 後使用；本文件**不是測試報告**，未宣稱任何測試已通過。所有 T00–T15 的執行結果欄目前一律為「未執行」。

---

## 1. Purpose

為 Home F28388D Sandbox 建立 Phase A 測試程序，使後續若解除 hold 進行小修或 build/run 時，每一步都有明確 PASS / FAIL 標準與停止/回復規則。目的是把「跑起來看起來對」轉成「依固定判準可重現地判定」。

本程序對應 ASR5K_HOME_SANDBOX_MINIMUM_SCOPE.md §8 Definition of Done 的逐條驗證，並把其中標為 NEEDS_VERIFICATION／UNMET 的項目（build、common layer 硬體依賴）納入明確測項（T01、T15）。

---

## 2. Scope

- 對象：CPU1-only sandbox common/board 現況 code（GSRAM_FAKE backend）。
- 範圍：build 完整性、init 序列、wave fake backend、DDS 讀指標、100kHz hook 存在性（僅觀察）、AM3352 fake 命令/狀態、FSI frame、MCBSP staged commit、M0 PEC 交易、fake Flash、debug overview、Output ON 缺口確認、common layer 硬體依賴缺口確認。
- 全部測試在**家中 F28388D 開發板 + CCS debugger**完成，**不需任何外部 IC**。

---

## 3. Test Governance Rules

1. 不改 code、不改 README、不改 D 系列文件、不做 refactor、不新增測試程式。
2. 不碰 DMA 配置、不做 CPU2 migration、不接 real IC driver。
3. 測試方法限於：CCS Expressions/Watch 觀察、Memory Browser、debugger 寫入既有 volatile 變數、呼叫既有 sandbox API（GEL/Expression）。**不得為測試新增程式或修改既有邏輯。**
4. 任一測試 FAIL 時，依該測項的 Stop/Rollback Rule 處理；不得為了讓測試通過而修改 code（那屬 implementation，仍 hold）。
5. T14/T15 為**缺口確認測試**——其 PASS 定義是「現況與已登記缺口一致」，不是「功能正確」。
6. 本程序不證明任何正式產品完成度（見文末 §What This Test Does Not Prove）。

---

## 4. Test Environment

| 項目 | 內容 |
|---|---|
| 硬體 | F28388D 開發板（家中板），USB JTAG/XDS |
| 工具 | CCS 12.8.1、C2000 CGT 22.6.1.LTS |
| 組態 | FLASH configuration、`WAVE_BACKEND_SELECT = WAVE_BACKEND_GSRAM_FAKE`（現況預設） |
| 觀察手段 | Expressions 視窗、Memory Browser、Breakpoints |
| 外部硬體 | **無**（不接 AM3352/SDRAM/M0/類比鏈/FSI 對接機） |
| 前置 | CCS 對專案執行 F5 Refresh（收編 `Sandbox_module/`）—— 即 T00 |

---

## 5. Files / Modules Under Test

| 模組 | 檔案 |
|---|---|
| Sandbox manager | `Sandbox_module/sandbox_manager.{c,h}` |
| Command dispatcher | `Sandbox_module/sandbox_cmd.{c,h}` |
| Output sink | `Sandbox_module/output_sink.{c,h}` |
| AM3352 fake | `Sandbox_module/am3352_sandbox.{c,h}` |
| FSI fake | `Sandbox_module/fsi_sandbox.{c,h}` |
| MCBSP fake | `Sandbox_module/mcbsp_sandbox.{c,h}` |
| M0 fake | `Sandbox_module/m0_sandbox.{c,h}` |
| Flash fake | `Sandbox_module/flash_sandbox.{c,h}` |
| CV/CC/EPWM/IPC/SEQ | `Sandbox_module/{cv,cc,epwm,ipc,seq}_sandbox.{c,h}` |
| Wave backend | `Wave_module/wave_memory_backend.{c,h}`、`wave_platform_config.h`、`wave_memory_if.h` |
| DDS | `dds/*` |
| 100kHz hook | `SPIC_module/meas_dds_module.c`（`dmaCh1ISR`，僅觀察，不改） |
| main 整合 | `main.c` |

---

## 6. Explicitly Not Tested Items

- real EMIF1 SDRAM（memtest/refresh）、real W25Q64、real AM3352 SPIB、real M0 I2C、real AD5543/LTC2353/AD7915 時序、real FSI daisy-chain。
- DMA runtime 整合（含 CH1/CH2 deviation 之裁決驗證）。
- CPU2 runtime / migration。
- 100kHz ISR worst-case timing、EMIF read latency（需 profiler/scope，屬 scope 文件 §9 deferred）。
- Output ON protection 之**功能正確性**（該功能尚未實作 = Fix A1，hold 中；T14 只確認缺口）。
- Fault policy、Calibration、Parameter、Power stage、Protection trip。
- 波形電氣正確性（示波器量測屬上機後續，非 Phase A）。

---

## 7. Phase A Test Sequence

```
T00 refresh/inclusion
 └─ T01 clean build ─────────────(FAIL→全部停止)
     └─ T02 InitAll 序列
         ├─ T03 wave 4096 page R/W
         │   └─ T04 validate/activate
         │       └─ T05 DDS 讀 active pointer
         │           └─ T06 100kHz hook 觀察
         ├─ T07 AM3352 命令注入 ── T08 AM3352 狀態讀回
         ├─ T09 FSI frame pack/unpack
         ├─ T10 MCBSP staged→commit
         ├─ T11 M0 PEC 計數
         └─ T12 fake Flash NOR
             └─ T13 debug overview 彙整
                 ├─ T14 Output ON guard 缺口確認
                 └─ T15 common layer 硬體依賴缺口確認
```

依存規則：T01 FAIL → 全程序停止。T02 FAIL → T03 以後不執行。T03/T04/T05 為線性鏈。T07–T15 在 T02 通過後可並行觀察。

---

## 8. Test Cases

> 共用前置：除 T00/T01 外，所有測項假設 T01 已 PASS（`.out` 已產出並載入）。「正常運行數秒」指 EINT 後 main loop 已跑過 ≥4096 圈（DDS 建表完成）。

---

### T00 — CCS project refresh / file inclusion check

- **Objective**：確認 `Sandbox_module/` 全部 .c 被專案收編，無遺漏/重複來源。
- **Preconditions**：CCS 開啟專案，磁碟上已有全部 sandbox 檔案。
- **Steps**：1) 專案按 F5 Refresh；2) 檢視 Project Explorer 確認 11 個 sandbox 模組 .c 皆在；3) 檢視 build variable 無未解析路徑（SYSCONFIG_TOOL_INCLUDE_PATH／SPIA_Master 應已移除）。
- **Watch Variables**：（無；視覺檢查）
- **Expected Result**：sandbox_manager/sandbox_cmd/output_sink/am3352/fsi/mcbsp/m0/flash/cv/cc/epwm/ipc/seq 全部列入編譯來源。
- **PASS Criteria**：所有模組 .c 在 build source 清單；無 "file not found" / 無未解析 build variable 警告新增。
- **FAIL Criteria**：任一 sandbox .c 未被收編，或出現新的路徑解析錯誤。
- **Stop / Rollback Rule**：FAIL 即停止；屬專案設定問題（非 code），登記後待修專案設定，不進 T01。

---

### T01 — Clean build check

- **Objective**：確認 common source 乾淨建置（對應 Scope DoD #1）。
- **Preconditions**：T00 PASS。
- **Steps**：1) Project → Clean；2) Build FLASH configuration；3) 檢視 Problems/Console。
- **Watch Variables**：（無）
- **Expected Result**：產出 `ASR5K_F28384D_CPU1.out`，0 error。
- **PASS Criteria**：build 成功；無 error；無 section overlap；無 multiple definition；無新 blocker warning（既有 Clock Tree 建議與 ePWM errata 提醒不計）。
- **FAIL Criteria**：任何 error、section overlap、multiple definition、或新增 blocker warning。
- **Stop / Rollback Rule**：**FAIL → 整個 Phase A 停止**。記錄 error 全文；不得為通過而改 code（implementation hold）。需 implementation 授權才修。

---

### T02 — Sandbox_InitAll init sequence

- **Objective**：確認 `Sandbox_InitAll()` 將 13 模組全部初始化到已知初值。
- **Preconditions**：T01 PASS；breakpoint 設在 `main.c` `Sandbox_InitAll()` 返回後一行。
- **Steps**：1) 載入、全速跑到該 breakpoint；2) 展開各 sandbox 全域 struct。
- **Watch Variables**：`g_sOutputSink.u16SinkMask`(=0x3)、`g_sSandboxCmd.u16Pending`(=0)、`g_sCvSandbox.u16Source`(=0)、`g_sCcSandbox.u16Sink`(=2, MCBSP)、`g_sFsiSandbox.u16Mode`(=0)、`g_sMcbspSandbox.u16SyncMode`(=1)、`g_sM0Sandbox.au16Reg[0]`(=0x3519)、`g_sFlashSandbox.u32JedecId`(=0x00EF4017)、`g_sIpcSandbox.u32PollCount`、`g_sEpwmSandbox.u32PollCount`。
- **Expected Result**：每個 struct 為其 Init 函式設定之初值。
- **PASS Criteria**：上列 watch 全部等於括號內預期初值。
- **FAIL Criteria**：任一初值不符，或執行未抵達 breakpoint（init 中途 hang/exception）。
- **Stop / Rollback Rule**：FAIL → 停止後續測項；記錄哪個模組初值錯；不改 code。

---

### T03 — Wave fake memory backend 4096-sample page write/read

- **Objective**：確認 GSRAM_FAKE backend 可承載並正確讀回 4096-sample page（Scope DoD #2）。
- **Preconditions**：T02 PASS；正常運行數秒（DDS 建表完成）。
- **Steps**：1) 暫停；2) 讀 `g_sWaveMem`；3) Memory Browser 開 `g_sWaveMem.u32BaseWordAddr`（RAMGS4，約 0x011xxx），檢視 4096 words 為 Q15 sine pattern（中段過零、首尾連續）。
- **Watch Variables**：`g_sWaveMem.u16Backend`(=0)、`.u32BaseWordAddr`、`.u16PageCount`(=1)、`.u32SizeWords`(=4096)、`.u16MemTestDone`(=1)、`.u16MemTestPass`(=1)。
- **Expected Result**：memtest pass；base 區 4096 words 為 sine 表。
- **PASS Criteria**：`u16MemTestPass==1` 且 `u16MemTestDone==1`；Memory Browser 顯示非平坦 sine pattern（至少可見正負半週）。
- **FAIL Criteria**：`u16MemTestPass==0`（記 `u32MemTestFailAddr`/`u16MemTestExpect`/`u16MemTestRead`），或記憶體區全 0/全同值。
- **Stop / Rollback Rule**：FAIL → 停 T04+；GSRAM_FAKE memtest 失敗代表 RAMGS4 section 或建表異常，記錄不改 code。

---

### T04 — Wave validate / activate fake page

- **Objective**：確認 page0 validate→activate→active pointer 發布流程。
- **Preconditions**：T03 PASS。
- **Steps**：1) 暫停（運行數秒後）；2) 讀 `g_sWaveMem` 的 valid/active 欄位與指標。
- **Watch Variables**：`g_sWaveMem.u16PageValidMask`(bit0=1)、`.u16ActivePageId`(=0)、`.u16Ready`(=1)、`.pu16ActiveWave`(非 NULL，等於 base)、`.u16PageChecksum`(非 0)。
- **Expected Result**：page0 已 validate 且 activate，指標已發布。
- **PASS Criteria**：`u16PageValidMask & 0x1 == 1` 且 `u16Ready==1` 且 `pu16ActiveWave != 0` 且 `u16ActivePageId==0`。
- **FAIL Criteria**：`u16Ready==0`、`pu16ActiveWave==NULL`、或 valid mask bit0=0。
- **Stop / Rollback Rule**：FAIL → 停 T05+；記錄 validate/activate 哪步未成立。

---

### T05 — DDS active wave pointer read path

- **Objective**：確認 DDS runtime 透過 active pointer 讀樣本（Scope DoD #3）。
- **Preconditions**：T04 PASS；DDS 已自動 Start（main.c 自動啟動）。
- **Steps**：1) 運行數秒；2) 暫停；3) 比對 `sDDS.pu16WaveTable` 與 `g_sWaveMem.pu16ActiveWave`。
- **Watch Variables**：`sDDS.pu16WaveTable`、`g_sWaveMem.pu16ActiveWave`、`sDDS.fgState`、`sDDS.bInitComplete`(=1)、`sDDS.u16RtIndex`。
- **Expected Result**：DDS 指標等於 backend active 指標；FSM 已離開 INIT_TABLE。
- **PASS Criteria**：`sDDS.pu16WaveTable == g_sWaveMem.pu16ActiveWave`（非 NULL）且 `bInitComplete==1` 且 `fgState` ∈ {STOPPED(0x02), DELAY_ON(0x04), RUNNING(0x08)}。
- **FAIL Criteria**：兩指標不一致、為 NULL、或 fgState==ERROR(0x80000000)。
- **Stop / Rollback Rule**：FAIL → 停 T06；記錄指標值與 fgState。

---

### T06 — 100kHz hook observation only

- **Objective**：**僅觀察** 100kHz hook 是否執行（不驗時序、不驗輸出正確性）。
- **Preconditions**：T05 PASS；EINT 後正常運行。
- **Steps**：1) 運行；2) 連續取樣 `g_sMeasDds.u16IsrCounter` 兩次（間隔約 1 秒）；3) 觀察 `g_sOutputSink.u32WriteCount`、`sDDS.u16RtIndex` 是否變動。
- **Watch Variables**：`g_sMeasDds.u16IsrCounter`、`g_sOutputSink.u32WriteCount`、`g_sOutputSink.u16LastCode`、`sDDS.u16RtIndex`、`g_u16TxSequenceBuf[2]`。
- **Expected Result**：counter 持續遞增；RtIndex 在 0–4095 變動；LastCode 變動。
- **PASS Criteria**：兩次取樣 `u16IsrCounter` 不同（遞增/wrap 皆可）且 `u32WriteCount` 同步遞增且 `u16RtIndex` 有變化。
- **FAIL Criteria**：counter 完全不動（hook 未執行）或 WriteCount 不動（sink 未被呼叫）。
- **Stop / Rollback Rule**：FAIL → 標記「100kHz 鏈未啟動」，記錄但不阻擋 T07–T15（命令/資料模型測試不依賴 ISR）。**注意**：本測項只證明 hook 執行，不證明時序或波形正確（見文末）。

---

### T07 — AM3352 fake command injection

- **Objective**：確認 fake AM3352 封包經 CRC 驗證後派發到 dispatcher（Scope DoD #7）。
- **Preconditions**：T02 PASS；DDS 已 RUNNING（供 DDS_FREQ 可見效果）。
- **Steps**：1) Expressions 呼叫 `Am3352Sandbox_InjectCommand(3, 50000)`（CmdID 3=DDS_FREQ，param 50000=500.00Hz）；2) 放行至少一個 main-loop 圈；3) 暫停讀計數與 DDS 頻率。
- **Watch Variables**：`g_sAm3352Sandbox.u32PktOkCount`、`.u16LastCmdId`(=3)、`.u16LastMapOk`(=1)、`.u32CrcErrCount`、`g_sSandboxCmd.u32DispatchCount`、`g_sSandboxCmd.u16LastResult`(=1)、`sDDS.u32Frequency_x100`(=50000)。
- **Expected Result**：封包通過 header+CRC、映射成功、dispatcher 接受、DDS 頻率更新。
- **PASS Criteria**：`u32PktOkCount`+1 且 `u16LastMapOk==1` 且 `g_sSandboxCmd.u32DispatchCount`+1 且 `sDDS.u32Frequency_x100==50000`；`u32CrcErrCount` 不變。
- **FAIL Criteria**：`u32CrcErrCount`/`u32HeaderErrCount` 遞增（封包建構/驗證錯）、或 DDS 頻率未變。
- **Stop / Rollback Rule**：FAIL → 記錄計數；不影響其他測項。可重置：注入 `DDS_FREQ` 還原原頻率。

---

### T08 — AM3352 fake status readback

- **Objective**：確認 TX ping-pong 狀態包正確組裝且 CRC 自洽（Scope DoD #8）。
- **Preconditions**：T07 PASS（一次成功命令後狀態包已填）。
- **Steps**：1) 暫停；2) 依 `g_sAm3352Sandbox.u16TxActiveIsPong` 決定剛填的是 Ping 或 Pong（注意：填完會翻轉，故「上一次填的」是 `!u16TxActiveIsPong` 指向者）；3) 讀該 buffer 10 words；4) 用 `Am3352Sandbox_Crc16(buffer, 9)` 比對 word[9]。
- **Watch Variables**：`g_sAm3352Sandbox.u32StatusTxCount`、`.au16TxPing[0..9]`、`.au16TxPong[0..9]`、`.u16TxActiveIsPong`。
- **Expected Result**：狀態包 [0]=0xA55A、[1]=0x8001(STATUS)、[2]=6、[3..4]=heartbeat、[5]=DDS state、[6]=dac code、[7]=wave ready、[8]=last result、[9]=CRC16。
- **PASS Criteria**：header==0xA55A 且 [1]==0x8001 且 [9]==`Am3352Sandbox_Crc16(buf,9)`（CRC 自洽）且 `u32StatusTxCount` 隨命令遞增。
- **FAIL Criteria**：header 錯、CRC 不符、或 StatusTxCount 不動。
- **Stop / Rollback Rule**：FAIL → 記錄該 buffer 內容；不影響其他測項。

---

### T09 — FSI 16-word frame pack/unpack

- **Objective**：確認 R02_2 16-word M+3S frame 正確 pack 並經 fake loopback unpack（Scope DoD #5）。
- **Preconditions**：T02 PASS；運行數秒（FSI poll 已多次執行）。
- **Steps**：1) 暫停；2) 比對 `au16TxFrame[0..15]` 與 `au16RxFrame[0..15]`；3) 核對 Master 欄位來自 shareram；4) 讀 `FsiSandbox_GetRxCvAd()`。
- **Watch Variables**：`g_sFsiSandbox.au16TxFrame[0..15]`、`.au16RxFrame[0..15]`、`.u32TxCount`、`.u32RxCount`、`.u32ErrorCount`(=0)、`.u16NodeCount`(=1)、`sAccessCPU1.u16L1_Ref`。
- **Expected Result**：RxFrame==TxFrame（fake wire）；frame[0..2]=L1/L2/L3_Ref、[3]=CV_AD、[4]=ID_Func、[7..15]=S1/S2/S3 各 3 word；S1/S2/S3 Status 低 12bit 隨 tick 變動。
- **PASS Criteria**：16 words `au16RxFrame[i]==au16TxFrame[i]` 全等且 `u32ErrorCount==0` 且 `au16TxFrame[0]==sAccessCPU1.u16L1_Ref`（Master 欄位對位）。
- **FAIL Criteria**：任一 word 不等、ErrorCount>0、或 Master 欄位未對位 shareram。
- **Stop / Rollback Rule**：FAIL → 記錄 frame dump；不影響其他測項。

---

### T10 — MCBSP staged→commit model

- **Objective**：確認 R02_3 staged→commit：commit 前輸出不變、commit 後才落地（Scope DoD 相關）。
- **Preconditions**：T02 PASS；`g_sMcbspSandbox.u16SyncMode==1`（staged 模式）。
- **Steps**：1) 暫停；2) 記 `u16CcDaCode` 現值；3) Expression 呼叫 `McbspSandbox_WriteCcDa(0x4000)`；4) **不放行**，立即讀 `u16CcDaStaged`(=0x4000) 與 `u16CcDaCode`(仍為舊值)；5) 呼叫 `McbspSandbox_CommitCcDa()`；6) 讀 `u16CcDaCode`(=0x4000)。
- **Watch Variables**：`g_sMcbspSandbox.u16CcDaStaged`、`.u16CcDaCode`、`.u32CommitCount`、`.u16CvAdSample`、`.u32TxCount`。
- **Expected Result**：Write 後 staged 更新但 code 不變；Commit 後 code 才等於 staged。
- **PASS Criteria**：Write 後 `u16CcDaStaged==0x4000 && u16CcDaCode==舊值`；Commit 後 `u16CcDaCode==0x4000` 且 `u32CommitCount`+1。
- **FAIL Criteria**：Write 後 `u16CcDaCode` 立即變（staged 機制失效）或 Commit 後未更新。
- **Stop / Rollback Rule**：FAIL → 記錄；屬模型缺陷登記，不改 code。

---

### T11 — M0 PEC pass/fail transaction counters

- **Objective**：確認 D02_2_3 交易層 PEC 正向通過，並能觀察計數（Scope DoD #9）。
- **Preconditions**：T02 PASS；運行數秒（M0 自測每 4096 polls 讀 WHOAMI）。
- **Steps**：**正向**：1) 暫停讀 `g_sM0Xfer.u32OkCount`(應持續遞增)、`u32PecErrCount`(=0)；2) 手動 `M0Sandbox_RequestRead(0,2)`，放行數圈，讀 `g_sM0Xfer.au8Data[0..1]`(=0x35,0x19) 與 `u16Result`(=1)。 **負向（可選，僅觀察既有路徑，不改 code）**：對越界暫存器 `M0Sandbox_RequestRead(0x3FE,2)` 觀察是否仍回 0xFF+合法 PEC（依 D02_2_3）。
- **Watch Variables**：`g_sM0Xfer.u32OkCount`、`.u32PecErrCount`、`.u16Result`、`.u16State`、`.au8Data[0..1]`、`g_sM0Sandbox.u32XferCount`、`g_sM0Sandbox.au16Reg[0]`(=0x3519)。
- **Expected Result**：WHOAMI 經完整虛擬位址+PEC 路徑回讀 0x3519；PEC 錯誤計數恆 0。
- **PASS Criteria**：`u32OkCount` 遞增 且 `u32PecErrCount==0` 且讀回 `au8Data[0]==0x35 && au8Data[1]==0x19`。
- **FAIL Criteria**：`u32PecErrCount`>0（FAKE 模式不應發生）或 WHOAMI 讀回非 0x3519。
- **Stop / Rollback Rule**：FAIL → 記錄交易狀態；不影響其他測項。

---

### T12 — fake Flash NOR semantic

- **Objective**：確認 fake Flash 的 NOR 語意（erase=0xFFFF、write 只清位）與計數（Scope DoD #10）。
- **Preconditions**：T02 PASS。
- **Steps**：1) `FlashSandbox_EraseSector()`；2) 讀 `au16Mem[0]`(=0xFFFF)；3) `FlashSandbox_Write(0, 0x0F0F)`；4) 讀 `au16Mem[0]`(=0x0F0F)；5) `FlashSandbox_Write(0, 0xF0F0)`→讀(=0x0000，NOR 只清位)；6) `FlashSandbox_ReadId()`(=0x00EF4017)；7) 越界 `FlashSandbox_Write(99,x)` 觀察 reject。
- **Watch Variables**：`g_sFlashSandbox.au16Mem[0]`、`.u32EraseCount`、`.u32WriteCount`、`.u32ReadCount`、`.u32RejectCount`、`.u32JedecId`。
- **Expected Result**：erase→0xFFFF；write 後為 old&data（NOR）；ID=0x00EF4017；越界 reject++。
- **PASS Criteria**：步驟 2/4/5 記憶體值符合（0xFFFF→0x0F0F→0x0000）且 `u32EraseCount`/`u32WriteCount` 遞增且越界時 `u32RejectCount`+1。
- **FAIL Criteria**：write 能把 0 變 1（非 NOR 語意）、erase 未填 0xFFFF、或 ID 不符。
- **Stop / Rollback Rule**：FAIL → 記錄；不影響其他測項。測後可 EraseSector 還原。

---

### T13 — debug overview counters

- **Objective**：確認 `g_sSandboxOverview` 正確彙整各模組關鍵狀態（Scope DoD #11）。
- **Preconditions**：T02 PASS；運行數秒。
- **Steps**：1) 暫停；2) 展開 `g_sSandboxOverview`；3) 與各來源模組逐欄比對。
- **Watch Variables**：`g_sSandboxOverview`（全欄）對比 `g_sEpwmSandbox.u32Tick100k`、`g_sWaveMem.u16Ready`、`sDDS.fgState`、`g_sOutputSink.u16LastCode`、`g_sFsiSandbox.u32TxCount`、`g_sMcbspSandbox.u32TxCount`、`g_sSandboxCmd.u32DispatchCount`、`g_sM0Sandbox.u32XferCount`、`g_sIpcSandbox.u32Cpu1Hb`。
- **Expected Result**：overview 各欄等於對應來源欄位（同一暫停點）。
- **PASS Criteria**：抽查 ≥6 欄，overview 值與來源模組值一致；`u32PollCount` 遞增。
- **FAIL Criteria**：任一抽查欄與來源不一致（彙整邏輯錯/未更新）。
- **Stop / Rollback Rule**：FAIL → 記錄不一致欄位；不影響其他測項。

---

### T14 — Output ON guard observation / expected current gap

- **Objective**：**確認 Output ON 保護缺口存在**（對應 Gap Report Top1 / Fix A1，尚未實作）——本測項 PASS 定義為「現況與已登記缺口一致」，非「保護有效」。
- **Preconditions**：T05 PASS；DDS 已 RUNNING（`sDDS.fgState==0x08`）。
- **Steps**：1) 確認 RUNNING；2) 注入 `Am3352Sandbox_InjectCommand(6, 0)`（CmdID 6=WAVE_ACTIVATE, page0）或 Expression `SandboxCmd_Inject(6,0)`；3) 放行數圈；4) 讀 dispatcher 結果。
- **Watch Variables**：`sDDS.fgState`(=0x08 RUNNING)、`g_sSandboxCmd.u16LastResult`、`g_sSandboxCmd.u32DispatchCount`、`g_sSandboxCmd.u32RejectCount`、`g_sWaveMem.u16ActivePageId`。
- **Expected Result（現況）**：activate **未被拒絕**——`u16LastResult==1`、`DispatchCount`+1、`RejectCount` 不變。這證明目前無 Output ON 保護（Fix A1 待實作）。
- **PASS Criteria（缺口確認）**：觀察到「RUNNING 中 WAVE_ACTIVATE 仍成功」= 與 DCR/Gap Report 記錄之缺口一致 → 標記 **GAP CONFIRMED**。
- **FAIL Criteria**：若觀察到 activate 被拒絕（`u16LastResult==0` 且 `RejectCount`+1）——表示有未登記的防護存在，與文件不符，需回查（**不是好消息，是文件與 code 不同步**）。
- **Stop / Rollback Rule**：本測項不阻擋流程；GAP CONFIRMED 即為預期結果，登記到缺口追蹤；不在此修（Fix A1 仍 hold）。測後注入 OUTPUT_OFF 還原。

---

### T15 — Common layer hardware dependency gap confirmation

- **Objective**：**確認 common layer 仍有真硬體依賴**（對應 Scope §5 落差表 / DoD #12 UNMET）——靜態檢查，確認缺口存在。
- **Preconditions**：T00 PASS（原始碼可檢視）。
- **Steps**（靜態，不需 run）：1) 檢視 `output_sink.h` 是否 include `meas_dds_module.h` 並呼叫 `DAC_setShadowValue`/寫 `g_u16TxSequenceBuf`；2) 檢視 `cv_sandbox.c` 是否呼叫 `ADC_readResult`；3) 檢視 `cc_sandbox.c` 是否 include `driverlib.h` 並呼叫 `DAC_setShadowValue(DACB_BASE,…)`；4) 對照 Scope §5 落差表。
- **Watch Variables**：（無；靜態原始碼檢查）
- **Expected Result（現況）**：上述硬體呼叫存在 = common/board 分層尚未落實（與 Scope §5 一致）。
- **PASS Criteria（缺口確認）**：三檔的硬體耦合點與 Scope §5 落差表逐一吻合 → 標記 **GAP CONFIRMED**（DoD #12 仍 UNMET，已知且已登記）。
- **FAIL Criteria**：發現 Scope §5 未列出的**額外**硬體耦合點（清單不完整，需更新 Scope §5）。
- **Stop / Rollback Rule**：本測項為登記性確認，不阻擋流程；發現額外耦合點則回報以補 Scope §5（文件更新，非 code）。

---

## Phase A Exit Criteria

Phase A 視為通過（可進入後續階段討論）需同時滿足：

1. **T00、T01 PASS**（硬性閘門：build 不過則全停）。
2. **T02–T13 全部 PASS**（init、wave、DDS、AM3352、FSI、MCBSP、M0、Flash、overview 的功能性測項）。
3. **T14、T15 為 GAP CONFIRMED**（缺口與文件一致；非 FAIL）。
4. 全部測項結果（含 watch 數值）記錄於測試紀錄，FAIL 項皆有登記與停止/回復處置。
5. T06 至少達成「hook 執行」層級（不要求時序/波形正確）。

> 任一 T00/T01 FAIL，或 T02–T13 任一 FAIL 未獲處置，Phase A **不通過**。

---

## What This Test Does Not Prove

本程序**不證明**以下任何一項：

- 不證明 100kHz ISR 滿足 10µs worst-case（T06 僅證明 hook 執行，無 profiler/scope）。
- 不證明任何波形電氣正確性（無示波器量測）。
- 不證明 EMIF1 SDRAM / HOME SRAM 可用（GSRAM_FAKE 不觸及外部記憶體）。
- 不證明任何 real IC（AM3352/W25Q64/M0/AD5543/LTC2353/AD7915）可通訊。
- 不證明 FSI daisy-chain、CPU2 runtime、DMA 整合可運作。
- 不證明 Output ON 保護有效（T14 反而確認其缺席）。
- 不證明 common/board 分層已落實（T15 反而確認其未落實）。
- **不證明 Home Sandbox 等於正式 ASR5K 完成**——產品完成度需正式控制卡，見 ASR5K_HOME_SANDBOX_MINIMUM_SCOPE.md §9。

---

## Conditions Required Before Implementation Hold Can Be Lifted

解除 implementation hold（進入 Phase A 小修實作）前，建議先具備：

1. 本測試程序經 review 接受（PASS/FAIL 判準確認）。
2. T14 缺口（Output ON protection）與 T15 缺口（common layer 硬體依賴）已被正式接受為「待修項」，且 Fix A1 範圍（ASR5K_SANDBOX_PHASE_A_FIX_PLAN.md）對應之 DCR-03（Output state proxy 依據 D12）已採用。
3. 明確授權範圍：解除 hold 是針對 Phase A 三項小修（Output ON guard／TRACK_MAX 16→32／DDS_STATE_ERROR export）＋（可選）common layer 解耦，**不擴及** DMA／CPU2／real IC／protocol 定案。
4. 確認 DCR-01/05/06 等「未裁決區」（SEQ 位址、M0 register map、協定位址）在本次實作中**不被觸碰**。
5. 解除 hold 後的執行順序：先跑 T00–T15 取得修改前基線 → 實作 Fix A1–A3 → 重跑相關測項（A1↔T14 應由 GAP CONFIRMED 轉為 guard PASS；A2↔新增 page18 追蹤測；A3↔ DDS_STATE_ERROR export 測）。

---

*本文件為唯讀測試程序產物。任何 code 變更、build、實際執行需另行授權；本文件未宣稱任何測試已執行或通過。*
