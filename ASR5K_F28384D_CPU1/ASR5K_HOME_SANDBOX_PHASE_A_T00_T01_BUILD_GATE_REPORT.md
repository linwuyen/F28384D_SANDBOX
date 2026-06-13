# ASR5K_HOME_SANDBOX_PHASE_A_T00_T01_BUILD_GATE_REPORT

建立：2026-06-14
依據：ASR5K_HOME_SANDBOX_PHASE_A_TEST_PROCEDURE.md（T00、T01）
性質：build gate 支援報告。**未修改 code／README／D 系列文件，未 refactor，未新增測試程式，未修 build error。**

---

## 1. Purpose

協助完成 Phase A baseline 的最小 build gate（T00 file inclusion / T01 clean build）。本報告做兩件事：(a) 在自動化環境可達範圍內完成 workspace 與 build-system 靜態檢查；(b) 提供使用者在 CCS 手動執行的 checklist。**真正的 T00/T01 build evidence 必須由使用者在 Windows CCS 執行後貼回**；本環境無 CCS GUI、無 F28388D、無法讀 target watch。

---

## 2. Governance Status

- Implementation hold：**仍有效**（本任務不解除）。
- 本報告：不改 code、不 refactor、不新增測試、不修 build error、不碰 DMA、不做 CPU2 migration、不接 real IC、不自行決定產品架構。
- 判定紀律：無使用者貼回 Console → T01 一律 **NOT RUN**；Console 有 error → T01 **FAIL** 且 baseline 停止；Console 明確 build success 且 0 error → 才可 **PASS**。本環境不得自行宣稱 PASS、不得推測成功、不得補寫觀測值。

---

## 3. Workspace Static Check（本環境實測）

| 檢查 | 結果 |
|---|---|
| Branch / HEAD | `main` / `e8d6faf` |
| Sandbox_module 目錄 | 存在，含 **13** 個 `.c` |
| 13 個 `.c` 清單 | am3352, cc, cv, epwm, flash, fsi, ipc, m0, mcbsp, output_sink, sandbox_cmd, sandbox_manager, seq |
| 已修改（M, uncommitted） | am3352(.c/.h)、cc.c、fsi(.c/.h)、m0(.c/.h)、mcbsp(.c/.h)、sandbox_cmd(.c/.h)、sandbox_manager.c、PHASE1_README.md（共 13 項） |
| Untracked（治理文件） | DOCUMENT_CONFLICT_DECISION_REGISTER、HOME_SANDBOX_MINIMUM_SCOPE、PHASE_A_TEST_PROCEDURE、SANDBOX_CONFORMANCE_GAP_REPORT、SANDBOX_PHASE_A_FIX_PLAN（5 份 .md） |
| `FLASH/sources.mk` SUBDIRS | 含 `Sandbox_module \` ✅ |
| `FLASH/Sandbox_module/subdir_vars.mk` | **C_SRCS 13 筆、C_DEPS 13 筆、OBJS 13 筆**，與磁碟 13 個 `.c` **逐一吻合** ✅ |
| `FLASH/Sandbox_module/*.obj`（殘留） | 13 個，檔名與 13 個 `.c` 1:1 對應 |

---

## 4. Sandbox_module Inclusion Risk

### ⚠ 更正前一份報告之誤判
ASR5K_HOME_SANDBOX_PHASE_A_BASELINE_TEST_REPORT.md §2 曾記「`subdir_vars.mk` 0 筆 Sandbox 參照」並評為 build-system staleness 高風險。**該判斷有誤**：當時查的是**頂層** `FLASH/subdir_vars.mk`（其職責只列子目錄、本就不列各檔 obj）。CCS 的 per-file obj 清單在**各子目錄自己的** `FLASH/Sandbox_module/subdir_vars.mk`，本次實測該檔 **C_SRCS/C_DEPS/OBJS 各 13 筆、與現有 13 個 `.c` 完全一致**。更正後 inclusion 風險為 **LOW**。

### 現況風險評估

| 風險 | 等級 | 依據 / 對策 |
|---|---|---|
| Sandbox 原始碼未收編 | **LOW** | 生成式 makefile 已含全 13 筆；F5 Refresh 後仍應目視確認 |
| 殘留 `.obj`/`.d` 對應的是「修改前」原始碼 | **MEDIUM** | 13 個 `.obj` 來自前次 build；本次有 13 檔已修改。**增量 build 雖會依 `.d` 重編，但仍須先 `Project → Clean`**，確保不沿用修改前產物 |
| 生成式 makefile 殘留舊 include 路徑 | **LOW–MEDIUM** | 先前已從 `.cproject` 移除 `SYSCONFIG_TOOL_INCLUDE_PATH` 與 `SPIA_Master`；但 `FLASH/*.mk` 須經 F5 Refresh 重生才會反映。build 後確認該兩項警告已消失 |
| 新增檔未被 CCS 掃描 | **NONE（本次）** | 13 個 `.c` 已全部在 makefile 中，無「新增但未收編」情況 |

**結論**：只要使用者**先 Clean 再 Build**，inclusion 與產物一致性風險即可控；無「原始碼遺漏收編」之阻擋級問題。

---

## 5. Required Manual CCS Steps（使用者在 Windows CCS 執行）

> 全程不改任何 source。若任一步出現需改 code 才能繼續，**停止並回報**（implementation hold）。

**T00 — Refresh / Inclusion**
1. Project Explorer 選 `ASR5K_F28384D_CPU1` → 按 **F5**（Refresh）。
2. 展開 `Sandbox_module`，目視確認 13 個 `.c` 皆在且未被標記排除（無 exclude-from-build 圖示）。
3. （可選）Project Properties → 確認無紅色未解析路徑。

**T01 — Clean Build**
4. **Project → Clean…**，勾選本專案，確定（這會清掉 13 個修改前的殘留 `.obj`）。
5. **Project → Build Project**（FLASH configuration）。
6. 等 build 完成，**全選 Console 內容複製**。
7. 在 Console 檢視最後是否有 `errors`/`error #`、`section ... overlap`、`multiple definition`。

**貼回規則**
8. 把 **完整 Console**（含結尾的 build finished / error 統計行）貼回給我。
9. 若出現 error：**立即停止**，不要嘗試改 code，直接貼回——我只記錄、不修。
10. 若 0 error 且 build success：貼回後，我才更新 T01=PASS，並提供 T02–T06 watch 收集清單。

---

## 6. T00 Result

**Status：STATIC-EVIDENCE PASS（inclusion 已驗）／待 CCS F5 Refresh 目視確認**

- 本環境實測：13 個 `.c` 全部在生成式 makefile 的 C_SRCS/OBJS 中（§3、§4），無遺漏、無未收編新檔。
- 形式上 T00 程序步驟（CCS 內 F5 Refresh + 目視）屬 GUI 操作，須由使用者完成最終確認；本環境無法執行 GUI，故 T00 的「CCS 內確認」標為**待使用者**。
- 判定：inclusion 層面證據充分（風險 LOW），但 T00 程序未由 CCS 完成 → 以 **STATIC-EVIDENCE PASS** 記載，不等同 runtime/GUI 確認。

## 7. T01 Result

**Status：NOT RUN**

- 原因：本環境無 CCS GUI、無法 build；亦不從命令列硬 build（makefile 須由 CCS F5 Refresh 重生以反映 `.cproject` 既有修正，命令列繞過會產生不忠實結果）。
- 依判定規則：未取得使用者貼回之 Console → T01 **一律 NOT RUN**。
- **未宣稱 PASS、未推測成功、未補寫觀測值。**

## 8. Build Console Evidence

```
(尚無：等待使用者於 CCS 執行 Project → Clean → Build 後貼回完整 Console)
```

> 使用者貼回後，本段落將原文收錄該 Console，並據其內容（且僅據其內容）更新 §7 T01 Result 為 PASS 或 FAIL。

---

## 9. Blocking Issues

1. **環境限制（非 code 問題）**：自動化環境無 CCS GUI / 無 F28388D / 無法讀 target watch → T01 必須由使用者在 CCS 執行。唯一解：使用者執行 §5 步驟並貼回 Console。
2. **必須先 Clean**：13 個殘留 `.obj` 對應修改前原始碼；未 Clean 直接 Build 雖通常會依 `.d` 重編，但為確保基線忠實，T01 要求 **Clean build**，不接受增量 build 作為 baseline 證據。

（註：先前報告所指「makefile 0 筆 Sandbox 參照」已於 §4 更正為誤判，非真實阻擋項。）

## 10. Recommendation

1. 使用者依 §5 在 CCS 執行 **F5 Refresh → Clean → Build FLASH**，貼回完整 Console。
2. 我據貼回內容更新本報告：
   - Console 有 error → **T01 FAIL**，Phase A baseline 停止；我只提出 **build-fix plan 建議**（不出 code patch、不改 source），等候另行授權。
   - Console 0 error 且 build success → **T01 PASS**；我才提供 **T02–T06 runtime watch 收集清單**（仍由使用者在 CCS 執行、貼回數值，我不代填）。
3. 在 T01 取得 PASS 前，**不進入** T02–T13、**不進入** Phase A implementation authorization。
4. 全程維持 implementation hold；本報告未宣稱 Home Sandbox product complete，未宣稱任何 real IC／DMA／CPU2／FSI／SDRAM／Flash 完成或已驗證。

---

*本報告為唯讀 build gate 產物。T01 之 PASS/FAIL 僅能由使用者貼回之真實 CCS Console 決定；在此之前 T01 維持 NOT RUN。*
