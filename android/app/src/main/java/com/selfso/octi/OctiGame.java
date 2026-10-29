/*
 * OctiGame.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class OctiGame {

    public static class MoveHistoryEntry {
        private final OctiPlayer player;
        private final OctiMove move;

        public MoveHistoryEntry(OctiPlayer player, OctiMove move) {
            this.player = player;
            this.move = move;
        }

        public OctiPlayer getPlayer() {
            return player;
        }

        public OctiMove getMove() {
            return move;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof MoveHistoryEntry)) {
                return false;
            }
            MoveHistoryEntry other = (MoveHistoryEntry) obj;
            return this.player == other.player && Objects.equals(this.move, other.move);
        }

        @Override
        public int hashCode() {
            return Objects.hash(player, move);
        }
    }

    private static class Coord {
        final int x;
        final int y;

        Coord(int x, int y) {
            this.x = x;
            this.y = y;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) return true;
            if (!(obj instanceof Coord)) return false;
            Coord c = (Coord) obj;
            return this.x == c.x && this.y == c.y;
        }

        @Override
        public int hashCode() {
            return Objects.hash(x, y);
        }
    }

    private final OctiBoard board;
    private final OctiGameMode mode;
    private final OctiGameBases bases;
    private final boolean useSuperProngs;
    private final boolean useEdgelessBoard;

    private boolean gameStarted = false;
    private boolean gameOver = false;
    private boolean gameResigned = false;
    private OctiPlayer startPlayer = OctiPlayer.PLAYER_A;
    private OctiPlayer turn = OctiPlayer.PLAYER_UNDEF;
    private OctiPlayer winner = OctiPlayer.PLAYER_UNDEF;

    private final OctiPlayerState[] players = new OctiPlayerState[5];

    private final List<MoveHistoryEntry> history = new ArrayList<>();
    private final List<OctiGameSnapshot> undoStack = new ArrayList<>();
    private final List<OctiGameSnapshot> redoStack = new ArrayList<>();

    private boolean hasPreparedMove = false;
    private OctiGameSnapshot prepSnapshot = new OctiGameSnapshot();
    private final List<OctiMoveOp> prepOps = new ArrayList<>();

    private final List<OctiGameListener> listeners = new ArrayList<>();

    public OctiGame(OctiBoard board,
                    OctiGameMode mode,
                    boolean useSuperProngs,
                    boolean useEdgelessBoard,
                    OctiGameBases bases) {
        this.board = board;
        this.mode = mode;
        this.bases = bases;
        this.useSuperProngs = (mode == OctiGameMode.GAME_MODE_OFK) ? false : useSuperProngs;
        this.useEdgelessBoard = useEdgelessBoard;

        for (int p = 0; p < 5; p++) {
            players[p] = new OctiPlayerState();
        }
    }

    public OctiGame(OctiBoard board) {
        this(board, OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
    }

    public void addGameListener(OctiGameListener listener) {
        if (listener != null && !listeners.contains(listener)) {
            listeners.add(listener);
        }
    }

    public void removeGameListener(OctiGameListener listener) {
        listeners.remove(listener);
    }

    private void notifyGameUpdated() {
        for (OctiGameListener l : listeners) {
            l.gameUpdated(this);
        }
    }

    private void notifyUndoRedoStatusChanged() {
        for (OctiGameListener l : listeners) {
            l.undoRedoStatusChanged(this, canUndo(), canRedo());
        }
    }

    public OctiBoard getBoard() {
        return board;
    }

    public OctiGameMode getMode() {
        return mode;
    }

    public OctiGameBases getBases() {
        return bases;
    }

    public boolean useSuperProngs() {
        return useSuperProngs;
    }

    public boolean useEdgelessBoard() {
        return useEdgelessBoard;
    }

    public boolean isGameStarted() {
        return gameStarted;
    }

    public boolean isGameOver() {
        return gameOver;
    }

    public boolean isGameResigned() {
        return gameResigned;
    }

    public OctiPlayer getTurn() {
        return turn;
    }

    public OctiPlayer getStartPlayer() {
        return startPlayer;
    }

    public OctiPlayer getWinner() {
        return winner;
    }

    public boolean isPlayerEliminated(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) {
            return false;
        }
        return players[p.getValue()].eliminated;
    }

    public int getFreePods(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) return 0;
        return players[p.getValue()].freePods;
    }

    public int getCapturedPods(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) return 0;
        return players[p.getValue()].capturedPods;
    }

    public int getFreeProngs(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) return 0;
        return players[p.getValue()].freeProngs;
    }

    public int getFreeSuperProngs(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) return 0;
        return players[p.getValue()].freeSuperProngs;
    }

    public long getTimeRemaining(OctiPlayer p) {
        if (p == null || p.getValue() < 1 || p.getValue() > 4) return 0;
        return players[p.getValue()].timeRemainingMs;
    }

    public void setTimeRemaining(OctiPlayer p, long ms) {
        if (p != null && p.getValue() >= 1 && p.getValue() <= 4) {
            players[p.getValue()].timeRemainingMs = ms;
        }
    }

    public int getNumMoves() {
        return history.size() + searchPlyOffset;
    }

    private boolean searchMode = false;
    private int searchPlyOffset = 0;

    public void setSearchMode(boolean searchMode) {
        this.searchMode = searchMode;
    }

    public boolean isSearchMode() {
        return searchMode;
    }

    public List<MoveHistoryEntry> getMoves() {
        return Collections.unmodifiableList(history);
    }

    public void getMoves(List<MoveHistoryEntry> outHistory) {
        if (outHistory != null) {
            outHistory.clear();
            outHistory.addAll(history);
        }
    }

    public boolean canUndo() {
        return !undoStack.isEmpty();
    }

    public boolean canRedo() {
        return !redoStack.isEmpty();
    }

    public void reset() {
        undoStack.clear();
        redoStack.clear();
        history.clear();

        gameStarted = false;
        gameOver = false;
        gameResigned = false;
        searchPlyOffset = 0;
        turn = OctiPlayer.PLAYER_UNDEF;
        winner = OctiPlayer.PLAYER_UNDEF;
        hasPreparedMove = false;
        prepOps.clear();

        for (int p = 0; p < 5; p++) {
            players[p] = new OctiPlayerState();
        }

        if (board != null) {
            board.clear();

            if (mode == OctiGameMode.GAME_MODE_2) {
                board.squareAt(2, 2).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(4, 2).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(6, 2).addPod(new OctiPod(OctiPodType.A_POD));

                board.squareAt(2, 6).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(4, 6).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(6, 6).addPod(new OctiPod(OctiPodType.B_POD));

                for (OctiPlayer p : new OctiPlayer[]{OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B}) {
                    int pVal = p.getValue();
                    players[pVal].freePods = 4;
                    players[pVal].capturedPods = 0;
                    if (useSuperProngs) {
                        players[pVal].freeProngs = 24;
                        players[pVal].freeSuperProngs = 1;
                    } else {
                        players[pVal].freeProngs = 25;
                        players[pVal].freeSuperProngs = 0;
                    }
                }
            } else if (mode == OctiGameMode.GAME_MODE_4) {
                board.squareAt(2, 2).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(2, 6).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(6, 6).addPod(new OctiPod(OctiPodType.C_POD));
                board.squareAt(6, 2).addPod(new OctiPod(OctiPodType.D_POD));

                for (OctiPlayer p : new OctiPlayer[]{OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B,
                        OctiPlayer.PLAYER_C, OctiPlayer.PLAYER_D}) {
                    int pVal = p.getValue();
                    players[pVal].freePods = 3;
                    players[pVal].capturedPods = 0;
                    if (useSuperProngs) {
                        players[pVal].freeProngs = 11;
                        players[pVal].freeSuperProngs = 1;
                    } else {
                        players[pVal].freeProngs = 12;
                        players[pVal].freeSuperProngs = 0;
                    }
                    players[pVal].eliminated = false;
                }
            } else if (mode == OctiGameMode.GAME_MODE_OFK) {
                board.squareAt(1, 1).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(2, 1).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(3, 1).addPod(new OctiPod(OctiPodType.A_POD));
                board.squareAt(4, 1).addPod(new OctiPod(OctiPodType.A_POD));

                board.squareAt(1, 5).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(2, 5).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(3, 5).addPod(new OctiPod(OctiPodType.B_POD));
                board.squareAt(4, 5).addPod(new OctiPod(OctiPodType.B_POD));

                for (OctiPlayer p : new OctiPlayer[]{OctiPlayer.PLAYER_A, OctiPlayer.PLAYER_B}) {
                    int pVal = p.getValue();
                    players[pVal].freePods = 0;
                    players[pVal].capturedPods = 0;
                    players[pVal].freeProngs = 12;
                    players[pVal].freeSuperProngs = 0;
                    players[pVal].eliminated = false;
                }
            }
        }

        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    public void start(OctiPlayer startPlayer) {
        this.startPlayer = startPlayer;
        this.turn = startPlayer;
        this.gameStarted = true;
        this.gameOver = false;
        this.gameResigned = false;
        this.winner = OctiPlayer.PLAYER_UNDEF;

        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    public void start() {
        start(OctiPlayer.PLAYER_A);
    }

    public void randomize() {
        if (board != null && mode != OctiGameMode.GAME_MODE_2) {
            board.setGameMode(OctiGameMode.GAME_MODE_2);
        }
        reset();
        start(OctiPlayer.PLAYER_A);

        String[] moves = {
            "33+b",
            "37+e",
            "33",
            "37",
            "33+c",
            "37+d",
            "53+a",
            "57+f",
            "33b+g",
            "37d+f",
            "73+h",
            "37df-46,37e-36",
            "53a+f",
            "57f-35",
            "33c-43,33bg-44",
            "36e+c",
            "53af-54",
            "35f+d"
        };

        for (String m : moves) {
            move(m);
        }
    }

    public void resign(OctiPlayer player) {
        if (!gameStarted || gameOver) {
            throw new IllegalMoveException("game is not in progress");
        }

        gameResigned = true;
        gameOver = true;

        if (mode == OctiGameMode.GAME_MODE_2 || mode == OctiGameMode.GAME_MODE_OFK) {
            winner = (player == OctiPlayer.PLAYER_A) ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A;
        } else if (mode == OctiGameMode.GAME_MODE_4) {
            if (player == OctiPlayer.PLAYER_A || player == OctiPlayer.PLAYER_C) {
                winner = OctiPlayer.PLAYER_B;
            } else {
                winner = OctiPlayer.PLAYER_A;
            }
        }

        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    public void move(String moveStr) {
        move(new OctiMove(moveStr));
    }

    public void move(List<OctiMoveOp> ops) {
        move(new OctiMove(ops));
    }

    public void move(OctiPlayer player, OctiMove m) {
        if (player != turn) {
            throw new IllegalMoveException("not player's turn");
        }
        move(m);
    }

    public void move(OctiPlayer player, String moveStr) {
        move(player, new OctiMove(moveStr));
    }

    public void move(OctiMove m) {
        executeMoveInternal(m, false);
    }

    private void executeMoveInternal(OctiMove m, boolean isRedo) {
        if (!gameStarted) {
            throw new IllegalMoveException("game has not started");
        }
        if (gameOver) {
            throw new IllegalMoveException("game is already over");
        }
        if (turn == OctiPlayer.PLAYER_UNDEF) {
            throw new IllegalMoveException("turn is undefined");
        }
        if (m == null || m.isEmpty()) {
            throw new IllegalMoveException("move is empty");
        }

        if (searchMode) {
            applyMoveOps(m.getOps());
            check4PlayerElimination();
            searchPlyOffset++;
            updateGameOverAndWinner();
            if (!gameOver) {
                advanceTurn();
            }
            return;
        }

        OctiGameSnapshot snapBefore = captureSnapshot();

        try {
            applyMoveOps(m.getOps());
            check4PlayerElimination();
        } catch (Throwable t) {
            restoreSnapshot(snapBefore);
            if (t instanceof RuntimeException) {
                throw (RuntimeException) t;
            }
            throw new RuntimeException(t);
        }

        if (!isRedo) {
            snapBefore.lastMove = m;
            snapBefore.lastPlayer = snapBefore.turn;
            undoStack.add(snapBefore);
            redoStack.clear();
            history.add(new MoveHistoryEntry(snapBefore.turn, m));
        }

        updateGameOverAndWinner();
        if (!gameOver) {
            advanceTurn();
        }

        if (board != null) {
            board.notifyBoardTurnUpdate(m);
        }
        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    // Ghost (vacated) pods exist only on the moving stack's own starting square.
    // Every other square is judged by plain emptiness so that an own pod with
    // identical prongs elsewhere on the board is never mistaken for a vacated pod.
    private static boolean isEmptyForJump(OctiSquare sq, int x, int y,
                                          int startX, int startY,
                                          List<OctiPod> ghostPods) {
        if (x == startX && y == startY) {
            return sq.isEmptyWithout(ghostPods);
        }
        return sq.isEmpty();
    }

    private static boolean findStepDelta(int cx, int cy, int tx, int ty,
                                         boolean edgeless, int width, int height,
                                         int[] outDelta) {
        if (!edgeless) {
            outDelta[0] = tx - cx;
            outDelta[1] = ty - cy;
            return true;
        }

        boolean foundX = false;
        int bestDx = 0;
        for (int d = -2; d <= 2; d++) {
            if (((cx + d) % width + width) % width == tx) {
                bestDx = d;
                foundX = true;
                break;
            }
        }
        if (!foundX) {
            return false;
        }

        boolean foundY = false;
        int bestDy = 0;
        for (int d = -2; d <= 2; d++) {
            if (((cy + d) % height + height) % height == ty) {
                bestDy = d;
                foundY = true;
                break;
            }
        }
        if (!foundY) {
            return false;
        }

        outDelta[0] = bestDx;
        outDelta[1] = bestDy;
        return true;
    }

    private void applyMoveOps(List<OctiMoveOp> ops) {
        if (ops == null || ops.isEmpty()) {
            throw new IllegalMoveException("no operations in move");
        }

        List<OctiMoveOp> bonusOps = new ArrayList<>();
        List<OctiMoveOp> regularOps = new ArrayList<>();
        for (OctiMoveOp op : ops) {
            if (op.isSuperProngBonusMove) {
                bonusOps.add(op);
            } else {
                regularOps.add(op);
            }
        }

        if (!bonusOps.isEmpty()) {
            if (!useSuperProngs) {
                throw new IllegalMoveException("superprongs not enabled");
            }
            if (bonusOps.size() > 2) {
                throw new IllegalMoveException("exceeded maximum superprong bonus moves");
            }

            Set<Coord> bonusCaptures = new HashSet<>();
            for (OctiMoveOp bOp : bonusOps) {
                if (bOp.hops.isEmpty()) {
                    throw new IllegalMoveException("superprong bonus move must contain hops");
                }
                if (!board.isValidCoord(bOp.x0, bOp.y0)) {
                    throw new IllegalMoveException("superprong bonus starting square out of bounds");
                }
                OctiSquare sq = board.squareAt(bOp.x0, bOp.y0);
                int podIdx = -1;
                for (int i = 0; i < sq.getNumPods(); i++) {
                    if (sq.getPodAt(i).getType() == turn.toPodType()
                            && (sq.getPodAt(i).rawConfig() & 0xFFFF) == (bOp.pod.rawConfig() & 0xFFFF)) {
                        podIdx = i;
                        break;
                    }
                }
                if (podIdx == -1) {
                    throw new IllegalMoveException("pod for superprong bonus move not found");
                }
                OctiPod pod = sq.getPodAt(podIdx).copy();
                sq.removePod(pod);

                int cx = bOp.x0;
                int cy = bOp.y0;
                Set<Coord> jumpedSquares = new HashSet<>();

                for (OctiHop hop : bOp.hops) {
                    int[] delta = new int[2];
                    if (!findStepDelta(cx, cy, hop.x, hop.y, useEdgelessBoard,
                            board.getBoardWidth(), board.getBoardHeight(), delta)) {
                        throw new IllegalMoveException("invalid displacement in bonus move");
                    }
                    int dx = delta[0];
                    int dy = delta[1];

                    boolean isDist1 = (Math.max(Math.abs(dx), Math.abs(dy)) == 1
                            && (Math.abs(dx) == 1 || Math.abs(dy) == 1));
                    if (isDist1) {
                        if (bOp.hops.size() != 1 || hop.capture) {
                            throw new IllegalMoveException("invalid distance-1 bonus hop");
                        }
                        OctiDir dir = OctiDir.deltaToDir(dx, dy);
                        if (dir == OctiDir.DIR_UNDEF || pod.prongAt(dir) != OctiProngType.SUPER_PRONG) {
                            throw new IllegalMoveException("bonus move must follow superprong direction");
                        }
                        OctiSquare destSq = board.squareAt(hop.x, hop.y);
                        if (destSq.hasEnemyOf(turn)) {
                            throw new IllegalMoveException("cannot move onto enemy square in bonus move");
                        }
                        cx = hop.x;
                        cy = hop.y;
                    } else {
                        boolean validJump = (Math.abs(dx) == 0 || Math.abs(dx) == 2)
                                && (Math.abs(dy) == 0 || Math.abs(dy) == 2)
                                && (Math.abs(dx) == 2 || Math.abs(dy) == 2)
                                && (Math.abs(dx) == 0 || Math.abs(dy) == 0 || Math.abs(dx) == Math.abs(dy));
                        if (!validJump) {
                            throw new IllegalMoveException("invalid jump geometry in bonus move");
                        }
                        OctiDir dir = OctiDir.deltaToDir(dx / 2, dy / 2);
                        if (dir == OctiDir.DIR_UNDEF || pod.prongAt(dir) != OctiProngType.SUPER_PRONG) {
                            throw new IllegalMoveException("bonus jump must follow superprong direction");
                        }
                        int bw = board.getBoardWidth();
                        int bh = board.getBoardHeight();
                        int jx = useEdgelessBoard ? (((cx + dx / 2) % bw + bw) % bw) : (cx + dx / 2);
                        int jy = useEdgelessBoard ? (((cy + dy / 2) % bh + bh) % bh) : (cy + dy / 2);
                        Coord jCoord = new Coord(jx, jy);
                        if (jumpedSquares.contains(jCoord)) {
                            throw new IllegalMoveException("cannot jump same square twice in bonus move");
                        }
                        jumpedSquares.add(jCoord);

                        OctiSquare jumpedSq = board.squareAt(jx, jy);
                        if (jumpedSq.isEmpty()) {
                            throw new IllegalMoveException("cannot jump empty square in bonus move");
                        }
                        if (hop.capture) {
                            if (!jumpedSq.hasEnemyOf(turn)) {
                                throw new IllegalMoveException("cannot capture non-enemy square in bonus move");
                            }
                            bonusCaptures.add(jCoord);
                        }
                        OctiSquare landingSq = board.squareAt(hop.x, hop.y);
                        if (!landingSq.isEmpty()) {
                            throw new IllegalMoveException("landing square must be empty in bonus move");
                        }
                        cx = hop.x;
                        cy = hop.y;
                    }
                }
                board.squareAt(cx, cy).addPod(pod);
            }

            for (Coord coord : bonusCaptures) {
                OctiSquare sq = board.squareAt(coord.x, coord.y);
                List<OctiPod> keep = new ArrayList<>();
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod sqPod = sq.getPodAt(i);
                    OctiPlayer owner = sqPod.getType().toPlayer();
                    if (OctiPlayer.areAllied(mode, turn, owner)) {
                        keep.add(sqPod);
                    } else {
                        players[turn.getValue()].freeProngs += sqPod.getNumNormalProngs();
                        players[turn.getValue()].freeSuperProngs += sqPod.getNumSuperProngs();
                        players[owner.getValue()].capturedPods++;
                    }
                }
                sq.clear();
                for (OctiPod kp : keep) {
                    sq.addPod(kp);
                }
            }
        }

        if (regularOps.isEmpty()) {
            throw new IllegalMoveException("move must include a regular turn action");
        }

        // Option V: Pod placement / liberation
        if (regularOps.get(0).addPod) {
            if (regularOps.size() != 1) {
                throw new IllegalMoveException("pod placement cannot be combined with other actions");
            }
            if (mode == OctiGameMode.GAME_MODE_OFK) {
                throw new IllegalMoveException("pod placement forbidden in OCTI for Kids");
            }
            OctiMoveOp op = regularOps.get(0);
            if (!board.isValidCoord(op.x0, op.y0)) {
                throw new IllegalMoveException("pod placement square out of bounds");
            }
            OctiSquare sq = board.squareAt(op.x0, op.y0);
            if (sq.getType() == OctiSquareType.NEUTRAL_SQUARE) {
                throw new IllegalMoveException("pod placement only allowed on OCTI squares");
            }
            OctiSquareType st = sq.getType();
            boolean isFriendlyHome = false;
            if (board.getNumPlayers() == 2) {
                isFriendlyHome = (turn == OctiPlayer.PLAYER_A && st == OctiSquareType.A_HOME_SQUARE)
                        || (turn == OctiPlayer.PLAYER_B && st == OctiSquareType.B_HOME_SQUARE);
            } else if (board.getNumPlayers() == 4) {
                isFriendlyHome = (st.getValue() == turn.getValue());
            }

            if (isFriendlyHome) {
                if (sq.hasEnemyOf(turn)) {
                    throw new IllegalMoveException("pod placement square has enemy pod");
                }
            } else {
                if (sq.isEmpty() || sq.hasEnemyOf(turn)) {
                    throw new IllegalMoveException("pod placement square must be occupied by friendly pod");
                }
            }

            int turnVal = turn.getValue();
            if (mode == OctiGameMode.GAME_MODE_2) {
                boolean isFriendlyBase = (sq.getType() == (turn == OctiPlayer.PLAYER_A
                        ? OctiSquareType.A_HOME_SQUARE : OctiSquareType.B_HOME_SQUARE));
                if (isFriendlyBase) {
                    if (op.liberate) {
                        if (!board.canLiberate(turn) || players[turnVal].capturedPods <= 0) {
                            throw new IllegalMoveException("liberation condition not met on friendly base");
                        }
                        if (!sq.isOccupiedBy(turn)) {
                            throw new IllegalMoveException("liberated pod must be stacked on an occupied OCTI square");
                        }
                        players[turnVal].capturedPods--;
                        sq.addPod(new OctiPod(turn.toPodType()));
                    } else {
                        if (players[turnVal].freePods > 0) {
                            players[turnVal].freePods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else if (board.canLiberate(turn) && players[turnVal].capturedPods > 0 && sq.isOccupiedBy(turn)) {
                            players[turnVal].capturedPods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else {
                            throw new IllegalMoveException("no free or capturable pods available to place");
                        }
                    }
                } else {
                    if (op.liberate) {
                        if (players[turnVal].capturedPods <= 0) {
                            throw new IllegalMoveException("no captured pods to liberate");
                        }
                        players[turnVal].capturedPods--;
                        sq.addPod(new OctiPod(turn.toPodType()));
                    } else {
                        if (players[turnVal].capturedPods > 0) {
                            players[turnVal].capturedPods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else if (players[turnVal].freePods > 0) {
                            players[turnVal].freePods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else {
                            throw new IllegalMoveException("no free or capturable pods available to place");
                        }
                    }
                }
            } else if (mode == OctiGameMode.GAME_MODE_4) {
                OctiSquareType homeType = (turn == OctiPlayer.PLAYER_A ? OctiSquareType.A_HOME_SQUARE :
                                           turn == OctiPlayer.PLAYER_B ? OctiSquareType.B_HOME_SQUARE :
                                           turn == OctiPlayer.PLAYER_C ? OctiSquareType.C_HOME_SQUARE
                                                                      : OctiSquareType.D_HOME_SQUARE);
                OctiPlayer partner = (turn == OctiPlayer.PLAYER_A ? OctiPlayer.PLAYER_C :
                                      turn == OctiPlayer.PLAYER_C ? OctiPlayer.PLAYER_A :
                                      turn == OctiPlayer.PLAYER_B ? OctiPlayer.PLAYER_D
                                                                 : OctiPlayer.PLAYER_B);
                OctiSquareType partnerHome = (partner == OctiPlayer.PLAYER_A ? OctiSquareType.A_HOME_SQUARE :
                                              partner == OctiPlayer.PLAYER_B ? OctiSquareType.B_HOME_SQUARE :
                                              partner == OctiPlayer.PLAYER_C ? OctiSquareType.C_HOME_SQUARE
                                                                             : OctiSquareType.D_HOME_SQUARE);

                int partnerVal = partner.getValue();
                if (sq.getType() == homeType) {
                    if (op.liberate || players[turnVal].freePods <= 0) {
                        throw new IllegalMoveException("cannot place reserve pod on own base");
                    }
                    players[turnVal].freePods--;
                    sq.addPod(new OctiPod(turn.toPodType()));
                } else if (sq.getType() == partnerHome) {
                    if (players[partnerVal].eliminated) {
                        if (players[partnerVal].capturedPods > 0) {
                            players[partnerVal].eliminated = false;
                            players[partnerVal].capturedPods--;
                            sq.addPod(new OctiPod(partner.toPodType()));
                        } else if (!op.liberate && players[turnVal].freePods > 0) {
                            players[turnVal].freePods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else {
                            throw new IllegalMoveException("cannot liberate partner: no captured pods");
                        }
                    } else {
                        if (!op.liberate && players[turnVal].freePods > 0) {
                            players[turnVal].freePods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else {
                            throw new IllegalMoveException("cannot place pod on partner base");
                        }
                    }
                } else {
                    if (op.liberate) {
                        if (players[turnVal].capturedPods <= 0) {
                            throw new IllegalMoveException("no captured pods to liberate");
                        }
                        players[turnVal].capturedPods--;
                        sq.addPod(new OctiPod(turn.toPodType()));
                    } else {
                        if (players[turnVal].capturedPods > 0) {
                            players[turnVal].capturedPods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else if (players[turnVal].freePods > 0) {
                            players[turnVal].freePods--;
                            sq.addPod(new OctiPod(turn.toPodType()));
                        } else {
                            throw new IllegalMoveException("no free or capturable pods available");
                        }
                    }
                }
            }
            return;
        }

        // Option I: Add Prong
        if (regularOps.get(0).addProngDir != OctiDir.DIR_UNDEF
                && regularOps.get(0).rmProngDir == OctiDir.DIR_UNDEF) {
            if (regularOps.size() != 1) {
                throw new IllegalMoveException("prong addition cannot be combined with other actions");
            }
            OctiMoveOp op = regularOps.get(0);
            if (!board.isValidCoord(op.x0, op.y0)) {
                throw new IllegalMoveException("prong addition square out of bounds");
            }
            OctiSquare sq = board.squareAt(op.x0, op.y0);
            int podIdx = -1;
            for (int i = 0; i < sq.getNumPods(); i++) {
                if (sq.getPodAt(i).getType() == turn.toPodType()
                        && (sq.getPodAt(i).rawConfig() & 0xFFFF) == (op.pod.rawConfig() & 0xFFFF)) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx == -1) {
                throw new IllegalMoveException("matching pod for prong addition not found on square");
            }
            if (sq.getPodAt(podIdx).prongAt(op.addProngDir) != OctiProngType.NO_PRONG) {
                throw new IllegalMoveException("slot already has a prong");
            }

            int turnVal = turn.getValue();
            if (op.superProng) {
                if (!useSuperProngs || players[turnVal].freeSuperProngs <= 0) {
                    throw new IllegalMoveException("no free superprongs available");
                }
                players[turnVal].freeSuperProngs--;
                sq.getPodAt(podIdx).addProng(op.addProngDir, true);
            } else {
                if (players[turnVal].freeProngs <= 0) {
                    throw new IllegalMoveException("no free normal prongs available");
                }
                players[turnVal].freeProngs--;
                sq.getPodAt(podIdx).addProng(op.addProngDir, false);
            }
            return;
        }

        // Option II: Reposition Prong
        if (regularOps.get(0).addProngDir != OctiDir.DIR_UNDEF
                && regularOps.get(0).rmProngDir != OctiDir.DIR_UNDEF) {
            if (regularOps.size() != 1) {
                throw new IllegalMoveException("prong repositioning cannot be combined with other actions");
            }
            if (mode == OctiGameMode.GAME_MODE_OFK) {
                throw new IllegalMoveException("prong repositioning forbidden in OCTI for Kids");
            }
            OctiMoveOp op = regularOps.get(0);
            if (op.superProng) {
                throw new IllegalMoveException("cannot add superprong during repositioning");
            }
            if (!board.isValidCoord(op.x0, op.y0)) {
                throw new IllegalMoveException("prong repositioning square out of bounds");
            }
            OctiSquare sq = board.squareAt(op.x0, op.y0);
            int podIdx = -1;
            for (int i = 0; i < sq.getNumPods(); i++) {
                if (sq.getPodAt(i).getType() == turn.toPodType()
                        && (sq.getPodAt(i).rawConfig() & 0xFFFF) == (op.pod.rawConfig() & 0xFFFF)) {
                    podIdx = i;
                    break;
                }
            }
            if (podIdx == -1) {
                throw new IllegalMoveException("matching pod for prong repositioning not found on square");
            }
            if (sq.getPodAt(podIdx).prongAt(op.rmProngDir) != OctiProngType.NORMAL_PRONG) {
                throw new IllegalMoveException("can only reposition normal prongs");
            }
            if (sq.getPodAt(podIdx).prongAt(op.addProngDir) != OctiProngType.NO_PRONG) {
                throw new IllegalMoveException("target prong slot must be empty");
            }
            sq.getPodAt(podIdx).removeProng(op.rmProngDir);
            sq.getPodAt(podIdx).addProng(op.addProngDir, false);
            return;
        }

        // Options III & IV: Movement and Jumps
        int sx = regularOps.get(0).x0;
        int sy = regularOps.get(0).y0;
        if (!board.isValidCoord(sx, sy)) {
            throw new IllegalMoveException("starting square out of bounds");
        }

        for (OctiMoveOp op : regularOps) {
            if (op.x0 != sx || op.y0 != sy) {
                throw new IllegalMoveException(
                        "composite move operations must share the same starting square");
            }
            if (op.hops.isEmpty()) {
                throw new IllegalMoveException("movement operation must contain at least one hop");
            }
        }

        OctiSquare startSq = board.squareAt(sx, sy);
        List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());
        boolean[] claimed = new boolean[ghostPods.size()];
        List<OctiPod> movingPods = new ArrayList<>();

        for (OctiMoveOp op : regularOps) {
            int foundIdx = -1;
            for (int i = 0; i < ghostPods.size(); i++) {
                if (!claimed[i]
                        && ghostPods.get(i).getType() == turn.toPodType()
                        && (ghostPods.get(i).rawConfig() & 0xFFFF) == (op.pod.rawConfig() & 0xFFFF)) {
                    foundIdx = i;
                    break;
                }
            }
            if (foundIdx == -1) {
                throw new IllegalMoveException("matching pod not found in starting stack");
            }
            claimed[foundIdx] = true;
            movingPods.add(ghostPods.get(foundIdx).copy());
        }

        startSq.clear();
        for (int i = 0; i < ghostPods.size(); i++) {
            if (!claimed[i]) {
                startSq.addPod(ghostPods.get(i));
            }
        }

        class LandedPod {
            final OctiPod pod;
            final int finalX;
            final int finalY;

            LandedPod(OctiPod pod, int finalX, int finalY) {
                this.pod = pod;
                this.finalX = finalX;
                this.finalY = finalY;
            }
        }

        List<LandedPod> landedPods = new ArrayList<>();
        List<Coord> capturedSquaresEndOfTurn = new ArrayList<>();

        for (int k = 0; k < regularOps.size(); k++) {
            OctiMoveOp op = regularOps.get(k);
            OctiPod pod = movingPods.get(k);
            int cx = sx;
            int cy = sy;
            List<Coord> jumpedSquaresThisPod = new ArrayList<>();

            int[] delta = new int[2];
            if (!findStepDelta(cx, cy, op.hops.get(0).x, op.hops.get(0).y, useEdgelessBoard,
                    board.getBoardWidth(), board.getBoardHeight(), delta)) {
                throw new IllegalMoveException("invalid displacement for first hop");
            }
            int dx = delta[0];
            int dy = delta[1];

            boolean isDist1 = (Math.max(Math.abs(dx), Math.abs(dy)) == 1
                    && (Math.abs(dx) == 1 || Math.abs(dy) == 1));

            if (isDist1) {
                if (op.hops.size() != 1) {
                    throw new IllegalMoveException("chaining distance-1 moves is forbidden");
                }
                if (op.hops.get(0).capture) {
                    throw new IllegalMoveException("capture not allowed on distance-1 move");
                }
                OctiDir dir = OctiDir.deltaToDir(dx, dy);
                if (dir == OctiDir.DIR_UNDEF || pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    throw new IllegalMoveException("pod lacks prong in distance-1 move direction");
                }
                OctiSquare destSq = board.squareAt(op.hops.get(0).x, op.hops.get(0).y);
                if (mode == OctiGameMode.GAME_MODE_OFK && !destSq.isEmpty()) {
                    throw new IllegalMoveException("pod stacking forbidden in OCTI for Kids");
                }
                if (mode != OctiGameMode.GAME_MODE_OFK && destSq.hasEnemyOf(turn)) {
                    throw new IllegalMoveException("cannot move onto enemy square");
                }
                landedPods.add(new LandedPod(pod, op.hops.get(0).x, op.hops.get(0).y));
            } else {
                for (OctiHop hop : op.hops) {
                    int[] jdelta = new int[2];
                    if (!findStepDelta(cx, cy, hop.x, hop.y, useEdgelessBoard,
                            board.getBoardWidth(), board.getBoardHeight(), jdelta)) {
                        throw new IllegalMoveException("invalid displacement for jump hop");
                    }
                    int jdx = jdelta[0];
                    int jdy = jdelta[1];

                    boolean validJump = (Math.abs(jdx) == 0 || Math.abs(jdx) == 2)
                            && (Math.abs(jdy) == 0 || Math.abs(jdy) == 2)
                            && (Math.abs(jdx) == 2 || Math.abs(jdy) == 2)
                            && (Math.abs(jdx) == 0 || Math.abs(jdy) == 0 || Math.abs(jdx) == Math.abs(jdy));
                    if (!validJump) {
                        throw new IllegalMoveException("invalid jump geometry");
                    }
                    OctiDir dir = OctiDir.deltaToDir(jdx / 2, jdy / 2);
                    if (dir == OctiDir.DIR_UNDEF || pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                        throw new IllegalMoveException("pod lacks prong in jump direction");
                    }
                    int bw = board.getBoardWidth();
                    int bh = board.getBoardHeight();
                    int jx = useEdgelessBoard ? (((cx + jdx / 2) % bw + bw) % bw) : (cx + jdx / 2);
                    int jy = useEdgelessBoard ? (((cy + jdy / 2) % bh + bh) % bh) : (cy + jdy / 2);
                    Coord jCoord = new Coord(jx, jy);

                    if (jumpedSquaresThisPod.contains(jCoord)) {
                        throw new IllegalMoveException("cannot jump same square twice in single pod chain");
                    }
                    jumpedSquaresThisPod.add(jCoord);

                    OctiSquare jumpedSq = board.squareAt(jx, jy);
                    if (isEmptyForJump(jumpedSq, jx, jy, sx, sy, ghostPods)) {
                        throw new IllegalMoveException("cannot jump square without stationary pods");
                    }

                    if (hop.capture) {
                        if (!jumpedSq.hasEnemyOf(turn)) {
                            throw new IllegalMoveException("cannot capture non-enemy square");
                        }
                        if (!capturedSquaresEndOfTurn.contains(jCoord)) {
                            capturedSquaresEndOfTurn.add(jCoord);
                        }
                    }

                    OctiSquare landingSq = board.squareAt(hop.x, hop.y);
                    if (!isEmptyForJump(landingSq, hop.x, hop.y, sx, sy, ghostPods)) {
                        throw new IllegalMoveException("landing square must be empty of stationary pods");
                    }

                    cx = hop.x;
                    cy = hop.y;
                }
                landedPods.add(new LandedPod(pod, cx, cy));
            }
        }

        for (LandedPod lp : landedPods) {
            OctiSquare destSq = board.squareAt(lp.finalX, lp.finalY);
            if (mode == OctiGameMode.GAME_MODE_OFK && !destSq.isEmpty()) {
                throw new IllegalMoveException("pod stacking forbidden in OCTI for Kids");
            }
            destSq.addPod(lp.pod);
        }

        int turnVal = turn.getValue();
        for (Coord coord : capturedSquaresEndOfTurn) {
            OctiSquare sq = board.squareAt(coord.x, coord.y);
            List<OctiPod> remaining = new ArrayList<>();
            for (int pIdx = 0; pIdx < sq.getNumPods(); pIdx++) {
                OctiPod sqPod = sq.getPodAt(pIdx);
                OctiPlayer owner = sqPod.getType().toPlayer();
                if (OctiPlayer.areAllied(mode, turn, owner)) {
                    remaining.add(sqPod);
                } else {
                    players[turnVal].freeProngs += sqPod.getNumNormalProngs();
                    players[turnVal].freeSuperProngs += sqPod.getNumSuperProngs();
                    players[owner.getValue()].capturedPods++;
                }
            }
            sq.clear();
            for (OctiPod rp : remaining) {
                sq.addPod(rp);
            }
        }
    }

    private void check4PlayerElimination() {
        if (mode != OctiGameMode.GAME_MODE_4) {
            return;
        }

        final int[][] homeCoords = {
            {-1, -1}, {2, 2}, {2, 6}, {6, 6}, {6, 2}
        };

        for (int p = 1; p <= 4; p++) {
            OctiPlayer player = OctiPlayer.fromValue(p);
            if (!players[p].eliminated) {
                OctiSquare hsq = board.squareAt(homeCoords[p][0], homeCoords[p][1]);
                if (hsq.isCaptured()) {
                    players[p].eliminated = true;
                    int bw = board.getBoardWidth();
                    int bh = board.getBoardHeight();
                    for (int x = 0; x < bw; x++) {
                        for (int y = 0; y < bh; y++) {
                            OctiSquare s = board.squareAt(x, y);
                            List<OctiPod> keep = new ArrayList<>();
                            for (int pi = 0; pi < s.getNumPods(); pi++) {
                                OctiPod pod = s.getPodAt(pi);
                                if (pod.getType().toPlayer() == player) {
                                    players[turn.getValue()].freeProngs += pod.getNumNormalProngs();
                                    players[turn.getValue()].freeSuperProngs += pod.getNumSuperProngs();
                                    players[p].capturedPods++;
                                } else {
                                    keep.add(pod);
                                }
                            }
                            s.clear();
                            for (OctiPod kp : keep) {
                                s.addPod(kp);
                            }
                        }
                    }
                }
            }
        }
    }

    private void updateGameOverAndWinner() {
        if (mode == OctiGameMode.GAME_MODE_2) {
            int aOccupiesB = 0;
            int bOccupiesA = 0;

            if (board.squareAt(2, 6).isCapturedBy(OctiPlayer.PLAYER_A)) aOccupiesB++;
            if (board.squareAt(4, 6).isCapturedBy(OctiPlayer.PLAYER_A)) aOccupiesB++;
            if (board.squareAt(6, 6).isCapturedBy(OctiPlayer.PLAYER_A)) aOccupiesB++;

            if (board.squareAt(2, 2).isCapturedBy(OctiPlayer.PLAYER_B)) bOccupiesA++;
            if (board.squareAt(4, 2).isCapturedBy(OctiPlayer.PLAYER_B)) bOccupiesA++;
            if (board.squareAt(6, 2).isCapturedBy(OctiPlayer.PLAYER_B)) bOccupiesA++;

            int neededBases = (bases == OctiGameBases.GAME_BASES_1) ? 1 : 3;
            if (aOccupiesB >= neededBases) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_A;
                return;
            }
            if (bOccupiesA >= neededBases) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_B;
                return;
            }

            int podsA = 0;
            int podsB = 0;
            int bw = board.getBoardWidth();
            int bh = board.getBoardHeight();
            for (int x = 0; x < bw; x++) {
                for (int y = 0; y < bh; y++) {
                    OctiSquare sq = board.squareAt(x, y);
                    for (int i = 0; i < sq.getNumPods(); i++) {
                        if (sq.getPodAt(i).getType() == OctiPodType.A_POD) podsA++;
                        else if (sq.getPodAt(i).getType() == OctiPodType.B_POD) podsB++;
                    }
                }
            }

            if (podsB == 0) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_A;
                return;
            }
            if (podsA == 0) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_B;
                return;
            }
        } else if (mode == OctiGameMode.GAME_MODE_OFK) {
            for (int x = 1; x <= 4; x++) {
                if (board.squareAt(x, 5).isCapturedBy(OctiPlayer.PLAYER_A)) {
                    gameOver = true;
                    winner = OctiPlayer.PLAYER_A;
                    return;
                }
                if (board.squareAt(x, 1).isCapturedBy(OctiPlayer.PLAYER_B)) {
                    gameOver = true;
                    winner = OctiPlayer.PLAYER_B;
                    return;
                }
            }

            int podsA = 0;
            int podsB = 0;
            for (int x = 0; x < board.getBoardWidth(); x++) {
                for (int y = 0; y < board.getBoardHeight(); y++) {
                    OctiSquare sq = board.squareAt(x, y);
                    for (int i = 0; i < sq.getNumPods(); i++) {
                        if (sq.getPodAt(i).getType() == OctiPodType.A_POD) podsA++;
                        else if (sq.getPodAt(i).getType() == OctiPodType.B_POD) podsB++;
                    }
                }
            }

            if (podsB == 0) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_A;
                return;
            }
            if (podsA == 0) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_B;
                return;
            }

            OctiPlayer nextP = (turn == OctiPlayer.PLAYER_A) ? OctiPlayer.PLAYER_B
                                                             : OctiPlayer.PLAYER_A;
            OctiPlayer savedTurn = turn;
            turn = nextP;
            List<OctiMove> moves = generateLegalMoves();
            turn = savedTurn;
            if (moves.isEmpty()) {
                gameOver = true;
                winner = turn;
                return;
            }
        } else if (mode == OctiGameMode.GAME_MODE_4) {
            boolean acBases = board.squareAt(2, 6).isCapturedByTeamOf(OctiPlayer.PLAYER_A)
                    && board.squareAt(6, 2).isCapturedByTeamOf(OctiPlayer.PLAYER_A);
            if (acBases) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_A;
                return;
            }

            boolean bdBases = board.squareAt(2, 2).isCapturedByTeamOf(OctiPlayer.PLAYER_B)
                    && board.squareAt(6, 6).isCapturedByTeamOf(OctiPlayer.PLAYER_B);
            if (bdBases) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_B;
                return;
            }

            int[] pods = new int[5];
            int bw4 = board.getBoardWidth();
            int bh4 = board.getBoardHeight();
            for (int x = 0; x < bw4; x++) {
                for (int y = 0; y < bh4; y++) {
                    OctiSquare sq = board.squareAt(x, y);
                    for (int i = 0; i < sq.getNumPods(); i++) {
                        OctiPlayer p = sq.getPodAt(i).getType().toPlayer();
                        if (p.getValue() >= 1 && p.getValue() <= 4) {
                            pods[p.getValue()]++;
                        }
                    }
                }
            }

            boolean bdElim = (players[OctiPlayer.PLAYER_B.getValue()].eliminated || pods[OctiPlayer.PLAYER_B.getValue()] == 0)
                    && (players[OctiPlayer.PLAYER_D.getValue()].eliminated || pods[OctiPlayer.PLAYER_D.getValue()] == 0);
            if (bdElim) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_A;
                return;
            }

            boolean acElim = (players[OctiPlayer.PLAYER_A.getValue()].eliminated || pods[OctiPlayer.PLAYER_A.getValue()] == 0)
                    && (players[OctiPlayer.PLAYER_C.getValue()].eliminated || pods[OctiPlayer.PLAYER_C.getValue()] == 0);
            if (acElim) {
                gameOver = true;
                winner = OctiPlayer.PLAYER_B;
                return;
            }
        }
    }

    private void advanceTurn() {
        if (mode == OctiGameMode.GAME_MODE_2 || mode == OctiGameMode.GAME_MODE_OFK) {
            turn = (turn == OctiPlayer.PLAYER_A) ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A;
        } else if (mode == OctiGameMode.GAME_MODE_4) {
            int current = turn.getValue();
            for (int step = 1; step <= 4; step++) {
                int nextP = (current - 1 + step) % 4 + 1;
                if (!players[nextP].eliminated) {
                    turn = OctiPlayer.fromValue(nextP);
                    break;
                }
            }
        }
    }

    public void undo() {
        if (undoStack.isEmpty()) {
            return;
        }

        OctiGameSnapshot currentSnap = captureSnapshot();
        OctiGameSnapshot prevSnap = undoStack.remove(undoStack.size() - 1);
        redoStack.add(currentSnap);

        if (!history.isEmpty()) {
            history.remove(history.size() - 1);
        }

        restoreSnapshot(prevSnap);

        if (board != null) {
            board.notifyBoardTurnUpdate(null);
        }
        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    public void redo() {
        if (redoStack.isEmpty()) {
            return;
        }

        OctiGameSnapshot targetSnap = redoStack.remove(redoStack.size() - 1);
        OctiGameSnapshot currentSnap = captureSnapshot();
        undoStack.add(currentSnap);

        if (targetSnap.lastPlayer != OctiPlayer.PLAYER_UNDEF && !targetSnap.lastMove.isEmpty()) {
            history.add(new MoveHistoryEntry(targetSnap.lastPlayer, targetSnap.lastMove));
        }

        restoreSnapshot(targetSnap);

        if (board != null) {
            board.notifyBoardTurnUpdate(null);
        }
        notifyGameUpdated();
        notifyUndoRedoStatusChanged();
    }

    public void prepareAddPod(int x, int y, boolean liberate) {
        if (!hasPreparedMove) {
            prepSnapshot = captureSnapshot();
            prepOps.clear();
            hasPreparedMove = true;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = x;
        op.y0 = y;
        op.addPod = true;
        op.liberate = liberate;
        prepOps.add(op);
    }

    public void prepareAddPod(int x, int y) {
        prepareAddPod(x, y, false);
    }

    public void prepareAddProng(int x, int y, OctiPod pod, OctiDir dir, boolean superProng) {
        if (!hasPreparedMove) {
            prepSnapshot = captureSnapshot();
            prepOps.clear();
            hasPreparedMove = true;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = x;
        op.y0 = y;
        op.pod = (pod != null) ? pod.copy() : new OctiPod();
        op.addProngDir = dir;
        op.superProng = superProng;
        prepOps.add(op);
    }

    public void prepareReposition(int x, int y, OctiPod pod, OctiDir srcDir, OctiDir tgtDir) {
        if (!hasPreparedMove) {
            prepSnapshot = captureSnapshot();
            prepOps.clear();
            hasPreparedMove = true;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = x;
        op.y0 = y;
        op.pod = (pod != null) ? pod.copy() : new OctiPod();
        op.rmProngDir = srcDir;
        op.addProngDir = tgtDir;
        prepOps.add(op);
    }

    public void prepareHops(int x, int y, OctiPod pod, List<OctiHop> hops) {
        if (!hasPreparedMove) {
            prepSnapshot = captureSnapshot();
            prepOps.clear();
            hasPreparedMove = true;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = x;
        op.y0 = y;
        op.pod = (pod != null) ? pod.copy() : new OctiPod();
        if (hops != null) {
            for (OctiHop h : hops) {
                op.hops.add(new OctiHop(h));
            }
        }
        prepOps.add(op);
    }

    public void prepareCapture(int x, int y) {
        if (!prepOps.isEmpty() && !prepOps.get(prepOps.size() - 1).hops.isEmpty()) {
            for (OctiHop h : prepOps.get(prepOps.size() - 1).hops) {
                if (h.x == x && h.y == y) {
                    h.capture = true;
                    break;
                }
            }
        }
    }

    public void commitPreparedMove() {
        if (!hasPreparedMove) {
            throw new IllegalMoveException("no prepared move to commit");
        }

        List<OctiMoveOp> ops = new ArrayList<>(prepOps);
        rollbackPreparedMove();

        OctiMove m = new OctiMove(ops);
        move(m);
    }

    public void rollbackPreparedMove() {
        if (hasPreparedMove) {
            restoreSnapshot(prepSnapshot);
            hasPreparedMove = false;
            prepOps.clear();
            notifyGameUpdated();
        }
    }

    public boolean hasPreparedMove() {
        return hasPreparedMove;
    }

    public String getPreparedMoveString() {
        if (!hasPreparedMove || prepOps.isEmpty()) {
            return "";
        }
        return OctiMove.formatOps(prepOps);
    }

    private static void dfsJumpChains(OctiBoard board,
                                      boolean edgeless,
                                      OctiPlayer turn,
                                      int cx, int cy,
                                      OctiPod pod,
                                      List<OctiPod> ghostPods,
                                      List<Coord> jumpedSquares,
                                      List<OctiHop> currentHops,
                                      int sx, int sy,
                                      List<OctiMove> outMoves) {
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                continue;
            }

            int dx = dir.deltaX() * 2;
            int dy = dir.deltaY() * 2;
            int jdx = dir.deltaX();
            int jdy = dir.deltaY();

            int jx = edgeless ? (((cx + jdx) % w + w) % w) : (cx + jdx);
            int jy = edgeless ? (((cy + jdy) % h + h) % h) : (cy + jdy);

            int lx = edgeless ? (((cx + dx) % w + w) % w) : (cx + dx);
            int ly = edgeless ? (((cy + dy) % h + h) % h) : (cy + dy);

            if (!edgeless) {
                if (jx < 0 || jx >= w || jy < 0 || jy >= h) continue;
                if (lx < 0 || lx >= w || ly < 0 || ly >= h) continue;
            }

            Coord jCoord = new Coord(jx, jy);
            if (jumpedSquares.contains(jCoord)) {
                continue;
            }

            OctiSquare jumpedSq = board.squareAt(jx, jy);
            if (isEmptyForJump(jumpedSq, jx, jy, sx, sy, ghostPods)) {
                continue;
            }

            OctiSquare landSq = board.squareAt(lx, ly);
            if (!isEmptyForJump(landSq, lx, ly, sx, sy, ghostPods)) {
                continue;
            }

            // Branch 1: Without capture
            {
                OctiHop hop = new OctiHop(lx, ly, false);
                currentHops.add(hop);
                jumpedSquares.add(jCoord);

                OctiMoveOp op = new OctiMoveOp();
                op.x0 = sx;
                op.y0 = sy;
                op.pod = pod.copy();
                for (OctiHop ch : currentHops) {
                    op.hops.add(new OctiHop(ch));
                }

                List<OctiMoveOp> singleOpList = new ArrayList<>();
                singleOpList.add(op);
                outMoves.add(new OctiMove(singleOpList));

                dfsJumpChains(board, edgeless, turn, lx, ly, pod, ghostPods,
                        jumpedSquares, currentHops, sx, sy, outMoves);

                jumpedSquares.remove(jumpedSquares.size() - 1);
                currentHops.remove(currentHops.size() - 1);
            }

            // Branch 2: With capture (if jumped square has enemy)
            if (jumpedSq.hasEnemyOf(turn)) {
                OctiHop hop = new OctiHop(lx, ly, true);
                currentHops.add(hop);
                jumpedSquares.add(jCoord);

                OctiMoveOp op = new OctiMoveOp();
                op.x0 = sx;
                op.y0 = sy;
                op.pod = pod.copy();
                for (OctiHop ch : currentHops) {
                    op.hops.add(new OctiHop(ch));
                }

                List<OctiMoveOp> singleOpList = new ArrayList<>();
                singleOpList.add(op);
                outMoves.add(new OctiMove(singleOpList));

                dfsJumpChains(board, edgeless, turn, lx, ly, pod, ghostPods,
                        jumpedSquares, currentHops, sx, sy, outMoves);

                jumpedSquares.remove(jumpedSquares.size() - 1);
                currentHops.remove(currentHops.size() - 1);
            }
        }
    }

    public void generateLegalMoves(List<OctiMove> outMoves) {
        if (outMoves == null) {
            return;
        }
        outMoves.clear();

        if (!gameStarted || gameOver || turn == OctiPlayer.PLAYER_UNDEF || board == null) {
            return;
        }

        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        int turnVal = turn.getValue();

        // 1. Pod placements & liberations
        if (mode != OctiGameMode.GAME_MODE_OFK) {
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    OctiSquare sq = board.squareAt(x, y);
                    if (sq.getType() == OctiSquareType.NEUTRAL_SQUARE) {
                        continue;
                    }

                    OctiSquareType st = sq.getType();
                    boolean isFriendlyHome = false;
                    if (board.getNumPlayers() == 2) {
                        isFriendlyHome = (turn == OctiPlayer.PLAYER_A && st == OctiSquareType.A_HOME_SQUARE)
                                || (turn == OctiPlayer.PLAYER_B && st == OctiSquareType.B_HOME_SQUARE);
                    } else if (board.getNumPlayers() == 4) {
                        isFriendlyHome = (st.getValue() == turn.getValue());
                    }

                    if (isFriendlyHome) {
                        if (sq.hasEnemyOf(turn)) {
                            continue;
                        }
                    } else {
                        if (sq.isEmpty() || sq.hasEnemyOf(turn)) {
                            continue;
                        }
                    }

                    if (mode == OctiGameMode.GAME_MODE_2) {
                        boolean isFriendlyBase = (sq.getType() == (turn == OctiPlayer.PLAYER_A
                                ? OctiSquareType.A_HOME_SQUARE : OctiSquareType.B_HOME_SQUARE));
                        if (isFriendlyBase) {
                            if (players[turnVal].freePods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                            if (board.canLiberate(turn) && players[turnVal].capturedPods > 0 && sq.isOccupiedBy(turn)) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = true;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                        } else {
                            // Enemy base
                            if (players[turnVal].freePods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                            if (players[turnVal].capturedPods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = true;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                        }
                    } else if (mode == OctiGameMode.GAME_MODE_4) {
                        OctiSquareType homeType = (turn == OctiPlayer.PLAYER_A ? OctiSquareType.A_HOME_SQUARE :
                                                   turn == OctiPlayer.PLAYER_B ? OctiSquareType.B_HOME_SQUARE :
                                                   turn == OctiPlayer.PLAYER_C ? OctiSquareType.C_HOME_SQUARE
                                                                              : OctiSquareType.D_HOME_SQUARE);
                        OctiPlayer partner = (turn == OctiPlayer.PLAYER_A ? OctiPlayer.PLAYER_C :
                                              turn == OctiPlayer.PLAYER_C ? OctiPlayer.PLAYER_A :
                                              turn == OctiPlayer.PLAYER_B ? OctiPlayer.PLAYER_D
                                                                         : OctiPlayer.PLAYER_B);
                        OctiSquareType partnerHome = (partner == OctiPlayer.PLAYER_A ? OctiSquareType.A_HOME_SQUARE :
                                                      partner == OctiPlayer.PLAYER_B ? OctiSquareType.B_HOME_SQUARE :
                                                      partner == OctiPlayer.PLAYER_C ? OctiSquareType.C_HOME_SQUARE
                                                                                     : OctiSquareType.D_HOME_SQUARE);

                        int partnerVal = partner.getValue();
                        if (sq.getType() == homeType) {
                            if (players[turnVal].freePods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                        } else if (sq.getType() == partnerHome) {
                            if (players[partnerVal].eliminated) {
                                if (players[partnerVal].capturedPods > 0 || players[turnVal].freePods > 0) {
                                    OctiMoveOp op = new OctiMoveOp();
                                    op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                    List<OctiMoveOp> opList = new ArrayList<>();
                                    opList.add(op);
                                    outMoves.add(new OctiMove(opList));
                                }
                            } else {
                                if (players[turnVal].freePods > 0) {
                                    OctiMoveOp op = new OctiMoveOp();
                                    op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                    List<OctiMoveOp> opList = new ArrayList<>();
                                    opList.add(op);
                                    outMoves.add(new OctiMove(opList));
                                }
                            }
                        } else {
                            // Enemy base or neutral OCTI square
                            if (players[turnVal].freePods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = false;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                            if (players[turnVal].capturedPods > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y; op.addPod = true; op.liberate = true;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                        }
                    }
                }
            }
        }

        // 2. Prong Additions, 3. Prong Repositions, 4. Distance-1 Moves, 5. Jumps
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                Set<Integer> seenConfigs = new HashSet<>();

                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    if (pod.getType() != turn.toPodType()) {
                        continue;
                    }
                    int prongsConf = pod.rawConfig() & 0xFFFF;
                    if (seenConfigs.contains(prongsConf)) {
                        continue;
                    }
                    seenConfigs.add(prongsConf);

                    // 2. Prong Additions
                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                            if (players[turnVal].freeProngs > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y;
                                op.pod = pod.copy();
                                op.addProngDir = dir;
                                op.superProng = false;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                            if (useSuperProngs && players[turnVal].freeSuperProngs > 0) {
                                OctiMoveOp op = new OctiMoveOp();
                                op.x0 = x; op.y0 = y;
                                op.pod = pod.copy();
                                op.addProngDir = dir;
                                op.superProng = true;
                                List<OctiMoveOp> opList = new ArrayList<>();
                                opList.add(op);
                                outMoves.add(new OctiMove(opList));
                            }
                        }
                    }

                    // 3. Prong Repositions
                    if (mode != OctiGameMode.GAME_MODE_OFK) {
                        for (int rm = 0; rm < OctiDir.DIR_MAX; rm++) {
                            OctiDir rmDir = OctiDir.fromIndex(rm);
                            if (pod.prongAt(rmDir) == OctiProngType.NORMAL_PRONG) {
                                for (int add = 0; add < OctiDir.DIR_MAX; add++) {
                                    OctiDir addDir = OctiDir.fromIndex(add);
                                    if (addDir != rmDir && pod.prongAt(addDir) == OctiProngType.NO_PRONG) {
                                        OctiMoveOp op = new OctiMoveOp();
                                        op.x0 = x; op.y0 = y;
                                        op.pod = pod.copy();
                                        op.rmProngDir = rmDir;
                                        op.addProngDir = addDir;
                                        op.superProng = false;
                                        List<OctiMoveOp> opList = new ArrayList<>();
                                        opList.add(op);
                                        outMoves.add(new OctiMove(opList));
                                    }
                                }
                            }
                        }
                    }

                    // 4. Distance-1 Moves
                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        if (pod.prongAt(dir) != OctiProngType.NO_PRONG) {
                            int dx = dir.deltaX();
                            int dy = dir.deltaY();
                            int nx = useEdgelessBoard ? (((x + dx) % w + w) % w) : (x + dx);
                            int ny = useEdgelessBoard ? (((y + dy) % h + h) % h) : (y + dy);

                            if (!useEdgelessBoard) {
                                if (nx < 0 || nx >= w || ny < 0 || ny >= h) {
                                    continue;
                                }
                            }

                            OctiSquare destSq = board.squareAt(nx, ny);
                            if (mode == OctiGameMode.GAME_MODE_OFK) {
                                if (!destSq.isEmpty()) continue;
                            } else {
                                if (destSq.hasEnemyOf(turn)) continue;
                            }

                            OctiHop hop = new OctiHop(nx, ny, false);
                            OctiMoveOp op = new OctiMoveOp();
                            op.x0 = x; op.y0 = y;
                            op.pod = pod.copy();
                            op.hops.add(hop);
                            List<OctiMoveOp> opList = new ArrayList<>();
                            opList.add(op);
                            outMoves.add(new OctiMove(opList));
                        }
                    }

                    // 5. Distance-2 Jumps
                    List<OctiPod> ghostPods = new ArrayList<>(sq.pods());
                    List<Coord> jumpedSquares = new ArrayList<>();
                    List<OctiHop> currentHops = new ArrayList<>();
                    dfsJumpChains(board, useEdgelessBoard, turn, x, y, pod, ghostPods,
                            jumpedSquares, currentHops, x, y, outMoves);
                }
            }
        }
    }

    public List<OctiMove> generateLegalMoves() {
        List<OctiMove> moves = new ArrayList<>();
        generateLegalMoves(moves);
        return moves;
    }

    public String serialize() {
        StringBuilder out = new StringBuilder();
        out.append("; OCTI Game Transcript v2.0\n");

        if (mode == OctiGameMode.GAME_MODE_2) {
            out.append("GAME_MODE: 2\n");
        } else if (mode == OctiGameMode.GAME_MODE_4) {
            out.append("GAME_MODE: 4\n");
        } else if (mode == OctiGameMode.GAME_MODE_OFK) {
            out.append("GAME_MODE: OFK\n");
        }

        out.append("GAME_BASES: ").append(bases.getValue()).append("\n");
        out.append("USE_SUPER_PRONGS: ").append(useSuperProngs ? "Yes" : "No").append("\n");
        out.append("USE_EDGELESS_BOARD: ").append(useEdgelessBoard ? "Yes" : "No").append("\n");

        out.append("START_PLAYER: ");
        switch (startPlayer) {
            case PLAYER_A:
                out.append("A\n");
                break;
            case PLAYER_B:
                out.append("B\n");
                break;
            case PLAYER_C:
                out.append("C\n");
                break;
            case PLAYER_D:
                out.append("D\n");
                break;
            default:
                out.append("A\n");
                break;
        }

        out.append("STATUS: ");
        if (!gameStarted) {
            out.append("NOT_STARTED\n");
        } else if (gameResigned) {
            out.append("RESIGNED\n");
        } else if (gameOver) {
            out.append("GAME_OVER\n");
        } else {
            out.append("IN_PROGRESS\n");
        }

        out.append("WINNER: ");
        switch (winner) {
            case PLAYER_A:
                out.append("A\n");
                break;
            case PLAYER_B:
                out.append("B\n");
                break;
            case PLAYER_C:
                out.append("C\n");
                break;
            case PLAYER_D:
                out.append("D\n");
                break;
            default:
                out.append("NONE\n");
                break;
        }

        out.append("MOVES: ").append(history.size()).append("\n");
        for (MoveHistoryEntry entry : history) {
            out.append(entry.getMove().asString()).append("\n");
        }

        return out.toString();
    }

    public void deserialize(String transcript) {
        if (board != null) {
            board.deserializeGame(transcript);
        }
    }

    public OctiGameSnapshot captureSnapshot() {
        OctiGameSnapshot snap = new OctiGameSnapshot();
        captureSnapshotInto(snap);
        return snap;
    }

    public void captureSnapshotInto(OctiGameSnapshot snap) {
        if (snap == null) {
            return;
        }
        if (board != null) {
            int w = board.getBoardWidth();
            int h = board.getBoardHeight();
            for (int x = 0; x < w; x++) {
                for (int y = 0; y < h; y++) {
                    List<OctiPod> dst = snap.squares[x][y];
                    dst.clear();
                    List<OctiPod> src = board.squareAt(x, y).pods();
                    for (int i = 0, n = src.size(); i < n; i++) {
                        dst.add(src.get(i).copy());
                    }
                }
            }
        }
        for (int p = 0; p < 5; p++) {
            OctiPlayerState dstP = snap.players[p];
            OctiPlayerState srcP = players[p];
            if (dstP != null && srcP != null) {
                dstP.freePods = srcP.freePods;
                dstP.capturedPods = srcP.capturedPods;
                dstP.freeProngs = srcP.freeProngs;
                dstP.freeSuperProngs = srcP.freeSuperProngs;
                dstP.eliminated = srcP.eliminated;
                dstP.timeRemainingMs = srcP.timeRemainingMs;
            }
        }
        snap.turn = turn;
        snap.winner = winner;
        snap.gameStarted = gameStarted;
        snap.gameOver = gameOver;
        snap.gameResigned = gameResigned;
        snap.searchPlyOffset = searchPlyOffset;
    }

    public void restoreSnapshot(OctiGameSnapshot snap) {
        if (snap == null) {
            return;
        }

        if (board != null) {
            int w = board.getBoardWidth();
            int h = board.getBoardHeight();
            for (int x = 0; x < w; x++) {
                for (int y = 0; y < h; y++) {
                    OctiSquare sq = board.squareAt(x, y);
                    sq.clear();
                    List<OctiPod> src = snap.squares[x][y];
                    if (src != null) {
                        for (int i = 0, n = src.size(); i < n; i++) {
                            sq.addPod(src.get(i).copy());
                        }
                    }
                }
            }
        }
        for (int p = 0; p < 5; p++) {
            OctiPlayerState srcP = snap.players[p];
            OctiPlayerState dstP = players[p];
            if (srcP != null && dstP != null) {
                dstP.freePods = srcP.freePods;
                dstP.capturedPods = srcP.capturedPods;
                dstP.freeProngs = srcP.freeProngs;
                dstP.freeSuperProngs = srcP.freeSuperProngs;
                dstP.eliminated = srcP.eliminated;
                dstP.timeRemainingMs = srcP.timeRemainingMs;
            }
        }
        this.turn = snap.turn;
        this.winner = snap.winner;
        this.gameStarted = snap.gameStarted;
        this.gameOver = snap.gameOver;
        this.gameResigned = snap.gameResigned;
        this.searchPlyOffset = snap.searchPlyOffset;
    }
}
