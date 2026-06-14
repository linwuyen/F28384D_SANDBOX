# F28384D_SANDBOX

F28384D board-level experiments 與 peripheral validation 用的公開 sandbox repository。

這個 repo 是 sandbox。用途是放隔離實驗、bring-up notes、可重現 validation snippets。它不是 ASR5K production firmware mainline。

## 1. Repository 角色

- **用途：** F28384D board-level experiments and peripheral validation。
- **狀態：** lab sandbox。
- **可見性：** public。
- **範圍：** isolated prototypes、hardware bring-up、driver experiments、validation code。
- **正式產品狀態：** 不是 production firmware，也不是 ASR5K source of truth。

## 2. Intended contents

此 repo 適合放小型、明確的實驗，例如：

- board bring-up checks
- GPIO / clock / peripheral validation
- driver experiments
- minimal reproduction projects
- hardware validation notes
- promotion 到正式 project 前的 temporary sandbox code

## 3. What should not go here

不要把此 repo 用於：

- ASR5K production firmware
- 未整理、未審查的產品資料
- 未確認可公開的 board documents
- bulk backups 或 build output
- unrelated MCU projects

## 4. Experiment documentation template

每個 experiment 應記錄：

```text
Experiment name:
Target board:
MCU:
CCS version:
C2000Ware version:
Wiring / hardware setup:
Expected result:
Observed result:
Status: PASS / FAIL / WIP
```

## 5. Promotion rule

Sandbox code 只有在完成下列事項後，才能移入 production 或 private project：

1. code review
2. build verification
3. target-hardware validation
4. clear ownership decision

## 6. Working rules

1. experiments 要小、獨立、可重現。
2. 每個 test 都要記錄 target board、toolchain、wiring、expected result。
3. sandbox code 未 review 與 hardware verification 前，不要 merge 到 production。
4. 不要 commit generated build output 或未審查的 binary。
