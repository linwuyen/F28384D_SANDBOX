# ASR5K_HOME_SANDBOX_PHASE_A_BASELINE_TEST_REPORT

建立：2026-06-14
依據：ASR5K_HOME_SANDBOX_PHASE_A_TEST_PROCEDURE.md（T00–T15）
性質：測試執行報告。**未修改 code／README／D 系列文件，未 refactor，未新增測試程式，未修 build error。**

---

## ⚠ 0. Execution Environment Constraint（必讀，決定本報告效力）

本報告由**無人值守自動化環境**產生，該環境**不具備**以下執行 T00–T13 所必需者：

- 無 CCS IDE GUI（F5 Refresh、Build、Load、Debugger run/halt、Expressions/Watch、Memory Browser 皆為 CCS GUI 操作）
- 無實體 F28388D 開發板、無 JTAG/XDS 連線
- 無法觀察「執行中目標」的 watch 變數（runtime 觀察為硬體在環行為）

本專案至今所有 build 皆由**使用者在 CCS 執行後貼回結果**；本環境從未、也無法驅動 CCS 或讀取執行中目標。

**因此**：凡需 build / load / run / halt / runtime watch 之測項（T00、T01、T02–T13、T14 的 runtime 注入步驟）一律據實標記 **NOT RUN**，並附「無法執行之原因」與「使用者端執行指引」。**本報告不捏造任何 PASS 或觀測值**——這是本專案治理紀律（NEEDS_VERIFICATION、證據優先、不宣稱未完成之事）的強制要求。

可在本環境**真實完成**者僅限：git/workspace 狀態、build 系統檔案收編狀態、原始碼靜態檢查（T15 全程、T14 靜態前提）。靜態檢查結果標為 **STATIC-EVIDENCE**，**不等於 runtime PASS**。

---

## 1. Test Environment（實際 vs 程序要求）

| 項目 | 程序要求 | 本次實際 |
|---|---|---|
| 硬體 | F28388D 開發板 + JTAG | **無**（自動化環境） |
| 工具 | CCS 12.8.1 GUI + CGT 22.6.1 | **無 CCS GUI**；CGT 安裝路徑存在但未經 CCS 驅動 |
| 組態 | FLASH, GSRAM_FAKE | 原始碼確認 `WAVE_BACKEND_SELECT=GSRAM_FAKE`（STATIC-EVIDENCE） |
| 觀察手段 | Expressions/Memory Browser | **不可用**（無執行中目標） |
| 可用手段 | — | git CLI、檔案系統檢視、ripgrep 靜態檢查 |

---

## 2. Git / Workspace Status Before Test

- Branch：`main`
- HEAD：`e8d6faf feat(spi): align SPIB slave with D02_2_1 design, fix CPU1 compile issues, and clean up obsolete CPU2 code`
- Working tree（uncommitted）：

```
 M Sandbox_module/am3352_sandbox.c   M Sandbox_module/am3352_sandbox.h
 M Sandbox_module/cc_sandbox.c       M Sandbox_module/fsi_sandbox.c
 M Sandbox_module/fsi_sandbox.h      M Sandbox_module/m0_sandbox.c
 M Sandbox_module/m0_sandbox.h       M Sandbox_module/mcbsp_sandbox.c
 M Sandbox_module/mcbsp_sandbox.h    M Sandbox_module/sandbox_cmd.c
 M Sandbox_module/sandbox_cmd.h      M Sandbox_module/sandbox_manager.c
 M ASR5K_SANDBOX_PHASE1_README.md
?? ASR5K_DOCUMENT_CONFLICT_DECISION_REGISTER.md
?? ASR5K_HOME_SANDBOX_MINIMUM_SCOPE.md
?? ASR5K_HOME_SANDBOX_PHASE_A_TEST_PROCEDURE.md
?? ASR5K_SANDBOX_CONFORMANCE_GAP_REPORT.md
?? ASR5K_SANDBOX_PHASE_A_FIX_PLAN.md
```

- **基線完整性註記**：工作區有未提交修改。測試所依據的 source 為「工作區現況」（含上列 13 個已修改檔），非 HEAD 提交狀態。建議使用者在 CCS 執行 T00–T13 前，先記錄此 working-tree 狀態作為基線指紋。

### Build-system staleness（T00/T01 風險前兆，STATIC-EVIDENCE）

| 檢查 | 觀察 | 意涵 |
|---|---|---|
| `FLASH/subdir_vars.mk` 含 "Sandbox_module" | **0 筆** | 生成式 makefile 變數未含 Sandbox_module 物件清單 |
| `FLASH/sources.mk` SUBDIRS | 含 `Sandbox_module \` | 子目錄有列入 |
| `FLASH/Sandbox_module/*.obj` | **13 個** | 曾有一次 build 編出 13 個 .obj |

**判讀**：build 產物與生成式 makefile 變數呈現不一致（subdir 列入、但 obj 變數 0 筆、且已有 13 個殘留 obj）。這正是 T00 設計要攔截的「檔案收編/ makefile 同步」狀態。**在使用者於 CCS 執行 F5 Refresh + Clean Build 前，T01 結果不可預測**——殘留 .obj 可能讓增量 build 假性成功而未納入最新原始碼。本環境無法執行 CCS 的 makefile 再生，故不嘗試命令列 build（會因 makefile 不同步而產生不忠實結果）。

---

## 3. T00–T15 Result Table

| Test | 名稱 | Result | 說明 |
|---|---|---|---|
| T00 | project refresh / inclusion | **NOT RUN** | 需 CCS GUI F5 Refresh；另見 build-system staleness |
| T01 | clean build | **NOT RUN** | 需 CCS Build；本環境不驅動 CCS makefile 再生 |
| T02 | InitAll 序列 | **NOT RUN** | 需 load + 斷點 + watch |
| T03 | wave 4096 page R/W | **NOT RUN** | 需 runtime + Memory Browser |
| T04 | validate / activate | **NOT RUN** | 需 runtime watch |
| T05 | DDS active pointer read | **NOT RUN** | 需 runtime watch |
| T06 | 100kHz hook 觀察 | **NOT RUN** | 需執行中目標 |
| T07 | AM3352 命令注入 | **NOT RUN** | 需 runtime + Expression 呼叫 |
| T08 | AM3352 狀態讀回 | **NOT RUN** | 需 runtime + Memory Browser |
| T09 | FSI 16-word frame | **NOT RUN** | 需 runtime watch |
| T10 | MCBSP staged→commit | **NOT RUN** | 需 runtime + Expression 呼叫 |
| T11 | M0 PEC 計數 | **NOT RUN** | 需 runtime watch |
| T12 | fake Flash NOR | **NOT RUN** | 需 runtime + Expression 呼叫 |
| T13 | debug overview | **NOT RUN** | 需 runtime watch |
| T14 | Output ON guard 缺口 | **GAP CONFIRMED (STATIC ONLY)** | 靜態確認無防護；runtime 注入步驟 NOT RUN |
| T15 | common layer 硬體依賴缺口 | **GAP CONFIRMED (STATIC, FULLY EXECUTED)** | 程序定義為靜態檢查，本環境可完整執行 |

統計：NOT RUN ×14，GAP CONFIRMED ×2（其中 T15 完整執行、T14 僅靜態前提）。PASS ×0、FAIL ×0。

---

## 4. 逐項結果

### T00 — project refresh / inclusion　**NOT RUN**
- Actual observed：（未執行 CCS）。靜態旁證：`sources.mk` SUBDIRS 含 Sandbox_module；`subdir_vars.mk` 0 筆 obj 參照；FLASH/Sandbox_module 有 13 殘留 .obj。
- Evidence note：見 §2 build-system staleness 表。
- Stop / Rollback action：無（未執行）。**使用者端指引**：CCS 專案 F5 Refresh，確認 13 個 sandbox .c 全列入 build source，再進 T01。

### T01 — clean build　**NOT RUN**
- Actual observed：（未執行）。本環境刻意不嘗試命令列 build——makefile 與最新原始碼不同步（§2），命令列 build 將產生不忠實結果，違反「不為通過而取巧」原則。
- Stop / Rollback action：依程序，T01 為硬性閘門；在使用者取得真實 build 結果前，T02–T13 維持 NOT RUN。**使用者端指引**：CCS Project → Clean → Build FLASH，貼回 Console（0 error / 無 section overlap / 無 multiple definition 即 PASS）。

### T02 — InitAll 序列　**NOT RUN**
- Actual observed：（未執行）。預期 watch（待使用者驗）：`g_sOutputSink.u16SinkMask=0x3`、`g_sCcSandbox.u16Sink=2`、`g_sM0Sandbox.au16Reg[0]=0x3519`、`g_sFlashSandbox.u32JedecId=0x00EF4017`、`g_sMcbspSandbox.u16SyncMode=1`。
- Stop / Rollback action：依程序，T02 FAIL 將停 T03–T13；本次因 NOT RUN 不觸發。

### T03–T13 — **NOT RUN（全部）**
- Actual observed：（未執行；皆需 load .out + debugger run/halt + Expressions/Memory Browser）。
- 各測項預期 watch 變數已於測試程序逐項定義，本報告不複述、不填造觀測值。
- Stop / Rollback action：無（未執行）。使用者端依程序 T03–T13 步驟逐項執行並記錄。

### T14 — Output ON guard / expected current gap　**GAP CONFIRMED (STATIC ONLY)**
- Objective 對應：確認 Output ON 保護缺口存在（Fix A1 未實作）。
- Actual observed（STATIC-EVIDENCE）：`Sandbox_module/sandbox_cmd.c` 對 `IsOutputOn` / output lock / `DDS_STATE_RUNNING` 之檢查 **grep 0 命中**——`sandboxCmdWaveActivate()` 與 dispatcher 不含任何 output-state 閘。
- runtime 部分（NOT RUN）：程序要求「RUNNING 中注入 WAVE_ACTIVATE 觀察是否被拒」之執行步驟未進行（需執行中目標）。
- 判定：靜態證據與 Gap Report Top1 / DCR-03 / Fix Plan A1 完全一致 → **缺口存在，與治理預期相符，非功能 PASS**。
- Stop / Rollback action：缺口登記，不在此修（Fix A1 仍 hold）。使用者上機時補做 runtime 注入步驟以完整確認。

### T15 — common layer hardware dependency gap　**GAP CONFIRMED (FULLY EXECUTED, STATIC)**
- Objective 對應：確認 common layer 仍有真硬體依賴（Scope §5 落差 / DoD #12 UNMET）。
- Actual observed（STATIC-EVIDENCE，ripgrep 實證）：
  - `output_sink.h:27` `#include <SPIC_module/meas_dds_module.h>`；`:66` `DAC_setShadowValue(DACA_BASE, …)`
  - `cv_sandbox.c:9` include `meas_dds_module.h`；`:41` `ADC_readResult(ADCARESULT_BASE, …)`
  - `cc_sandbox.c:10` `#include "driverlib.h"`；`:34` `DAC_setShadowValue(DACB_BASE, …)`
- 與 Scope §5 落差表逐點比對：三檔耦合點**完全吻合**，未發現額外未列出之耦合點。
- 判定：common/board 分層尚未落實，DoD #12 確認 **UNMET**，與文件一致 → **GAP CONFIRMED**（非 FAIL，非功能 PASS）。本測項為程序中唯一可在本環境完整執行者。
- Stop / Rollback action：登記確認；解耦屬 implementation（hold 中），不在此修。

---

## 5. T01 Build-FAIL 處置聲明

程序規定：T01 build FAIL 立即停止後續、只記錄 error、不做修正。
本次 T01 為 **NOT RUN（非 FAIL）**——未取得 build error 可記錄。**未對任何 source 做修正**（亦無 build error 觸發修正之情境）。T02–T13 依「T01 未取得 PASS」原則維持 NOT RUN。

---

## 6. Baseline Verdict

**INCONCLUSIVE — RUNTIME BASELINE NOT ESTABLISHED.**

- 功能性基線（T01–T13）**無法於本自動化環境建立**，需使用者在 CCS + F28388D 板執行。
- 兩項缺口確認測（T14 靜態、T15 完整）結果為 **GAP CONFIRMED**，與既有治理文件（Gap Report / DCR / Fix Plan / Scope）**一致**——代表「現況缺口如文件所述、無未登記偏差」，但**不構成功能基線**。
- 本報告**未**宣稱：Home Sandbox product complete；任何 real IC（AM3352/W25Q64/M0/AD5543/LTC2353/AD7915）、DMA、CPU2、FSI daisy-chain、SDRAM、Flash 已完成或已驗證。以上一律未執行、未驗證。

---

## 7. Blocking Issues（阻擋取得有效基線）

1. **無 CCS GUI / 無實體板**：T00–T13 無法於本環境執行（環境限制，非 code 問題）。唯一解：使用者在 CCS 端執行並貼回結果。
2. **Build-system staleness**：`subdir_vars.mk` 0 筆 Sandbox obj 參照 vs 13 個殘留 .obj（§2）。在 F5 Refresh + Clean Build 前，T01 結果不可信；殘留 obj 有讓增量 build 假性成功之風險。**必須先 Clean。**

## 8. Non-blocking Issues（已登記、不擋基線取得）

1. T14 Output ON guard 缺口（Fix A1，hold 中）——預期內缺口。
2. T15 common layer 硬體依賴（output_sink/cv/cc，DoD #12 UNMET）——預期內缺口。
3. `WAVE_PAGE_TRACK_MAX=16`（STATIC-EVIDENCE 確認 < D03 現用 19 頁，Fix A2，hold 中）——本次未獨立列測項，但靜態值已確認，與 Fix Plan 一致。
4. 工作區有未提交修改——建議測試前記錄基線指紋。

## 9. Phase A Fix Readiness

| 面向 | 狀態 |
|---|---|
| 缺口定義清晰度 | ✅ 三缺口（A1 output guard／A2 TRACK_MAX／A3 ERROR export）皆有 source 級靜態證據佐證，與 Fix Plan 一致 |
| 缺口與文件一致性 | ✅ T14/T15 GAP CONFIRMED 與 Gap Report/DCR 無出入 |
| **功能基線（修改前綠燈）** | ❌ **未建立**——T01–T13 NOT RUN，無「修改前可運作」之證據 |
| Build 健康度 | ❌ 未知（T01 NOT RUN + makefile staleness） |

**Readiness 結論**：缺口面已就緒，但**功能基線缺席**。在沒有「修改前綠燈基線」的情況下授權 implementation，將無法判別後續修改造成的回歸——違反 Fix Plan §6 的「修改前先跑基線」前提。

## 10. Recommendation — 是否可進入 Phase A Implementation Authorization

**尚不建議進入 Phase A implementation authorization。** 理由：實作授權的前提（測試程序 §Conditions #5：先取得修改前基線）尚未滿足——T01–T13 從未實際執行。

建議路徑（順序）：

1. **使用者在 CCS 端執行 T00–T13**（F5 Refresh → **Clean** Build → Load → run/halt → 依程序記錄 watch），把 Console 與 watch 數值貼回。本環境據此補完「runtime baseline」段落。
2. 確認 T01 PASS（0 error/無 overlap/無 multiple definition）且 T02–T13 PASS、T06 達「hook 執行」、T14 runtime 注入亦 GAP CONFIRMED。
3. 上述達成後 → 滿足 Phase A Exit Criteria → **才**進入 implementation authorization（範圍仍限 Fix A1–A3 + 可選 common layer 解耦；不擴及 DMA/CPU2/real IC/protocol；DCR-01/05/06 未裁決區不得觸碰）。
4. 若 T01 FAIL：依程序只記錄 error、停止、不修——另循 build 修復授權處理。

---

*本報告為唯讀測試執行產物。所列 NOT RUN 測項需在使用者 CCS + F28388D 環境補執行；本報告未宣稱任何 runtime 測項已通過，未宣稱任何正式產品/真硬體完成。*
