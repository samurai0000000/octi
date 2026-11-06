# OCTI Cross-Platform GUI Look-and-Feel & Interaction Design Book

**Authoritative User Interface, Visual Rendering, and Interaction Specification for OCTI**  
Target Implementations:
- **Web Client**: `server/src/WebUi.cxx` (`buildPlayableGameWebAppHtml`) & `~/work/www/octi/html/index.html`
- **Desktop Client (Linux & Windows Qt6)**: `desktop/src/` (`OctiMainWindow`, `OctiBoardCanvas`, `OctiReservePanel`, `LobbyView`)
- **Android Client (Jetpack Compose)**: `android/app/src/main/java/com/selfso/octi/ui/` (`OctiBoardCanvas.kt`, `OctiReserveHud.kt`, `LobbyScreen.kt`, `MainActivity.kt`, `OctiAppViewModel.kt`)
- **Shared Core Geometry**: `core/include/octi/OctiBoardGeometry.hxx`

Copyright (C) 2026, Charles Chiou  
Version 1.0 (2026)

---

## 1. Purpose & Authority (`GDB-01`)

This document is the **single authoritative source of truth** for the visual appearance, layout geometry, board animations, mouse/touch gestures, move staging workflow, and online/local control parity of the OCTI application across **Web**, **Desktop (Qt6)**, and **Android (Jetpack Compose)**.

1. **Zero Platform Divergence**: From the user's point of view, Web, Desktop, and Android must behave identically. No platform may invent bespoke buttons (such as an `Undo` button in online games), omit required visual indicators (such as the animated pink `#FF69B4` active-turn edge light or auto-rotation when sitting at Seat B), or diverge in how clicks/taps on home squares, pods, and zoomed squares operate.
2. **Compliance Enforcement**: Every GUI feature request, bug fix, and regression test across `server/`, `desktop/`, and `android/` must cite and satisfy the invariant IDs (`GDB-01` through `GDB-12`) defined in this Design Book.

---

## 2. Color Palette, Faction Identity & Luxury Obsidian Theme (`GDB-02`)

All three platforms share a unified dark-luxury obsidian, walnut, and brushed-brass visual palette by default, expanded into a full ten-theme catalogue with dedicated style layers:

| Semantic Token | Hex / RGBA | Usage Across Web, Desktop, and Android |
|---|---|---|
| **App Background** | `#0B0F17` | Root window / viewport background |
| **Card / Panel Surface** | `#141B29` | Sidebar cards, reserve HUD cards, lobby cards |
| **Card Border (Default)** | `#B49148` | Standard structural border (`1px solid`) |
| **Metallic Brass Accent** | `#C8A253` / `#F2ECE0` | Active headers, primary outlines, coordinate labels, own chat text |
| **Board Outer Frame** | `#101622` | Beveled dark frame surrounding the playable grid |
| **Normal Board Square** | `#151E30` (alt `#1D2940`) | Standard non-home grid cell fill with `#B8944C73` grid hairlines |
| **Home Square (`OCTI` Base)** | `#0A0E16` (`rgb(10,14,22)`) | Deep recessed dark slate fill with `#070A10` inner well |
| **Player A Faction (Emerald)** | `#10B981` (Border `#059669`) | Seat 1 / South in 2P & OFK; SW Corner (`33`) in 4P |
| **Player B Faction (Ruby)** | `#F43F5E` (Border `#BE123C`) | Seat 2 / North in 2P & OFK; NW Corner (`37`) in 4P |
| **Player C Faction (Sapphire)** | `#3B82F6` (Border `#1D4ED8`) | Seat 3 / NE Corner (`77`) in 4P |
| **Player D Faction (Gold)** | `#F59E0B` (Border `#B45309`) | Seat 4 / SE Corner (`73`) in 4P |
| **Standard Prong Blade** | `#D6DCE8` $\rightarrow$ `#F8FAFC` | Brushed metallic silver prong protruding from octagon facet |
| **Superprong Blade** | `#FDE047` $\rightarrow$ `#FFFBEB` | Gold and incandescent core fluted prong blade |
| **Active Turn Running Light** | `#FF69B4` (Hot Pink) | Animated 30%-edge sweeping line segment on active player's board edge |
| **Last-Move Square Orbit** | `#FF8C00` (Dark Orange) | Animated 30%-perimeter clockwise orbit around squares modified by last ply |
| **Guided Destination Ring** | `#34D399` (Step) / `#FBBF24` (Jump) | Legal step/jump target square highlight & 1 Hz Commit button pulse |
| **Danger / Capture / Resign** | `#F43F5E` (Capture Glow) | Capture glow indicator, Resign button, Delete/Purge confirmation |

### `GDB-02.1` Ten-Theme Cross-Platform Palette Catalogue
OCTI defines exactly 10 first-class visual themes maintained in full cross-platform parity across Desktop (Qt6/C++), Android (Java/Jetpack Compose/Views), and Web (HTML5 Canvas/CSS):

1. **`OBSIDIAN_BRASS` (`obsidian`) — "Obsidian & Brass"**: Legacy flagship luxury dark obsidian stone, deep charcoal panels, brushed warm brass borders, and classic emerald/ruby/sapphire/gold factions.
2. **`ROSEWOOD_IVORY` (`rosewood`) — "Rosewood & Ivory"**: Warm dark walnut and rosewood grains, polished ivory faction pods with carved bezels, amber citrine accents, and warm parchment highlights.
3. **`NEON_CIRCUIT` (`neon`) — "Neon Circuit"**: Cyberpunk command deck with dark phosphor slate surfaces, printed circuit board (PCB) traces, luminous neon tube prongs, and vibrant cyan/magenta/lime/amber factions with intense edge glow (`glowStrength = 0.85`).
4. **`JADE_DYNASTY` (`jade`) — "Jade Dynasty"**: Imperial lacquerware with cloud-scroll lacquer board textures, carved nephrite and jadeite pods, gold filigree prongs, and cinnabar/lapis/saffron faction tones.
5. **`ARCTIC_AURORA` (`aurora`) — "Arctic Aurora"**: Glacial crystalline ice sheets with frosted board textures, rounded cell corners, faceted glass pods, icicle-styled prongs, and polar teal/crimson/cobalt/solar factions (`glowStrength = 0.60`).
6. **`VOLCANIC_FORGE` (`forge`) — "Volcanic Forge"**: Smoldering basalt rock, cooled magma fissures, hammered cast-iron pods with ember patterns, heavy forged industrial prongs, and verdigris/molten/tempered-steel factions (`glowStrength = 0.70`).
7. **`CANDY_POP` (`candy`) — "Candy Pop"**: Pastel confectionary arcade with rounded candy cells, sugar-sprinkle board texture, glossy glazed pods with faction-specific patterns (`STRIPES`, `DOTS`, `CHECKS`, `SWIRL`), and lollipop prongs.
8. **`OCEAN_PEARL` (`pearl`) — "Ocean Pearl"**: Deep abyssal trench with underwater caustics, iridescent nacre shell pods, branching organic coral prongs, and seafoam/coral/sapphire/amber-pearl factions.
9. **`STEAMPUNK_BRASS` (`steam`) — "Steampunk Brass"**: Victorian industrial workshop with riveted iron boiler plates, interlocking clockwork brass gear pods, copper pipe prongs, and patina/oxblood/aether factions.
10. **`COSMIC_NEBULA` (`nebula`) — "Cosmic Nebula"**: Deep space observatory with distant starfield textures, planetary banded gas-giant pods, high-energy ionized comet prongs, and luminous exoplanet/pulsar/helios factions (`glowStrength = 0.80`).

### `GDB-02.2` Style Layer Architecture
In addition to 46 semantic color tokens per theme, each palette includes a 6-attribute physical rendering style layer:
- **`PodFinish`**: Defines surface lighting model (`CLASSIC`, `GLASS`, `CARVED`, `HAMMERED`, `GLOSS`, `NACRE`, `BRASS`, `PLANETARY`).
- **`PodPattern`**: Micro-texture rendered across pod domes (`NONE`, `CIRCUIT`, `CLOUD_SCROLL`, `FACET`, `EMBER`, `STRIPES`, `DOTS`, `CHECKS`, `SWIRL`, `SHELL`, `GEARS`, `BANDS`, `FACTION_SPECIFIC`).
- **`ProngStyle`**: Geometric shape profile of protruding prong blades (`CLASSIC`, `NEON_TUBE`, `FILIGREE`, `ICICLE`, `FORGED`, `LOLLIPOP`, `CORAL`, `PIPE`, `COMET`).
- **`BoardTexture`**: Procedural or cached canvas background texture (`NONE`, `PCB`, `LACQUER_SCROLL`, `FROST`, `BASALT_LAVA`, `SPRINKLES`, `CAUSTICS`, `RIVET_PLATE`, `STARFIELD`).
- **`CellShape`**: Grid geometry (`SQUARE` sharp corners vs. `ROUNDED` radiused tiles).
- **`glowStrength`**: Scalar float $\in [0.0, 1.0]$ governing outer luminescence, neon glow falloff, and portal aura intensity.

---

## 3. Main Viewport Layout & Responsive Geometry (`GDB-03`)

### `GDB-03.1` Strict `1:1` Square Board Invariant
- The playable board canvas must **always** render with a strict **`1:1` square aspect ratio** (`width == height`).
- Stretching the board into a non-square rectangle or rendering octagonal pods as ellipses is strictly forbidden on any resolution or window size.

### `GDB-03.2` Desktop & Desktop Web (`> 980px`) 3-Column Symmetric Layout
When viewing a match on Desktop (`OctiMainWindow`) or Desktop Web (`> 980px`), the workspace uses a 3-column layout centered around the square board:
1. **Left Column (`240px–260px` width) — Reserve & Table Info Column**:
   - The top edge of the top card and the bottom edge of the bottom card **must align flush with the outer top and bottom edges of the square board frame**.
   - **Top Card**: Opponent / Top-position Reserve HUD (`Player B` in 2P/OFK when unrotated; swaps dynamically with board rotation so the card matches the player whose home edge is at the top of the screen).
   - **Middle Card**: **Table / Match Info Card** displaying Game Mode (`2-Player`, `4-Player`, `OCTI for Kids`), Room ID & Status (online), Time Control, Active Turn, and Seating buttons when waiting for players.
   - **Strict Prohibition on Static `"Quick Guide"` Box**: The legacy `"Quick Guide"` static instruction card must **not** appear in the left column on any platform.
   - **Bottom Card**: Local / Bottom-position Reserve HUD (`Player A` when at `0°`; `Player B` when rotated to `180°`).
2. **Center Column — Square OCTI Board**:
   - Surrounded by a dark beveled frame with coordinate gutter labels (`1..9` for 2P/4P; `1..6` columns and `1..7` rows for OFK) on all four sides.
3. **Right Column (`320px–360px` width) — Strict 50/50 Vertical Split**:
   - **Top 50% (Move History, VCR & Match Controls)**:
     - **Move History List**: Displays numbered turns (`1.`, `2.`, ...) with moves color-coded in each player's faction color (`A` Red, `B` Green, `C` Blue, `D` Amber). Auto-scrolls to the bottom on every new ply. Strictly **no raw `OCTI-FEN` string display** in the sidebar.
     - **VCR Transport Bar**: `|<<` (Start), `<` (Prev), `Ply X / Y` indicator, `>` (Next), `>>|` (Live/End), plus a compact **icon-only brass Export button** on the right end of the VCR bar that exports the match transcript in canonical JSON v3.0 format.
     - **Match Action Button Strip**: Contextual action buttons governed by `GDB-10.1`.
   - **Bottom 50% (Table Chat / Match Event Log)**:
     - Auto-scrolling chat & room event log with message input row at the bottom (governed by `GDB-11.2`).

### `GDB-03.3` Android & Mobile Portrait Layout
- **Edge-to-Edge Full-Width Square Board**: On Android portrait (and narrow mobile viewports), the 9×9 (or 6×7) grid renders **edge-to-edge across 100% of the available width** (`outerFramePad = 0`, `coordGutter = 0`) to maximize touch target size.
- **In-Cell Corner Coordinate Digits**: Because outer gutters are omitted on mobile portrait, subtle brass coordinate digits (`1..9`) are drawn inside the outer corners of column `1` and row `1` squares.
- **Top & Bottom Reserve HUD Strips**: Compact horizontal Reserve HUD cards sit immediately above and below the square board, dynamically swapped with `boardRotationDeg` so the bottom HUD always belongs to the player whose home edge is at the bottom of the screen.
- **Bottom Navigation Bar Removal & Top `Menu ▾` Relocation**: The legacy bottom navigation bar is completely removed from mobile portrait layouts. Global navigation items (Theme Selector, Account Settings, Sound, Voice Settings, About/Updates) are consolidated into a top-right `Menu ▾` overflow dropdown.
- **Reclaimed Space for Expanded Table Chat**: The space previously occupied by the bottom bar is reclaimed to expand the docked `Table Chat` and event log, ensuring messages, room status transitions, and keyboard-friendly chat inputs have ample vertical clearance without obscuring the board.
- **Three Swipeable Virtual Screens (Horizontal Pager)**: On mobile portrait, the main interface is organized as a smooth 3-page horizontal swipeable layout ordered strictly left-to-right:
  1. **Left Screen (`0`) — `Online Lobby`**: Room browser, table cards, seating slots, and split rosters for `Connected Users` and `Connected Robots`.
  2. **Center Screen (`1`) — `Board` (Default View)**: 1:1 square playable board, top/bottom Reserve HUDs, turn clock, AI progress bar, and expanded table chat.
  3. **Right Screen (`2`) — `Lobby Chat`**: Full-screen global community chat feed and player presence list.
- **Quick-Switch Navigation Buttons**: Quick-switch shortcuts (`To Lobby` and `To Chat`) are docked in the app header and sidebar, allowing instantaneous jumping directly between `Online Lobby` and `Lobby Chat` without requiring repetitive horizontal swipes across the center `Board` screen.

---

## 4. Home Squares (`OCTI` Bases), Pod Rendering & Stack Sorting (`GDB-04`)

### `GDB-04.1` Dark Recessed Home Squares & Player-POV `"OCTI"` Label
1. **Recessed Dark Fill**: All home squares (`33, 53, 73` for A and `37, 57, 77` for B in 2P; `22, 32, 42, 52` for A and `26, 36, 46, 56` for B in OFK; `33, 37, 77, 73` corner bases + `53, 57` neutral bases in 4P) are rendered with a deep recessed dark slate fill (`#0A0E16`) and a subtle metallic inner bevel (`#3C485C`).
2. **Zero Color Tinting or Corner Pips**: Home squares must **never** be tinted red or green and must **never** draw corner dots/pips when empty.
3. **Player-POV `"OCTI"` Watermark**: Each player home square displays `"OCTI"` in muted brass (`rgba(212, 175, 55, 0.35)`), rotated so it reads right-side-up from its **owner's point of view**:
   $$\text{labelRotDeg} = ((\text{ownerSeatRotDeg} - \text{boardRotationDeg}) \bmod 360 + 360) \bmod 360$$
   - In 2P/OFK at `boardRotationDeg = 0°`: Player A's `"OCTI"` labels are at `0°` (reading up from South), while Player B's `"OCTI"` labels are at `180°` (upside-down from South, reading up from North). When rotated to `180°`, Player B's `"OCTI"` labels read right-side-up (`0°`) and Player A's are at `180°`.
   - In 4P: Owner seat rotations are `A = 0°`, `B = 90°`, `C = 180°`, `D = 270°`.

### `GDB-04.2` Inverse Pod Stack Sorting (Board View vs. Zoom View)
When multiple pods occupy the same square, the visual stacking order is intentionally inverted between the main board and `ZoomedSquareDialog`:
1. **Main Board View — Most Prongs at Bottom (`GDB-04.2a`)**:
   - On the main board, pods on a square **must be sorted descending by prong count from bottom (layer 0) to top (layer $N-1$)**, with a subtle per-layer upward/diagonal pixel offset (`~2.5px`).
   - *Rationale*: Placing the pod with the most prongs at the bottom of the stack allows its prongs to protrude visibly around the perimeter of the upper pods, whereas placing a 0-prong pod at the bottom would hide it completely beneath an 8-prong pod.
2. **Zoomed Square View (`ZoomedSquareDialog`) — Most Prongs on Top (`GDB-04.2b`)**:
   - Inside `ZoomedSquareDialog`, pods on a square **must be sorted descending by prong count from top (index 0, active by default) to bottom**, with lower pods rendered as a translucent 3D offset underlay (`alpha = 0.28`, `12px` diagonal step per layer) and selectable via stack tabs or scroll/slide gestures.

### `GDB-04.3` Center Faction Letter vs. `"xN"` Stack Multiplier Badge
- **Single Pod (`count == 1`)**: Displays the pod's faction letter (`A`, `B`, `C`, or `D`) centered inside the octagonal body.
- **Stacked Pods (`count >= 2`)**: Replaces the center faction letter on the top-most pod with the stack count **`"x2"`**, **`"x3"`**, **`"x4"`**, etc.
- **No Corner Badge Overlays**: Never draw a separate rectangular or circular `"xN"` badge in the corner of the square where it would occlude prong slots.

---

## 5. Animated Board Indicators (`GDB-05`)

### `GDB-05.1` Pink (`#FF69B4`) Running Turn Light on Active Player's Board Edge
1. **Visual Specification**:
   - A continuous glowing hot-pink (`#FF69B4`, outer glow `rgba(255, 105, 180, 0.40)`) line segment covering **30% of the board edge length (`segFrac = 0.30`)**.
   - Sweeps smoothly back-and-forth (triangle wave, **`2000 ms` full period**, 60 FPS) along the screen-space board edge corresponding to the active turn player (`OctiBoardGeometry::activeTurnEdgeSide(mode, activeTurn, boardRotationDeg)`).
2. **Mandatory Visibility Across Local & Online Play**:
   - The pink running light **must be visible and animating whenever a game is in progress (`status == IN_PROGRESS`, `!isGameOver`, viewing live ply)** in **both Local and Online games**—regardless of whether it is the local player's turn, the opponent's/AI's turn, or the user is an observer.
   - It must **never** be hidden simply because the local player is waiting for an online opponent's move.
3. **Board-Edge Exclusivity**:
   - The pink running light is drawn **only** along the edge of the board canvas—never around the Reserve HUD cards.
4. **Pre-Game Suppression & Monotonic Turn Streak Rendering**:
   - **Pre-Game Suppression**: Before a match begins—locally when `!matchStarted` (awaiting "Start Match" card confirmation) and online when room phase is not active (`phase !== "IN_PROGRESS"` / `WAITING_FOR_PLAYERS`)—the animated pink running line must be **strictly suppressed** across Web, Desktop, and Android. No ghost turn streak or premature turn indicator may be drawn while players are waiting or configuring game options.
   - **Android Monotonic Turn Streak Rendering**: To eliminate bitmap cache ghosting, tearing, or double-rendering in Android, `showTurnIndicator = false` is strictly enforced when rendering the cached off-screen board bitmap. The dynamic pink turn streak is composited live during view `onDraw()` driven by monotonic system uptime (`SystemClock.uptimeMillis()`).

### `GDB-05.2` Orange (`#FF8C00`) Last-Move Square Perimeter Indicator
1. **Visual Specification**:
   - Whenever a move is committed (by local human, local AI, or remote online player), every square modified by that move (origin square, jumped/captured squares, landing square, or pod/prong placement square) displays a glowing dark-orange (`#FF8C00`) line segment covering **30% of the square's perimeter (`segLen = 1.2` of `4.0` sides)** orbiting clockwise with a **`2000 ms` period** at 60 FPS.
2. **5-Second Auto-Timeout & Immediate Interaction Clear**:
   - The orange last-move indicator automatically expires and disappears **`5000 ms` (`5.0 seconds`)** after the move is recorded.
   - Any user interaction on the board or controls (clicking/tapping a square, opening `ZoomedSquareDialog`, dragging, or clicking a control button) **immediately clears** the orange indicator (`0 ms`).

### `GDB-05.3` 1 Hz Flashing Commit Button
- Whenever a move/ply is staged and awaiting confirmation (`awaitingPlyConfirmation == true`), the **`Commit`** button flashes smoothly at **1 Hz (`1000 ms` period)**:
  $$\text{pulse} = \frac{1 + \sin(2\pi \cdot t / 1000)}{2} \in [0, 1]$$
  interpolating its border and glow between emerald (`#10B981`) and bright gold-green (`#34D399` / `#D4AF37`).

---

## 6. Board Orientation, Auto-Rotation by Seat, Zoom (`1.0x..3.0x`) & Free Rotation (`GDB-06`)

### `GDB-06.1` Automatic Board Orientation by Player Seat
- Every player views the board looking **up** from their own home edge at the bottom of the screen.
- Canonical seat rotation mapping (`OctiBoardGeometry::rotationForSeat`):
  - **2-Player & OCTI for Kids**:
    - Seat 1 (`Player A`, South): **`0°`**
    - Seat 2 (`Player B`, North): **`180°`**
  - **4-Player Partnership**:
    - Seat 1 (`Player A`, South/SW): **`0°`**
    - Seat 2 (`Player B`, West/NW): **`90°`**
    - Seat 3 (`Player C`, North/NE): **`180°`**
    - Seat 4 (`Player D`, East/SE): **`270°`**
- **Mandatory Trigger Points**:
  1. When starting a Local Game where only `Player B` is Human (and `Player A` is AI), auto-rotate immediately to `180°`.
  2. When sitting down at an Online Table in Seat $k$ (`sitAtOnlineTable` / `seat_updated` / initial room sync as seated player), **immediately auto-rotate `boardRotationDeg`** to `rotationForSeat(mode, seat)`—both while `WAITING_FOR_PLAYERS` and when `IN_PROGRESS`.

### `GDB-06.2` Always-Active Viewport Navigation (On-Turn, Off-Turn, AI Thinking & Observing)
- Board zoom (`1.0x..3.0x`), pan, continuous rotation drag/twist, manual `Rotate 90°`, and read-only `ZoomedSquareDialog` inspection **must remain active at all times**, including when waiting for an online opponent, during AI thinking, or while observing a table.

### `GDB-06.3` Board Zoom (`1.0x..3.0x`) & Pan
- **Desktop & Web**:
  - Scrolling the mouse wheel over the board zooms smoothly between **`1.0x` and `3.0x`**, keeping the board point under the cursor stationary.
  - While zoomed (`scale > 1.001`), left-dragging on the board pans the viewport within clamped board bounds.
  - Pressing `Esc`, or right-clicking / double-clicking on an empty non-home square, resets zoom to `1.0x`.
- **Android**:
  - Two-finger pinch-to-zoom scales the board between **`1.0x` and `3.0x`** around the gesture focal point and smoothly snaps back to `1.0x` when the fingers lift.

### `GDB-06.4` Continuous Board Rotation & 90° Snap
- **Desktop & Web**:
  - When unzoomed (`scale == 1.0`), pressing and dragging on an empty board square or the outer board frame rotates the entire board continuously in real time (`liveRotationOffsetDeg`) following the cursor angle around the board center.
  - Releasing the mouse button snaps `boardRotationDeg` to the nearest `90°` cardinal angle (`0°`, `90°`, `180°`, `270°`).
- **Android**:
  - Two-finger twist gesture rotates the board continuously in real time (`gestureRotationDeg`) and snaps to the nearest `90°` cardinal angle on release.

---

## 7. Square Selection, Pod Placement, Guided Moves & `ZoomedSquareDialog` (`GDB-07`)

### `GDB-07.1` Zoom-Only Prong Addition & Repositioning
- **Strict Rule**: Prongs and superprongs are **never** added or repositioned directly on the small squares of the main board.
- All prong additions (`+a..h`, `!`) and prong repositionings (`+dst-src`, disallowed in OFK) are performed **exclusively inside `ZoomedSquareDialog`**.

### `GDB-07.2` Single-Click / Single-Tap Behavior on the Main Board
During the active player's interactive turn (when viewing the live ply):
1. **Single-Click / Single-Tap on an Empty Friendly Home Square**:
   - If no guided move is currently in progress and the active player has `freePods > 0` (in 2P/4P, where pod placement is legal), **single-clicking or single-tapping an empty friendly home square immediately stages placing a reserve pod (`<sq>`)** on that square and displays the non-blocking `Cancel` / `Commit` confirmation bar.
   - *(Players may also click the Reserve `Pods (+)` well or drag a pod from the Reserve `Pods (+)` well onto an empty friendly home square).*
2. **Single-Click / Single-Tap on an Occupied Friendly Square with `>= 1` Legal Move/Jump**:
   - Enters **Guided Move Mode** on the main board for that square, illuminating all legal 1-step moves and jumps with emerald green target rings (`#10B981`).
   - Clicking/tapping a green target ring stages the hop/move to that destination.
   - *(Dragging a friendly pod directly to a legal destination square also stages the hop/move).*
3. **Single-Click / Single-Tap on an Occupied Friendly Square with `0` Legal Moves (e.g., a 0-Prong Pod)**:
   - Because the pod has no legal moves on the board, **single-clicking or single-tapping it immediately opens `ZoomedSquareDialog`** on that square so the player can add a prong in one click/tap.
4. **Single-Click / Single-Tap on the Same Origin Square a 2nd Time (at 0 Hops)**:
   - If a friendly pod is selected in Guided Move Mode at 0 hops (no move taken yet), clicking/tapping that **same square a second time** cancels the guided selection and **immediately opens `ZoomedSquareDialog`** on that square.
5. **Single-Click / Single-Tap on a Different Friendly Square (at 0 Hops)**:
   - If a friendly pod is selected in Guided Move Mode at 0 hops, clicking/tapping a **different friendly square** immediately switches selection to that new square in a single click/tap.
6. **Captured Pod Liberation (`<sq>*`) Priority & Eligibility**:
   - By official OCTI rules, liberating a captured pod (`<sq>*`) on a friendly home square is **strictly legal only when that friendly home square is currently occupied by at least one friendly pod** (`sq.isOccupiedBy(activeTurn)` and `capturedPods > 0`).
   - An **empty** friendly home square can **only** receive a reserve pod (`freePods > 0`), never a liberated pod.
   - When an **occupied** friendly home square is opened in `ZoomedSquareDialog` and the player has `capturedPods > 0`, the **`Liberate Captured Pod (<sq>*)`** action is offered first.

### `GDB-07.3` Explicit Gestures to Open `ZoomedSquareDialog`
- **Desktop & Web**: **Right-click** (suppressing the browser context menu) or **Double-click** on any occupied square or eligible friendly home square (works at any board zoom level `1.0x..3.0x`).
- **Android & Touch Web**: **Double-tap** or **Long-press (`~420 ms`)** on any occupied square or eligible friendly home square.

### `GDB-07.4` Frameless `ZoomedSquareDialog` Appearance & Interaction
1. **Pure Borderless Card**:
   - Rendered as a frameless, borderless dark-obsidian card (`#111A2E`, `18px` rounded corners, `#D4AF37` border glow) centered over the board.
   - Strictly **no OS window title bar**, **no `[X]` corner button**, and **no `"Close Inspector"` button**.
   - **Dismissal**: Clicking/tapping anywhere outside the card boundary or pressing `Esc` / Android `Back` closes the modal.
2. **Interactive Prong Slots & Live Ghost Preview**:
   - The active pod is rendered as a large octagon with 8 directional prong sockets (`N, NE, E, SE, S, SW, W, NW`) rotated to match the current `boardRotationDeg`.
   - Hovering or touching an empty prong socket previews a semi-transparent metallic prong blade (`alpha = 0.72`).
   - Clicking/tapping an empty socket stages adding a prong (`+a..h`), **automatically closes `ZoomedSquareDialog`**, and activates the non-blocking `Cancel` / `Commit` bar so the user sees the updated pod on the board.
   - In 2P/4P (never in OFK), dragging an existing prong from one socket to an empty socket on the same pod stages a prong reposition (`+dst-src`) and closes `ZoomedSquareDialog`.
3. **3D Translucent Stack Underlay & Pod Cycling**:
   - When multiple pods occupy the square, the pod with the **most prongs is selected on top by default (`GDB-04.2b`)**, and the remaining pods in the stack are drawn underneath with a `12px` diagonal 3D offset at `alpha = 0.28`.
   - **Desktop & Web**: Scrolling the mouse wheel up/down inside `ZoomedSquareDialog` (or clicking the pod stack tabs) cycles which pod in the stack is inspected/edited.
   - **Android & Touch**: Vertical single-finger tap-and-slide up/down inside `ZoomedSquareDialog` (or tapping the pod stack tabs) cycles the active pod in the stack.

---

## 8. Move Staging, Non-Blocking Confirmation, Composite Plies & Capture Prompts (`GDB-08`)

### `GDB-08.1` Mandatory Two-Button Non-Blocking Confirmation (`Cancel` & `Commit`)
1. **Universal Preview State**:
   - Every complete or partial ply action (placing a pod, liberating a pod, adding/repositioning a prong, moving, or jumping) enters a staged preview state (`awaitingPlyConfirmation = true`) on the board before being sent to the engine or server.
2. **Strictly Two Buttons (`Cancel [Esc]` and `Commit [Enter]`)**:
   - The confirmation bar/banner displays **only**:
     - **`Cancel [Esc]`**: Discards the staged ply and restores the board to the start of the turn.
     - **`Commit [Enter]`**: Pulses at **1 Hz (`GDB-05.3`)** and commits the staged ply.
   - **Strict Prohibition on `"More Moves"` Button**: There must **never** be a third `"More Moves"` button. Whenever continuation hops, an 8th-prong bonus move (OFK), or composite multi-pod moves from the same origin square remain legal, their green target rings remain directly clickable on the board while the `Cancel` / `Commit` bar is visible.
3. **Protection Against Accidental Cancellation**:
   - While `awaitingPlyConfirmation` is active, clicking/tapping an empty or unrelated square that is not a legal continuation hop (and not another friendly pod on the same multi-pod origin square) **must not** cancel or reset the staged ply. Instead, it pulses/flashes the `Cancel` / `Commit` bar to remind the player to confirm or cancel.

### `GDB-08.2` Capture Decision Prompt & Drag-Off-Board Toggle
1. **Per-Hop Capture Prompt (2P & 4P Only)**:
   - When a pod jumps over an occupied square in 2P or 4P, if capturing is optional, a compact non-blocking prompt (**`Capture`** vs. **`Leave on Board`**) appears **only for the newly jumped square on that specific hop** (never re-prompting squares already decided on earlier hops of the same chain).
2. **Drag-Off-Board Capture Toggle**:
   - During a staged jump chain, the player can also toggle whether a jumped square is captured (`x`) by dragging the jumped pod off the outer edge of the board (or clicking the jumped square's capture toggle).

---

## 9. Overlays — Local `"Start Match"`, `"AI Thinking"` Progress Bar & Endgame Summary (`GDB-09`)

### `GDB-09.1` Non-Modal Board-Scoped `"Start Match"` Overlay (Local Mode)
- When entering Local Mode or changing local game settings before ply 1, a compact **"Start Match"** card is centered over the board canvas only (leaving top navigation, mode tabs, and sidebars 100% interactive).
- Contains two buttons—**`Change Type`** (opens Local Match Setup dialog) and **`Start Match`**—plus a concise 2-line summary of the selected mode, seats, and AI difficulty/time limit.

### `GDB-09.2` 100% Opaque Bottom-Docked `"AI Thinking"` Progress Bar
- Whenever a local AI engine is searching for a move, a **100% opaque (`alpha = 1.0`) download-style progress bar** is docked below/outside the 9×9 (or 6×7) grid so **all board squares remain 100% visible and unobstructed**.
- The progress fill animates smoothly from `0%` to `100%` of the configured AI time budget (or depth target), with high-contrast text overlaid directly inside the bar:
  `AI Thinking (Seat B) • Depth D • N nodes • X.Xs / Y.Ys (P%)`

### `GDB-09.3` Endgame Summary Banner & Modal
- When a game concludes (`status == COMPLETED` or `isGameOver()`), displays the winner (`Player A Wins!`, `Team A+C Wins!`, `Draw`), the exact reason (`Base Captured`, `Opponent Eliminated`, `Resignation`, `Mutual Draw`, `Timeout`), and buttons for **`Review Board`** (dismisses modal to inspect final position & VCR), **`Export Moves`**, and **`New Game`** / **`Return to Lobby`**.

---

## 10. Local vs. Online Match Controls, Reserve HUD & Seating Lifecycle (`GDB-10`)

### `GDB-10.1` Match Action Button Matrix by Mode & State

| Match Context | Permitted Action Buttons | Strictly Forbidden Buttons |
|---|---|---|
| **Local Match (`IN_PROGRESS`)** | `New Game`, `Undo`, `Draw`, `Resign`, `Rotate 90°` | `Leave Table`, `Delete Table`, `Sit Seat` |
| **Online Match (`WAITING_FOR_PLAYERS`)** | `Sit Seat 1 (A)` .. `Sit Seat N`, `Leave Table`, `Delete/Purge Table` (Host/Admin with confirmation modal), `Rotate 90°` | **`Undo`**, **`Draw`**, **`Resign`**, `New Game` |
| **Online Match (`IN_PROGRESS`, Seated Player)** | `Draw` (Offer / Accept), `Resign`, `Leave Table`, `Rotate 90°` | **`Undo`** (and `Ctrl+Z`), `New Game` |
| **Online Match (`IN_PROGRESS`, Observer)** | `Leave Table`, `Rotate 90°` | **`Undo`**, `Draw`, `Resign`, `New Game` |

> **Critical Rule (`GDB-10.1a` — No Undo in Online Games)**: There must **never** be an `Undo` button or keyboard shortcut in any online game on Web, Desktop, or Android.

### `GDB-10.2` Reserve HUD Card Contents & Resource Wells
Each player's Reserve HUD card displays:
1. **Header Row**: Faction color indicator, Seat name (`Player A` / `Player B`), Player Name / AI Badge / `[Open Seat]`, Online Rating (`ELO`), and live Turn Clock (`MM:SS`).
2. **Open Seat Action (Online `WAITING_FOR_PLAYERS`)**: If a seat is unoccupied in an online waiting room, its Reserve HUD card displays an inline **`Sit Seat k`** button (disabled with a clear tooltip if the viewer is already seated in another seat).
3. **Graphical Resource Wells**:
   - **Pods (`0..4` or `0..2`)**: Vector octagon pod icon + remaining count. On the active player's card during an interactive turn, displays a `+` badge; clicking or dragging from it onto a friendly empty home square stages pod placement.
   - **Prongs (`0..12` or `0..10`)**: Vector gold prong icon + remaining count.
   - **Superprongs (`0..2`)**: Vector cyan-white diamond prong icon + remaining count. **Completely hidden** whenever `!useSuperProngs` (Standard 2P without superprongs, and OFK).
   - **Captured (`0..N`)**: Displays **opponent pods captured by this player**, rendered in the **opponent's pod color** with a crimson capture ring + count. **Completely hidden** whenever `captured == 0` or in OFK.

### `GDB-10.3` Web UI Compact Match Actions Card & Leave Table Location
1. **Compact Actions Card Placement**:
   - In the Web client (`WebUi.cxx`), the match actions card is positioned inside `#web-side-top-half` directly below `Lobby Chat`.
   - Displays contextual game controls (`Undo`, `Draw`, `Resign`, `Rotate 90°`) without redundant or duplicate `Sit/Sit` buttons (seating actions remain exclusively on Reserve HUD cards and Lobby table cards).
2. **`Leave Table` Docked Inside Table Chat Card**:
   - The primary **`Leave Table`** button (`#btn-web-leave-table`) is located directly inside the `Table Chat` card header (`#web-board-chat-panel`).
   - Ensures room departure is always immediately accessible in online play without cluttering the match action card or requiring scrolling.

### `GDB-10.4` Archive Upload Policy (Ignoring Level 1 & Level 2 AI Matches)
1. **Low-Difficulty Bot Upload Filtering**:
   - Matches played against Level 1 (`CASUAL`) and Level 2 (`TACTICAL`) AI bots are strictly ignored for online archive database storage across Web, Android, Desktop, and `octi-server`.
   - Prevents database bloating while reserving global match archives for human-vs-human online play and Level 3 (`MASTER`) AI games.
2. **Server API Response Contract**:
   - When a client sends `POST /api/v1/archive/upload` for a Level 1 or Level 2 match, `octi-server` returns HTTP `200 OK` with JSON:
     `{"ok": true, "ignored": true, "reason": "ai_level_below_threshold"}`
   - Clients treat this response as an immediate success without alerting the user or retrying, while existing database records and indexes remain intact.

---

## 11. Online Lobby, Table Chat & Borderless Chat Zoom Modal (`GDB-11`)

### `GDB-11.1` Online Lobby Table Cards & Segregated Participant Rosters
- **Active Table Cards**: Active online rooms are displayed as visual table cards showing:
  - Room ID (`g-YYYYMMDD-NNNN`), Game Mode badge (`2P`, `4P`, `OFK`), Time Control (`10+5`, etc.), and Room Status (`Waiting for Players` or `In Progress • Move N`).
  - Seat slots (`Seat 1 (A)` .. `Seat N`) listing the seated player's name and rating or `[Open]`.
  - Action buttons: **`Join Table`** (opens the room view and sits if an open seat was clicked), **`Observe`**, and **`Delete/Purge Table`** (for table creator when no opponent is seated, or admin `@samurai`, always guarded by a confirmation dialog).
- **Clean Lobby Return**: Leaving an `IN_PROGRESS` table as an observer or player via **`Leave Table`** returns cleanly to the Lobby view without hanging or blocking UI threads.
- **Segregated `Connected Users` and `Connected Robots` Rosters**:
  - Across Web, Android, and Desktop lobbies, connected clients are partitioned into two strictly separate, non-overlapping rosters:
    1. **`Connected Users`**: Displays registered human players currently connected to the server.
    2. **`Connected Robots`**: Displays autonomous robot engine clients connected via the bot API.
  - Human and robot accounts must **never** be mingled into a single undifferentiated list or count.

### `GDB-11.2` 3-Color Table Chat, `@bot` Support & Borderless `BoardChatZoomDialog`
1. **Complete Room Event & Chat Stream**:
   - The Table Chat box displays both player/bot chat messages and chronological room lifecycle events (`created table`, `sat at Seat A`, `stood up`, `match started`, `offered a draw`, `resigned`, `won by base capture`).
   - When an AI bot (`octibot`) is seated at the table, typing `@bot <message>` (or chatting in a 1v1 human-vs-bot room) triggers a contextual bot reply.
2. **Strict 3-Color Text Hierarchy**:
   - **Own Messages**: Warm Brass Gold (`#F3E5AB` / `#D4AF37`).
   - **Other Players & Bots**: Crisp Slate White (`#E2E8F0`).
   - **System / Room Lifecycle Events**: Muted Steel Gray (`#94A3B8`, italic).
3. **Auto-Scroll to Newest**:
   - The chat log **must auto-scroll to the bottom** when entering a room and whenever a new message or room event arrives.
4. **Borderless Click-to-Zoom Modal (`BoardChatZoomDialog`)**:
   - Clicking/tapping the chat message list opens a **frameless/borderless enlarged chat modal** (`640×460` on Desktop/Web; full-width card on Android) with a larger message history viewport and bottom input row.
   - Contains **no `[X]` button or title bar frame**; dismissed by clicking/tapping outside the modal card or pressing `Esc` / `Back`.

---

## 12. Real-Time Synchronization & Autonomous 3-Platform QA Verification (`GDB-12`)

### `GDB-12.1` SSE Event Stream & Non-Blocking Networking Invariants
1. **Cursor-Advancing SSE (`since_seq`)**:
   - When subscribing to `/api/v1/lobby/events?since_seq=<N>` or `/api/v1/games/<id>/events?since_seq=<M>`, clients **must pass the latest `last_event_seq`** received from the initial REST snapshot (`GET /api/v1/lobby` or `GET /api/v1/games/<id>`).
   - Never reconnect or restart an SSE stream with `since_seq=0` in response to an SSE event, as doing so replays the entire room history in an infinite storm.
2. **Zero Main-Thread Network or Socket Blocking**:
   - On Desktop (Qt6) and Android, **all** HTTP requests, SSE stream connections, and socket close/disconnect operations (`stopGameEventStream()`, `stopLobbyEventStream()`, `leaveVoiceRoom()`) **must execute on background worker threads**—never synchronously on the UI / main thread.

### `GDB-12.2` Cross-Platform Compliance Checklist & Automated QA Gates
Every release and GUI modification must pass automated end-to-end verification across all 3 platforms against the following checklist:

| ID | Compliance Invariant | Web (`WebUi.cxx`) | Desktop (`desktop/`) | Android (`android/`) |
|---|---|---|---|---|
| `CHK-01` | **No `Undo` button or shortcut in Online Games** (`GDB-10.1a`) | Required | Required | Required |
| `CHK-02` | **Animated 30%-edge Pink (`#FF69B4`) Running Light** active on turn player's board edge in **both Local & Online** `IN_PROGRESS` games (on-turn, off-turn, and observing) (`GDB-05.1`) | Required | Required | Required |
| `CHK-03` | **Auto-Rotate Board (`180°` for Seat B, `0°/90°/180°/270°` in 4P)** immediately upon sitting at an online seat or starting as Seat B (`GDB-06.1`) | Required | Required | Required |
| `CHK-04` | **Single-Click / Single-Tap on Empty Friendly Home Square** immediately stages placing a reserve pod (`<sq>`) (`GDB-07.2`) | Required | Required | Required |
| `CHK-05` | **Single-Click / Single-Tap on 0-Move (0-Prong) Friendly Pod** immediately opens `ZoomedSquareDialog` (`GDB-07.2`) | Required | Required | Required |
| `CHK-06` | **Zoom-Only Prong Addition/Repositioning** (no direct outer-ring prong addition on main board) (`GDB-07.1`) | Required | Required | Required |
| `CHK-07` | **Inverse Pod Stack Sorting**: Most-prongs-at-bottom on Main Board (`GDB-04.2a`); Most-prongs-on-top in `ZoomedSquareDialog` (`GDB-04.2b`) | Required | Required | Required |
| `CHK-08` | **Dark Recessed Home Squares (`#0A0E16`)** with Player-POV rotated `"OCTI"` label (`GDB-04.1`) | Required | Required | Required |
| `CHK-09` | **Orange (`#FF8C00`) Last-Move Square Orbit** with `5000 ms` auto-timeout & instant clear on interaction (`GDB-05.2`) | Required | Required | Required |
| `CHK-10` | **Two-Button Confirmation (`Cancel` + 1 Hz Flashing `Commit`)** with no `"More Moves"` button (`GDB-08.1`, `GDB-05.3`) | Required | Required | Required |
| `CHK-11` | **Always-Active Board Zoom (`1.0x..3.0x`), Pan, Free Rotation with 90° Snap & Read-Only Square Zoom** off-turn, during AI thinking, and when observing (`GDB-06.2..06.4`) | Required | Required | Required |
| `CHK-12` | **No Legacy `"Quick Guide"` Card**; Reserve cards show Superprongs only when enabled and Captured pods in opponent's color only when `> 0` (`GDB-03.2`, `GDB-10.2`) | Required | Required | Required |
| `CHK-13` | **3-Color Auto-Scrolling Table Chat** with borderless click-to-zoom `BoardChatZoomDialog` (`GDB-11.2`) | Required | Required | Required |
| `CHK-14` | **Cursor-Advancing SSE (`since_seq > 0`) & Zero UI-Thread Socket Calls** (`GDB-12.1`) | Required | Required | Required |
| `CHK-15` | **Ten-Theme Cross-Platform Parity & Style Layer Invariants** (`GDB-02.1`, `GDB-02.2`) | Required | Required | Required |
| `CHK-16` | **Segregated `Connected Users` and `Connected Robots` Rosters** (`GDB-11.1`) | Required | Required | Required |
| `CHK-17` | **Pre-Game Pink Running Light Suppression** (`!matchStarted` / `WAITING_FOR_PLAYERS`) (`GDB-05.1`) | Required | Required | Required |
| `CHK-18` | **Casual & Tactical AI Archive Upload Ignoring** (`HTTP 200 {"ok":true,"ignored":true}`) (`GDB-10.4`) | Required | Required | Required |
