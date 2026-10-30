/*
 * OctiBotEngine.java
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
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class OctiBotEngine {

    public enum BotDifficulty {
        CASUAL(1),
        TACTICAL(2),
        MASTER(3);

        private final int level;

        BotDifficulty(int level) {
            this.level = level;
        }

        public int getLevel() {
            return level;
        }
    }

    public static final class BotSearchStats {
        public int depthReached = 0;
        public long nodesVisited = 0;
        public int bestScore = 0;
        public long elapsedMs = 0;
    }

    private static final int WIN_SCORE = 1_000_000;
    private static final int INF_SCORE = 2_000_000;
    public static final int TT_SIZE = 262144; // 1 << 18 (256K entries)
    private static final int TT_MASK = TT_SIZE - 1;
    private static final int TT_LOCK_COUNT = 1024;
    private static final int TT_LOCK_MASK = TT_LOCK_COUNT - 1;

    public static final class TranspositionEntry {
        public volatile long key = 0L;
        public volatile int depth = -1;
        public volatile int score = 0;
        public volatile int flag = 0; // 0 = exact, 1 = lowerbound, 2 = upperbound
        public volatile String bestMove = "";
    }

    private static final class PathKey {
        final int x0;
        final int y0;
        final int podConf;
        final List<Integer> hopPackedCoords;

        PathKey(int x0, int y0, int podConf, List<Integer> hopPackedCoords) {
            this.x0 = x0;
            this.y0 = y0;
            this.podConf = podConf;
            this.hopPackedCoords = hopPackedCoords;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof PathKey)) {
                return false;
            }
            PathKey other = (PathKey) obj;
            return x0 == other.x0
                    && y0 == other.y0
                    && podConf == other.podConf
                    && Objects.equals(hopPackedCoords, other.hopPackedCoords);
        }

        @Override
        public int hashCode() {
            return Objects.hash(x0, y0, podConf, hopPackedCoords);
        }
    }

    // Direct Binary 64-bit Zobrist Tables
    private static final long[][][][] ZOB_POD_PRESENCE = new long[9][9][7][5];
    private static final long[][][][][][] ZOB_PRONG = new long[9][9][7][5][9][3];
    private static final long[][][] ZOB_BASE_CAPTURED = new long[9][9][5];
    private static final long[] ZOB_TURN = new long[5];
    private static final long[] ZOB_MODE = new long[5];
    private static final long[][][] ZOB_RESERVE = new long[5][4][32];

    static {
        long seed = 0x5EED2026_C0DEC001L;
        // SplitMix64 generator for deterministic, high-entropy 64-bit Zobrist keys
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                for (int s = 0; s < 7; s++) {
                    for (int p = 0; p < 5; p++) {
                        seed += 0x9E3779B97F4A7C15L;
                        long z = seed;
                        z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
                        z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
                        ZOB_POD_PRESENCE[x][y][s][p] = z ^ (z >>> 31);

                        for (int d = 0; d < 9; d++) {
                            for (int pt = 0; pt < 3; pt++) {
                                seed += 0x9E3779B97F4A7C15L;
                                z = seed;
                                z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
                                z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
                                ZOB_PRONG[x][y][s][p][d][pt] = z ^ (z >>> 31);
                            }
                        }
                    }
                }
                for (int p = 0; p < 5; p++) {
                    seed += 0x9E3779B97F4A7C15L;
                    long z = seed;
                    z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
                    z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
                    ZOB_BASE_CAPTURED[x][y][p] = z ^ (z >>> 31);
                }
            }
        }
        for (int p = 0; p < 5; p++) {
            seed += 0x9E3779B97F4A7C15L;
            long z = seed;
            z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
            z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
            ZOB_TURN[p] = z ^ (z >>> 31);
        }
        for (int m = 0; m < 5; m++) {
            seed += 0x9E3779B97F4A7C15L;
            long z = seed;
            z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
            z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
            ZOB_MODE[m] = z ^ (z >>> 31);
        }
        for (int p = 0; p < 5; p++) {
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 32; c++) {
                    seed += 0x9E3779B97F4A7C15L;
                    long z = seed;
                    z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
                    z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
                    ZOB_RESERVE[p][r][c] = z ^ (z >>> 31);
                }
            }
        }
    }

    private BotDifficulty difficulty;
    private int timeBudgetMs;
    private boolean customTimeBudget;
    private long rngState;
    private final long[] ttKeys = new long[TT_SIZE];
    private final int[] ttDepths = new int[TT_SIZE];
    private final int[] ttScores = new int[TT_SIZE];
    private final byte[] ttFlags = new byte[TT_SIZE];
    private final String[] ttBestMoves = new String[TT_SIZE];
    private final Object[] ttLocks = new Object[TT_LOCK_COUNT];

    private final java.util.concurrent.atomic.AtomicInteger liveDepth =
            new java.util.concurrent.atomic.AtomicInteger(0);
    private final java.util.concurrent.atomic.AtomicLong liveNodes =
            new java.util.concurrent.atomic.AtomicLong(0L);
    private final java.util.concurrent.atomic.AtomicInteger liveScore =
            new java.util.concurrent.atomic.AtomicInteger(0);

    public int liveDepthReached() {
        return liveDepth.get();
    }

    public long liveNodesVisited() {
        return liveNodes.get();
    }

    public int liveBestScore() {
        return liveScore.get();
    }

    public OctiBotEngine() {
        this(BotDifficulty.TACTICAL, 0x5EED2026L);
    }

    public OctiBotEngine(BotDifficulty difficulty) {
        this(difficulty, 0x5EED2026L);
    }

    public OctiBotEngine(BotDifficulty difficulty, long rngSeed) {
        this.difficulty = (difficulty != null) ? difficulty : BotDifficulty.TACTICAL;
        this.timeBudgetMs = defaultBudgetForDifficulty(this.difficulty);
        this.customTimeBudget = false;
        this.rngState = (rngSeed != 0L) ? rngSeed : 0x5EED2026L;
        java.util.Arrays.fill(ttDepths, -1);
        for (int i = 0; i < TT_LOCK_COUNT; i++) {
            ttLocks[i] = new Object();
        }
    }

    public void setDifficulty(BotDifficulty d) {
        this.difficulty = (d != null) ? d : BotDifficulty.TACTICAL;
        if (!customTimeBudget) {
            this.timeBudgetMs = defaultBudgetForDifficulty(this.difficulty);
        }
    }

    public BotDifficulty difficulty() {
        return difficulty;
    }

    public BotDifficulty getDifficulty() {
        return difficulty;
    }

    public void setTimeBudgetMs(int maxTimeMs) {
        this.timeBudgetMs = Math.max(10, maxTimeMs);
        this.customTimeBudget = true;
    }

    public boolean isCustomTimeBudget() {
        return customTimeBudget;
    }

    public void restoreDefaultBudget() {
        this.customTimeBudget = false;
        this.timeBudgetMs = defaultBudgetForDifficulty(this.difficulty);
    }

    public int timeBudgetMs() {
        return timeBudgetMs;
    }

    public int getTimeBudgetMs() {
        return timeBudgetMs;
    }

    public static int defaultBudgetForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 8000;
        }
        switch (d) {
            case CASUAL:
                return 3000;
            case TACTICAL:
                return 8000;
            case MASTER:
                return 15000;
            default:
                return 8000;
        }
    }

    public static int maxDepthForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 7;
        }
        switch (d) {
            case CASUAL:
                return 5;
            case TACTICAL:
                return 7;
            case MASTER:
                return 9;
            default:
                return 7;
        }
    }

    public static int rootCapForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 40;
        }
        switch (d) {
            case CASUAL:
                return 28;
            case TACTICAL:
                return 40;
            case MASTER:
                return 56;
            default:
                return 40;
        }
    }

    public static int branchCapForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 24;
        }
        switch (d) {
            case CASUAL:
                return 18;
            case TACTICAL:
                return 24;
            case MASTER:
                return 32;
            default:
                return 24;
        }
    }

    private long nextRandom() {
        long x = rngState;
        x ^= (x << 13);
        x ^= (x >>> 7);
        x ^= (x << 17);
        rngState = x;
        return x;
    }

    private static long steadyClockMs() {
        return System.nanoTime() / 1_000_000L;
    }

    /**
     * Computes a fast, deterministic 64-bit Zobrist binary hash over square pod configs,
     * captured base flags, reserve counts, turn, and game mode (zero string allocations!).
     */
    public static long computePositionHash(OctiBoard board) {
        if (board == null) {
            return 0L;
        }
        long h = 0xCBF29CE484222325L;
        OctiGame game = board.getGame();
        if (game == null) {
            return h;
        }

        OctiGameMode mode = board.getGameMode();
        h ^= ZOB_MODE[mode != null ? mode.ordinal() : 0];

        OctiPlayer turn = game.getTurn();
        int turnVal = (turn != null) ? turn.getValue() : 0;
        if (turnVal >= 0 && turnVal < 5) {
            h ^= ZOB_TURN[turnVal];
        }

        int w = board.getBoardWidth();
        int hRows = board.getBoardHeight();

        for (int y = 0; y < hRows; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                if (sq.isCaptured() && sq.getNumPods() > 0) {
                    OctiPlayer capPlayer = sq.getPodAt(sq.getNumPods() - 1).getType().toPlayer();
                    int cpVal = (capPlayer != null) ? capPlayer.getValue() : 0;
                    if (cpVal >= 0 && cpVal < 5) {
                        h ^= ZOB_BASE_CAPTURED[x][y][cpVal];
                    }
                }
                int numPods = sq.getNumPods();
                for (int s = 0; s < numPods && s < 7; s++) {
                    OctiPod pod = sq.getPodAt(s);
                    OctiPlayer owner = pod.getType().toPlayer();
                    int oVal = (owner != null) ? owner.getValue() : 0;
                    if (oVal >= 0 && oVal < 5) {
                        h ^= ZOB_POD_PRESENCE[x][y][s][oVal];
                        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                            OctiDir dir = OctiDir.fromIndex(d);
                            OctiProngType pt = pod.prongAt(dir);
                            if (pt != OctiProngType.NO_PRONG) {
                                h ^= ZOB_PRONG[x][y][s][oVal][d][pt.getValue()];
                            }
                        }
                    }
                }
            }
        }

        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pIdx = 1; pIdx <= numPlayers; pIdx++) {
            OctiPlayer p = OctiPlayer.fromValue(pIdx);
            int fp = Math.min(31, Math.max(0, game.getFreePods(p)));
            int cp = Math.min(31, Math.max(0, game.getCapturedPods(p)));
            int fpr = Math.min(31, Math.max(0, game.getFreeProngs(p)));
            int fspr = Math.min(31, Math.max(0, game.getFreeSuperProngs(p)));
            h ^= ZOB_RESERVE[pIdx][0][fp];
            h ^= ZOB_RESERVE[pIdx][1][cp];
            h ^= ZOB_RESERVE[pIdx][2][fpr];
            h ^= ZOB_RESERVE[pIdx][3][fspr];
        }

        return h;
    }

    private static boolean isEnemyHomeSquareFor(OctiSquareType sqType,
                                                OctiGameMode mode,
                                                OctiPlayer p) {
        if (mode == OctiGameMode.GAME_MODE_2 || mode == OctiGameMode.GAME_MODE_OFK) {
            if (p == OctiPlayer.PLAYER_A) {
                return sqType == OctiSquareType.B_HOME_SQUARE;
            }
            if (p == OctiPlayer.PLAYER_B) {
                return sqType == OctiSquareType.A_HOME_SQUARE;
            }
            return false;
        }
        if (mode == OctiGameMode.GAME_MODE_4) {
            if (p == OctiPlayer.PLAYER_A || p == OctiPlayer.PLAYER_C) {
                return sqType == OctiSquareType.B_HOME_SQUARE
                        || sqType == OctiSquareType.D_HOME_SQUARE;
            }
            if (p == OctiPlayer.PLAYER_B || p == OctiPlayer.PLAYER_D) {
                return sqType == OctiSquareType.A_HOME_SQUARE
                        || sqType == OctiSquareType.C_HOME_SQUARE;
            }
        }
        return false;
    }

    private static boolean isAlliedHomeSquareFor(OctiSquareType sqType,
                                                 OctiGameMode mode,
                                                 OctiPlayer p) {
        if (mode == OctiGameMode.GAME_MODE_2 || mode == OctiGameMode.GAME_MODE_OFK) {
            if (p == OctiPlayer.PLAYER_A) {
                return sqType == OctiSquareType.A_HOME_SQUARE;
            }
            if (p == OctiPlayer.PLAYER_B) {
                return sqType == OctiSquareType.B_HOME_SQUARE;
            }
            return false;
        }
        if (mode == OctiGameMode.GAME_MODE_4) {
            if (p == OctiPlayer.PLAYER_A || p == OctiPlayer.PLAYER_C) {
                return sqType == OctiSquareType.A_HOME_SQUARE
                        || sqType == OctiSquareType.C_HOME_SQUARE;
            }
            if (p == OctiPlayer.PLAYER_B || p == OctiPlayer.PLAYER_D) {
                return sqType == OctiSquareType.B_HOME_SQUARE
                        || sqType == OctiSquareType.D_HOME_SQUARE;
            }
        }
        return false;
    }

    public static List<int[]> targetEnemyBases(OctiBoard board, OctiPlayer p) {
        List<int[]> uncaptured = new ArrayList<>();
        List<int[]> allEnemy = new ArrayList<>();
        if (board == null || p == null) {
            return uncaptured;
        }
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                if (isEnemyHomeSquareFor(sq.getType(), mode, p)) {
                    int[] coord = new int[]{x, y};
                    allEnemy.add(coord);
                    if (!sq.isCapturedByTeamOf(p)) {
                        uncaptured.add(coord);
                    }
                }
            }
        }
        return !uncaptured.isEmpty() ? uncaptured : allEnemy;
    }

    private static int torusDist(int a, int b, int dim, boolean edgeless) {
        int d = Math.abs(a - b);
        if (edgeless && dim > 0) {
            d = Math.min(d, dim - d);
        }
        return d;
    }

    public static int minChebyshevToTargets(int x, int y,
                                            List<int[]> targets,
                                            int w, int h,
                                            boolean edgeless) {
        if (targets.isEmpty()) {
            return 0;
        }
        int best = 999;
        for (int[] t : targets) {
            int dx = torusDist(x, t[0], w, edgeless);
            int dy = torusDist(y, t[1], h, edgeless);
            int dist = Math.max(dx, dy);
            if (dist < best) {
                best = dist;
            }
        }
        return best;
    }

    private static List<OctiMove> filterDominatedMoves(List<OctiMove> moves) {
        if (moves.size() <= 1) {
            return moves;
        }

        Set<PathKey> hasCapturingVariant = new HashSet<>();
        for (OctiMove m : moves) {
            List<OctiMoveOp> ops = m.getOps();
            if (ops.size() != 1 || ops.get(0).hops.isEmpty()) {
                continue;
            }
            OctiMoveOp op = ops.get(0);
            boolean anyCap = false;
            List<Integer> coords = new ArrayList<>(op.hops.size());
            for (OctiHop h : op.hops) {
                coords.add(h.x * 16 + h.y);
                if (h.capture) {
                    anyCap = true;
                }
            }
            if (anyCap) {
                hasCapturingVariant.add(new PathKey(op.x0, op.y0, op.pod.rawConfig() & 0xFFFF, coords));
            }
        }

        if (hasCapturingVariant.isEmpty()) {
            return moves;
        }

        List<OctiMove> filtered = new ArrayList<>(moves.size());
        for (OctiMove m : moves) {
            List<OctiMoveOp> ops = m.getOps();
            if (ops.size() == 1 && !ops.get(0).hops.isEmpty()) {
                OctiMoveOp op = ops.get(0);
                boolean anyCap = false;
                List<Integer> coords = new ArrayList<>(op.hops.size());
                for (OctiHop h : op.hops) {
                    coords.add(h.x * 16 + h.y);
                    if (h.capture) {
                        anyCap = true;
                    }
                }
                if (!anyCap) {
                    PathKey key = new PathKey(op.x0, op.y0, op.pod.rawConfig() & 0xFFFF, coords);
                    if (hasCapturingVariant.contains(key)) {
                        continue;
                    }
                }
            }
            filtered.add(m);
        }

        return filtered.isEmpty() ? moves : filtered;
    }

    /**
     * Appends legal 2-pod (and 3-pod) composite moves from any square (x, y)
     * containing 2+ friendly pods of `turn` with prongs.
     */
    public static void appendCompositeMoves(OctiBoard board, OctiPlayer turn, List<OctiMove> outMoves) {
        if (board == null || board.getGame() == null || turn == null || turn == OctiPlayer.PLAYER_UNDEF) {
            return;
        }
        OctiGame game = board.getGame();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = game.useEdgelessBoard();
        OctiGameSnapshot snap = game.captureSnapshot();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                List<OctiPod> eligiblePods = new ArrayList<>();
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    if (pod.getType() == turn.toPodType() && pod.getNumProngs() > 0) {
                        eligiblePods.add(pod.copy());
                    }
                }
                if (eligiblePods.size() < 2) {
                    continue;
                }

                // Generate candidate single ops for each eligible pod on this square
                List<List<OctiMoveOp>> perPodOps = new ArrayList<>(eligiblePods.size());
                for (OctiPod pod : eligiblePods) {
                    List<OctiMoveOp> podOps = new ArrayList<>();
                    // Distance-1 hops
                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        if (pod.prongAt(dir) != OctiProngType.NO_PRONG) {
                            int nx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
                            int ny = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
                            if (board.isValidCoord(nx, ny)) {
                                OctiSquare destSq = board.squareAt(nx, ny);
                                if (!destSq.hasEnemyOf(turn)) {
                                    OctiMoveOp op = new OctiMoveOp();
                                    op.x0 = x;
                                    op.y0 = y;
                                    op.pod = pod.copy();
                                    op.hops.add(new OctiHop(nx, ny, false));
                                    podOps.add(op);
                                }
                            }
                        }
                    }
                    // Distance-2 Jumps
                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        if (pod.prongAt(dir) != OctiProngType.NO_PRONG) {
                            int jx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
                            int jy = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
                            int lx = edgeless ? (((x + 2 * dir.deltaX()) % w + w) % w) : (x + 2 * dir.deltaX());
                            int ly = edgeless ? (((y + 2 * dir.deltaY()) % h + h) % h) : (y + 2 * dir.deltaY());
                            if (board.isValidCoord(jx, jy) && board.isValidCoord(lx, ly)) {
                                OctiSquare jumpedSq = board.squareAt(jx, jy);
                                OctiSquare landSq = board.squareAt(lx, ly);
                                if (!jumpedSq.isEmpty() && (!landSq.hasEnemyOf(turn))) {
                                    // Landing without capture
                                    OctiMoveOp opNoCap = new OctiMoveOp();
                                    opNoCap.x0 = x;
                                    opNoCap.y0 = y;
                                    opNoCap.pod = pod.copy();
                                    opNoCap.hops.add(new OctiHop(lx, ly, false));
                                    podOps.add(opNoCap);

                                    // Landing with capture
                                    if (jumpedSq.hasEnemyOf(turn)) {
                                        OctiMoveOp opCap = new OctiMoveOp();
                                        opCap.x0 = x;
                                        opCap.y0 = y;
                                        opCap.pod = pod.copy();
                                        opCap.hops.add(new OctiHop(lx, ly, true));
                                        podOps.add(opCap);
                                    }
                                }
                            }
                        }
                    }
                    perPodOps.add(podOps);
                }

                // Combine pairs of ops for pod 0 and pod 1 (and 3-pod if available)
                int addedForSquare = 0;
                int maxPerSquare = 20;

                for (int p1 = 0; p1 < perPodOps.size() && addedForSquare < maxPerSquare; p1++) {
                    for (int p2 = p1 + 1; p2 < perPodOps.size() && addedForSquare < maxPerSquare; p2++) {
                        List<OctiMoveOp> list1 = perPodOps.get(p1);
                        List<OctiMoveOp> list2 = perPodOps.get(p2);

                        for (OctiMoveOp op1 : list1) {
                            for (OctiMoveOp op2 : list2) {
                                // Conflict rejection: cannot jump/capture the same enemy square twice
                                boolean conflict = false;
                                for (OctiHop h1 : op1.hops) {
                                    if (!h1.capture) continue;
                                    for (OctiHop h2 : op2.hops) {
                                        if (h2.capture && h1.x == h2.x && h1.y == h2.y) {
                                            conflict = true;
                                            break;
                                        }
                                    }
                                    if (conflict) break;
                                }
                                if (conflict) {
                                    continue;
                                }

                                List<OctiMoveOp> compOps = new ArrayList<>(2);
                                compOps.add(new OctiMoveOp(op1));
                                compOps.add(new OctiMoveOp(op2));
                                OctiMove compMove = new OctiMove(compOps);

                                // Validate against scratch board state
                                try {
                                    game.move(compMove);
                                    outMoves.add(compMove);
                                    addedForSquare++;
                                } catch (Throwable ignored) {
                                } finally {
                                    game.restoreSnapshot(snap);
                                }
                                if (addedForSquare >= maxPerSquare) {
                                    break;
                                }
                            }
                            if (addedForSquare >= maxPerSquare) {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    private static boolean isOrthogonalDir(OctiDir dir) {
        return dir == OctiDir.DIR_N || dir == OctiDir.DIR_E
                || dir == OctiDir.DIR_S || dir == OctiDir.DIR_W;
    }

    private static boolean isDiagonalDir(OctiDir dir) {
        return dir == OctiDir.DIR_NE || dir == OctiDir.DIR_SE
                || dir == OctiDir.DIR_SW || dir == OctiDir.DIR_NW;
    }

    private static boolean hasUnarmedPodOnHomeSquare(OctiBoard board, OctiPlayer turn) {
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                if (!isAlliedHomeSquareFor(sq.getType(), mode, turn)) {
                    continue;
                }
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    if (pod.getType() == turn.toPodType() && pod.getNumProngs() < 2) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private static int scoreMoveHeuristic(OctiBoard board,
                                          OctiMove move,
                                          OctiPlayer turn,
                                          List<int[]> enemyTargets) {
        OctiGame game = board.getGame();
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = (game != null && game.useEdgelessBoard());
        boolean earlyDevelopment = (game != null && game.getFreeProngs(turn) >= 8);

        int score = 0;
        List<OctiMoveOp> ops = move.getOps();

        // High priority for composite multi-pod coordinated moves!
        if (ops.size() > 1) {
            score += 8000 + (ops.size() - 1) * 4000;
        }

        for (OctiMoveOp op : ops) {
            if (!op.hops.isEmpty()) {
                int startDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
                OctiHop lastHop = op.hops.get(op.hops.size() - 1);
                int endDist = minChebyshevToTargets(lastHop.x, lastHop.y, enemyTargets, w, h, edgeless);

                score += (startDist - endDist) * 550;

                OctiSquare destSq = board.squareAt(lastHop.x, lastHop.y);
                if (isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                        && !destSq.isCapturedByTeamOf(turn)) {
                    score += 55000;
                }
                if (destSq.getType() == OctiSquareType.NEUTRAL_OCTI_SQUARE && destSq.isEmpty()) {
                    score += 3000;
                }

                for (OctiHop hop : op.hops) {
                    if (hop.capture) {
                        score += 14000;
                    } else {
                        score += 250;
                    }
                }

                OctiSquare startSq = board.squareAt(op.x0, op.y0);
                if (isEnemyHomeSquareFor(startSq.getType(), mode, turn)) {
                    if (startSq.getNumPods() <= 1
                            && !isEnemyHomeSquareFor(destSq.getType(), mode, turn)) {
                        score -= 22000;
                    }
                }

                // Avoid stacking multiple friendly pods on non-enemy-base squares
                if (!isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                        && destSq.getNumPods() >= 1) {
                    score -= 1200 * destSq.getNumPods();
                }

                // Penalize premature 1-prong lone-pod rushes during development
                if (earlyDevelopment && op.pod.getNumProngs() <= 1 && endDist <= 3) {
                    score -= 3200;
                }
            } else if (op.addProngDir != OctiDir.DIR_UNDEF && op.rmProngDir == OctiDir.DIR_UNDEF) {
                score += 650;
                if (op.superProng) {
                    score += 200;
                }
                int nx = op.x0 + op.addProngDir.deltaX();
                int ny = op.y0 + op.addProngDir.deltaY();
                int curDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
                int nextDist = minChebyshevToTargets(nx, ny, enemyTargets, w, h, edgeless);
                if (nextDist < curDist) {
                    score += 600;
                }
                if (op.pod.getNumProngs() == 0) {
                    score += 900; // Strongly prioritize arming 0-prong pods first!
                } else if (op.pod.getNumProngs() < 3) {
                    score += 450;
                }

                // Check orthogonal + diagonal versatility so pods don't get stuck on one parity
                boolean hasOrth = isOrthogonalDir(op.addProngDir);
                boolean hasDiag = isDiagonalDir(op.addProngDir);
                for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                    OctiDir dir = OctiDir.fromIndex(d);
                    if (op.pod.prongAt(dir) != OctiProngType.NO_PRONG) {
                        if (isOrthogonalDir(dir)) hasOrth = true;
                        if (isDiagonalDir(dir)) hasDiag = true;
                    }
                }
                if (hasOrth && hasDiag) {
                    score += 650;
                }

                // Prioritize straight-forward prong highest, then forward diagonals
                if (turn == OctiPlayer.PLAYER_A || turn == OctiPlayer.PLAYER_C) {
                    if (op.addProngDir == OctiDir.DIR_N) {
                        score += 1100;
                    } else if (op.addProngDir == OctiDir.DIR_NE || op.addProngDir == OctiDir.DIR_NW) {
                        score += 750;
                    } else if (op.addProngDir == OctiDir.DIR_E || op.addProngDir == OctiDir.DIR_W) {
                        score += 400;
                    }
                } else if (turn == OctiPlayer.PLAYER_B || turn == OctiPlayer.PLAYER_D) {
                    if (op.addProngDir == OctiDir.DIR_S) {
                        score += 1100;
                    } else if (op.addProngDir == OctiDir.DIR_SE || op.addProngDir == OctiDir.DIR_SW) {
                        score += 750;
                    } else if (op.addProngDir == OctiDir.DIR_E || op.addProngDir == OctiDir.DIR_W) {
                        score += 400;
                    }
                }
            } else if (op.addPod) {
                if (op.liberate) {
                    score += 6500;
                } else {
                    OctiSquare sq = board.squareAt(op.x0, op.y0);
                    if (sq.getNumPods() > 0 || hasUnarmedPodOnHomeSquare(board, turn)) {
                        // Never dump bare reserve pods onto occupied home squares or while home pods lack prongs
                        score -= 6000;
                    } else {
                        score += 150;
                    }
                }
            } else if (op.rmProngDir != OctiDir.DIR_UNDEF) {
                score += 60;
            }
        }
        return score;
    }

    private static List<OctiMove> orderAndCapMoves(OctiBoard board,
                                                   List<OctiMove> rawMoves,
                                                   OctiPlayer turn,
                                                   String ttBestMove,
                                                   int maxBranch) {
        List<OctiMove> moves = filterDominatedMoves(rawMoves);
        List<int[]> targets = targetEnemyBases(board, turn);

        int n = moves.size();
        int[][] scored = new int[n][2];
        for (int i = 0; i < n; i++) {
            int s = scoreMoveHeuristic(board, moves.get(i), turn, targets);
            if (ttBestMove != null && !ttBestMove.isEmpty()
                    && moves.get(i).asString().equals(ttBestMove)) {
                s += 250000;
            }
            scored[i][0] = s;
            scored[i][1] = i;
        }

        java.util.Arrays.sort(scored, (a, b) -> Integer.compare(b[0], a[0]));

        int limit = Math.min(n, maxBranch);
        List<OctiMove> result = new ArrayList<>(limit);
        for (int i = 0; i < limit; i++) {
            result.add(moves.get(scored[i][1]));
        }
        return result;
    }

    /**
     * Recursive 2001 Jump-Capture & Reachability evaluation (zero-allocation bitmask).
     */
    private static void evaluateJumpCaptureRecursive(OctiBoard board,
                                                     int x, int y,
                                                     OctiPod pod,
                                                     OctiPlayer p,
                                                     boolean edgeless,
                                                     int depth,
                                                     long[] visitedMask,
                                                     long[] capturedSqMask,
                                                     int[] outStats) {
        // outStats[0] = reachableSquares, outStats[1] = capturableEnemyPods, outStats[2] = capturableEnemyProngs
        if (depth > 4) {
            return;
        }
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                continue;
            }
            int jx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
            int jy = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
            int lx = edgeless ? (((x + 2 * dir.deltaX()) % w + w) % w) : (x + 2 * dir.deltaX());
            int ly = edgeless ? (((y + 2 * dir.deltaY()) % h + h) % h) : (y + 2 * dir.deltaY());

            if (board.isValidCoord(jx, jy) && board.isValidCoord(lx, ly)) {
                OctiSquare jumpedSq = board.squareAt(jx, jy);
                OctiSquare landSq = board.squareAt(lx, ly);
                if (!jumpedSq.isEmpty() && !landSq.hasEnemyOf(p)) {
                    int sqBit = ly * 9 + lx;
                    int wordIdx = (sqBit >>> 6) & 1;
                    long bitMask = 1L << (sqBit & 63);
                    if ((visitedMask[wordIdx] & bitMask) == 0L) {
                        visitedMask[wordIdx] |= bitMask;
                        outStats[0]++;
                        if (jumpedSq.hasEnemyOf(p)) {
                            int jBit = jy * 9 + jx;
                            int jWord = (jBit >>> 6) & 1;
                            long jMask = 1L << (jBit & 63);
                            if ((capturedSqMask[jWord] & jMask) == 0L) {
                                capturedSqMask[jWord] |= jMask;
                                int enemyPods = jumpedSq.getNumPods();
                                outStats[1] += enemyPods;
                                for (int pi = 0; pi < enemyPods; pi++) {
                                    OctiPod ep = jumpedSq.getPodAt(pi);
                                    if (ep != null) {
                                        outStats[2] += ep.getNumProngs();
                                    }
                                }
                            }
                        }
                        evaluateJumpCaptureRecursive(board, lx, ly, pod, p, edgeless,
                                depth + 1, visitedMask, capturedSqMask, outStats);
                    }
                }
            }
        }
    }

    /**
     * Hybrid 2001+2026 Evaluation Function:
     * - Armed pod scaling (unarmed home-square pods worth less than reserve pods to prevent 0-prong dumping)
     * - Stack vulnerability & hanging-pod jump-capture threat penalties
     * - Orthogonal + diagonal color-parity versatility & straight-forward prong priority
     * - 2001 recursive evaluateJumpCapture threat & mobility pass
     * - 2001 multi-base minimum Chebyshev distance sum across all uncaptured enemy bases (d1 + d2 + d3)
     * - Allied home base defense against nearby enemy pods
     */
    public static int evaluatePosition(OctiBoard board, OctiPlayer perspective) {
        if (board == null || board.getGame() == null
                || perspective == null || perspective == OctiPlayer.PLAYER_UNDEF) {
            return 0;
        }
        OctiGame game = board.getGame();
        OctiGameMode mode = board.getGameMode();
        if (game.isGameOver()) {
            OctiPlayer w = game.getWinner();
            if (w == OctiPlayer.PLAYER_UNDEF) {
                return 0;
            }
            return OctiPlayer.areAllied(mode, w, perspective) ? WIN_SCORE : -WIN_SCORE;
        }

        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = game.useEdgelessBoard();

        List<int[]> allyTargets = targetEnemyBases(board, perspective);
        OctiPlayer sampleEnemy = (perspective == OctiPlayer.PLAYER_A
                || perspective == OctiPlayer.PLAYER_C)
                ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A;
        List<int[]> enemyTargets = targetEnemyBases(board, sampleEnemy);

        int totalScore = 0;
        int totalAllyPodsOnBoard = 0;
        int totalEnemyPodsOnBoard = 0;
        boolean allyHasUnarmedHomePod = false;
        boolean enemyHasUnarmedHomePod = false;

        // 1. Base Occupation & Square Stacking Assessment
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                OctiSquareType sqType = sq.getType();

                if (isEnemyHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        totalScore += 28000;
                    }
                } else if (isAlliedHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCaptured() && !sq.isCapturedByTeamOf(perspective)) {
                        totalScore -= 32000;
                    }
                } else if (sqType == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        totalScore += 4000;
                    } else if (sq.isCaptured()) {
                        totalScore -= 4000;
                    }
                }

                int numPods = sq.getNumPods();
                if (numPods > 0) {
                    OctiPlayer sqOwner = sq.getPodAt(0).getType().toPlayer();
                    boolean sqAlly = OctiPlayer.areAllied(mode, sqOwner, perspective);
                    if (sqAlly) {
                        totalAllyPodsOnBoard += numPods;
                    } else {
                        totalEnemyPodsOnBoard += numPods;
                    }

                    boolean onCapturedEnemyBase = isEnemyHomeSquareFor(sqType, mode, sqOwner)
                            && sq.isCapturedByTeamOf(sqOwner);
                    boolean onAlliedHome = isAlliedHomeSquareFor(sqType, mode, sqOwner);
                    int unarmedCount = 0;
                    for (int i = 0; i < numPods; i++) {
                        OctiPod p = sq.getPodAt(i);
                        if (p.getNumProngs() == 0) {
                            unarmedCount++;
                        }
                        if (onAlliedHome && p.getNumProngs() < 2) {
                            if (sqAlly) {
                                allyHasUnarmedHomePod = true;
                            } else {
                                enemyHasUnarmedHomePod = true;
                            }
                        }
                    }

                    // Penalize stacking multiple pods on a non-enemy-base square (single-jump multi-capture bait!)
                    if (numPods > 1 && !onCapturedEnemyBase) {
                        int stackPen = (numPods - 1) * 450 + unarmedCount * 950;
                        totalScore += sqAlly ? -stackPen : stackPen;
                    }
                }
            }
        }

        // 2. 2001 Multi-Base Chebyshev Distance Sum across all uncaptured enemy bases (d1 + d2 + d3)
        if (!allyTargets.isEmpty()) {
            int allyBaseDistSum = 0;
            for (int[] target : allyTargets) {
                OctiSquare tSq = board.squareAt(target[0], target[1]);
                if (tSq.isCapturedByTeamOf(perspective)) {
                    continue;
                }
                int bestDist = 999;
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        OctiSquare sq = board.squareAt(x, y);
                        for (int i = 0; i < sq.getNumPods(); i++) {
                            OctiPod pod = sq.getPodAt(i);
                            if (OctiPlayer.areAllied(mode, pod.getType().toPlayer(), perspective)
                                    && pod.getNumProngs() > 0) {
                                int dx = torusDist(x, target[0], w, edgeless);
                                int dy = torusDist(y, target[1], h, edgeless);
                                bestDist = Math.min(bestDist, Math.max(dx, dy));
                            }
                        }
                    }
                }
                if (bestDist < 999) {
                    allyBaseDistSum += bestDist;
                } else {
                    allyBaseDistSum += 8;
                }
            }
            totalScore += Math.max(0, 30 - allyBaseDistSum) * 320;
        }

        if (!enemyTargets.isEmpty()) {
            int enemyBaseDistSum = 0;
            for (int[] target : enemyTargets) {
                OctiSquare tSq = board.squareAt(target[0], target[1]);
                if (tSq.isCapturedByTeamOf(sampleEnemy)) {
                    continue;
                }
                int bestDist = 999;
                for (int y = 0; y < h; y++) {
                    for (int x = 0; x < w; x++) {
                        OctiSquare sq = board.squareAt(x, y);
                        for (int i = 0; i < sq.getNumPods(); i++) {
                            OctiPod pod = sq.getPodAt(i);
                            if (OctiPlayer.areAllied(mode, pod.getType().toPlayer(), sampleEnemy)
                                    && pod.getNumProngs() > 0) {
                                int dx = torusDist(x, target[0], w, edgeless);
                                int dy = torusDist(y, target[1], h, edgeless);
                                bestDist = Math.min(bestDist, Math.max(dx, dy));
                            }
                        }
                    }
                }
                if (bestDist < 999) {
                    enemyBaseDistSum += bestDist;
                } else {
                    enemyBaseDistSum += 8;
                }
            }
            totalScore -= Math.max(0, 30 - enemyBaseDistSum) * 320;
        }

        // 3. Pod Material, Prongs, Versatility, Jump-Capture Reachability & Threats
        long[] allyJumpThreatMask = new long[2];
        long[] enemyJumpThreatMask = new long[2];
        int[] mobStats = new int[3];
        long[] visitedMask = new long[2];

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                OctiSquareType sqType = sq.getType();
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    OctiPlayer owner = pod.getType().toPlayer();
                    boolean isAlly = OctiPlayer.areAllied(mode, owner, perspective);
                    List<int[]> targets = isAlly ? allyTargets : enemyTargets;

                    int numProngs = pod.getNumProngs();
                    int dist = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                    boolean onCapturedEnemyBase = isEnemyHomeSquareFor(sqType, mode, owner)
                            && sq.isCapturedByTeamOf(owner);

                    int podVal;
                    if (numProngs == 0 && !onCapturedEnemyBase
                            && sqType != OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                        // Unarmed pod on non-target square has zero mobility: worth less than a reserve pod (450)!
                        podVal = 320;
                    } else if (numProngs == 1) {
                        podVal = 950 + Math.max(0, (10 - dist)) * 110;
                        // Penalize premature lone 1-prong pod rushing far from home without backup
                        if (!isAlliedHomeSquareFor(sqType, mode, owner) && dist <= 4 && dist >= 2) {
                            podVal -= 550;
                        }
                    } else if (numProngs == 2) {
                        podVal = 1450 + Math.max(0, (10 - dist)) * 180;
                    } else {
                        podVal = 1750 + Math.max(0, (10 - dist)) * 210;
                    }

                    boolean hasOrth = false;
                    boolean hasDiag = false;

                    // Direct Step & Jump Threat Evaluation + Prong Direction Quality
                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        OctiProngType pt = pod.prongAt(dir);
                        if (pt == OctiProngType.NO_PRONG) {
                            continue;
                        }
                        podVal += (pt == OctiProngType.SUPER_PRONG) ? 350 : 220;
                        if (isOrthogonalDir(dir)) hasOrth = true;
                        if (isDiagonalDir(dir)) hasDiag = true;

                        // Forward prong alignment bonus
                        if (owner == OctiPlayer.PLAYER_A || owner == OctiPlayer.PLAYER_C) {
                            if (dir == OctiDir.DIR_N) {
                                podVal += 380;
                            } else if (dir == OctiDir.DIR_NE || dir == OctiDir.DIR_NW) {
                                podVal += 260;
                            } else if (dir == OctiDir.DIR_E || dir == OctiDir.DIR_W) {
                                podVal += 120;
                            }
                        } else if (owner == OctiPlayer.PLAYER_B || owner == OctiPlayer.PLAYER_D) {
                            if (dir == OctiDir.DIR_S) {
                                podVal += 380;
                            } else if (dir == OctiDir.DIR_SE || dir == OctiDir.DIR_SW) {
                                podVal += 260;
                            } else if (dir == OctiDir.DIR_E || dir == OctiDir.DIR_W) {
                                podVal += 120;
                            }
                        }

                        int nx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
                        int ny = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
                        if (board.isValidCoord(nx, ny)) {
                            int ndist = minChebyshevToTargets(nx, ny, targets, w, h, edgeless);
                            if (ndist < dist) {
                                podVal += 140;
                            }
                            if (ndist == 0 && !board.squareAt(nx, ny).hasEnemyOf(owner)) {
                                podVal += 8500; // 1-step base entry threat!
                            }
                        }

                        int jx = edgeless ? (((x + 2 * dir.deltaX()) % w + w) % w) : (x + 2 * dir.deltaX());
                        int jy = edgeless ? (((y + 2 * dir.deltaY()) % h + h) % h) : (y + 2 * dir.deltaY());
                        if (board.isValidCoord(nx, ny) && board.isValidCoord(jx, jy)) {
                            if (!board.squareAt(nx, ny).isEmpty() && !board.squareAt(jx, jy).hasEnemyOf(owner)) {
                                int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                                if (jdist == 0) {
                                    podVal += 12000; // 1-jump base entry threat!
                                }
                            }
                        }
                    }

                    // Color-parity versatility: orthogonal + diagonal prongs can reach all squares
                    if (hasOrth && hasDiag) {
                        podVal += 550;
                    } else if (numProngs >= 2 && hasDiag && !hasOrth) {
                        podVal -= 350; // Diagonal-only pod is trapped on one checkerboard parity!
                    }

                    // 2001 Recursive Jump-Capture Reachability pass (zero-allocation bitmask)
                    if (numProngs > 0) {
                        mobStats[0] = 0;
                        mobStats[1] = 0;
                        mobStats[2] = 0;
                        visitedMask[0] = 0L;
                        visitedMask[1] = 0L;
                        long[] capMask = isAlly ? allyJumpThreatMask : enemyJumpThreatMask;
                        evaluateJumpCaptureRecursive(board, x, y, pod, owner, edgeless,
                                0, visitedMask, capMask, mobStats);
                        podVal += mobStats[0] * 55;   // reachable squares
                        podVal += mobStats[1] * 1100; // capturable enemy pods
                        podVal += mobStats[2] * 180;  // capturable enemy prongs
                    }

                    totalScore += isAlly ? podVal : -podVal;
                }
            }
        }

        // 4. Hanging-Pod Vulnerability Penalty (squares that the opponent can jump over and capture)
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                int numPods = sq.getNumPods();
                if (numPods == 0) {
                    continue;
                }
                int sqBit = y * 9 + x;
                int wordIdx = (sqBit >>> 6) & 1;
                long bitMask = 1L << (sqBit & 63);
                OctiPlayer sqOwner = sq.getPodAt(0).getType().toPlayer();
                boolean sqAlly = OctiPlayer.areAllied(mode, sqOwner, perspective);

                int prongsOnSq = 0;
                for (int i = 0; i < numPods; i++) {
                    prongsOnSq += sq.getPodAt(i).getNumProngs();
                }

                if (sqAlly && (enemyJumpThreatMask[wordIdx] & bitMask) != 0L) {
                    int vulnPen = numPods * 1800 + prongsOnSq * 250;
                    if (numPods >= totalAllyPodsOnBoard) {
                        vulnPen += 15000; // Wipeout danger: losing all remaining pods on the board!
                    }
                    totalScore -= vulnPen;
                } else if (!sqAlly && (allyJumpThreatMask[wordIdx] & bitMask) != 0L) {
                    int vulnPen = numPods * 1800 + prongsOnSq * 250;
                    if (numPods >= totalEnemyPodsOnBoard) {
                        vulnPen += 15000;
                    }
                    totalScore += vulnPen;
                }
            }
        }

        // 5. Reserve Material, Premature Reserve Placement Penalty & Liberation Bonuses
        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pIdx = 1; pIdx <= numPlayers; pIdx++) {
            OctiPlayer p = OctiPlayer.fromValue(pIdx);
            boolean isAlly = OctiPlayer.areAllied(mode, p, perspective);
            int freePods = game.getFreePods(p);
            int resVal = freePods * 450
                    - game.getCapturedPods(p) * 380
                    + game.getFreeProngs(p) * 50
                    + game.getFreeSuperProngs(p) * 150;
            // Penalize pulling reserve pods onto the board while home pods are still unarmed
            if (freePods < 4 && (isAlly ? allyHasUnarmedHomePod : enemyHasUnarmedHomePod)
                    && game.getFreeProngs(p) >= 6) {
                resVal -= (4 - freePods) * 1500;
            }
            if (mode == OctiGameMode.GAME_MODE_4 && game.isPlayerEliminated(p)) {
                resVal -= 20000;
            }
            if (board.canLiberate(p) && game.getCapturedPods(p) > 0) {
                resVal += 2500;
            }
            totalScore += isAlly ? resVal : -resVal;
        }

        return totalScore;
    }

    private static final class SearchContext {
        long nodesVisited = 0;
        long deadlineMs = 0;
        volatile boolean timedOut = false;
        java.util.concurrent.atomic.AtomicBoolean stopSignal = null;
        final OctiGameSnapshot[] plySnapshots = new OctiGameSnapshot[32];

        OctiGameSnapshot snapshotForPly(int ply) {
            int idx = Math.max(0, Math.min(plySnapshots.length - 1, ply));
            OctiGameSnapshot s = plySnapshots[idx];
            if (s == null) {
                s = new OctiGameSnapshot();
                plySnapshots[idx] = s;
            }
            return s;
        }
    }

    private int negamax(OctiBoard scratchBoard,
                        int depth,
                        int ply,
                        int alpha,
                        int beta,
                        OctiPlayer currentPerspective,
                        SearchContext ctx) {
        ctx.nodesVisited++;
        if ((ctx.nodesVisited & 63L) == 0L) {
            liveNodes.set(ctx.nodesVisited);
        }
        if ((ctx.nodesVisited & 31L) == 0L) {
            if ((ctx.stopSignal != null && ctx.stopSignal.get())
                    || steadyClockMs() >= ctx.deadlineMs
                    || Thread.currentThread().isInterrupted()) {
                ctx.timedOut = true;
                return evaluatePosition(scratchBoard, currentPerspective);
            }
        }

        OctiGame game = scratchBoard.getGame();
        OctiGameMode mode = scratchBoard.getGameMode();
        if (game.isGameOver()) {
            OctiPlayer w = game.getWinner();
            if (w == OctiPlayer.PLAYER_UNDEF) {
                return 0;
            }
            return OctiPlayer.areAllied(mode, w, currentPerspective)
                    ? (WIN_SCORE - ply)
                    : (-WIN_SCORE + ply);
        }

        if (depth <= 0) {
            return evaluatePosition(scratchBoard, currentPerspective);
        }

        long hashKey = computePositionHash(scratchBoard);
        int ttIdx = (int) (hashKey & TT_MASK);
        Object ttLock = ttLocks[ttIdx & TT_LOCK_MASK];
        String ttMove = "";
        synchronized (ttLock) {
            if (ttKeys[ttIdx] == hashKey) {
                String m = ttBestMoves[ttIdx];
                ttMove = (m != null) ? m : "";
                if (ttDepths[ttIdx] >= depth) {
                    int entryFlag = ttFlags[ttIdx];
                    int entryScore = ttScores[ttIdx];
                    if (entryFlag == 0) {
                        return entryScore;
                    }
                    if (entryFlag == 1 && entryScore > alpha) {
                        alpha = entryScore;
                    } else if (entryFlag == 2 && entryScore < beta) {
                        beta = entryScore;
                    }
                    if (alpha >= beta) {
                        return entryScore;
                    }
                }
            }
        }

        List<OctiMove> rawMoves = game.generateLegalMoves();
        OctiPlayer turn = game.getTurn();

        // Enumerate composite moves at root/shallow ply only to keep interior search fast
        if (ply <= 1 && depth >= 2) {
            appendCompositeMoves(scratchBoard, turn, rawMoves);
        }

        if (rawMoves.isEmpty()) {
            return evaluatePosition(scratchBoard, currentPerspective);
        }

        int maxBranch = branchCapForDifficulty(difficulty);
        List<OctiMove> ordered = orderAndCapMoves(
                scratchBoard, rawMoves, turn, ttMove, maxBranch);

        int origAlpha = alpha;
        int bestVal = -INF_SCORE;
        String bestMoveStr = ordered.get(0).asString();
        OctiGameSnapshot snap = ctx.snapshotForPly(ply);
        game.captureSnapshotInto(snap);

        for (OctiMove mv : ordered) {
            try {
                game.move(mv);
            } catch (Throwable t) {
                game.restoreSnapshot(snap);
                continue;
            }

            int val;
            if (game.isGameOver()) {
                OctiPlayer w = game.getWinner();
                val = OctiPlayer.areAllied(mode, w, turn)
                        ? (WIN_SCORE - (ply + 1))
                        : (-WIN_SCORE + (ply + 1));
            } else {
                OctiPlayer nextTurn = game.getTurn();
                boolean sameTeamNext = OctiPlayer.areAllied(mode, turn, nextTurn);
                if (sameTeamNext) {
                    val = negamax(scratchBoard, depth - 1, ply + 1,
                            alpha, beta, turn, ctx);
                } else {
                    OctiPlayer oppPerspective = (nextTurn != OctiPlayer.PLAYER_UNDEF) ? nextTurn :
                            ((turn == OctiPlayer.PLAYER_A || turn == OctiPlayer.PLAYER_C)
                                    ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A);
                    val = -negamax(scratchBoard, depth - 1, ply + 1,
                            -beta, -alpha, oppPerspective, ctx);
                }
            }

            game.restoreSnapshot(snap);

            if (ctx.timedOut) {
                break;
            }

            if (val > bestVal) {
                bestVal = val;
                bestMoveStr = mv.asString();
            }
            if (val > alpha) {
                alpha = val;
            }
            if (alpha >= beta) {
                break;
            }
        }

        if (!ctx.timedOut && bestVal > -INF_SCORE) {
            byte storeFlag;
            if (bestVal <= origAlpha) {
                storeFlag = 2;
            } else if (bestVal >= beta) {
                storeFlag = 1;
            } else {
                storeFlag = 0;
            }
            synchronized (ttLock) {
                if (ttKeys[ttIdx] != hashKey || depth >= ttDepths[ttIdx]) {
                    ttKeys[ttIdx] = hashKey;
                    ttDepths[ttIdx] = depth;
                    ttScores[ttIdx] = bestVal;
                    ttBestMoves[ttIdx] = bestMoveStr;
                    ttFlags[ttIdx] = storeFlag;
                }
            }
        }

        return bestVal;
    }

    public String chooseMove(OctiBoard board) {
        return chooseMove(board, null);
    }

    public String chooseMove(OctiBoard board, BotSearchStats outStats) {
        liveDepth.set(0);
        liveNodes.set(0L);
        liveScore.set(0);

        if (outStats != null) {
            outStats.depthReached = 0;
            outStats.nodesVisited = 0;
            outStats.bestScore = 0;
            outStats.elapsedMs = 0;
        }

        if (board == null) {
            return "";
        }
        final String rootPositionFen;
        synchronized (board) {
            OctiGame srcGame = board.getGame();
            if (srcGame == null || !srcGame.isGameStarted() || srcGame.isGameOver()
                    || srcGame.getTurn() == OctiPlayer.PLAYER_UNDEF) {
                return "";
            }
            rootPositionFen = board.toPositionString();
        }

        long startMs = steadyClockMs();
        long deadlineMs = startMs + timeBudgetMs;

        OctiBoard scratchBoard = new OctiBoard();
        scratchBoard.suppressBoardUpdateNotification(true);
        scratchBoard.fromPositionString(rootPositionFen);
        OctiGame game = scratchBoard.getGame();
        if (game == null) {
            return "";
        }
        game.setSearchMode(true);

        List<OctiMove> legalMoves = game.generateLegalMoves();
        OctiPlayer rootTurn = game.getTurn();

        // Enumerate composite multi-pod moves
        appendCompositeMoves(scratchBoard, rootTurn, legalMoves);

        if (legalMoves.isEmpty()) {
            return "";
        }

        OctiGameMode mode = scratchBoard.getGameMode();
        final java.util.concurrent.atomic.AtomicBoolean stopSignal =
                new java.util.concurrent.atomic.AtomicBoolean(false);
        SearchContext ctx = new SearchContext();
        ctx.deadlineMs = deadlineMs;
        ctx.stopSignal = stopSignal;

        // 1. Immediate 1-ply victory check across ALL candidates
        OctiGameSnapshot rootSnap = game.captureSnapshot();
        for (OctiMove mv : legalMoves) {
            ctx.nodesVisited++;
            if ((ctx.nodesVisited & 63L) == 0L) {
                liveNodes.set(ctx.nodesVisited);
            }
            try {
                game.move(mv);
                if (game.isGameOver()
                        && OctiPlayer.areAllied(mode, game.getWinner(), rootTurn)) {
                    game.restoreSnapshot(rootSnap);
                    liveDepth.set(1);
                    liveNodes.set(ctx.nodesVisited);
                    liveScore.set(WIN_SCORE);
                    if (outStats != null) {
                        outStats.depthReached = 1;
                        outStats.nodesVisited = ctx.nodesVisited;
                        outStats.bestScore = WIN_SCORE;
                        outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
                    }
                    return mv.asString();
                }
            } catch (Throwable ignored) {
            }
            game.restoreSnapshot(rootSnap);
        }

        int rootCap = rootCapForDifficulty(difficulty);
        List<OctiMove> rootCandidates = orderAndCapMoves(
                scratchBoard, legalMoves, rootTurn, "", rootCap);

        String bestMoveStr = rootCandidates.get(0).asString();
        int bestScore = -INF_SCORE;
        int maxTargetDepth = maxDepthForDifficulty(difficulty);
        int completedDepth = 0;

        // Multi-Core Lazy SMP helper threads (Level 2 or Level 3 when budget >= 500ms)
        ExecutorService smpExecutor = null;
        if (timeBudgetMs >= 500 && difficulty.getLevel() >= 2) {
            int numHelpers = Math.min(2, Math.max(1, Runtime.getRuntime().availableProcessors() - 1));
            smpExecutor = Executors.newFixedThreadPool(numHelpers);
            for (int h = 0; h < numHelpers; h++) {
                final int helperDepthOffset = (h % 2);
                smpExecutor.submit(() -> {
                    try {
                        OctiBoard helperBoard = new OctiBoard();
                        helperBoard.suppressBoardUpdateNotification(true);
                        helperBoard.fromPositionString(rootPositionFen);
                        if (helperBoard.getGame() != null) {
                            helperBoard.getGame().setSearchMode(true);
                        }
                        SearchContext helperCtx = new SearchContext();
                        helperCtx.deadlineMs = deadlineMs;
                        helperCtx.stopSignal = stopSignal;

                        for (int hd = 1 + helperDepthOffset;
                                hd <= maxTargetDepth && !helperCtx.timedOut && !stopSignal.get();
                                hd++) {
                            if (steadyClockMs() >= deadlineMs || stopSignal.get()) {
                                break;
                            }
                            negamax(helperBoard, hd, 0, -INF_SCORE, INF_SCORE, rootTurn, helperCtx);
                        }
                    } catch (Throwable ignored) {
                    }
                });
            }
        }

        try {
            for (int d = 1; d <= maxTargetDepth; d++) {
                ctx.timedOut = false;
                int alpha = -INF_SCORE;
                int beta = INF_SCORE;
                String iterBestMove = bestMoveStr;
                int iterBestScore = -INF_SCORE;

                for (OctiMove mv : rootCandidates) {
                    try {
                        game.move(mv);
                    } catch (Throwable t) {
                        game.restoreSnapshot(rootSnap);
                        continue;
                    }

                    int score;
                    if (game.isGameOver()) {
                        OctiPlayer w = game.getWinner();
                        score = OctiPlayer.areAllied(mode, w, rootTurn)
                                ? (WIN_SCORE - 1)
                                : (-WIN_SCORE + 1);
                    } else {
                        OctiPlayer nextTurn = game.getTurn();
                        boolean sameTeamNext = OctiPlayer.areAllied(mode, rootTurn, nextTurn);
                        if (sameTeamNext) {
                            score = negamax(scratchBoard, d - 1, 1,
                                    alpha, beta, rootTurn, ctx);
                        } else {
                            OctiPlayer oppPerspective = (nextTurn != OctiPlayer.PLAYER_UNDEF) ? nextTurn :
                                    ((rootTurn == OctiPlayer.PLAYER_A || rootTurn == OctiPlayer.PLAYER_C)
                                            ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A);
                            score = -negamax(scratchBoard, d - 1, 1,
                                    -beta, -alpha, oppPerspective, ctx);
                        }
                    }

                    game.restoreSnapshot(rootSnap);

                    if (ctx.timedOut && d > 1) {
                        break;
                    }

                    if (difficulty == BotDifficulty.CASUAL
                            && Math.abs(score) < WIN_SCORE / 2) {
                        int jitter = (int) (Math.abs(nextRandom()) % 31L) - 15;
                        score += jitter;
                    }

                    if (score > iterBestScore) {
                        iterBestScore = score;
                        iterBestMove = mv.asString();
                    }
                    if (score > alpha) {
                        alpha = score;
                    }
                }

                if (!ctx.timedOut || d == 1) {
                    bestMoveStr = iterBestMove;
                    bestScore = iterBestScore;
                    completedDepth = d;
                    liveDepth.set(completedDepth);
                    liveNodes.set(ctx.nodesVisited);
                    liveScore.set(bestScore);
                    for (int idx = 1; idx < rootCandidates.size(); idx++) {
                        if (rootCandidates.get(idx).asString().equals(bestMoveStr)) {
                            OctiMove promoted = rootCandidates.remove(idx);
                            rootCandidates.add(0, promoted);
                            break;
                        }
                    }
                }

                if (ctx.timedOut || steadyClockMs() >= deadlineMs
                        || Math.abs(bestScore) >= WIN_SCORE - 100) {
                    break;
                }
            }
        } catch (Throwable ignored) {
            // Preserve bestMoveStr from deepest completed iteration
        } finally {
            stopSignal.set(true);
            if (smpExecutor != null) {
                smpExecutor.shutdownNow();
                try {
                    smpExecutor.awaitTermination(500, TimeUnit.MILLISECONDS);
                } catch (InterruptedException ignored) {
                }
            }
        }

        liveDepth.set(Math.max(1, completedDepth));
        liveNodes.set(ctx.nodesVisited);
        liveScore.set(bestScore);

        if (outStats != null) {
            outStats.depthReached = Math.max(1, completedDepth);
            outStats.nodesVisited = ctx.nodesVisited;
            outStats.bestScore = bestScore;
            outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
        }

        return bestMoveStr;
    }
}
