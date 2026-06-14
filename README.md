# F28384D_SANDBOX

Public F28384D sandbox repository for board-level experiments and peripheral validation.

This repository is a sandbox. It is meant for isolated experiments, bring-up notes, and reproducible validation snippets. It is not an ASR5K production firmware mainline.

## Repository Role

- **Purpose:** F28384D board-level experiments and peripheral validation.
- **Status:** lab sandbox.
- **Visibility:** public.
- **Scope:** isolated prototypes, hardware bring-up, driver experiments, and validation code.
- **Production status:** not production firmware and not an ASR5K source of truth.

## Intended Contents

Use this repository for small, explicit experiments such as:

- board bring-up checks
- GPIO / clock / peripheral validation
- driver experiments
- minimal reproduction projects
- hardware validation notes
- temporary sandbox code before promotion to a real project

## What Should Not Go Here

Do not use this repository for:

- ASR5K production firmware
- confidential product code
- private board documents that are not cleared for public release
- bulk backups or build output
- unrelated MCU projects

## Experiment Documentation Template

Each experiment should record:

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

## Promotion Rule

Sandbox code can move into a production or private project only after:

1. code review
2. build verification
3. target-hardware validation
4. clear ownership decision

## Working Rules

1. Keep experiments small and independently reproducible.
2. Record the target board, toolchain, wiring, and expected result for each test.
3. Do not merge sandbox code into production without review and hardware verification.
4. Do not commit credentials, generated build output, or licensed binaries without review.
