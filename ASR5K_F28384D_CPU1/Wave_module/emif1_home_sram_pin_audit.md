# EMIF1 Home SRAM Pin Audit — IS61LV51216 ↔ F28388D

建立日期：2026-06-11
更新：2026-06-12 取得家中板 U9 原理圖（E6），以第二功能網路標籤解碼出
GPIO 層級接線；**第 2 節之 GPIO 判定已被第 6 節取代**，最終結論以第 4 節
（已同步更新）與第 6 節為準。
對象：家中開發板外擴 SRAM IS61LV51216（512K x 16，EMIF1 CS3n，0x00300000）
關聯程式：[emif1_home_sram.c](emif1_home_sram.c) / [emif1_home_sram.h](emif1_home_sram.h)

---

## 0. 證據來源定義

| 代號 | 來源 | 證據等級 | 備註 |
|---|---|---|---|
| E1 | 開發板手冊（使用者轉述之 U9 接線清單） | 訊號層級 CONFIRMED | 只給 EMIF「訊號名」，**不含 GPIO 編號** |
| E2 | 使用者貼附之原理圖影像 | **不適用於家中板** | 該圖為**產品板 U2009 IS42S16160J SDRAM**，非家中板 U9 SRAM；本審計不以它作為家中板證據，僅用於核對產品側 syscfg 一致性 |
| E3 | `pinmux.syscfg`（本韌體 Board_init 實際驅動的腳位） | 韌體側 CONFIRMED | 此為「韌體會把訊號開在哪個 GPIO」的事實，但**不證明家中板 PCB 接到同一腳** |
| E4 | `device/driverlib/pin_map.h`（F2838x mux 全表） | 元件層級 CONFIRMED | 列出每個 EMIF1 訊號「可能」出現的所有 GPIO；若某訊號只有一個選項，則為元件強制解 |
| E5 | 開發板手冊 `InitEmif1()` 原始碼 | **未取得** | 手冊確知存在此函式，但本審計未拿到內容；它是 GPIO 層級的最終證據 |
| E6 | **家中板 U9 原理圖**（使用者提供，2026-06-12） | GPIO 層級 CONFIRMED（限可解碼者） | 網路名稱帶第二功能標籤（`ESC_*` / `ENET_*`），與 E4 交叉比對可唯一決定 ball；**plain 標籤（無第二功能）之網路無法解碼** |

判定規則：
- **CONFIRMED**＝E1 確認訊號被使用，且 E4 顯示該訊號在元件上只有唯一 GPIO（板子無選擇空間）。
- **INFERRED**＝E4 有多個 GPIO 選項，本審計採用 E3（syscfg 主腳位群）作為預設推論；**排除法與「最可能」均不作為最終證據**。
- **UNKNOWN**＝多選項且無任何家中板側證據可收斂。

---

## 1. SRAM ↔ EMIF1 訊號層級接線（證據：E1，全部 CONFIRMED）

### 1.1 控制線

| SRAM 腳 | EMIF1 訊號 | 證據 |
|---|---|---|
| /CE | EM1CS3n | E1 |
| /WE | EM1WEn | E1 |
| /OE | EM1OEn | E1 |
| /UB, /LB | DGND（常態致能，DQM 不參與） | E1 |

### 1.2 資料線（順序對應）

| SRAM | EMIF1 | 證據 |
|---|---|---|
| D0–D15 | EM1D0–EM1D15（依序） | E1 |

### 1.3 位址線（亂序、一對一）

| SRAM | EMIF1 | | SRAM | EMIF1 |
|---|---|---|---|---|
| A0 | **EM1BA1** | | A10 | EM1A9 |
| A1 | EM1A0 | | A11 | EM1A10 |
| A2 | EM1A1 | | A12 | EM1A11 |
| A3 | EM1A2 | | A13 | EM1A12 |
| A4 | EM1A3 | | A14 | EM1A13 |
| A5 | EM1A8 | | A15 | EM1A15 |
| A6 | EM1A7 | | A16 | EM1A16 |
| A7 | EM1A6 | | A17 | EM1A17 |
| A8 | EM1A5 | | A18 | **EM1A14** |
| A9 | EM1A4 | | | |

訊號層級結論（E1）：
- 實際使用之 EMIF1 訊號 = **EM1BA1 + EM1A0–EM1A17**（共 19 條，恰為 512K words）。
- **EM1A18、EM1A19 未使用 — CONFIRMED（E1 清單中不存在）**。
- SRAM A0 接 EM1BA1 與 C2000 EMIF 16-bit async 規範一致（word address LSB 由 EMxBA1 輸出），軟體不需任何處理。
- 亂序為一對一映射（bijection）：讀寫走同一實體路徑，正確性不受影響；2 的冪次 offset 之 address-bus memory test 每次仍只翻轉一條實體線，**測試有效性不受亂序影響**。

---

## 2. GPIO 層級逐訊號審計

「mux 選項」欄取自 E4（pin_map.h 全表掃描）；「韌體設定」欄取自 E3。

### 2.1 控制線

| EMIF1 訊號 | mux 選項 (E4) | 韌體設定 (E3) | 判定 | 說明 |
|---|---|---|---|---|
| EM1WEn | **GPIO31（唯一）** | GPIO31（syscfg） | **CONFIRMED** | 元件唯一解 |
| EM1OEn | GPIO32 / GPIO37 | GPIO32（HOME build 由 `Emif1HomeSram_Init()` 設定） | **INFERRED** | GPIO37 被本韌體用作 EM1A2，故推 GPIO32；此為排除法推論，**非最終證據**。若家中板把 A2 接在替代腳 GPIO40，OEn 接 GPIO37 在元件上是合法的 |
| EM1CS3n | GPIO19 / GPIO29 / GPIO35 | GPIO29（HOME build 預設，可用 `HOME_SRAM_CS3N_PINCFG` 改 GPIO19） | **UNKNOWN** | 三選項中 GPIO35 與「A0=GPIO35」推論互斥，但仍剩 19/29 兩可能，無家中板側證據可收斂。**程式碼目前的 GPIO29 是假設值** |
| EM1DQM0/1 | （不適用） | GPIO24/25（syscfg，SDRAM 用） | 不影響 | SRAM UB/LB 接地，DQM 訊號未接到 SRAM；韌體照常驅動無害 |

### 2.2 位址線

| EMIF1 訊號 | mux 選項 (E4) | 韌體設定 (E3/HOME) | 判定 |
|---|---|---|---|
| EM1BA1 | GPIO21 / 34 / 92 / 94 | GPIO21（syscfg） | **INFERRED**（四選項，採 syscfg 主群） |
| EM1A0 | GPIO35 / 38 | GPIO35（syscfg） | INFERRED |
| EM1A1 | GPIO36 / 39 | GPIO36（syscfg） | INFERRED |
| EM1A2 | GPIO37 / 40 | GPIO37（syscfg） | INFERRED |
| EM1A3 | GPIO38 / 41 | GPIO38（syscfg） | INFERRED |
| EM1A4 | GPIO39 / 44 | GPIO39（syscfg） | INFERRED |
| EM1A5 | GPIO45 / 49 | GPIO45（syscfg） | INFERRED |
| EM1A6 | GPIO46 / 50 | GPIO46（syscfg） | INFERRED |
| EM1A7 | GPIO47 / 51 | GPIO47（syscfg） | INFERRED |
| EM1A8 | GPIO48 / 52 | GPIO48（syscfg） | INFERRED |
| EM1A9 | GPIO49 / 53 | GPIO49（syscfg） | INFERRED |
| EM1A10 | GPIO50 / 54 | GPIO50（syscfg） | INFERRED |
| EM1A11 | **GPIO51（唯一）** | GPIO51（syscfg） | **CONFIRMED** |
| EM1A12 | **GPIO52（唯一）** | GPIO52（syscfg） | **CONFIRMED** |
| EM1A13 | **GPIO86（唯一）** | GPIO86（HOME build 設定） | **CONFIRMED** |
| EM1A14 | **GPIO87（唯一）** | GPIO87（HOME build 設定） | **CONFIRMED** |
| EM1A15 | **GPIO88（唯一）** | GPIO88（HOME build 設定） | **CONFIRMED** |
| EM1A16 | **GPIO89（唯一）** | GPIO89（HOME build 設定） | **CONFIRMED** |
| EM1A17 | **GPIO90（唯一）** | GPIO90（HOME build 設定） | **CONFIRMED** |
| EM1A18 | GPIO91（唯一） | 不設定 | **CONFIRMED 未使用**（E1） |

### 2.3 資料線

| EMIF1 訊號 | mux 選項 (E4) | 韌體設定 (E3) | 判定 |
|---|---|---|---|
| EM1D0 | GPIO55 / 85 | GPIO55 | INFERRED |
| EM1D1 | GPIO56 / 83 | GPIO56 | INFERRED |
| EM1D2 | GPIO57 / 82 | GPIO57 | INFERRED |
| EM1D3 | GPIO58 / 81 | GPIO58 | INFERRED |
| EM1D4 | GPIO59 / 80 | GPIO59 | INFERRED |
| EM1D5 | GPIO60 / 79 | **GPIO79（替代腳！）** | INFERRED |
| EM1D6 | GPIO61 / 78 | **GPIO78（替代腳！）** | INFERRED |
| EM1D7 | GPIO62 / 77 | GPIO62 | INFERRED |
| EM1D8 | **GPIO76（唯一）** | GPIO76 | **CONFIRMED** |
| EM1D9 | **GPIO75（唯一）** | GPIO75 | **CONFIRMED** |
| EM1D10 | **GPIO74（唯一）** | GPIO74 | **CONFIRMED** |
| EM1D11 | **GPIO73（唯一）** | GPIO73 | **CONFIRMED** |
| EM1D12 | **GPIO72（唯一）** | GPIO72 | **CONFIRMED** |
| EM1D13 | **GPIO71（唯一）** | GPIO71 | **CONFIRMED** |
| EM1D14 | **GPIO70（唯一）** | GPIO70 | **CONFIRMED** |
| EM1D15 | **GPIO69（唯一）** | GPIO69 | **CONFIRMED** |

> ⚠️ 注意 D5/D6：本韌體（依產品板）選用的是**替代腳** GPIO79/GPIO78，而非主群 GPIO60/61。
> 這證明「板子確實會選替代腳」——所以所有 INFERRED 項都不能想當然，家中板可能選不同邊。

---

## 3. 特別確認事項（依任務指定逐項回答）

| 項目 | 結論 | 判定 |
|---|---|---|
| EM1BA1 / GPIO21 | SRAM A0 接 EM1BA1（E1 確認）；BA1 訊號在元件上有 GPIO21/34/92/94 四個選項，本韌體開在 GPIO21（E3） | 訊號 CONFIRMED；GPIO **INFERRED** |
| EM1A0–A17 | 全部被 SRAM 使用（E1，含亂序映射）；其中 A11–A17 為元件唯一腳（GPIO51/52/86/87/88/89/90）、A0–A10 各有兩個 mux 選項 | A11–A17 GPIO **CONFIRMED**；A0–A10 GPIO **INFERRED** |
| EM1A18 是否未使用 | **未使用**。SRAM 19 條位址線由 BA1 + A0–A17 覆蓋；E1 接線清單無 EM1A18 | **CONFIRMED** |
| EM1OEn = GPIO32？ | 元件上 OEn 可在 GPIO32 或 GPIO37。GPIO32 之推論依據是「GPIO37 已被本韌體用作 A2」——**這是排除法，不是接線證據** | **INFERRED（GPIO32）** |
| EM1CS3n = GPIO19 或 GPIO29？ | 元件上可在 GPIO19/29/35。無任何家中板側 GPIO 證據。程式碼預設 GPIO29 為**假設** | **UNKNOWN** |
| EM1WEn | SRAM /WE 接 EM1WEn（E1）；元件唯一腳 GPIO31，syscfg 亦為 GPIO31 | **CONFIRMED（GPIO31）** |

---

## 4. 總結清單（2026-06-12 依 E6 原理圖解碼更新）

### CONFIRMED pins

元件唯一解（E4）× E1 訊號確認：

```
EM1WEn  = GPIO31
EM1A11  = GPIO51        EM1D8  = GPIO76
EM1A12  = GPIO52        EM1D9  = GPIO75
EM1A13  = GPIO86        EM1D10 = GPIO74
EM1A14  = GPIO87        EM1D11 = GPIO73
EM1A15  = GPIO88        EM1D12 = GPIO72
EM1A16  = GPIO89        EM1D13 = GPIO71
EM1A17  = GPIO90        EM1D14 = GPIO70
EM1A18  = 未使用         EM1D15 = GPIO69
```

E6 原理圖第二功能標籤解碼（詳見第 6 節）：

```
與 syscfg 一致：
EM1A6 = GPIO46   EM1A9  = GPIO49   EM1D5 = GPIO79
EM1A7 = GPIO47   EM1A10 = GPIO50   EM1D6 = GPIO78

與 syscfg 不同（家中板用替代腳，HOME build 已改在 C code 設定）：
EM1A1 = GPIO39 (syscfg: 36)    EM1D0 = GPIO85 (syscfg: 55)
EM1A2 = GPIO40 (syscfg: 37)    EM1D1 = GPIO83 (syscfg: 56)
EM1A3 = GPIO41 (syscfg: 38)    EM1D2 = GPIO82 (syscfg: 57)
EM1A4 = GPIO44 (syscfg: 39)    EM1D3 = GPIO81 (syscfg: 58)
                               EM1D4 = GPIO80 (syscfg: 59)
                               EM1D7 = GPIO77 (syscfg: 62)
```

### INFERRED pins（E6 網路標籤為 plain，無法解碼；推論依據如註）

```
EM1A5  = GPIO45 (alt: 49；GPIO49 已被 A9 佔用 + syscfg 一致 → 強推論)
EM1A8  = GPIO48 (alt: 52；GPIO52 已被 A12 佔用 + syscfg 一致 → 強推論)
EM1A0  = GPIO35 (alt: 38；GPIO38 因 A3=41 而空出，無法排除 → 弱推論)
EM1BA1 = GPIO21 (alt: 34/92/94；採 syscfg 主群 → 弱推論)
```

### UNKNOWN pins

```
EM1CS3n = GPIO19 或 GPIO29（若 A0 實際在 GPIO38，則 GPIO35 也可能）
          程式碼預設 GPIO29（HOME_SRAM_CS3N_PINCFG 可切換）
EM1OEn  = GPIO32 或 GPIO37（E6 證實 A2 在 GPIO40，GPIO37 是空的，
          先前「GPIO37 被 A2 佔用」之排除法失效）
          程式碼預設 GPIO32（HOME_SRAM_OEN_PINCFG 可切換）
```

### 上機前必查項目

1. **取得手冊 `InitEmif1()` 原始碼（E5）**——剩餘 6 個未定腳一次定案：
   CS3n（19/29）、OEn（32/37）、BA1（21/34/92/94）、A0（35/38）、
   A5（45/49）、A8（48/52）。
2. 燒錄後第一檢查點：`g_sWaveMem.u16MemTestPass`。失敗模式對照
   （`u32MemTestFailAddr` 為 word 位址；offset bit = 位址 − 0x00300000）：
   - **第一步 data-bus walk 全面亂碼** → CS3n 腳錯：切
     `HOME_SRAM_CS3N_PINCFG = GPIO_19_EMIF1_CS3N` 重試
   - **寫入正常但讀回全為 0xFFFF（浮接）** → OEn 腳錯：切
     `HOME_SRAM_OEN_PINCFG = GPIO_37_EMIF1_OEN` 重試
   - **個別 data bit 固定 0/1** → 對應 D 線（理論上 E6 已全數定案，
     若仍發生請回查第 6 節解碼）
   - **address-bus 在特定 2 冪次 offset fail** →
     offset bit0 → EM1BA1（BA1 腳推論錯，試 34/92/94）
     offset bit1 → EM1A0（試 GPIO38）
     offset bit6 → EM1A5（試 GPIO49）／offset bit9 → EM1A8（試 GPIO52）
     其餘 bit → 對照第 6 節（理論上已定案）
3. 若 CS3n 需用 GPIO19：本韌體 syscfg 將 GPIO19 建議給 SPIA STE
   （`spi1.spi_ptePin`），HOME build 需確認 SPIA 功能在家中板上不被需要。
4. 全部 pass 後，把實測結果回填本文件，將 INFERRED/UNKNOWN 升級為
   CONFIRMED（註記證據 = E5 或實機 memtest）。

---

## 5. 附註

- ~~E2（產品板 U2009 原理圖）紅字標註「EMIF2 CS0n、0x90000000」與網路
  名稱（`EMIF1_*`）矛盾~~ → **已結案（2026-06-12）：產品 SDRAM 確認走
  EMIF1 CS0 @ 0x80000000**（使用者確認）。佐證：產品 block diagram 若採
  EMIF2，其 A2–A5（GPIO100–103）與 SPIC_MEAS_DDS、D2/D3（GPIO135/136）
  與 SCI Boot Debug 直接撞腳，EMIF2 方案在該腳位規劃下不可行；「EMIF2 /
  0x90000000」為早期規劃殘留標註。PROD backend 維持
  `WAVE_MEM_BASE_WORD_ADDR = 0x80000000`，syscfg EMIF1_SDRAM 組態即正解。
- 本審計之 E1 為使用者轉述之手冊內容；若日後發現手冊原文與轉述有出入，
  以手冊原文為準並更新本文件。

---

## 6. E6 原理圖 GPIO 解碼（2026-06-12）

解碼方法：U9 原理圖的網路名稱形如 `EM1xx/第二功能`。第二功能（`ESC_*`、
`ENET_*`）在 F2838x 上有固定的 GPIO mux（E4 pin_map.h），與該 EMIF 訊號的
mux 選項交集若唯一，即可確定 ball。E6 同時逐腳重新驗證了 E1 的訊號層級
接線（U9 44 腳全數吻合，含 /UB、/LB 接地與 EM1A18 未使用）。

| U9 腳 | SRAM 訊號 | 網路標籤 | EMIF mux 選項 (E4) | 第二功能所在 GPIO | 判定 |
|---|---|---|---|---|---|
| 1 | A0 | `EM1BA1`（plain） | 21/34/92/94 | — | INFERRED 21 |
| 2 | A1 | `EM1A0`（plain） | 35/38 | — | INFERRED 35 |
| 3 | A2 | `EM1A1/ENET_MII_COL` | 36/39 | COL→39（36 無 ENET 功能） | **GPIO39** |
| 4 | A3 | `EM1A2/ENET_MII_CRS` | 37/40 | CRS→40（37 無 ENET 功能） | **GPIO40** |
| 5 | A4 | `EM1A3/ENET_REVMII_MDIO_RST` | 38/41 | REVMII_MDIO_RST→41 | **GPIO41** |
| 6 | /CE | `EM1CS3n`（plain） | 19/29/35 | — | UNKNOWN（預設 29） |
| 7 | D0 | `EM1D0/ESC_TX0_CLK` | 55/85 | TX0_CLK→85 | **GPIO85** |
| 8 | D1 | `EM1D1/ESC_RX0_DATA3` | 56/83 | →83 | **GPIO83** |
| 9 | D2 | `EM1D2/ESC_RX0_DATA2` | 57/82 | →82 | **GPIO82** |
| 10 | D3 | `EM1D3/ESC_RX0_DATA1` | 58/81 | →81 | **GPIO81** |
| 13 | D4 | `EM1D4/ESC_RX0_DATA0` | 59/80 | →80 | **GPIO80** |
| 14 | D5 | `EM1D5/ESC_RX0_ERR` | 60/79 | →79 | **GPIO79**（=syscfg） |
| 15 | D6 | `EM1D6/ESC_RX0_DV` | 61/78 | →78 | **GPIO78**（=syscfg） |
| 16 | D7 | `EM1D7/ESC_RX0_CLK` | 62/77 | →77 | **GPIO77** |
| 17 | /WE | `EM1WEn` | 31（唯一） | — | **GPIO31** |
| 18 | A5 | `EM1A8`（plain） | 48/52 | —（52 已被 A12 佔用） | INFERRED 48 |
| 19 | A6 | `EM1A7/ESC_MDIO_DATA` | 47/51 | →47 | **GPIO47**（=syscfg） |
| 20 | A7 | `EM1A6/ESC_MDIO_CLK` | 46/50 | →46 | **GPIO46**（=syscfg） |
| 21 | A8 | `EM1A5`（plain） | 45/49 | —（49 已被 A9 佔用） | INFERRED 45 |
| 22 | A9 | `EM1A4/ENET_MII_TX_CLK` | 39/44 | TX_CLK→44 | **GPIO44** |
| 23 | A10 | `EM1A9/ENET_MII_RX_CLK` | 49/53 | →49 | **GPIO49**（=syscfg） |
| 24 | A11 | `EM1A10/ENET_MII_RX_DV` | 50/54 | →50 | **GPIO50**（=syscfg） |
| 25 | A12 | `EM1A11/ENET_MII_RX_ERR` | 51（唯一） | →51 雙重吻合 | **GPIO51** |
| 26 | A13 | `EM1A12/ENET_MII_RX_DATA0` | 52（唯一） | →52 雙重吻合 | **GPIO52** |
| 27 | A14 | `EM1A13/ESC_PHY0_LINKSTATUS` | 86（唯一） | →86 雙重吻合 | **GPIO86** |
| 28 | A18 | `EM1A14/ESC_TX0_DATA0` | 87（唯一） | →87 雙重吻合 | **GPIO87** |
| 29 | D8 | `EM1D8/ESC_PHY_RESETn` | 76（唯一） | →76 雙重吻合 | **GPIO76** |
| 30 | D9 | `EM1D9/ENET_MII_TX_DATA0` | 75（唯一） | →75 雙重吻合 | **GPIO75** |
| 31 | D10 | `EM1D10/ENET_MII_TX_DATA1` | 74（唯一） | →74 雙重吻合 | **GPIO74** |
| 32 | D11 | `EM1D11/ENET_MII_TX_DATA2` | 73（唯一） | →73 雙重吻合 | **GPIO73** |
| 35 | D12 | `EM1D12/ENET_MII_TX_DATA3` | 72（唯一） | →72 雙重吻合 | **GPIO72** |
| 36–38 | D13–D15 | `EM1D13/14/15`（plain） | 71/70/69（唯一） | — | **GPIO71/70/69** |
| 41 | /OE | `EM1OEn`（plain） | 32/37 | — | UNKNOWN（預設 32） |
| 42 | A15 | `EM1A15/ESC_TX0_DATA1` | 88（唯一） | →88 雙重吻合 | **GPIO88** |
| 43 | A16 | `EM1A16/ESC_TX0_DATA2` | 89（唯一） | →89 雙重吻合 | **GPIO89** |
| 44 | A17 | `EM1A17/ESC_TX0_DATA3` | 90（唯一） | →90 雙重吻合 | **GPIO90** |

### 6.1 對韌體的影響（已落實於 emif1_home_sram.c）

1. 家中板在 A1–A4、D0–D4、D7 共 **10 個訊號使用替代 mux 腳**，與產品
   syscfg 不同。HOME build 的 `Emif1HomeSram_Init()` 已逐腳以
   `GPIO_setPinConfig()` 改設。
2. **必須釋放 syscfg 的舊腳**（GPIO36/37/38、55–59、62 改回 plain GPIO）：
   資料腳若同時有兩個 ball mux 到同一個 EMIF1_Dx，讀取路徑未定義，
   memtest 會以難以判讀的方式失敗。位址腳重複只是多驅動一個懸空 ball，
   但一併釋放以免干擾開發板上其他電路。
3. 先前「OEn=GPIO32（排除法）」的推論隨 A2=GPIO40 而失效，OEn 降為
   UNKNOWN，與 CS3n 同樣做成可切換 define。
4. plain 標籤腳（BA1、A0、A5、A8、CS3n、OEn）維持 INFERRED/UNKNOWN，
   等 E5（手冊 InitEmif1 原始碼）或實機 memtest 失敗特徵定案。
