# OCTI

**Version: 2.0.3**

An open-source platform for playing **OCTI** and **OCTI for Kids (OFK)**—combining an Android mobile client, a Qt desktop client for Linux (X11) and Windows, a shared C++17 core engine with an AI opponent, a real-time multiplayer and matchmaking server, and a web-based game archive.

---

## About the Game

**OCTI** is an abstract strategy board game designed by Don Green. Players begin with octagonal **pods** on their home OCTI squares and build directional movement capabilities by inserting wooden **prongs** into their pods. On each turn, a player chooses between building/repositioning a prong, moving a pod (or stack of pods), jumping and optionally capturing other pods, or introducing reserve/captured pods back onto the board. Victory is achieved by occupying the opponent's OCTI home squares.

Full rules, board diagrams, and variant specifications are available in [`doc/octi_game_instructions.md`](doc/octi_game_instructions.md).

---

## Project Overview & Goals

### 1. Android Mobile App
- Play **OCTI** and **OCTI for Kids (OFK)** with an intuitive touch interface for pod movement, prong insertion/repositioning, stack management, and multi-jump capture selection.
- Full support for official game variants and player options:
  - **Two-Player** and **Four-Player (2v2 Team)** modes
  - **Single-Square** (quick game) and **All-Three-Squares** (standard) victory conditions
  - **Edgeless Board** (wrap-around board edges)
  - **Superprongs** (bonus pre-turn movement/jump)

### 2. On-Device AI Opponent
- Play offline or practice locally against a built-in AI opponent directly on the mobile device across supported rulesets and difficulty levels.

### 3. Live Multiplayer & Matchmaking Server
- Connect from the mobile app to a central game server to find opponents and launch live matches.
- **Matchmaking & Lobbies**: Pair players automatically or host/join custom game tables.
- **Live Spectating**: Allow observers to watch active games in real time.
- **In-Game Chat**: Real-time messaging between active players and observers connected to a board.

### 4. Game Recording & Web Archive
- Record all server-hosted games, participating players, and complete move-by-move histories.
- Publish completed and ongoing game records via a web server for browsing, analysis, and move replay.

---

## Documentation

| Path | Description |
| :--- | :--- |
| [`doc/octi_game_instructions.md`](doc/octi_game_instructions.md) | Complete OCTI rules, setup, four-player rules, player options, and illustrated board examples |
| [`doc/octi_for_kids_instructions.md`](doc/octi_for_kids_instructions.md) | Official OCTI for Kids (6×7 board) rules, setup, and board diagram |
| [`doc/octi_notation.md`](doc/octi_notation.md) | Formal EBNF ply move syntax, canonical normalization, `.octi` game transcript format, and OCTI-FEN position snapshot specification |
| [`doc/octi_game_instructions.pdf`](doc/octi_game_instructions.pdf) | Searchable OCR-processed PDF of the original OCTI instruction leaflet |
| [`doc/octi_game_instructions.png`](doc/octi_game_instructions.png) | High-resolution scan of the original OCTI instruction leaflet |
