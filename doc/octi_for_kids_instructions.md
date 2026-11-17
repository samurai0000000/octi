# OCTI for Kids Instructions

Official Rules and Game Reference for **OCTI for Kids** (*Octi-6×7*)  
Invented by Don Green — Great American Trading Company / FoxMind  
US Patent 6,182,967

---

## 1. Contents & Board Geometry

### 1.1 Contents

- **1 Game Board**: 6 columns × 7 rows grid (42 squares total), featuring 8 specially marked OCTI squares.
- **8 Pods**: Large octagon-shaped playing pieces (4 Green / Light for Player A, 4 Orange / Dark for Player B).
- **24 Prongs**: Wooden or plastic peg pieces inserted into pods to define movement vectors (12 prongs per player).

### 1.2 Board Geometry & Coordinate System

The board consists of 6 columns (labeled West-to-East as `'1'` through `'6'`, 0-indexed as `x = 0..5`) and 7 rows (labeled South-to-North as `'1'` through `'7'`, 0-indexed as `y = 0..6`).

- **Player A's OCTI Squares (Home Bases)**: Located on row 2 across columns 2 through 5 (`22, 32, 42, 52`).
- **Player B's OCTI Squares (Home Bases)**: Located on row 6 across columns 2 through 5 (`26, 36, 46, 56`).
- **Row 1**: Behind Player A's home bases.
- **Row 7**: Behind Player B's home bases.

```text
      1       2       3       4       5       6
  ┌───────┬───────┬───────┬───────┬───────┬───────┐
7 │       │       │       │       │       │       │ 7
  ├───────┼───────┼───────┼───────┼───────┼───────┤
6 │       │ OCTI  │ OCTI  │ OCTI  │ OCTI  │       │ 6  ◄ Player B Bases (26, 36, 46, 56)
  │       │   B   │   B   │   B   │   B   │       │
  ├───────┼───────┼───────┼───────┼───────┼───────┤
5 │       │       │       │       │       │       │ 5
  ├───────┼───────┼───────┼───────┼───────┼───────┤
4 │       │       │       │       │       │       │ 4
  ├───────┼───────┼───────┼───────┼───────┼───────┤
3 │       │       │       │       │       │       │ 3
  ├───────┼───────┼───────┼───────┼───────┼───────┤
2 │       │ OCTI  │ OCTI  │ OCTI  │ OCTI  │       │ 2  ◄ Player A Bases (22, 32, 42, 52)
  │       │   A   │   A   │   A   │   A   │       │
  ├───────┼───────┼───────┼───────┼───────┼───────┤
1 │       │       │       │       │       │       │ 1
  └───────┴───────┴───────┴───────┴───────┴───────┘
      1       2       3       4       5       6
```

---

## 2. Initial Setup

1. Place the board between the two players such that Player A sits on the South side (facing row 1 and row 2) and Player B sits on the North side (facing row 7 and row 6).
2. **Pod Setup**: Each player receives 4 pods. All 4 pods are placed empty on the player's 4 friendly OCTI squares at the start of the game:
   - Player A places 1 empty pod on each of `22`, `32`, `42`, and `52`.
   - Player B places 1 empty pod on each of `26`, `36`, `46`, and `56`.
3. **Reserve Pods**: Unlike standard 9×9 OCTI, there are **0 reserve pods** in OCTI for Kids. All pods begin on the board immediately.
4. **Prong Supply**: Each player receives 12 prongs kept in their off-board supply.
5. **Arrow Orientation**: Pods have directional indicators (arrows) that must point directly toward the opponent's side of the board at all times.

---

## 3. Rules of Play

Players alternate turns, beginning with Player A. On each turn, a player must choose and perform **one and only one** of the following three actions:

### Option 1: Insert 1 Prong into a Friendly Pod

- The player inserts 1 prong from their personal supply into any of the 8 prong slots of a friendly pod currently on the board.
- **Permanent Placement**: Across all OCTI variants (including OCTI for Kids), **prongs cannot be repositioned** once inserted. Once a prong is placed in a pod, it remains in that slot for the duration of the pod's life on the board.
- A single pod may hold up to 8 prongs (one in each of the 8 compass directions: N, NE, E, SE, S, SW, W, NW).

### Option 2: Move 1 Friendly Pod One Square

- A player moves 1 friendly pod exactly 1 square to an adjacent square in the direction of one of its existing prongs.
- **Empty Square Requirement**: The destination square must be completely **empty**.
- **No Stacking**: Pod stacking is **strictly forbidden** in OCTI for Kids. A pod can never enter or land on a square occupied by any other pod (friendly or enemy).

### Option 3: Jump Over Adjacent Pods (With Optional Capture)

- A pod may jump over an adjacent pod (friendly or enemy) in the direction of one of its prongs, provided that the square immediately beyond the jumped pod is **empty**.
- **Landing Square**: The jumping pod must land directly on the empty square immediately behind the jumped pod.
- **Multi-Jumps**: A pod may execute a series of consecutive jumps during a single turn, changing directions between jumps provided each jump is aligned with an available prong on the jumping pod.
  - A pod cannot jump over the same square twice in a single turn.
  - Intermediate landing squares must be empty at the moment of landing.
- **Capturing (Optional)**:
  - Jumped pods (friendly or enemy) may be captured at the conclusion of the turn (denoted with `'x'` in move notation). Capturing is optional.
  - **Permanent Removal**: Captured pods are permanently removed from the game. There is no reserve pool and no liberation mechanic in OCTI for Kids.
  - **Prong Seizure**: All prongs inside a captured pod are seized by the capturing player and immediately added to that player's prong supply for future insertion into their own remaining pods.

---

## 4. Victory, Stalemate & No-Progress Draw Conditions

A player wins immediately when any of the following conditions is met:

1. **Base Occupation (1-Base Victory)**: A player moves or jumps a friendly pod onto **any one** of the opponent's 4 OCTI squares (e.g. Player A occupies `26`, `36`, `46`, or `56`; or Player B occupies `22`, `32`, `42`, or `52`).
2. **Elimination by Capture**: A player captures all 4 of the opponent's pods.
3. **No Legal Moves**: The opponent has no legal moves available on their turn (e.g., all remaining opponent pods are immobilized and their prong supply is exhausted).
4. **Universal No-Progress Draw (`NO_PROGRESS_DRAW`, 30 Plies / 15 Turns per Player)**: If **30 consecutive plies (`15` full turns per player)** complete without any **progress event**—either a **pod capture (`x`)** or a **prong insertion (`+d`)**—the game terminates automatically in a draw (`NO_PROGRESS_DRAW`, `noProgressPlies == 30`). Base-occupation victory, total capture wipeout, and zero-legal-move stalemate are evaluated before the 30-ply draw check, so winning on the 30th non-progress ply still wins immediately.

---

## 5. Variant Comparison: Standard 2P, 4P Partnership & OCTI for Kids

| Feature | Standard 2-Player OCTI | 4-Player Partnership OCTI | OCTI for Kids (*Octi-6×7*) |
|---|---|---|---|
| **Board Dimensions** | 9×9 (81 squares) | 9×9 (81 squares) | 6×7 (42 squares) |
| **OCTI Squares Total** | 6 squares (3 per player) | 6 squares (4 corners + 2 neutral) | 8 squares (4 per player) |
| **Home Base Coordinates** | A: `33, 53, 73`; B: `37, 57, 77` | A: `33`; B: `37`; C: `77`; D: `73` (Neutral: `53, 57`) | A: `22, 32, 42, 52`; B: `26, 36, 46, 56` |
| **Pods per Player** | 7 (3 on board, 4 reserve) | 4 (1 on board, 3 reserve) | 4 (all 4 on board, 0 reserve) |
| **Total Pods in Game** | 14 pods | 16 pods | 8 pods |
| **Prongs per Player** | 25 prongs (or 24 + 1 superprong) | 12 prongs | 12 prongs |
| **Pod Stacking** | Allowed (up to 7 pods) | Allowed (allied pods up to 8) | **Forbidden** (strictly 1 pod/square) |
| **Prong Repositioning** | **Forbidden** (prongs are permanent) | **Forbidden** (prongs are permanent) | **Forbidden** (prongs are permanent) |
| **Reserve Pod Placement** | Allowed on friendly/occupied base | Allowed on friendly/occupied base | **None** (0 reserve pods) |
| **Captured Pod Liberation** | Allowed in `2P_3B*` when occupying enemy base | Allowed in `4P*` when occupying enemy/neutral base | **Forbidden** (captures are permanent) |
| **Superprongs Supported** | Yes (`<= 2` bonus preludes, 1 per SP) | Yes (`<= 4` bonus preludes, 1 per SP) | No |
| **Edgeless Board Variant** | Yes (optional variant) | Yes (optional variant) | No |
| **Turn Options Count** | 4 distinct turn options | 4 distinct turn options | 3 distinct turn options |
| **No-Progress Draw Limit** | 30 plies (15 turns per player) | 60 plies (15 full 4-seat rounds) | 30 plies (15 turns per player) |
| **Victory Condition** | Occupy 3 bases (or 1 for short game) | Occupy opponents' 2 team bases | Occupy any 1 opponent base |

---

## 6. Appendix A: OCTI Project Interpretations

Where the classic rules require formalization for edge cases, Don Green's November 2026 review establishes the exact rules enforced across C++ `octi_core`, the Web JS engine, Desktop Qt6, and the Android Java engine below.

### A.1 Starting-Square Vacancy & Loop-Back Jumps

- In OCTI for Kids (`OFK`), pod stacking is forbidden, so a jumping pod's starting square `S` is always vacated when the pod jumps out of `S`.
- A jumping pod can never jump over `S` during its jump chain, and a multi-hop loop (`S -> ... -> S`) that lands back on `S` is legal. Non-capturing circular multi-hop jumps (`S -> ... -> S`) count as movement-only plies (`+1` toward the 30-ply no-progress draw threshold).

### A.2 Universal No-Progress Draw (Don Green, November 2026)

1. **Universal No-Progress Draw (`NO_PROGRESS_DRAW`, `noProgressPlies`):** A match in `OFK` (or any `2P*` variant, `OCTI_NO_PROGRESS_DRAW_PLIES_2P = 30`) terminates automatically in a draw (`NO_PROGRESS_DRAW`) when **`30` consecutive plies (`15` full turns per player)** complete without any **progress event** (in `4P*` variants, the threshold is **`60` consecutive plies / `15` full rounds**, `OCTI_NO_PROGRESS_DRAW_PLIES_4P = 60`).
2. **Progress Events in `OFK` (Reset `noProgressPlies = 0`):** Because `OFK` has 0 reserve pods and no liberation mechanic, `noProgressPlies` resets to `0` on any turn that:
   - **Captures a pod (`x`)** during a single or multi-hop jump, or
   - **Inserts a prong (`+d`)** from the player's supply into a friendly pod.
3. **Movement-Only Plies (`noProgressPlies += 1`):** Every 1-square step and every non-capturing single, multi-hop, or circular jump (`S -> ... -> S`) increments `noProgressPlies` by `+1`.
4. **Evaluation Precedence (Victory & Stalemate First):** Base-occupation victory, total capture wipeout, and zero-legal-move stalemate are evaluated at turn completion **before** checking `noProgressPlies >= 30` (so winning or stalemating the opponent on ply `30` still wins).
