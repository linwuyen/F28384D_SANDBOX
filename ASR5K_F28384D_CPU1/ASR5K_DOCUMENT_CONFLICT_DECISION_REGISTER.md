# ASR5K_DOCUMENT_CONFLICT_DECISION_REGISTER

建立：2026-06-13
性質：文件治理與架構裁決登記簿。**本文件不修改 code／README／D 系列文件，不含任何 code patch，不自行裁決正式設計**——僅登記衝突、暫定裁決（interim ruling）與待辦狀態。
上游：ASR5K_SANDBOX_CONFORMANCE_GAP_REPORT.md（PASS WITH DEVIATIONS）＋ 2026-06-13 全面交叉比對（已接受）。
Implementation hold 持續有效：所有暫定裁決在正式裁決前**不得**轉化為 code 修改。

---

## Decision Register（總表）

| ID | 衝突項 | 衝突雙方 | 暫定裁決（Interim Ruling） | Status |
|---|---|---|---|---|
| DCR-01 | SEQ memory area 位址重疊 | `Wave_module/wave_seq_layout.h`（SEQ 區 @ wave 區後 2MB 處） vs **D05 §5 Wave Checksum Area（offset 0x00200000，2MB）** | **D05 優先**；SEQ 不得佔用 Wave Checksum Area；SEQ 區域在 D05 佈局中目前無歸屬 | **NEEDS_FORMAL_LAYOUT_DECISION** |
| DCR-02 | SPIA/SPIB 角色標示衝突 | 產品系統全圖（圖4：AM3352=SPIA GPIO16-19、W25Q64=SPIB GPIO60/61/65/66） vs **D01 §2.4／D02 §2.1／D11 §5（AM3352=SPIB GPIO63-66、W25Q64=SPIA）** | **D01/D02/D11 文件鏈優先**：AM3352=SPIB、W25Q64=SPIA；圖4 標註視為舊版痕跡 | **DECIDED_PENDING_DRAWING_UPDATE** |
| DCR-03 | Output 狀態判斷的正式依據 | Phase A 計畫之 `Sandbox_IsOutputOn()`（DDS 狀態 proxy） vs **D12 System State Machine（IDLE/RUN＋Command Permission Matrix §6）** | `Sandbox_IsOutputOn()` **僅作為 D12 落地前的 proxy**；reject 規則須逐格對齊 D12 §6；D12 system state 實作後，判斷來源切換、呼叫端不變 | **DECIDED_FOR_PROXY_ONLY** |
| DCR-04 | 空殼文件 | D06_EMIF1_DRIVER／D08_FLASH_PARTITION_LAYOUT／D13_FAULT_POLICY 僅有標題、無 enforceable content | D06 可由現有 EMIF work（home SRAM driver＋SDRAM 參數審查）回填；**D08/D13 在撰寫完成前不得作為 code rule 的強約束來源**（僅能引用其「未來歸屬」） | **NEEDS_DOCUMENT_AUTHORING** |
| DCR-05 | M0 Virtual Register Map 不存在 | `m0_sandbox` 自創之 32 暫存器 map vs **ASR5K_M0_COMM.md（M0 晶片內部 memory map，非 C2000↔M0 協定表）** | `m0_sandbox` register map 定性為 **sandbox-only**，不得作為協定依據；FAN 暫存器佈局差異（合一 32-bit vs 分離 16-bit）一併待正式表裁決 | **NEEDS_FORMAL_M0_PROTOCOL** |
| DCR-06 | D11 位址 vs legacy cmd_id.h | **D11 §8**（0x0900/0x0901、0x0910–0x0912、0x0958、0x0960/0x0961、0x0970） vs `SPIB_module/cmd_id.h`（0x0704 page select、0x0902 output set；無 0x0960/0x0961） | 不做任何 code 修改；雙軌現狀凍結，待 protocol owner 裁決何者為準（D11 v0.1 自稱「暫定」且引用 legacy screenshots，版本錯位可能性存在） | **NEEDS_PROTOCOL_OWNER_DECISION** |
| DCR-07 | 位址單位語言不一致 | **D05**（byte 語言：`8192 Bytes`、`sample_index * 2`、offset 以 byte 計） vs C28x code（word addressing：`sizeof(uint16_t)==1`，stride 4096 words） | 物理佈局一致（4096 words ≡ 8192 bytes），純文件表述問題；D05 應補註 C28x word 對應表述 | **NEEDS_DOCUMENT_CLARIFICATION** |
| DCR-08 | CPU2 ownership deviation | `fsi_sandbox`／`mcbsp_sandbox` 跑在 CPU1 vs **D01 §1.1／D02 §4.2（FSI/MCBSP/補償迴路屬 CPU2）** | 接受為 sandbox 階段刻意偏差（家中板便利性）；模組介面已自足，遷移時整包搬至 CPU2 專案、呼叫介面不變；正式分工不變更 | **ACCEPTED_SANDBOX_DEVIATION** |

---

## 逐項說明

### DCR-01：SEQ memory area vs D05 Wave Checksum Area

**衝突內容**：`wave_seq_layout.h` 將 SEQ 表（35 項 × 32-bit × 1000 steps ≈ 1.12Mbit）排在 Wave 區之後，即 `SEQ_LAYOUT_REGION_BASE_WORDS = 0x100000` words（= byte offset 0x00200000）。D05 §5 在同一 offset 明文配置 **Wave Checksum Area（2MB）**，供 D10 per-sample checksum 使用。兩者直接重疊。
**根因**：SEQ 概念出自產品系統全圖（圖4「波形資料」框），但 D03/D05 的正式記憶體佈局**從未定義 SEQ 區域**——`wave_seq_layout.h` 是在無文件背書下自行排址。
**暫定裁決**：D05 為記憶體佈局唯一權威；SEQ 不得佔用 Wave Checksum Area。`wave_seq_layout.h` 中的 SEQ base 在正式裁決前**不得被任何實作引用**（現況：僅 `seq_sandbox` 引用其型別與常數、未引用位址做實際存取——維持此狀態）。
**待辦**：layout owner 裁決 SEQ 歸屬（候選：D05 Parameter Area／OTA Area／Reserved Area 內配置，或新增專屬區域），裁決後修訂 D05 與 `wave_seq_layout.h`（屆時才動 code）。
**Status：NEEDS_FORMAL_LAYOUT_DECISION**

### DCR-02：SPIA/SPIB diagram conflict

**衝突內容**：圖4（系統全圖）標 AM3352→SPIA(GPIO16-19)、External Flash→SPIB(GPIO60/61/65/66)；D01 §2.4 腳位表、D02 §2.1/§2.2、D11 §5、pinmux.syscfg、圖3（block diagram）五方一致為 AM3352=SPIB(GPIO63-66)、W25Q64=SPIA(GPIO16-19)。
**暫定裁決**：正式文件鏈（D01/D02/D11）優先——**AM3352=SPIB、W25Q64=SPIA**。圖4 為孤例，且該圖另有已證實的舊版痕跡（SDRAM 標 EMIF2，已由使用者裁決為 EMIF1），同源誤標可能性高。
**待辦**：硬體/文件側更新圖4（SPI 角色與 EMIF 標註一併修）；在圖面更新前，任何依圖4 做的 SPI 角色假設一律無效。
**Status：DECIDED_PENDING_DRAWING_UPDATE**

### DCR-03：D12 System State Machine 與 Phase A 的關係

**事實**：D12 已存在且完整——七狀態（BOOT/INIT/IDLE/RUN/FAULT/MAINTENANCE/OTA）＋ §6 Command Permission Matrix（含 WAVE_DOWNLOAD/VALIDATE/ACTIVATE/FLASH_SAVE/LOAD 在 RUN=NO、DDS_FREQ/AMP/OFFSET 在 RUN=YES 等逐格定義）。
**暫定裁決**：
1. Phase A 計畫的 `Sandbox_IsOutputOn()` **只能作為 D12 尚未落地前的 proxy**（語意對應：proxy 回傳 1 ≈ D12 RUN、回傳 0 ≈ D12 IDLE）。
2. Phase A 的 reject 規則內容**以 D12 §6 matrix 為準據逐格對齊**（取代原計畫單以 D04/D10/D11 禁止清單推導），但不得提前實作 FAULT/MAINTENANCE/OTA 三態。
3. 未來 D12 System State Machine 落地時，正式依據切換為 system state；proxy 函式內部替換、所有呼叫端不變——此為 proxy 設計的存在目的。
**Status：DECIDED_FOR_PROXY_ONLY**

### DCR-04：D06/D08/D13 空殼

**事實**：三份文件僅有 H1 標題行，無任何 enforceable content（2026-06-13 讀檔確認）。
**暫定裁決**：
- **D06_EMIF1_DRIVER**：可由現有 EMIF work 回填——素材已齊：`emif1_home_sram.c/.h`（async CS3 driver＋timing）、`emif1_home_sram_pin_audit.md`（腳位證據鏈）、SDRAM 控制器參數審查（CL2/4bank/refresh 781 等 12 項，已對 IS42S16160J-7BL 核驗）。回填屬文件撰寫，不在本登記簿執行。
- **D08/D13**：在撰寫完成前**不得作為 code rule 的強約束來源**；既有報告中對 D08/D13 的引用一律解讀為「未來歸屬指向」而非「現行依據」。
**Status：NEEDS_DOCUMENT_AUTHORING**

### DCR-05：M0 Virtual Register Map 不存在

**事實**：`ASR5K_M0_COMM.md` 內容為 M0 晶片**內部** memory map（Code Flash/SRAM/I2C 周邊暫存器 I2C_CON/I2C_TAR/I2C_FAN_CTRL），不是 C2000↔M0 的應用層虛擬暫存器表（D02_2_3 的 10-bit 位址空間 0x000–0x3FF 內容無任何文件定義）。另記：ASR5K_M0_COMM 的 FAN 暫存器為「SPD_CMD＋SPD_ECAP 合一 32-bit」，與 `m0_sandbox` 的「FANx_PWM／FANx_CAP 分離 16-bit」佈局不同。
**暫定裁決**：`m0_sandbox` 的 32 暫存器 map 定性為 **sandbox-only**，僅供模擬資料流觀察，不得作為 C2000 或 M0 韌體任一側的協定依據。
**待辦**：建立正式「M0 Virtual Register Map」文件（建議掛在 D02_2_3 之下），定案後 m0_sandbox 對齊。
**Status：NEEDS_FORMAL_M0_PROTOCOL**

### DCR-06：D11 位址 vs legacy cmd_id.h

**衝突內容**：D11 §8 配置 0x0900/0x0901（OUTPUT）、0x0910–0x0912（DDS）、0x0958（PAGE_SELECT）、0x0960/0x0961（VALIDATE/ACTIVATE）、0x0970（CLEAR_FAULT）；現行 `cmd_id.h` 為 0x0704（page select）、0x0902（output waveform set）、0x0707–0x070A（block 狀態組），且 0x0960/0x0961 不存在。0x3000–0x3FFF window 兩邊一致。
**暫定裁決**：不做任何 code 修改；雙軌現狀凍結。註記兩個判斷線索供 owner 參考：(a) D11 自稱 v0.1「暫定」；(b) D11 §3 引用「Existing ASR5K SPI Slave Register Protocol screenshots」為來源——cmd_id.h 與 screenshots 的對應關係需 owner 核對，版本錯位可能性存在。
**Status：NEEDS_PROTOCOL_OWNER_DECISION**

### DCR-07：D05 byte address vs C28x word addressing

**衝突內容**：D05 以 byte 語言撰寫（page 容量「8192 Bytes」、`sample_addr = wave_addr + sample_index * 2`、各區 offset 以 byte 計）；C28x 為 16-bit word addressing（`sizeof(uint16_t)==1`），code 一律以 word 計（page stride 4096 words）。物理佈局兩者一致（4096 words ≡ 8192 bytes），但依文件公式直接寫 C28x code 會產生 2 倍位址錯誤。
**暫定裁決**：純文件表述問題，非佈局衝突；code 維持 word 計（現況正確）。D05 應補一節「C28x word addressing 對應表述」（byte offset ÷2 = word offset）。
**Status：NEEDS_DOCUMENT_CLARIFICATION**

### DCR-08：CPU2 ownership deviation

**衝突內容**：D01 §1.1／D02 §4.2 規定 FSI、MCBSP、補償迴路屬 CPU2（且使用 CPU2 專屬 6 通道 DMA）；sandbox 的 `fsi_sandbox`／`mcbsp_sandbox`（連同 staged-commit 與電流環模型）全部跑在 CPU1。
**暫定裁決**：接受為 sandbox 階段**刻意偏差**——家中板單核開發便利性優先，且兩模組為純軟體 stub、不占 CPU1 DMA 通道、不影響 100kHz 鏈。遷移條件已備：模組介面自足（init/poll/struct），移植時整包搬至 CPU2 專案、跨核資料走 D02 §4.3 MSGRAM 橋（欄位清單見 Gap Report §7）。正式分工**不因 sandbox 而變更**。
**Status：ACCEPTED_SANDBOX_DEVIATION**

---

## 狀態彙總

| Status | 項目 |
|---|---|
| NEEDS_FORMAL_LAYOUT_DECISION | DCR-01 |
| DECIDED_PENDING_DRAWING_UPDATE | DCR-02 |
| DECIDED_FOR_PROXY_ONLY | DCR-03 |
| NEEDS_DOCUMENT_AUTHORING | DCR-04 |
| NEEDS_FORMAL_M0_PROTOCOL | DCR-05 |
| NEEDS_PROTOCOL_OWNER_DECISION | DCR-06 |
| NEEDS_DOCUMENT_CLARIFICATION | DCR-07 |
| ACCEPTED_SANDBOX_DEVIATION | DCR-08 |

**對 Phase A 的直接影響**：Phase A 實作（仍 hold 中）開工前，僅需 DCR-03 之裁決生效（已 DECIDED_FOR_PROXY_ONLY，可直接採用）；DCR-01/05/06 不阻擋 Phase A 三項小修，但其裁決結果出爐前，相關區域（SEQ 位址、M0 map、協定位址）禁止任何新實作引用。

---

*本登記簿為唯讀治理產物；任何 DCR 的正式裁決需由對應 owner 簽核後，另行授權實作。*
