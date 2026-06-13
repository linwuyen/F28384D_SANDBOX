# ASR5K_SANDBOX_PHASE_A_FIX_PLAN

建立：2026-06-13
上游：ASR5K_SANDBOX_CONFORMANCE_GAP_REPORT.md（判定 PASS WITH DEVIATIONS，本計畫對應其 Top 10 之 #1、#2，及 §4 已知小瑕疵 #1）
性質：**純計畫文件。本文件不修改任何 code。**

---

## 1. Scope

本階段（Phase A）只處理三項低風險必修缺口：

- **Fix A1**：Output ON Protection（D04 §11／D10 §12／D11 §11／D05 §10）
- **Fix A2**：`WAVE_PAGE_TRACK_MAX` 16 < 19（D03 §5）
- **Fix A3**：DDS_STATE_ERROR 在 AM3352 status 截斷為 0

明確排除（不做）：

- 不修改 protocol（Legacy 主線與 Packet Candidate 內容皆不動；A3 只修 status export 編碼，不動封包格式）
- 不修改 DMA（CH1–CH6 配置與 deviation 現狀全部維持）
- 不修改 CPU1/CPU2 分工
- 不修改 D01–D11 任何文件
- 不碰 Packet Candidate 的 header/checksum/CmdID 對齊（屬 Gap Report Top 10 #5，另排程）
- 不做 SPIB DMA、FSI HW_LOOP、Modbus、CPU2 migration

---

## 2. Fix A1: Output ON Protection

### 2.1 入口盤點（所有可能修改 wave page／active pointer／backend／cal／param／flash 的路徑）

| # | 入口 | 位置 | 觸發來源 | Phase A 處置 |
|---|---|---|---|---|
| E1 | `sandboxCmdWaveActivate()`（validate＋activate＋換 pointer） | sandbox_cmd.c:37 | dispatcher `SANDBOX_CMD_WAVE_ACTIVATE`（debugger 信箱／`Am3352Sandbox_Poll` 封包／`SeqSandbox_Poll` FireActivate） | **加閘（主防線）** |
| E2 | `WaveMem_ActivatePage()` 直接呼叫 | wave_memory_backend.c:213 | 任何 code | **加閘（第二防線：backend lock 旗標）** |
| E3 | `WaveMem_GetPageWritePtr()` 寫入路徑 | wave_memory_backend.c:172 | `initDDS()` 建表（dds.c:33）；未來 M5 下載 | 第二防線 lock 同步保護（寫入指標在 lock 時回 NULL） |
| E4 | `WaveMem_ValidatePage()`（會覆寫 checksum 記錄） | wave_memory_backend.c:180 | dispatcher／initDDS | 第二防線 lock |
| E5 | `FlashSandbox_Write()` / `FlashSandbox_EraseSector()` | flash_sandbox.c | Poll 自測／debugger | **加閘**（D04 §13：Output ON 禁 Flash Write/Erase；fake 也要示範政策） |
| E6 | `WaveMem_MemTest()`（破壞性） | wave_memory_backend.c:95 | 只在 main.c 開機序（EINT 前、DDS 啟動前） | 不加閘；以註解記錄「僅限開機序」順序不變量 |
| E7 | `initDDS()` 建表→validate→activate | dds.c:29 | 只在 `DDS_STATE_INIT_TABLE`（必然 Output OFF） | 不加閘；狀態機天然保證，記錄不變量 |
| E8 | `HwVerification_SDRAM_RunTest/StressTest`（寫 0x80000000＝PROD wave 區） | HwVerification.c:212/229 | debugger 手動設 `u16Ctrl`，預設 0 | 本階段不改 code；列為已記錄危害（PROD backend 上機前另議） |
| E9 | Calibration update 路徑 | 不存在 | — | 無可閘之物；規則由閘函式預留（D14 落地時直接套用） |
| E10 | Parameter update 路徑 | 不存在 | — | 同 E9 |

E1 為唯一的命令層入口（seq/AM3352/debugger 三來源全部收斂進 dispatcher），故 dispatcher 閘＝完整覆蓋命令面；E2–E4 的 backend lock 為縱深防禦，擋掉繞過 dispatcher 的直接 API 呼叫。

### 2.2 Output state 判斷來源

定義單一判斷函式（新增於 sandbox_cmd 模組，不新增檔案）：

```
Sandbox_IsOutputOn() :=
    sDDS.fgState ∈ { DDS_STATE_DELAY_ON, DDS_STATE_RUNNING,
                     DDS_STATE_AMP_RAMP_DOWN, DDS_STATE_DELAY_OFF,
                     DDS_STATE_PHASE_OFF }
```

依據：dds_config.h 的狀態定義——上列五態涵蓋「已下啟動命令至完全停止之間」的全部狀態（DELAY_ON 雖只輸出 DC offset，但已屬 started，採保守解釋納入禁區）。STOPPED／IDLE／INIT_TABLE／ERROR 視為 Output OFF。
不讀 GPIO、不讀 power stage（本階段無此硬體）；未來 D12 System State Machine 落地時，此函式改讀正式 system state，**呼叫端不變**——這就是把判斷收斂成單一函式的目的。

### 2.3 Reject 規則

Output ON 時必須 reject（對應 D04 §11 禁止清單）：

| 禁止項 | 本階段對應入口 |
|---|---|
| Wave Download | E3（寫入指標回 NULL）；正式下載路徑屬 M5，落地時沿用同一 lock |
| Wave Modification | E3／E4 |
| Wave Activation | E1／E2 |
| Flash Save / Load / Erase / Write | E5 |
| Calibration Update | E9（規則預留） |
| Parameter Update | E10（規則預留） |

行為要求：

1. **失敗時不得改 active page**——E2 的 reject 在任何 pointer 寫入之前返回；`pu16ActiveWave`／`u16ActivePageId`／`u16Ready` 三者皆不動（沿用既有「先檢查後發布」結構，gap report 已確認此機制存在）。
2. **必須保留 previous valid page**——同上；validate mask 亦不得被清。
3. **明確 result code**——`g_sSandboxCmd` 新增 `u16LastErrorCode` 欄位，編碼**直接沿用 D11 §13 測試碼**（不自創）：`0x0000 OK`／`0x0002 OUTPUT_ON_REJECT`；既有 `u16LastResult`（1/0）語意不變，向下相容。
4. **debug counter / status flag**——`g_sSandboxCmd.u32OutputOnRejectCount`（命令層）＋`g_sWaveMem.u16OutputLock`（backend lock 現值，本身即 status flag）＋`g_sFlashSandbox.u32RejectCount`（既有欄位，沿用計入）。

lock 的設定點：dispatcher 的 `OUTPUT_ON` 處理成功後設 `u16OutputLock=1`，`OUTPUT_OFF` 並待 DDS 回到 STOPPED 後清 0（過渡期間 DELAY_OFF/PHASE_OFF 仍輸出中，須維持 lock——由 `Sandbox_IsOutputOn()` 在 poll 中同步）。main.c 的自動 `DDS_Start()` 同樣經 lock 設定路徑。

---

## 3. Fix A2: WAVE_PAGE_TRACK_MAX

### 3.1 定義與引用盤點

| 位置 | 內容 |
|---|---|
| wave_platform_config.h:41 | `#define WAVE_PAGE_TRACK_MAX 16U`（定義） |
| wave_memory_backend.h:37 | `uint16_t u16PageValidMask`（16-bit 容器，與 16 綁定） |
| wave_memory_backend.c:207 | `ValidatePage`：`pageId < TRACK_MAX` 才設 mask（**≥16 時回傳 1 但不記錄**） |
| wave_memory_backend.c:218 | `ActivatePage`：`pageId < TRACK_MAX` 才查 mask（**≥16 時跳過檢查直接啟用**） |
| 其他引用 | 無（grep 確認僅此四處） |

### 3.2 違反 D03 之說明

[DOC: D03 §5] 現行產品保留 **Page 0x0000–0x0012 共 19 頁**。`TRACK_MAX=16` 使 page 16/17/18（0x0010–0x0012）：(a) validate 成功卻無記錄、(b) activate 跳過 valid 檢查——**對產品現用頁面範圍，validation 政策形同失效**，且行為不一致（前 16 頁嚴格、後 3 頁放任）。

### 3.3 方案比較

| | Option A：改 32（`uint32_t` mask） | Option B：對齊 D05 256 頁（bitmap） |
|---|---|---|
| 作法 | `TRACK_MAX=32U`；`u16PageValidMask`→`u32PageValidMask`；兩處 `1U<<`→`1UL<<` | `TRACK_MAX=256U`；`uint16_t au16PageValidBitmap[16]`＋index/bit 運算 |
| RAM 成本 | +2 bytes（1 word） | +30 bytes（15 words） |
| 程式變更量 | 4 行級，無新迴圈 | 新增 bitmap 索引函式（set/test/clear），改 3 處呼叫 |
| 覆蓋 | 產品 19 頁 ✅；HOME 板 128 頁中 32–127 仍不追蹤（記錄為已知限制） | 全部 ✅（HOME 128、D05 256） |
| 風險 | 極低 | 低，但新增索引運算＝新增出錯面 |
| 與 M5 關係 | M5 多頁下載落地時若需 >32 再升級 | 一步到位 |

### 3.4 建議：**Option A（TRACK_MAX=32）**

理由：Phase A 定位是 low-risk mandatory fix。Option A 以最小變更修正對 D03 的違反（19 ≤ 32 ✅），不引入新的索引邏輯；Option B 留待 M5 wave download 實際需要多頁管理時一併實作（屆時連同 D10 七狀態機、per-sample checksum 一起動 backend，避免同一檔案兩次手術）。

**一併修正一致性缺陷**：`ValidatePage`／`ActivatePage` 對 `pageId ≥ TRACK_MAX` 改為**直接回傳 0（reject）**，消除「超界即放任」的雙重標準。GSRAM fake（1 頁）與 PROD（前 19 頁）皆不受影響；HOME 板 page 32–127 在 Phase A 後會被明確拒絕而非默許——此為刻意收緊，記錄於計畫。

**驗收確認**：page 0x0000–0x0012（0–18）全部 < 32，可 validate／activate／track ✅。

---

## 4. Fix A3: DDS_STATE_ERROR Status Truncation

### 4.1 問題位置

[CODE: am3352_sandbox.c `am3352FillStatusTx()`] 狀態包 word[5]：

```c
p[5] = (uint16_t)sDDS.fgState;
```

`DDS_STATE_ERROR = 0x80000000`（dds_config.h）→ 截斷成 `0x0000`，**AM3352 視角下 ERROR 與 IDLE 不可區分**。其餘狀態（最大 `AMP_RAMP_DOWN=0x0080`）不受影響。
（對照：sandbox_manager 的 `u32DdsState` 為 32-bit，無此問題，不在修正範圍。）

### 4.2 修正方式（只動 status export layer）

在 am3352_sandbox.c 新增 static helper（**不動 dds_config.h、不動 FSM**）：

```
ddsStatusExport(fgState) :=
    low15  = (uint16_t)(fgState & 0x7FFF)      // 全部非 error 狀態無損保留
    bit15  = (fgState & DDS_STATE_ERROR) ? 0x8000 : 0
    return low15 | bit15
```

編碼語意：bit15 = ERROR 旗標；bit14:0 = 既有狀態位（IDLE=0、INIT_TABLE=0x01、STOPPED=0x02、…、AMP_RAMP_DOWN=0x80）。非 error 狀態的數值**完全不變**（向下相容既有觀察值）；ERROR 從不可見變為 `0x8000`。

此編碼僅作用於 AM3352 status export；DDS FSM、`fgState` 本體、其他消費者一概不動。

---

## 5. Test Plan

### Fix A1（Output ON Protection）

| # | 測試 | 預期 |
|---|---|---|
| A1-T1（正向） | Output OFF、page0 已 valid → 注入 `WAVE_ACTIVATE(0)` | result=1、ErrorCode=0x0000、pointer 更新 |
| A1-T2（**負面**） | 注入 `OUTPUT_ON` 待 RUNNING → 注入 `WAVE_ACTIVATE(0)` | result=0、**ErrorCode=0x0002**、`u32OutputOnRejectCount`+1、`pu16ActiveWave`／`u16ActivePageId` 不變 |
| A1-T3（**負面**） | RUNNING 中呼叫 `FlashSandbox_Write(0,x)`／`EraseSector()` | 回 0、`u32RejectCount`+1、`au16Mem` 不變；flash 自測在 ON 期間暫停 |
| A1-T4（**負面，端到端**） | RUNNING 中由 AM3352 封包路徑送 WAVE_ACTIVATE | 封包 CRC pass、dispatcher reject、ErrorCode=0x0002（證明閘在事件層而非僅 debugger 路徑） |
| A1-T5（恢復） | `OUTPUT_OFF` → 待 STOPPED → 重注 `WAVE_ACTIVATE(0)` | 恢復可 activate；lock 旗標歸 0 |

### Fix A2（TRACK_MAX）

| # | 測試 | 預期 |
|---|---|---|
| A2-T1（正向，HOME backend 上機或 PROD） | 寫 page18 波形 → validate → activate | mask bit18=1、activate pass、`u16ActivePageId=18` |
| A2-T2（回歸，GSRAM fake） | 既有開機流程 page0 | 行為與 Phase A 前完全一致 |
| A2-T3（**負面**） | `ValidatePage(32)`／`ActivatePage(32)` | 皆回 0（超界明確拒絕，不再默許） |
| A2-T4（**負面**） | activate 未 validate 的 page17 | 回 0、active page 不變（previous valid page 保留） |

### Fix A3（ERROR export）

| # | 測試 | 預期 |
|---|---|---|
| A3-T1（**負面**） | debugger 強制 `sDDS.fgState=DDS_STATE_ERROR` → 觸發一次狀態包 | word[5]=0x8000（**AM3352 可見 ERROR**） |
| A3-T2（回歸） | RUNNING 狀態觸發狀態包 | word[5]=0x0008，與修正前數值相同 |
| A3-T3（回歸） | STOPPED 狀態 | word[5]=0x0002；bit15=0 |

---

## 6. Rollback Plan

| Fix | Rollback 方式 | 獨立性 |
|---|---|---|
| A1 | 移除 dispatcher 閘呼叫＋backend lock 旗標檢查（新增欄位可留存無害）；flash 閘同理 | 不依賴 A2/A3 |
| A2 | `TRACK_MAX` 還原 16、mask 型別還原 `uint16_t`、兩處 reject 還原舊判斷 | 不依賴 A1/A3 |
| A3 | helper 還原為原 cast | 不依賴 A1/A2 |

三項皆**不觸及**：正式 legacy SPIB 主線（spi_b_slave.c／cmd_parser.h／cmd_id.h 零修改）、DDS FSM（dds_core.h／dds.c 狀態機零修改）、CH1–CH6 DMA ownership（meas_dds_module.c DMA 設定零修改）。

---

## 7. Files Expected To Change（僅預告，本階段不修改）

| 檔案 | Fix | 變更性質 |
|---|---|---|
| Sandbox_module/sandbox_cmd.h | A1 | `u16LastErrorCode`／`u32OutputOnRejectCount` 欄位＋guard 函式宣告 |
| Sandbox_module/sandbox_cmd.c | A1 | `Sandbox_IsOutputOn()`＋WAVE_ACTIVATE 閘＋OUTPUT_ON/OFF 設清 lock |
| Wave_module/wave_memory_backend.h | A1, A2 | `u16OutputLock` 欄位；mask 型別 `uint32_t` |
| Wave_module/wave_memory_backend.c | A1, A2 | Activate/Validate/GetPageWritePtr lock 檢查；TRACK_MAX 邊界 reject |
| Wave_module/wave_platform_config.h | A2 | `WAVE_PAGE_TRACK_MAX 16U → 32U` |
| Sandbox_module/flash_sandbox.c | A1 | Write/Erase 閘（呼叫 guard） |
| Sandbox_module/am3352_sandbox.c | A3 | `ddsStatusExport()` helper＋word[5] 改用 |
| main.c | A1 | 自動 DDS_Start 改經 lock 設定路徑（1–2 行） |

不新增任何檔案；不修改 README／D 系列文件／dds/／SPIB_module/／SPIC_module/ DMA 設定。

---

*本計畫經核可後方進入實作；實作與測試各自獨立回報。*
