# OCTI Instructions

## Contents

- **OCTI game board** — 9x9 grid of squares, 6 of which are marked "OCTI"
- **16 pods** (8 light/8 dark) — octagon shaped pieces
- **52 prongs** — (50 dark/2 light) — wooden peg pieces

## Objective

The winner is the first player to occupy all three enemy OCTI squares at the same time. For a shorter game, play to occupy a single enemy OCTI square.

---

## Setup: Two-player game

- Place game board between players, so that OCTI squares face each player.
- Each player receives 7 pods of a color and 25 dark colored prongs.
- At the start, players place one empty pod on each of the friendly OCTI squares.
- Each player holds four extra pods in reserve for future use.
- Arrows on pods should be visible and *always* pointed toward the opponent.

## Rules of play

OCTI is a game of options. Players, in turn, must do one and only one of the following:

1. **I.** Insert a prong into a friendly pod.
2. **II.** Move a friendly pod (or pods if more than one occupy the same square).
3. **III.** Jump other pods, capturing them if desired.
4. **IV.** Bring a reserve or captured pod onto the board.

When exercising one of these options, keep the following guidelines in mind:

- Pods may only move or jump in the direction of their prongs.
- A prong may only be added to your own pod. Once inserted, prongs are permanent and cannot be repositioned to another socket.
- Some or all of the pods that occupy a square may move or jump in a single turn.
- Pods stacked together in a single square may move or jump in separate directions.
- Pods move only to adjacent squares (unless jumping).
- Pods can never move to a square occupied by an enemy pod, but any number of friendly pods may stack together.
- Pods may jump other pods, whether friendly or enemy.
- A jumping pod must land in an empty square immediately behind the jumped pod.
- Multiple jumps are allowed, but a pod cannot jump the same square twice in a turn.
- Any pod that is jumped may be captured at the conclusion of the turn.
- Captured pods are removed from the board and kept separate from reserve pods.
- Pods are not required to jump, and capturing is optional.
- If jumped with capture (`x`), **all pods in the jumped stack are captured** (whole-stack capture; partial stack capture is never allowed).
- Pods stacked together may not jump each other.
- A stack may jump to the same destination.
- Players seize all prongs from pods that they capture.
- A reserve pod may be brought onto any friendly OCTI square.
- A reserve pod may be brought onto any enemy OCTI square you occupy.
- In 3-Base (`2P_3B*`) and 4-Player (`4P*`) games, if you occupy an enemy (or neutral 4P) OCTI square, you may liberate one of your own captured pods (`xy*`); this liberated pod is stacked on any OCTI square that you already occupy (liberation does not apply to 1-Base sudden-death variants or `OFK`).
- OCTI squares do not affect movement or capture. They are not "safe" squares.

---

## Illustrated Movement & Jump Examples

### 1. Moving Stacked Pods

```text
┌─────┬─────┬─────┬─────┐
│     │     │     │     │
├─────┼─────┼─────┼─────┤
│     │  A  │  B  │     │
├─────┼─────┼─────┼─────┤
│     │     │ ⬢×3 │     │  (Top: ↖, Middle: ↑, Bottom: ↖→)
├─────┼─────┼─────┼─────┤
│     │     │     │     │
└─────┴─────┴─────┴─────┘
```

- Pieces stacked together in one square may each move on a single turn.
- On this turn, the top pod moves to **A**; the middle pod moves to **B**; and the bottom pod remains in place.

### 2. Jumping & Capturing with Stacked Pods

```text
┌─────┬─────┬─────┬─────┐
│     │     │  C  │     │
├─────┼─────┼─────┼─────┤
│     │     │  ⬡  │     │
├─────┼─────┼─────┼─────┤
│  A  │  ⬢  │  B  │     │
├─────┼─────┼─────┼─────┤
│     │  ⬡  │  ⬢  │     │
├─────┼─────┼─────┼─────┘
│     │     │ ⬢×2 │  ◄ Bottom pod jumps to A to B,
└─────┴─────┴─────┘    and top pod jumps to B to C.
```

- Stacked pods ignore each other when moving or jumping.
- Any pieces that are jumped may be removed at the end of the turn.
- Bottom pod jumps to **A** to **B**, and top pod jumps to **B** to **C**.

### 3. Edgeless Board Examples

```text
  This pod may move to square b.
  ↓
┌───┬───┬───┬───┬───┬───┬───┬───┬───┐
│⬡↖ │   │   │ a │   │   │   │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│   │   │   │   │   │   │   │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│   │ d │ ░ │   │ ░ │   │ ░ │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│⬢↖ │   │   │   │   │   │   │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│ c │   │   │   │   │   │   │   │⬢↗→│ ← This pod may move to square c
├───┼───┼───┼───┼───┼───┼───┼───┼───┤   or jump to square d.
│   │   │   │   │   │   │   │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│   │   │ ░ │   │ ░ │   │ ░ │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│   │   │   │   │   │   │   │   │   │
├───┼───┼───┼───┼───┼───┼───┼───┼───┤
│   │   │⬢↘ │   │   │   │   │   │ b │
└───┴───┴───┴───┴───┴───┴───┴───┴───┘
          ↑
          This pod may move to square a.
```

---

## Setup: Four-player game

- Players form two teams, and partners sit diagonally across from one another.
- Each team receives 8 pods of one color (4 for each partner) and 24 prongs (12 for each partner).
- Each player places one empty pod on a corner OCTI square, which is that player's home base. Each player's three extra pods are held in reserve.
- No communication between partners is allowed after the start of play.
- To distinguish between partner's pieces, make arrows visible for one player and hidden for the other player.

## Rules of play (Four-player game)

Above game rules apply except:

- Victory is achieved by occupying the opponents' two home bases at the completion of a full turn.
- If an enemy pod occupies a player's home base at the end of a full turn, all of that player's pieces on the board are swept to their captured pool, **all prongs from those swept pods transfer to the capturing enemy player**, and that player is **permanently eliminated** for the remainder of the game (there is no partner revival).
- Partners cannot share pods or prongs, but allied pieces may stack together.
- A surviving player who personally occupies an enemy or neutral (`53`/`57`) OCTI square may liberate one of their **own** captured pods (`xy*`) onto any OCTI square that they personally occupy (including an eliminated partner's corner base if occupied by the surviving player).

---

## Player Options

**Edgeless Board.** Increase the range of strategic possibilities by allowing pods to exit the edge of the board and return at the opposite end.

**Superprongs.** Players receive 25 prongs, one of which is a light color (gold SuperProng). Each SuperProng you control on the board entitles you to **1 bonus prelude move per turn** (up to 2 in `2P_SP` and up to 4 in `4P_SP`). Before the start of the regular turn, you may move 1 square or execute a straight-line (collinear) jump chain in the direction of each distinct SuperProng socket. You may now start the regular part of your turn, moving (using any prong on any pod) or building.

- Once placed in a pod, SuperProngs (like normal prongs) are permanent and may not be repositioned.
- SuperProngs may be captured from friendly or enemy pods.
- Stacks jumped with capture (`x`) during a SuperProng bonus prelude are **removed immediately** after that prelude before the next SuperProng prelude or regular turn action begins.
- Victory and 4-Player base-capture elimination are evaluated **only at the completion of the full turn**.

The superprong version is best when played on an edgeless board.

---

Copyright 1999 Great American Trading Company  
Developed by Don Green — All rights reserved.  
Patent Pending  
Visit our website: www.OCTI.net

> **⚠ WARNING: CHOKING HAZARD** - small parts.  
> Not for children under 3 years.

---

## Appendix A: OCTI Project Interpretations

Where the classic rules require formalization for edge cases, Don Green's November 2026 review establishes the exact rules enforced across C++ `octi_core`, the Web JS engine, Desktop Qt6, and the Android Java engine below.

### A.1 Starting-Square Vacancy, Loop-Back Jumps & Stacked-Pod Non-Jumping

**Situation.** A pod starts a multi-jump chain on square `S` where one or more friendly pods are stacked.

**Ruling (Don Green, November 2026).**
1. **No Jumping Over Starting Stack-Mates (`jx == sx && jy == sy` Forbidden):** You cannot jump other pods with which you started stacked for the duration of your jump. Therefore, a jumping pod can **never jump over its starting square `S`** during its jump chain—even on an Edgeless (`_ED`) wraparound trajectory (for example, `44bcd-66-86-16-34-54` jumping over stationary `A44a` on `44` is illegal).
2. **Landing Back on `S` Requires Full Stack Clear-Out:** A jumping pod may **land back on its starting square `S`** at the end of a loop **only when all pods that started the turn on `S` vacate `S`** during the move. For example, composite clear-out `44a-45,44beg-66x-64x-44` is legal because `A44a` steps off `44` so `44` is completely empty when `A44beg` lands back on `44`; conversely, single move `44beg-66x-64x-44` while `A44a` remains stationary on `44` is illegal.

**Scope.**
- Every square is judged by physical vacancy: only pods that actually move out of `S` vacate `S` for landing, and `S` can never be jumped over during the chain.
- In OCTI for Kids (`OFK`), stacking is forbidden, so no stack-mates exist on `S`.
