/*
 * OctiBotEngine.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

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
    private static final int TT_SIZE = 65536;

    private static final class TranspositionEntry {
        long key = 0;
        int depth = -1;
        int score = 0;
        int flag = 0; // 0 = exact, 1 = lowerbound, 2 = upperbound
        String bestMove = "";
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

    private BotDifficulty difficulty;
    private int timeBudgetMs;
    private boolean customTimeBudget;
    private long rngState;
    private final TranspositionEntry[] tt = new TranspositionEntry[TT_SIZE];

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
        for (int i = 0; i < TT_SIZE; i++) {
            tt[i] = new TranspositionEntry();
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

    public int timeBudgetMs() {
        return timeBudgetMs;
    }

    public int getTimeBudgetMs() {
        return timeBudgetMs;
    }

    private static int defaultBudgetForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 180;
        }
        switch (d) {
            case CASUAL:
                return 60;
            case TACTICAL:
                return 180;
            case MASTER:
                return 450;
            default:
                return 180;
        }
    }

    private static int maxDepthForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 3;
        }
        switch (d) {
            case CASUAL:
                return 2;
            case TACTICAL:
                return 3;
            case MASTER:
                return 4;
            default:
                return 3;
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

    private static long computePositionHash(OctiBoard board) {
        String pos = board.toPositionString();
        long h = 0xCBF29CE484222325L;
        for (int i = 0; i < pos.length(); i++) {
            h ^= (pos.charAt(i) & 0xFFL);
            h *= 0x100000001B3L;
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

    private static List<int[]> targetEnemyBases(OctiBoard board, OctiPlayer p) {
        List<int[]> targets = new ArrayList<>();
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                if (isEnemyHomeSquareFor(sq.getType(), mode, p)) {
                    targets.add(new int[]{x, y});
                }
            }
        }
        return targets;
    }

    private static int torusDist(int a, int b, int dim, boolean edgeless) {
        int d = Math.abs(a - b);
        if (edgeless && dim > 0) {
            d = Math.min(d, dim - d);
        }
        return d;
    }

    private static int minChebyshevToTargets(int x, int y,
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

    private static int scoreMoveHeuristic(OctiBoard board,
                                          OctiMove move,
                                          OctiPlayer turn,
                                          List<int[]> enemyTargets) {
        OctiGame game = board.getGame();
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = (game != null && game.useEdgelessBoard());

        int score = 0;
        for (OctiMoveOp op : move.getOps()) {
            if (!op.hops.isEmpty()) {
                int startDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
                OctiHop lastHop = op.hops.get(op.hops.size() - 1);
                int endDist = minChebyshevToTargets(lastHop.x, lastHop.y, enemyTargets, w, h, edgeless);

                score += (startDist - endDist) * 450;

                OctiSquare destSq = board.squareAt(lastHop.x, lastHop.y);
                if (isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                        && !destSq.isCapturedByTeamOf(turn)) {
                    score += 45000;
                }
                if (destSq.getType() == OctiSquareType.NEUTRAL_OCTI_SQUARE && destSq.isEmpty()) {
                    score += 2000;
                }

                for (OctiHop hop : op.hops) {
                    if (hop.capture) {
                        score += 9000;
                    } else {
                        score += 250;
                    }
                }

                OctiSquare startSq = board.squareAt(op.x0, op.y0);
                if (isEnemyHomeSquareFor(startSq.getType(), mode, turn)) {
                    if (startSq.getNumPods() <= 1
                            && !isEnemyHomeSquareFor(destSq.getType(), mode, turn)) {
                        score -= 12000;
                    }
                }
            } else if (op.addProngDir != OctiDir.DIR_UNDEF && op.rmProngDir == OctiDir.DIR_UNDEF) {
                score += 350;
                if (op.superProng) {
                    score += 150;
                }
                int nx = op.x0 + op.addProngDir.deltaX();
                int ny = op.y0 + op.addProngDir.deltaY();
                int curDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
                int nextDist = minChebyshevToTargets(nx, ny, enemyTargets, w, h, edgeless);
                if (nextDist < curDist) {
                    score += 400;
                }
                if (op.pod.getNumProngs() < 3) {
                    score += 200;
                }
            } else if (op.addPod) {
                if (op.liberate) {
                    score += 1800;
                } else {
                    score += 300;
                }
            } else if (op.rmProngDir != OctiDir.DIR_UNDEF) {
                score += 40;
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
                s += 200000;
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

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                OctiSquareType sqType = sq.getType();

                if (isEnemyHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        totalScore += 26000;
                    }
                } else if (isAlliedHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCaptured() && !sq.isCapturedByTeamOf(perspective)) {
                        totalScore -= 30000;
                    }
                } else if (sqType == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        totalScore += 3500;
                    } else if (sq.isCaptured()) {
                        totalScore -= 3500;
                    }
                }

                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    OctiPlayer owner = pod.getType().toPlayer();
                    boolean isAlly = OctiPlayer.areAllied(mode, owner, perspective);
                    List<int[]> targets = isAlly ? allyTargets : enemyTargets;

                    int podVal = 1400;
                    int dist = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                    podVal += Math.max(0, (10 - dist)) * 160;

                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        OctiProngType pt = pod.prongAt(dir);
                        if (pt == OctiProngType.NO_PRONG) {
                            continue;
                        }
                        podVal += (pt == OctiProngType.SUPER_PRONG) ? 320 : 180;

                        int nx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
                        int ny = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
                        if (board.isValidCoord(nx, ny)) {
                            int ndist = minChebyshevToTargets(nx, ny, targets, w, h, edgeless);
                            if (ndist < dist) {
                                podVal += 110;
                            }
                            if (ndist == 0) {
                                podVal += 6500;
                            }
                        }

                        int jx = edgeless
                                ? (((x + 2 * dir.deltaX()) % w + w) % w)
                                : (x + 2 * dir.deltaX());
                        int jy = edgeless
                                ? (((y + 2 * dir.deltaY()) % h + h) % h)
                                : (y + 2 * dir.deltaY());
                        if (board.isValidCoord(nx, ny) && board.isValidCoord(jx, jy)) {
                            if (!board.squareAt(nx, ny).isEmpty()
                                    && board.squareAt(jx, jy).isEmpty()) {
                                int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                                if (jdist == 0) {
                                    podVal += 9500;
                                }
                            }
                        }
                    }

                    totalScore += isAlly ? podVal : -podVal;
                }
            }
        }

        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pIdx = 1; pIdx <= numPlayers; pIdx++) {
            OctiPlayer p = OctiPlayer.fromValue(pIdx);
            boolean isAlly = OctiPlayer.areAllied(mode, p, perspective);
            int resVal = game.getFreePods(p) * 380
                    - game.getCapturedPods(p) * 300
                    + game.getFreeProngs(p) * 45
                    + game.getFreeSuperProngs(p) * 140;
            if (mode == OctiGameMode.GAME_MODE_4 && game.isPlayerEliminated(p)) {
                resVal -= 18000;
            }
            totalScore += isAlly ? resVal : -resVal;
        }

        return totalScore;
    }

    private static final class SearchContext {
        long nodesVisited = 0;
        long deadlineMs = 0;
        boolean timedOut = false;
    }

    private int negamax(OctiBoard scratchBoard,
                        int depth,
                        int ply,
                        int alpha,
                        int beta,
                        OctiPlayer currentPerspective,
                        SearchContext ctx) {
        ctx.nodesVisited++;
        if ((ctx.nodesVisited & 63L) == 0L && steadyClockMs() >= ctx.deadlineMs) {
            ctx.timedOut = true;
            return evaluatePosition(scratchBoard, currentPerspective);
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
        int ttIdx = (int) (hashKey & (TT_SIZE - 1));
        TranspositionEntry ttEntry = tt[ttIdx];
        String ttMove = "";
        if (ttEntry.key == hashKey && ttEntry.depth >= depth) {
            if (ttEntry.flag == 0) {
                return ttEntry.score;
            }
            if (ttEntry.flag == 1 && ttEntry.score > alpha) {
                alpha = ttEntry.score;
            } else if (ttEntry.flag == 2 && ttEntry.score < beta) {
                beta = ttEntry.score;
            }
            if (alpha >= beta) {
                return ttEntry.score;
            }
            ttMove = ttEntry.bestMove;
        }

        List<OctiMove> rawMoves = game.generateLegalMoves();
        if (rawMoves.isEmpty()) {
            return evaluatePosition(scratchBoard, currentPerspective);
        }

        OctiPlayer turn = game.getTurn();
        int maxBranch = (difficulty == BotDifficulty.MASTER) ? 20 : 14;
        List<OctiMove> ordered = orderAndCapMoves(
                scratchBoard, rawMoves, turn, ttMove, maxBranch);

        int origAlpha = alpha;
        int bestVal = -INF_SCORE;
        String bestMoveStr = ordered.get(0).asString();

        for (OctiMove mv : ordered) {
            OctiGameSnapshot snap = game.captureSnapshot();
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
            ttEntry.key = hashKey;
            ttEntry.depth = depth;
            ttEntry.score = bestVal;
            ttEntry.bestMove = bestMoveStr;
            if (bestVal <= origAlpha) {
                ttEntry.flag = 2;
            } else if (bestVal >= beta) {
                ttEntry.flag = 1;
            } else {
                ttEntry.flag = 0;
            }
        }

        return bestVal;
    }

    public String chooseMove(OctiBoard board) {
        return chooseMove(board, null);
    }

    public String chooseMove(OctiBoard board, BotSearchStats outStats) {
        if (outStats != null) {
            outStats.depthReached = 0;
            outStats.nodesVisited = 0;
            outStats.bestScore = 0;
            outStats.elapsedMs = 0;
        }

        if (board == null || board.getGame() == null) {
            return "";
        }
        OctiGame srcGame = board.getGame();
        if (!srcGame.isGameStarted() || srcGame.isGameOver()
                || srcGame.getTurn() == OctiPlayer.PLAYER_UNDEF) {
            return "";
        }

        long startMs = steadyClockMs();
        long deadlineMs = startMs + timeBudgetMs;

        OctiBoard scratchBoard = new OctiBoard();
        scratchBoard.suppressBoardUpdateNotification(true);
        scratchBoard.fromPositionString(board.toPositionString());
        OctiGame game = scratchBoard.getGame();
        if (game == null) {
            return "";
        }

        List<OctiMove> legalMoves = game.generateLegalMoves();
        if (legalMoves.isEmpty()) {
            return "";
        }

        OctiPlayer rootTurn = game.getTurn();
        OctiGameMode mode = scratchBoard.getGameMode();
        SearchContext ctx = new SearchContext();
        ctx.deadlineMs = deadlineMs;

        // 1. Check for immediate 1-ply victory across ALL legal moves first
        OctiGameSnapshot rootSnap = game.captureSnapshot();
        for (OctiMove mv : legalMoves) {
            ctx.nodesVisited++;
            try {
                game.move(mv);
                if (game.isGameOver()
                        && OctiPlayer.areAllied(mode, game.getWinner(), rootTurn)) {
                    game.restoreSnapshot(rootSnap);
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

        int rootCap = (difficulty == BotDifficulty.MASTER) ? 32 :
                      (difficulty == BotDifficulty.TACTICAL) ? 24 : 20;
        List<OctiMove> rootCandidates = orderAndCapMoves(
                scratchBoard, legalMoves, rootTurn, "", rootCap);

        String bestMoveStr = rootCandidates.get(0).asString();
        int bestScore = -INF_SCORE;
        int maxTargetDepth = maxDepthForDifficulty(difficulty);
        int completedDepth = 0;

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

                if (ctx.timedOut && d > 1) {
                    break;
                }
            }

            if (!ctx.timedOut || d == 1) {
                bestMoveStr = iterBestMove;
                bestScore = iterBestScore;
                completedDepth = d;
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

        if (outStats != null) {
            outStats.depthReached = Math.max(1, completedDepth);
            outStats.nodesVisited = ctx.nodesVisited;
            outStats.bestScore = bestScore;
            outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
        }

        return bestMoveStr;
    }
}
