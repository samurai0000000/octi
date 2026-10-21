# OCTI Game Notation Specification

**Standard Notation, Serialization Formats, and Compact State Snapshots for OCTI**  
Authoritative Reference for `core/` (C++17 `octi_core`), `android/` (`com.selfso.octi`), and Game AI / ML Engines  
Version 2.0 (2026)

---

## 1. Overview & Purpose

This document formally specifies the complete move notation, canonical normalization rules, full game transcript format (`.octi`), and compact single-line position snapshot format (`OCTI-FEN`) for the board game OCTI and its variants.

The objectives of this specification are:
1. **Deterministic Move Replay**: Lossless representation of every turn action across 2-Player (9×9), 4-Player Partnership (9×9), and OCTI for Kids (6×7).
2. **Database & Storage Efficiency**: Compact, human-readable, and machine-parseable representation suitable for relational databases, file systems, and network protocols.
3. **Machine Learning & Neural Network Readiness**: Deterministic state serialization facilitating tabular state-action caching, AlphaZero/MCTS training pipelines, and tensor feature extraction.

---

## 2. Coordinate System

The board coordinate system uses two decimal digits `XY`:
- `X`: Column digit (West-to-East horizontal axis).
- `Y`: Row digit (South-to-North vertical axis).

### 2.1 Standard 9×9 Board (2-Player & 4-Player)
- Columns: `'1'` (West edge) through `'9'` (East edge).
- Rows: `'1'` (South edge / Player A rear) through `'9'` (North edge / Player B rear).
- 0-indexed conversion:
  $$\text{locX} = X - \text{'1'} \in [0, 8], \quad \text{locY} = Y - \text{'1'} \in [0, 8]$$
  $$\text{locId} = (X - \text{'0'}) \times 10 + (Y - \text{'0'}) = (\text{locX} + 1) \times 10 + (\text{locY} + 1)$$
- Player A bases: `33, 53, 73` (row 3, columns 3, 5, 7).
- Player B bases: `37, 57, 77` (row 7, columns 3, 5, 7).
- 4-Player Corner bases: A at `33`, B at `37`, C at `77`, D at `73` (Neutral bases at `53, 57`).

### 2.2 6×7 Board (OCTI for Kids)
- Columns: `'1'` through `'6'` ($\text{locX} \in [0, 5]$).
- Rows: `'1'` through `'7'` ($\text{locY} \in [0, 6]$).
- Player A bases: `22, 32, 42, 52` (row 2, columns 2..5).
- Player B bases: `26, 36, 46, 56` (row 6, columns 2..5).

---

## 3. Direction and Prong Encoding

The 8 compass movement and prong directions are encoded with lowercase letters `'a'` through `'h'`, traversing clockwise starting from North:

| Token | Compass Direction | Displacement $(\Delta x, \Delta y)$ | Notes |
|:---:|:---:|:---:|---|
| `a` | North (N) | $(0, +1)$ | Forward toward Player B |
| `b` | Northeast (NE) | $(+1, +1)$ | Forward-right |
| `c` | East (E) | $(+1, 0)$ | Right |
| `d` | Southeast (SE) | $(+1, -1)$ | Backward-right |
| `e` | South (S) | $(0, -1)$ | Backward toward Player A |
| `f` | Southwest (SW) | $(-1, -1)$ | Backward-left |
| `g` | West (W) | $(-1, 0)$ | Left |
| `h` | Northwest (NW) | $(-1, +1)$ | Forward-left |

### 3.1 Normal vs Superprongs
- **Normal Prong**: Represented by the single direction letter (e.g., `a`, `c`, `e`).
- **Superprong**: Represented by the direction letter immediately followed by an exclamation mark `'!'` (e.g., `a!`, `c!`).
- **Pod Configuration (`<config>`)**: A sequence of 0 to 8 direction specifications describing the prongs currently installed in a pod. In canonical form, directions appear in strictly ascending alphabetical order `a` through `h` (e.g., `""` for an empty pod, `"b"`, `"ac!e"`, `"abcdefgh"`).

---

## 4. Formal EBNF Ply Grammar

A single ply (turn) executed by a player is represented by `<ply>`:

```ebnf
<ply>              ::= [ <super-prelude> ";" ] <regular-move>

<super-prelude>    ::= <hop-chain> [ ";" <hop-chain> ]

<regular-move>     ::= <pod-place>
                     | <prong-add>
                     | <prong-reposition>
                     | <composite-hops>

<pod-place>        ::= <sq> [ "*" ]

<prong-add>        ::= <sq> <config> "+" <dir-spec>

<prong-reposition> ::= <sq> <config> "+" <dir> "-" <dir>
                     | <sq> <config> "-" <dir> "+" <dir>

<composite-hops>   ::= <hop-chain> { "," <hop-chain> }

<hop-chain>        ::= <sq> <config> "-" <sq> [ "x" ] { "-" <sq> [ "x" ] }

<dir-spec>         ::= <dir> [ "!" ]

<dir>              ::= "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h"

<config>           ::= { <dir-spec> }

<sq>               ::= <digit> <digit>

<digit>            ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
```

### 4.1 Ply Structure Explanations

1. **Pod Placement & Disambiguation (`<pod-place>`)**:
   - `<sq>` (e.g. `"33"`): Brings a pod onto square `XY`. If the player occupies an enemy or neutral OCTI square with `capturedPods > 0` or has `freePods == 0`, this liberates a captured pod onto `XY`; otherwise, it brings a reserve pod onto `XY` (`freePods--`).
   - `<sq>*` (e.g. `"37*"`): Explicitly designates a **liberation move** onto square `XY`, even if the friendly reserve supply is non-empty (`freePods > 0`). Throws `IllegalMoveException` if the player does not currently occupy an enemy OCTI square or has 0 captured pods.
2. **Prong Insertion (`<prong-add>`)**:
   - `<sq><config>+<dir>[!]` (e.g. `"33+b"`, `"33b+a!"`): Inserts a prong in direction `<dir>` (or superprong `<dir>!`) into the friendly pod matching `<config>` located at `<sq>`.
3. **Prong Repositioning (`<prong-reposition>`)**:
   - `<sq><config>+<addDir>-<rmDir>` or `<sq><config>-<rmDir>+<addDir>` (e.g. `"33a+b-a"` or `"33a-a+b"`): Moves a normal prong from `<rmDir>` to `<addDir>` on the pod matching `<config>` at `<sq>`.
   - Repositioning a superprong, or adding a superprong during a reposition, is strictly forbidden and throws `IllegalMoveException`.
4. **Movement and Jumps (`<hop-chain>`)**:
   - `<sq><config>-<sq>[x]{-<sq>[x]}`:
     - Distance-1 single step: `"33a-34"` (pod at `33` with prong `a` moves North to `34`). Captures are forbidden on distance-1 steps.
     - Single jump with capture: `"33a-35x"` (pod jumps over intermediate pod at `34` landing on empty square `35`, capturing the jumped pod).
     - Multi-jump chain: `"33ac-35x-55"` (jumps over `34` landing on `35` with capture, then jumps over `45` landing on `55` without capture).
5. **Composite Multi-Pod Stack Moves (`<composite-hops>`)**:
   - `<hop-chain> { "," <hop-chain> }` (e.g. `"37df-46,37e-36"`): Multiple pods belonging to the same starting stack move or jump in different directions on the same turn. All hop-chains in a composite move **must share the same starting coordinate** `(x0, y0)`.
6. **Superprong Prelude Moves (`<super-prelude> ";"`)**:
   - A player with superprongs may execute one (or two, if possessing two superprongs) extra move or jump in the superprong direction prior to their regular turn action (e.g. `"33a!-34;34a!+b"`: moves North via superprong to `34`, then adds prong `b`).

---

## 5. Canonical Normalization Rules

When generating, transmitting, or comparing moves, implementations must emit the unique canonical form:

1. **Alphabetical Direction Ordering in `<config>`**:
   Any uppercase characters are converted to lowercase, and directions are sorted clockwise `'a'..'h'`, keeping `'!'` attached to its direction token:
   - Input `"33ba"` $\to$ Canonical `"33ab"`
   - Input `"33eA!"` $\to$ Canonical `"33a!e"`
2. **Reposition Operation Ordering**:
   Regardless of whether the input specifies addition first (`+b-a`) or removal first (`-a+b`), the canonical serializer emits `+<addDir>-<rmDir>`:
   - Input `"33a-a+b"` $\to$ Canonical `"33a+b-a"`
3. **Capture Marker**:
   The capture indicator must always be lowercase `'x'`:
   - Input `"33a-35X"` $\to$ Canonical `"33a-35x"`
4. **Explicit Liberation Marker**:
   When an op has `liberate == true`, the trailing `'*'` is preserved:
   - Input `"37*"` $\to$ Canonical `"37*"`
5. **Separators**:
   - Superprong prelude moves are separated from each other and from the regular move by a semicolon `';'`.
   - Composite stack hops within the regular move are separated by a comma `','`.

---

## 6. Full Game Transcript Format (`.octi`)

Full games are serialized as UTF-8 plain-text files using the `.octi` format.

### 6.1 Header Section
The file begins with a mandatory header comment followed by key-value pairs defining the game parameters:

```text
; OCTI Game Transcript v2.0
GAME_MODE: 2
GAME_BASES: 3
USE_SUPER_PRONGS: No
USE_EDGELESS_BOARD: No
START_PLAYER: A
STATUS: IN_PROGRESS
WINNER: NONE
MOVES: 4
```

#### Core Engine Header Keys
- `GAME_MODE`: `2` (Standard 2-Player), `4` (4-Player Partnership), or `OFK` (OCTI for Kids). Also accepts legacy integer representations `1`, `2`, `3`.
- `GAME_BASES`: `1` (single enemy base victory) or `3` (all three enemy bases required).
- `USE_SUPER_PRONGS`: `Yes` or `No`.
- `USE_EDGELESS_BOARD`: `Yes` or `No` (toroidal wrap-around).
- `START_PLAYER`: `A`, `B`, `C`, or `D`.
- `STATUS`: `NOT_STARTED`, `IN_PROGRESS`, `GAME_OVER`, or `RESIGNED`.
- `WINNER`: `NONE`, `A`, `B`, `C`, or `D`.
- `MOVES`: Decimal count of ply lines following the header.

#### Extensible Metadata Tags
Any additional `KEY: VALUE` headers appearing before `MOVES:` are formally valid and gracefully parsed (or ignored by older parsers without failure), including:
- `EVENT`: Tournament or match name.
- `DATE`: ISO-8601 date (e.g. `2026-10-21`).
- `ROUND`: Round number.
- `PLAYER_A`, `PLAYER_B`, `PLAYER_C`, `PLAYER_D`: Player usernames or ratings.
- `TIME_CONTROL`: Clock specification (e.g. `600+5` for 10 min + 5 sec increment).
- `RESULT_REASON`: Explanation of termination (e.g. `BASE_OCCUPATION`, `TOTAL_CAPTURE`, `TIMEOUT`, `RESIGNATION`).

### 6.2 Move Section
Following `MOVES: <N>`, each ply is listed on its own line. Both minimal and rich annotated formats are supported:

- **Minimal Log Format**: Bare canonical ply:
  ```text
  33+b
  37+e
  33
  37
  ```
- **Rich Annotated Format**: Optional move numbering, player token, and trailing `{ key=value ... }` dictionary for clock times, evaluation scores, and search depths:
  ```text
  1. A 33+b { clk=600000 emt=1200 eval=0.15 depth=12 }
  1. B 37+e { clk=598500 emt=2100 eval=-0.10 depth=11 }
  2. A 33 { clk=595200 emt=800 }
  2. B 37 { clk=596000 emt=1400 }
  ```

---

## 7. Compact Position Snapshot Format (OCTI-FEN)

OCTI-FEN encodes the complete, unambiguous Markov state of an OCTI game in a single ASCII string, ideal for database keys, hashing, and neural network state tensor reconstruction.

### 7.1 Schema

```text
<mode>:<bases>:<super>:<edgeless>:<status>:<turn>:<winner>:<elimMask>:<p1Res>:<p2Res>[:<p3Res>:<p4Res>]:<squares>
```

### 7.2 Field Definitions

1. `<mode>`: `2`, `4`, or `OFK`.
2. `<bases>`: `1` or `3`.
3. `<super>`: `1` (superprongs enabled) or `0` (disabled).
4. `<edgeless>`: `1` (edgeless board enabled) or `0` (standard bounded board).
5. `<status>`:
   - `N`: Not started
   - `P`: Playing / In Progress
   - `O`: Game Over (decided by rules)
   - `R`: Resigned
6. `<turn>`: Active player `A`, `B`, `C`, `D`, or `-` if game is over/not started.
7. `<winner>`: Winner player `A`, `B`, `C`, `D`, or `-` if none.
8. `<elimMask>`: 4-bit integer mask indicating eliminated players in 4-Player mode (`1` = A, `2` = B, `4` = C, `8` = D; `0` in 2P and OFK).
9. `<pNRes>`: Comma-separated resource tuple for each active player:
   `freePods,capPods,freeProngs,freeSuperProngs`
   - In 2-Player mode, two tuples appear: `<p1Res>:<p2Res>`.
   - In 4-Player mode, four tuples appear: `<p1Res>:<p2Res>:<p3Res>:<p4Res>`.
   - In OFK mode, two tuples appear: `<p1Res>:<p2Res>`.
10. `<squares>`: Comma-separated list of non-empty pod tokens ordered lexicographically by coordinate `(x, y)` and bottom-to-top stack index:
    `<Owner><X><Y><config>`
    - `<Owner>`: `A`, `B`, `C`, or `D`.
    - `<X><Y>`: Coordinate digits.
    - `<config>`: Pod prongs in canonical sorted order.
    - If the board has no pods, `<squares>` is empty or `-`.

### 7.3 Canonical Examples

#### Standard 2-Player Starting Position (3 Bases, Standard Prongs)
```text
2:3:0:0:P:A:-:0:4,0,25,0:4,0,25,0:A33,A53,A73,B37,B57,B77
```

#### OCTI for Kids Starting Position
```text
OFK:1:0:0:P:A:-:0:0,0,12,0:0,0,12,0:A22,A32,A42,A52,B26,B36,B46,B56
```

#### 2-Player Midgame with Prongs, Stacked Pods, and Captures
```text
2:3:0:0:P:B:-:0:2,1,18,0:3,0,21,0:A33b,A33c,A53a,A55ad,B37df,B37e,B57f,B77
```

---

## 8. Summary of Parser Requirements & Error Handling

An engine implementing this specification must strictly enforce:
1. **Rejection of Malformed Syntax**:
   - `nullptr` or empty/whitespace strings.
   - Out-of-bounds coordinates (digits outside `'1'..'9'`, or outside `'1'..'6'` / `'1'..'7'` for OFK).
   - Duplicate direction tokens in `<config>` (e.g. `"33aa+b"`).
   - Illegal characters outside the grammar.
   - Any violation must immediately throw `IllegalMoveSyntaxException`.
2. **Rejection of Semantic Invariants**:
   - Repositioning with identical source and destination (`"33a+a-a"`).
   - Removing a prong that does not exist on the pod (`"33a+b-c"`).
   - Removing or repositioning a superprong (`"33a!+b-a"`).
   - Composite moves whose component hops do not share the exact same `(x0, y0)`.
   - Any semantic rule violation must immediately throw `IllegalMoveException`.
