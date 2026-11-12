/*
 * OctiBotEngine.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class OctiBotEngine {

    public enum BotDifficulty {
        CASUAL(1),
        TACTICAL(2),
        MASTER(3),
        EXPERT(4),
        GRANDMASTER(5);

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

    public static class BotTuningProfile extends com.selfso.octi.BotTuningProfile {
        public BotTuningProfile() {
            super();
        }

        public BotTuningProfile(com.selfso.octi.BotTuningProfile other) {
            super(other);
        }

        @Override
        public BotTuningProfile copy() {
            return new BotTuningProfile(this);
        }
    }

    private static final int WIN_SCORE = 1_000_000;
    private static final int INF_SCORE = 2_000_000;
    public static final int TT_SIZE = 262144; // 1 << 18 (256K entries)
    private static final int TT_MASK = TT_SIZE - 1;
    private static final int TT_LOCK_COUNT = 1024;
    private static final int TT_LOCK_MASK = TT_LOCK_COUNT - 1;
    private static final int MAX_SEARCH_PLY = 128;
    private static final int HISTORY_SIZE = 4096;
    private static final int HISTORY_MASK = HISTORY_SIZE - 1;
    private static final int MAX_HISTORY_VAL = 4096;

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
        final int friendlyCaptureMask;

        PathKey(int x0, int y0, int podConf, List<Integer> hopPackedCoords, int friendlyCaptureMask) {
            this.x0 = x0;
            this.y0 = y0;
            this.podConf = podConf;
            this.hopPackedCoords = hopPackedCoords;
            this.friendlyCaptureMask = friendlyCaptureMask;
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
                    && friendlyCaptureMask == other.friendlyCaptureMask
                    && Objects.equals(hopPackedCoords, other.hopPackedCoords);
        }

        @Override
        public int hashCode() {
            return Objects.hash(x0, y0, podConf, hopPackedCoords, friendlyCaptureMask);
        }
    }

    // Direct Binary 64-bit Zobrist Tables
    private static final long[][][] ZOB_PIECE = new long[81][7][4];
    private static final long[][][][] ZOB_PRONG = new long[81][7][8][3];
    private static final long[][] ZOB_BASE_CAPTURED = new long[81][5];
    private static final long[][] ZOB_FREE_PODS = new long[5][8];
    private static final long[][] ZOB_CAPTURED_PODS = new long[5][8];
    private static final long[][] ZOB_FREE_PRONGS = new long[5][33];
    private static final long[][] ZOB_FREE_SUPER_PRONGS = new long[5][9];
    private static final long[] ZOB_TURN = new long[5];
    private static final long[] ZOB_MODE = new long[4];
    private static final long[] ZOB_BASES_TO_WIN = new long[5];
    private static final long[] ZOB_EDGELESS = new long[2];
    private static final long[] ZOB_SUPER_PRONGS = new long[2];

    static {
        long state = 0x9E3779B97F4A7C15L;
        class SplitMix64Gen {
            long s;
            SplitMix64Gen(long seed) { this.s = seed; }
            long next() {
                s += 0x9E3779B97F4A7C15L;
                long z = s;
                z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
                z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
                return z ^ (z >>> 31);
            }
        }
        SplitMix64Gen sm = new SplitMix64Gen(state);

        for (int sq = 0; sq < 81; ++sq) {
            for (int p = 0; p < 7; ++p) {
                for (int t = 0; t < 4; ++t) {
                    ZOB_PIECE[sq][p][t] = sm.next();
                }
                for (int d = 0; d < 8; ++d) {
                    for (int pt = 0; pt < 3; ++pt) {
                        ZOB_PRONG[sq][p][d][pt] = sm.next();
                    }
                }
            }
            for (int c = 0; c < 5; ++c) {
                ZOB_BASE_CAPTURED[sq][c] = sm.next();
            }
        }
        for (int pl = 0; pl < 5; ++pl) {
            for (int i = 0; i < 8; ++i) {
                ZOB_FREE_PODS[pl][i] = sm.next();
                ZOB_CAPTURED_PODS[pl][i] = sm.next();
            }
            for (int i = 0; i < 33; ++i) {
                ZOB_FREE_PRONGS[pl][i] = sm.next();
            }
            for (int i = 0; i < 9; ++i) {
                ZOB_FREE_SUPER_PRONGS[pl][i] = sm.next();
            }
            ZOB_TURN[pl] = sm.next();
        }
        for (int m = 0; m < 4; ++m) {
            ZOB_MODE[m] = sm.next();
        }
        for (int b = 0; b < 5; ++b) {
            ZOB_BASES_TO_WIN[b] = sm.next();
        }
        for (int e = 0; e < 2; ++e) {
            ZOB_EDGELESS[e] = sm.next();
        }
        for (int s = 0; s < 2; ++s) {
            ZOB_SUPER_PRONGS[s] = sm.next();
        }
    }

    private BotDifficulty difficulty;
    private BotTuningProfile profile;
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
        this.profile = defaultProfileForDifficulty(this.difficulty);
        this.timeBudgetMs = this.profile.timeBudgetMs;
        this.customTimeBudget = false;
        this.rngState = (rngSeed != 0L) ? rngSeed : 0x5EED2026L;
        java.util.Arrays.fill(ttDepths, -1);
        for (int i = 0; i < TT_LOCK_COUNT; i++) {
            ttLocks[i] = new Object();
        }
    }

    public void setDifficulty(BotDifficulty d) {
        this.difficulty = (d != null) ? d : BotDifficulty.TACTICAL;
        int oldBudget = this.timeBudgetMs;
        boolean wasCustom = this.customTimeBudget;
        this.profile = defaultProfileForDifficulty(this.difficulty);
        if (wasCustom) {
            this.timeBudgetMs = oldBudget;
            this.profile.timeBudgetMs = oldBudget;
            this.customTimeBudget = true;
        } else {
            this.timeBudgetMs = this.profile.timeBudgetMs;
        }
    }

    public BotDifficulty difficulty() {
        return difficulty;
    }

    public void setTuningProfile(BotTuningProfile p) {
        this.profile = (p != null) ? new BotTuningProfile(p) : defaultProfileForDifficulty(this.difficulty);
        this.timeBudgetMs = this.profile.timeBudgetMs;
    }

    public BotTuningProfile tuningProfile() {
        return this.profile;
    }

    public static BotTuningProfile defaultProfileForDifficulty(BotDifficulty d) {
        return defaultProfileForDifficulty(d != null ? d.getLevel() : 2);
    }

    public static BotTuningProfile defaultProfileForDifficulty(int diffLevel) {
        BotTuningProfile p = new BotTuningProfile();
        if (diffLevel <= 1) { // CASUAL (Level 1)
            p.timeBudgetMs = 5000;
            p.maxSearchDepth = 3;
            p.useOpeningBook = false;
            p.maxQuiesceDepth = 0;
            p.quiesceBranchCap = 0;
            p.rootBranchCap = 24;
            p.treeBranchCap = 16;
            p.lazySmpThreads = 1;
            p.compositeMoveMaxPly = -1;
            p.rootScoreJitter = 160;
            p.closestPodDistWeight = 280;
            p.secondClosestPodDistWeight = 0;
            p.oneStepBaseThreatBonus = 7500;
            p.oneJumpBaseThreatBonus = 10000;
            p.twoPlyBaseThreatWeight = 0;
            p.firstArmedPodBonus = 500;
            p.secondArmedPodBonus = 400;
            p.thirdArmedPodBonus = 200;
            p.singlePodMonopolyPenalty = 300;
            p.leapfrogBatteryWeight = 200;
            p.leapfrogForwardBonus = 150;
            p.jumpReachableSquareWeight = 60;
            p.jumpReachStepWeight = 150;
            p.hangingPodBasePenalty = 1200;
            p.hangingProngWeight = 180;
            p.ofkProngSeizureWeight = 320;
            p.opponentTurnHangingPct = 100;
            p.soleArmedPodDangerPenalty = 1000;
            p.moveOrderBlunderPenalty = 2000;
        } else if (diffLevel == 2) { // TACTICAL (Level 2)
            p.timeBudgetMs = 10000;
            p.maxSearchDepth = 64;
            p.maxQuiesceDepth = 2;
            p.quiesceBranchCap = 6;
            p.rootBranchCap = 40;
            p.treeBranchCap = 24;
            p.lazySmpThreads = 2;
            p.compositeMoveMaxPly = 1;
            p.rootScoreJitter = 20;
            p.usePvs = true;
            p.useKillers = true;
            p.useHistory = true;
            p.qsearchUseTt = true;
            p.lazySmpDiversifyHelpers = true;
            p.closestPodDistWeight = 320;
            p.secondClosestPodDistWeight = 150;
            p.oneStepBaseThreatBonus = 8500;
            p.oneJumpBaseThreatBonus = 12000;
            p.twoPlyBaseThreatWeight = 2400;
            p.firstArmedPodBonus = 600;
            p.secondArmedPodBonus = 1100;
            p.thirdArmedPodBonus = 600;
            p.singlePodMonopolyPenalty = 1000;
            p.leapfrogBatteryWeight = 550;
            p.leapfrogForwardBonus = 400;
            p.jumpReachableSquareWeight = 130;
            p.jumpReachStepWeight = 450;
            p.hangingPodBasePenalty = 1400;
            p.hangingProngWeight = 220;
            p.ofkProngSeizureWeight = 450;
            p.opponentTurnHangingPct = 125;
            p.soleArmedPodDangerPenalty = 3500;
            p.moveOrderBlunderPenalty = 7000;
        } else if (diffLevel == 4) { // EXPERT (Level 4)
            p.timeBudgetMs = 40000;
            p.maxSearchDepth = 64;
            p.maxQuiesceDepth = 5;
            p.quiesceBranchCap = 12;
            p.rootBranchCap = 64;
            p.treeBranchCap = 36;
            p.lazySmpThreads = 4;
            p.compositeMoveMaxPly = 2;
            p.rootScoreJitter = 0;
            p.usePvs = true;
            p.aspirationWindowCp = 250;
            p.useKillers = true;
            p.useHistory = true;
            p.lmrMinDepth = 3;
            p.lmrMinMoveIndex = 4;
            p.nullMoveMinDepth = 3;
            p.qsearchUseTt = true;
            p.lazySmpDiversifyHelpers = true;
            p.closestPodDistWeight = 350;
            p.secondClosestPodDistWeight = 220;
            p.oneStepBaseThreatBonus = 9500;
            p.oneJumpBaseThreatBonus = 14000;
            p.twoPlyBaseThreatWeight = 4200;
            p.firstArmedPodBonus = 700;
            p.secondArmedPodBonus = 1500;
            p.thirdArmedPodBonus = 900;
            p.singlePodMonopolyPenalty = 1400;
            p.ofkSinglePodMonopolyPenalty = 5200;
            p.enemyHeavyPodThreatPenalty = 800;
            p.unarmedHomePodPenalty = 650;
            p.canLiberateBonus = 2500;
            p.leapfrogBatteryWeight = 800;
            p.leapfrogForwardBonus = 600;
            p.jumpReachableSquareWeight = 180;
            p.jumpReachStepWeight = 650;
            p.hangingPodBasePenalty = 1600;
            p.hangingProngWeight = 260;
            p.ofkProngSeizureWeight = 550;
            p.opponentTurnHangingPct = 140;
            p.soleArmedPodDangerPenalty = 6000;
            p.moveOrderBlunderPenalty = 11000;
        } else if (diffLevel >= 5) { // GRANDMASTER (Level 5)
            p.timeBudgetMs = 60000;
            p.maxSearchDepth = 64;
            p.maxQuiesceDepth = 6;
            p.quiesceBranchCap = 14;
            p.rootBranchCap = 72;
            p.treeBranchCap = 40;
            p.lazySmpThreads = 4;
            p.compositeMoveMaxPly = 3;
            p.rootScoreJitter = 0;
            p.usePvs = true;
            p.aspirationWindowCp = 250;
            p.useKillers = true;
            p.useHistory = true;
            p.lmrMinDepth = 3;
            p.lmrMinMoveIndex = 4;
            p.nullMoveMinDepth = 3;
            p.qsearchUseTt = true;
            p.lazySmpDiversifyHelpers = true;
            p.closestPodDistWeight = 350;
            p.secondClosestPodDistWeight = 220;
            p.oneStepBaseThreatBonus = 9500;
            p.oneJumpBaseThreatBonus = 14000;
            p.twoPlyBaseThreatWeight = 4200;
            p.firstArmedPodBonus = 700;
            p.secondArmedPodBonus = 1500;
            p.thirdArmedPodBonus = 900;
            p.singlePodMonopolyPenalty = 1400;
            p.ofkSinglePodMonopolyPenalty = 5200;
            p.enemyHeavyPodThreatPenalty = 800;
            p.unarmedHomePodPenalty = 650;
            p.canLiberateBonus = 2500;
            p.leapfrogBatteryWeight = 800;
            p.leapfrogForwardBonus = 600;
            p.jumpReachableSquareWeight = 180;
            p.jumpReachStepWeight = 650;
            p.hangingPodBasePenalty = 1600;
            p.hangingProngWeight = 260;
            p.ofkProngSeizureWeight = 550;
            p.opponentTurnHangingPct = 140;
            p.soleArmedPodDangerPenalty = 6000;
            p.moveOrderBlunderPenalty = 11000;
        } else { // MASTER (Level 3)
            p.timeBudgetMs = 20000;
            p.maxSearchDepth = 64;
            p.maxQuiesceDepth = 4;
            p.quiesceBranchCap = 10;
            p.rootBranchCap = 56;
            p.treeBranchCap = 32;
            p.lazySmpThreads = 4;
            p.compositeMoveMaxPly = 2;
            p.rootScoreJitter = 0;
            p.usePvs = true;
            p.aspirationWindowCp = 250;
            p.useKillers = true;
            p.useHistory = true;
            p.lmrMinDepth = 3;
            p.lmrMinMoveIndex = 4;
            p.nullMoveMinDepth = 3;
            p.qsearchUseTt = true;
            p.lazySmpDiversifyHelpers = true;
            p.closestPodDistWeight = 350;
            p.secondClosestPodDistWeight = 220;
            p.oneStepBaseThreatBonus = 9500;
            p.oneJumpBaseThreatBonus = 14000;
            p.twoPlyBaseThreatWeight = 4200;
            p.firstArmedPodBonus = 700;
            p.secondArmedPodBonus = 1500;
            p.thirdArmedPodBonus = 900;
            p.singlePodMonopolyPenalty = 1400;
            p.ofkSinglePodMonopolyPenalty = 5200;
            p.enemyHeavyPodThreatPenalty = 800;
            p.unarmedHomePodPenalty = 650;
            p.canLiberateBonus = 2500;
            p.leapfrogBatteryWeight = 800;
            p.leapfrogForwardBonus = 600;
            p.jumpReachableSquareWeight = 180;
            p.jumpReachStepWeight = 650;
            p.hangingPodBasePenalty = 1600;
            p.hangingProngWeight = 260;
            p.ofkProngSeizureWeight = 550;
            p.opponentTurnHangingPct = 140;
            p.soleArmedPodDangerPenalty = 6000;
            p.moveOrderBlunderPenalty = 11000;
        }
        return p;
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
            return 10000;
        }
        switch (d) {
            case CASUAL:
                return 5000;
            case TACTICAL:
                return 10000;
            case MASTER:
                return 20000;
            case EXPERT:
                return 40000;
            case GRANDMASTER:
                return 60000;
            default:
                return 10000;
        }
    }

    public static int maxDepthForDifficulty(BotDifficulty d) {
        if (d == null) {
            return 64;
        }
        switch (d) {
            case CASUAL:
                return 3;
            case TACTICAL:
            case MASTER:
            case EXPERT:
            case GRANDMASTER:
                return 64;
            default:
                return 64;
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
            case EXPERT:
                return 64;
            case GRANDMASTER:
                return 72;
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
            case EXPERT:
                return 36;
            case GRANDMASTER:
                return 40;
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

    public static OctiDir mirrorDirHorizontal(OctiDir d) {
        if (d == null) {
            return OctiDir.DIR_UNDEF;
        }
        switch (d) {
            case DIR_N:  return OctiDir.DIR_N;
            case DIR_NE: return OctiDir.DIR_NW;
            case DIR_E:  return OctiDir.DIR_W;
            case DIR_SE: return OctiDir.DIR_SW;
            case DIR_S:  return OctiDir.DIR_S;
            case DIR_SW: return OctiDir.DIR_SE;
            case DIR_W:  return OctiDir.DIR_E;
            case DIR_NW: return OctiDir.DIR_NE;
            default:     return OctiDir.DIR_UNDEF;
        }
    }

    private static long computePositionHashInternal(OctiBoard board, boolean mirrorHorizontal) {
        return computePositionHashInternal(board, mirrorHorizontal, false);
    }

    private static long computePositionHashInternal(OctiBoard board, boolean mirrorHorizontal, boolean exclude4PBases) {
        long h = 0L;
        if (board == null) {
            return h;
        }
        OctiGame game = board.getGame();
        if (game == null) {
            return h;
        }

        int w = board.getBoardWidth();
        int hgt = board.getBoardHeight();
        OctiGameMode mode = board.getGameMode();
        int modeVal = (mode != null) ? mode.getValue() : 0;
        h ^= ZOB_MODE[modeVal % 4];
        OctiPlayer turn = game.getTurn();
        int turnVal = (turn != null) ? turn.getValue() : 0;
        h ^= ZOB_TURN[turnVal % 5];
        if (!exclude4PBases || mode != OctiGameMode.GAME_MODE_4) {
            int btw = Math.min(4, Math.max(0, game.getBasesToWin()));
            h ^= ZOB_BASES_TO_WIN[btw];
        }
        h ^= ZOB_EDGELESS[game.useEdgelessBoard() ? 1 : 0];
        h ^= ZOB_SUPER_PRONGS[game.useSuperProngs() ? 1 : 0];

        for (int y = 0; y < hgt; ++y) {
            for (int x = 0; x < w; ++x) {
                int effX = mirrorHorizontal ? ((w - 1) - x) : x;
                int sqIdx = y * 9 + effX;
                if (sqIdx < 0 || sqIdx >= 81) {
                    continue;
                }
                OctiSquare sq = board.squareAt(x, y);
                if (sq.isCaptured()) {
                    for (int pl = 1; pl <= 4; ++pl) {
                        if (sq.isCapturedBy(OctiPlayer.fromValue(pl))) {
                            h ^= ZOB_BASE_CAPTURED[sqIdx][pl];
                            break;
                        }
                    }
                }
                int numPods = Math.min(7, sq.getNumPods());
                for (int p = 0; p < numPods; ++p) {
                    OctiPod pod = sq.getPodAt(p);
                    int pType = (pod.getType() != null ? pod.getType().getValue() : 0) - 1;
                    if (pType >= 0 && pType < 4) {
                        h ^= ZOB_PIECE[sqIdx][p][pType];
                    }
                    for (int d = 0; d < 8; ++d) {
                        OctiDir srcDir = OctiDir.fromIndex(d);
                        OctiDir effDir = mirrorHorizontal ? mirrorDirHorizontal(srcDir) : srcDir;
                        OctiProngType pt = pod.prongAt(srcDir);
                        int ptIdx = (pt != null) ? pt.getValue() : 0;
                        int dIdx = (effDir != null && effDir != OctiDir.DIR_UNDEF) ? effDir.getIndex() : 0;
                        if (ptIdx >= 0 && ptIdx < 3 && dIdx >= 0 && dIdx < 8) {
                            h ^= ZOB_PRONG[sqIdx][p][dIdx][ptIdx];
                        }
                    }
                }
            }
        }

        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pl = 1; pl <= numPlayers; ++pl) {
            OctiPlayer p = OctiPlayer.fromValue(pl);
            int fp = Math.min(7, Math.max(0, game.getFreePods(p)));
            int cp = Math.min(7, Math.max(0, game.getCapturedPods(p)));
            int fpr = Math.min(32, Math.max(0, game.getFreeProngs(p)));
            int fspr = Math.min(8, Math.max(0, game.getFreeSuperProngs(p)));
            h ^= ZOB_FREE_PODS[pl][fp];
            h ^= ZOB_CAPTURED_PODS[pl][cp];
            h ^= ZOB_FREE_PRONGS[pl][fpr];
            h ^= ZOB_FREE_SUPER_PRONGS[pl][fspr];
        }

        return h;
    }

    public static long computePositionHashStatic(OctiBoard board) {
        return computePositionHashInternal(board, false);
    }

    public static long computePositionHash(OctiBoard board) {
        return computePositionHashStatic(board);
    }

    public static long computeCanonicalOpeningHash(OctiBoard board, boolean[] outMirrored) {
        long hDirect = computePositionHashInternal(board, false, true);
        if (board == null || board.getGameMode() == OctiGameMode.GAME_MODE_4) {
            if (outMirrored != null && outMirrored.length > 0) {
                outMirrored[0] = false;
            }
            return hDirect;
        }

        long hMirror = computePositionHashInternal(board, true, true);
        if (Long.compareUnsigned(hMirror, hDirect) < 0) {
            if (outMirrored != null && outMirrored.length > 0) {
                outMirrored[0] = true;
            }
            return hMirror;
        }
        if (outMirrored != null && outMirrored.length > 0) {
            outMirrored[0] = false;
        }
        return hDirect;
    }

    public static long computeCanonicalOpeningHash(OctiBoard board) {
        return computeCanonicalOpeningHash(board, null);
    }

    public static String mirrorMoveNotation(String moveNotation, int boardWidth) {
        if (moveNotation == null || boardWidth < 1 || boardWidth > 9) {
            return "";
        }
        try {
            OctiMove parsed = new OctiMove(moveNotation);
            if (parsed.isEmpty()) {
                return "";
            }
            List<OctiMoveOp> mirroredOps = new ArrayList<>();
            for (OctiMoveOp srcOp : parsed.getOps()) {
                OctiMoveOp op = new OctiMoveOp(srcOp);
                int mx0 = (boardWidth - 1) - op.x0;
                if (mx0 < 0 || mx0 >= boardWidth) {
                    return "";
                }
                op.x0 = mx0;

                OctiPod mirroredPod = new OctiPod(op.pod.getType());
                for (int d = 0; d < 8; ++d) {
                    OctiDir dir = OctiDir.fromIndex(d);
                    OctiProngType pt = op.pod.prongAt(dir);
                    if (pt != OctiProngType.NO_PRONG) {
                        mirroredPod.addProng(mirrorDirHorizontal(dir),
                                pt == OctiProngType.SUPER_PRONG);
                    }
                }
                op.pod = mirroredPod;

                if (op.addProngDir != OctiDir.DIR_UNDEF) {
                    op.addProngDir = mirrorDirHorizontal(op.addProngDir);
                }
                if (op.rmProngDir != OctiDir.DIR_UNDEF) {
                    op.rmProngDir = mirrorDirHorizontal(op.rmProngDir);
                }
                for (OctiHop h : op.hops) {
                    int mhx = (boardWidth - 1) - h.x;
                    if (mhx < 0 || mhx >= boardWidth) {
                        return "";
                    }
                    h.x = mhx;
                }
                mirroredOps.add(op);
            }
            OctiMove mirroredMove = new OctiMove(mirroredOps);
            return mirroredMove.asString();
        } catch (Throwable t) {
            return "";
        }
    }

    public static final class OpeningBookCandidate {
        public final String move;
        public final short score;

        public OpeningBookCandidate(String move, int score) {
            this.move = move;
            this.score = (short) score;
        }
    }

    public static final class OpeningBookEntry {
        public final long canonicalKey;
        public final byte depth;
        public final OpeningBookCandidate[] candidates;

        public OpeningBookEntry(long canonicalKey, int depth, OpeningBookCandidate... candidates) {
            this.canonicalKey = canonicalKey;
            this.depth = (byte) depth;
            this.candidates = candidates;
        }
    }

    private static final OpeningBookEntry[] OPENING_BOOK = OctiOpeningBookData.ENTRIES;

    public static boolean probeOpeningBookTable(OctiBoard board,
                                                OpeningBookEntry[] entries,
                                                com.selfso.octi.BotTuningProfile profile,
                                                long[] rngStateHolder,
                                                String[] outMove,
                                                int[] outScore,
                                                int[] outDepth) {
        if (outMove != null && outMove.length > 0) {
            outMove[0] = "";
        }
        if (profile == null || !profile.useOpeningBook || entries == null || entries.length == 0 || board == null) {
            return false;
        }
        OctiGame srcGame = board.getGame();
        if (srcGame == null || !srcGame.isGameStarted() || srcGame.isGameOver()
                || srcGame.getTurn() == OctiPlayer.PLAYER_UNDEF) {
            return false;
        }

        boolean[] mirroredHolder = new boolean[1];
        long key = computeCanonicalOpeningHash(board, mirroredHolder);
        boolean mirrored = mirroredHolder[0];

        int lo = 0;
        int hi = entries.length;
        OpeningBookEntry found = null;
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            int cmp = Long.compareUnsigned(entries[mid].canonicalKey, key);
            if (cmp < 0) {
                lo = mid + 1;
            } else if (cmp > 0) {
                hi = mid;
            } else {
                found = entries[mid];
                break;
            }
        }

        if (found == null || found.candidates == null || found.candidates.length == 0) {
            return false;
        }

        List<OctiMove> legalMoves = srcGame.generateLegalMoves();
        if (legalMoves.isEmpty()) {
            return false;
        }

        int w = board.getBoardWidth();
        int maxCands = Math.min(3, found.candidates.length);

        class ValidCand {
            final String moveStr;
            final int score;
            ValidCand(String moveStr, int score) {
                this.moveStr = moveStr;
                this.score = score;
            }
        }
        List<ValidCand> validCands = new ArrayList<>(maxCands);

        for (int i = 0; i < maxCands; ++i) {
            OpeningBookCandidate cand = found.candidates[i];
            if (cand == null || cand.move == null || cand.move.isEmpty()) {
                continue;
            }
            String candMoveStr = mirrored ? mirrorMoveNotation(cand.move, w) : cand.move;
            if (candMoveStr == null || candMoveStr.isEmpty()) {
                continue;
            }
            try {
                OctiMove parsed = new OctiMove(candMoveStr);
                String canonicalStr = parsed.asString();
                boolean isLegal = false;
                for (OctiMove lm : legalMoves) {
                    if (lm.equals(parsed) || lm.asString().equals(canonicalStr)) {
                        isLegal = true;
                        candMoveStr = lm.asString();
                        break;
                    }
                }
                if (!isLegal && parsed.getOps().size() > 1 &&
                        board.getGameMode() != OctiGameMode.GAME_MODE_OFK) {
                    try {
                        OctiBoard testBoard = new OctiBoard();
                        testBoard.suppressBoardUpdateNotification(true);
                        testBoard.fromPositionString(board.toPositionString());
                        if (testBoard.getGame() != null) {
                            testBoard.getGame().setSearchMode(true);
                            testBoard.getGame().move(parsed);
                            isLegal = true;
                            candMoveStr = canonicalStr;
                        }
                    } catch (Throwable ignored) {
                        isLegal = false;
                    }
                }
                if (isLegal) {
                    validCands.add(new ValidCand(candMoveStr, cand.score));
                }
            } catch (Throwable ignored) {
            }
        }

        if (validCands.isEmpty()) {
            return false;
        }

        int bestScore = validCands.get(0).score;
        int window = (profile.rootScoreJitter > 0) ? profile.rootScoreJitter : 15;
        int eligibleCount = 1;
        for (int i = 1; i < validCands.size(); ++i) {
            if (validCands.get(i).score >= bestScore - window) {
                eligibleCount++;
            }
        }

        int chosenIdx = 0;
        if (eligibleCount > 1 && profile.rootScoreJitter > 0 && rngStateHolder != null && rngStateHolder.length > 0) {
            long x = rngStateHolder[0] != 0L ? rngStateHolder[0] : 0x5EED2026L;
            x ^= (x << 13);
            x ^= (x >>> 7);
            x ^= (x << 17);
            rngStateHolder[0] = x;
            chosenIdx = (int) Math.floorMod(x, eligibleCount);
        }

        ValidCand chosen = validCands.get(chosenIdx);
        if (outMove != null && outMove.length > 0) {
            outMove[0] = chosen.moveStr;
        }
        if (outScore != null && outScore.length > 0) {
            outScore[0] = chosen.score;
        }
        if (outDepth != null && outDepth.length > 0) {
            outDepth[0] = found.depth;
        }
        return true;
    }

    public boolean probeOpeningBook(OctiBoard board, String[] outMove, int[] outScore, int[] outDepth) {
        long[] rngHolder = new long[] { this.rngState };
        boolean hit = probeOpeningBookTable(board, OPENING_BOOK, this.profile, rngHolder, outMove, outScore, outDepth);
        this.rngState = rngHolder[0];
        return hit;
    }

    public boolean probeOpeningBook(OctiBoard board, String[] outMove) {
        return probeOpeningBook(board, outMove, null, null);
    }

    public String probeOpeningBook(OctiBoard board) {
        String[] outMove = new String[1];
        return probeOpeningBook(board, outMove, null, null) && outMove[0] != null ? outMove[0] : "";
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

    private static long mixPathKey64(long z) {
        z += 0x9E3779B97F4A7C15L;
        z = (z ^ (z >>> 30)) * 0xBF58476D1CE4E5B9L;
        z = (z ^ (z >>> 27)) * 0x94D049BB133111EBL;
        return z ^ (z >>> 31);
    }

    private static boolean sameMovePathIgnoringCapture(List<OctiMoveOp> a,
                                                       List<OctiMoveOp> b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (int i = 0; i < a.size(); i++) {
            OctiMoveOp oa = a.get(i);
            OctiMoveOp ob = b.get(i);
            if (oa.x0 != ob.x0
                    || oa.y0 != ob.y0
                    || (oa.pod.rawConfig() & 0xFFFF) != (ob.pod.rawConfig() & 0xFFFF)
                    || oa.addPod != ob.addPod
                    || oa.liberate != ob.liberate
                    || oa.addProngDir != ob.addProngDir
                    || oa.rmProngDir != ob.rmProngDir
                    || oa.superProng != ob.superProng
                    || oa.isSuperProngBonusMove != ob.isSuperProngBonusMove
                    || oa.hops.size() != ob.hops.size()) {
                return false;
            }
            for (int h = 0; h < oa.hops.size(); h++) {
                if (oa.hops.get(h).x != ob.hops.get(h).x
                        || oa.hops.get(h).y != ob.hops.get(h).y) {
                    return false;
                }
            }
        }
        return true;
    }

    public static List<OctiMove> filterDominatedMoves(OctiBoard board,
                                                      OctiPlayer turn,
                                                      List<OctiMove> moves) {
        if (moves == null || moves.size() <= 1) {
            return moves;
        }
        if (board == null || turn == null) {
            return moves;
        }

        int boardW = board.getBoardWidth();
        int boardH = board.getBoardHeight();
        boolean edgeless = board.getGame() != null && board.getGame().useEdgelessBoard();
        int freeProngs = board.getGame() != null ? board.getGame().getFreeProngs(turn) : 0;

        int n = moves.size();
        long[] pathKeys = new long[n];
        int[] friendlyCapMasks = new int[n];
        int[] enemyCapMasks = new int[n];
        boolean[] hasAnyHops = new boolean[n];
        boolean[] pointlessSelfCapture = new boolean[n];
        int[] validHopIndices = new int[n];
        int numValidHopMoves = 0;
        boolean anyPointlessSelfCap = false;
        boolean anyEnemyCap = false;

        for (int i = 0; i < n; i++) {
            OctiMove m = moves.get(i);
            List<OctiMoveOp> ops = m.getOps();
            if (ops.size() == 1 && ops.get(0).hops.isEmpty()) {
                continue;
            }

            long h = mixPathKey64(ops.size());
            int globalHopIdx = 0;
            boolean badSelfCap = false;
            int fMask = 0;
            int eMask = 0;
            boolean anyHops = false;

            for (OctiMoveOp op : ops) {
                h = mixPathKey64(h ^ (op.x0 & 0xFFL));
                h = mixPathKey64(h ^ ((op.y0 & 0xFFL) << 8));
                h = mixPathKey64(h ^ (op.pod.rawConfig() & 0xFFFFL));
                long flags = (op.addPod ? 1L : 0L)
                        | ((op.liberate ? 1L : 0L) << 1)
                        | (((op.addProngDir != null ? op.addProngDir.getIndex() : 255) & 0xFFL) << 2)
                        | (((op.rmProngDir != null ? op.rmProngDir.getIndex() : 255) & 0xFFL) << 10)
                        | ((op.superProng ? 1L : 0L) << 18)
                        | ((op.isSuperProngBonusMove ? 1L : 0L) << 19);
                h = mixPathKey64(h ^ flags);
                h = mixPathKey64(h ^ op.hops.size());

                int cx = op.x0;
                int cy = op.y0;
                for (int hIdx = 0; hIdx < op.hops.size(); hIdx++) {
                    anyHops = true;
                    OctiHop hopItem = op.hops.get(hIdx);
                    h = mixPathKey64(h ^ ((hopItem.x & 0xFFL) | ((hopItem.y & 0xFFL) << 8)));

                    int dx = hopItem.x - cx;
                    int dy = hopItem.y - cy;
                    if (edgeless) {
                        for (int d = -2; d <= 2; d++) {
                            if (((cx + d) % boardW + boardW) % boardW == hopItem.x) { dx = d; break; }
                        }
                        for (int d = -2; d <= 2; d++) {
                            if (((cy + d) % boardH + boardH) % boardH == hopItem.y) { dy = d; break; }
                        }
                    }
                    boolean isJump = (Math.abs(dx) == 2 || Math.abs(dy) == 2);
                    if (isJump && hopItem.capture) {
                        int mx = edgeless ? (((cx + dx / 2) % boardW + boardW) % boardW) : (cx + dx / 2);
                        int my = edgeless ? (((cy + dy / 2) % boardH + boardH) % boardH) : (cy + dy / 2);
                        if (board.isValidCoord(mx, my)) {
                            OctiSquare midSq = board.squareAt(mx, my);
                            if (midSq.hasEnemyOf(turn)) {
                                eMask |= (1 << (globalHopIdx & 31));
                            } else {
                                if (freeProngs > 0) {
                                    int prongsOnMid = 0;
                                    for (int pi = 0; pi < midSq.getNumPods(); pi++) {
                                        prongsOnMid += midSq.getPodAt(pi).getNumProngs();
                                    }
                                    if (prongsOnMid == 0) {
                                        badSelfCap = true;
                                        break;
                                    }
                                }
                                fMask |= (1 << (globalHopIdx & 31));
                            }
                        }
                    }
                    cx = hopItem.x;
                    cy = hopItem.y;
                    globalHopIdx++;
                }
                if (badSelfCap) {
                    break;
                }
            }

            if (badSelfCap) {
                pointlessSelfCapture[i] = true;
                anyPointlessSelfCap = true;
                continue;
            }

            pathKeys[i] = h;
            friendlyCapMasks[i] = fMask;
            enemyCapMasks[i] = eMask;
            hasAnyHops[i] = anyHops;
            if (anyHops) {
                validHopIndices[numValidHopMoves++] = i;
                if (eMask != 0) {
                    anyEnemyCap = true;
                }
            }
        }

        if (!anyPointlessSelfCap && !anyEnemyCap) {
            return moves;
        }

        List<OctiMove> filtered = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            if (pointlessSelfCapture[i]) {
                continue;
            }
            if (anyEnemyCap && hasAnyHops[i]) {
                boolean isDominated = false;
                int eMask = enemyCapMasks[i];
                int fMask = friendlyCapMasks[i];
                long pKey = pathKeys[i];
                for (int k = 0; k < numValidHopMoves; k++) {
                    int j = validHopIndices[k];
                    int otherEMask = enemyCapMasks[j];
                    if (otherEMask != eMask
                            && (otherEMask & eMask) == eMask
                            && pathKeys[j] == pKey
                            && friendlyCapMasks[j] == fMask
                            && sameMovePathIgnoringCapture(moves.get(i).getOps(), moves.get(j).getOps())) {
                        isDominated = true;
                        break;
                    }
                }
                if (isDominated) {
                    continue;
                }
            }
            filtered.add(moves.get(i));
        }

        return filtered.isEmpty() ? moves : filtered;
    }

    public static List<OctiMove> filterDominatedMoves(OctiBoard board, List<OctiMove> moves) {
        OctiPlayer turn = (board != null && board.getGame() != null) ? board.getGame().getTurn() : OctiPlayer.PLAYER_A;
        return filterDominatedMoves(board, turn, moves);
    }

    public static List<OctiMove> filterDominatedMoves(List<OctiMove> moves) {
        return moves;
    }

    /**
     * Appends legal 2-pod (and 3-pod) composite moves from any square (x, y)
     * containing 2+ friendly pods of `turn` with prongs.
     */
    public static void appendCompositeMoves(OctiBoard board, OctiPlayer turn, List<OctiMove> outMoves) {
        if (board == null || board.getGame() == null || turn == null || turn == OctiPlayer.PLAYER_UNDEF) {
            return;
        }
        if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) {
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

    private static boolean squareHasAllyOf(OctiSquare sq, OctiGameMode mode, OctiPlayer player) {
        if (sq == null || sq.isEmpty()) {
            return false;
        }
        for (int i = 0; i < sq.getNumPods(); i++) {
            OctiPod p = sq.getPodAt(i);
            if (p != null && OctiPlayer.areAllied(mode, p.getType().toPlayer(), player)) {
                return true;
            }
        }
        return false;
    }

    public static final class CoalitionPhaseContext {
        public int basesNeeded = 1;
        public int availPods = 0;
        public int capturedPods = 0;
        public boolean feasible = true;
        public int baseThreatScalePct = 100;
    }

    private static CoalitionPhaseContext analyzeCoalitionPhase(OctiBoard board, OctiPlayer side) {
        CoalitionPhaseContext ctx = new CoalitionPhaseContext();
        if (board == null || board.getGame() == null || side == null || side == OctiPlayer.PLAYER_UNDEF) {
            return ctx;
        }
        OctiGame game = board.getGame();
        OctiGameMode mode = board.getGameMode();
        int basesToWin = game.getBases() != null ? game.getBases().getValue() : 1;
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();

        if (mode == OctiGameMode.GAME_MODE_OFK || basesToWin <= 1) {
            ctx.basesNeeded = 1;
        } else {
            int capturedEnemyBases = 0;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    OctiSquare sq = board.squareAt(x, y);
                    if (isEnemyHomeSquareFor(sq.getType(), mode, side)
                            && sq.isCapturedByTeamOf(side)) {
                        capturedEnemyBases++;
                    }
                }
            }
            ctx.basesNeeded = Math.max(1, basesToWin - capturedEnemyBases);
        }

        int totalBoardPods = 0;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                OctiSquare sq = board.squareAt(x, y);
                if (squareHasAllyOf(sq, mode, side)) {
                    totalBoardPods += sq.getNumPods();
                }
            }
        }

        int allyFreePods = 0;
        int allyCapturedPods = 0;
        boolean allyCanLiberate = false;
        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pIdx = 1; pIdx <= numPlayers; ++pIdx) {
            OctiPlayer p = OctiPlayer.fromValue(pIdx);
            if (OctiPlayer.areAllied(mode, p, side)) {
                allyFreePods += game.getFreePods(p);
                allyCapturedPods += game.getCapturedPods(p);
                if (board.canLiberate(p)) {
                    allyCanLiberate = true;
                }
            }
        }

        ctx.capturedPods = allyCapturedPods;
        ctx.availPods = totalBoardPods + allyFreePods + (allyCanLiberate ? allyCapturedPods : 0);
        ctx.feasible = (ctx.availPods >= ctx.basesNeeded);

        if (!ctx.feasible) {
            ctx.baseThreatScalePct = (ctx.capturedPods > 0) ? 20 : 5;
        } else if (ctx.basesNeeded <= 1) {
            ctx.baseThreatScalePct = 100;
        } else if (ctx.basesNeeded == 2) {
            ctx.baseThreatScalePct = 35;
        } else {
            ctx.baseThreatScalePct = 20;
        }

        return ctx;
    }

    private static long[] computeEnemyThreatMask(OctiBoard board, OctiPlayer turn) {
        return computeEnemyThreatMask(board, turn, targetEnemyBases(board, turn));
    }

    private static long[] computeEnemyThreatMask(OctiBoard board, OctiPlayer turn, List<int[]> targets) {
        long[] mask = new long[2];
        if (board == null || board.getGame() == null) {
            return mask;
        }
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = board.getGame().useEdgelessBoard();

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                if (sq.hasEnemyOf(turn)) {
                    for (int pi = 0; pi < sq.getNumPods(); pi++) {
                        OctiPod p = sq.getPodAt(pi);
                        if (p != null && p.getNumProngs() > 0 && !OctiPlayer.areAllied(mode, p.getType().toPlayer(), turn)) {
                            int[] mobStats = new int[4];
                            mobStats[3] = 999;
                            long[] visitedMask = new long[2];
                            evaluateJumpCaptureRecursive(board, x, y, x, y, p, p.getType().toPlayer(), edgeless,
                                    0, new long[2], visitedMask, mask, targets, mobStats);

                            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                                OctiDir dir = OctiDir.fromIndex(d);
                                if (p.prongAt(dir) != OctiProngType.NO_PRONG) {
                                    int dx = dir.deltaX();
                                    int dy = dir.deltaY();
                                    int mx = edgeless ? (((x + dx) % w + w) % w) : (x + dx);
                                    int my = edgeless ? (((y + dy) % h + h) % h) : (y + dy);
                                    int lx = edgeless ? (((x + 2 * dx) % w + w) % w) : (x + 2 * dx);
                                    int ly = edgeless ? (((y + 2 * dy) % h + h) % h) : (y + 2 * dy);
                                    if (board.isValidCoord(mx, my) && board.isValidCoord(lx, ly)) {
                                        OctiSquare landSq = board.squareAt(lx, ly);
                                        if (landSq.isEmpty()) {
                                            int midIdx = my * 9 + mx;
                                            int midWord = (midIdx >>> 6) & 1;
                                            mask[midWord] |= (1L << (midIdx & 63));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return mask;
    }

    public static int prongDirectionBonus(OctiBoard board,
                                          OctiGameMode mode,
                                          OctiPlayer seat,
                                          OctiDir dir,
                                          int primaryBonus,
                                          int diagBonus,
                                          int sideBonus) {
        if (dir == null || dir == OctiDir.DIR_UNDEF) {
            return 0;
        }
        if (mode != OctiGameMode.GAME_MODE_4) {
            if (seat == OctiPlayer.PLAYER_A || seat == OctiPlayer.PLAYER_C) {
                if (dir == OctiDir.DIR_N) {
                    return primaryBonus;
                } else if (dir == OctiDir.DIR_NE || dir == OctiDir.DIR_NW) {
                    return diagBonus;
                } else if (dir == OctiDir.DIR_E || dir == OctiDir.DIR_W) {
                    return sideBonus;
                }
            } else if (seat == OctiPlayer.PLAYER_B || seat == OctiPlayer.PLAYER_D) {
                if (dir == OctiDir.DIR_S) {
                    return primaryBonus;
                } else if (dir == OctiDir.DIR_SE || dir == OctiDir.DIR_SW) {
                    return diagBonus;
                } else if (dir == OctiDir.DIR_E || dir == OctiDir.DIR_W) {
                    return sideBonus;
                }
            }
            return 0;
        }

        OctiSquareType homeType;
        if (seat == OctiPlayer.PLAYER_A) {
            homeType = OctiSquareType.A_HOME_SQUARE;
        } else if (seat == OctiPlayer.PLAYER_B) {
            homeType = OctiSquareType.B_HOME_SQUARE;
        } else if (seat == OctiPlayer.PLAYER_C) {
            homeType = OctiSquareType.C_HOME_SQUARE;
        } else if (seat == OctiPlayer.PLAYER_D) {
            homeType = OctiSquareType.D_HOME_SQUARE;
        } else {
            return 0;
        }

        int w = (board != null) ? board.getBoardWidth() : 9;
        int h = (board != null) ? board.getBoardHeight() : 9;
        int hx = -1;
        int hy = -1;
        if (board != null) {
            for (int y = 0; y < h && hx < 0; y++) {
                for (int x = 0; x < w; x++) {
                    if (board.squareAt(x, y).getType() == homeType) {
                        hx = x;
                        hy = y;
                        break;
                    }
                }
            }
        }
        if (hx < 0 || hy < 0) {
            return 0;
        }

        int cx = w / 2;
        int cy = h / 2;
        int fx = (hx < cx) ? 1 : ((hx > cx) ? -1 : 0);
        int fy = (hy < cy) ? 1 : ((hy > cy) ? -1 : 0);
        int dx = dir.deltaX();
        int dy = dir.deltaY();

        if ((dx == 0 && dy == fy) || (dx == fx && dy == 0)) {
            return primaryBonus;
        }
        if (dx == fx && dy == fy) {
            return diagBonus;
        }
        if (dx != 0 && dy != 0 && (dx == fx || dy == fy)) {
            return sideBonus;
        }
        return 0;
    }

    private static int scoreMoveHeuristic(OctiBoard board,
                                          OctiMove move,
                                          OctiPlayer turn,
                                          List<int[]> enemyTargets,
                                          BotTuningProfile profile,
                                          long[] enemyThreatMask) {
        return scoreMoveHeuristic(board, move, turn, enemyTargets, profile, enemyThreatMask, 100);
    }

    private static int scoreMoveHeuristic(OctiBoard board,
                                          OctiMove move,
                                          OctiPlayer turn,
                                          List<int[]> enemyTargets,
                                          BotTuningProfile profile,
                                          long[] enemyThreatMask,
                                          int baseThreatScalePct) {
        OctiGame game = board.getGame();
        OctiGameMode mode = board.getGameMode();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = (game != null && game.useEdgelessBoard());
        boolean earlyDevelopment = (game != null && game.getFreeProngs(turn) >= 8);

        int score = 0;
        List<OctiMoveOp> ops = move.getOps();
        int regularOps = 0;
        int preludeOps = 0;
        for (OctiMoveOp op : ops) {
            if (op.isSuperProngBonusMove) {
                preludeOps++;
            } else {
                regularOps++;
            }
        }

        // Prioritize multi-pod composite moves (multiple regular ops), not superprong preludes
        if (regularOps > 1) {
            score += 8000 + (regularOps - 1) * 4000;
        }

        // Pre-scan move-wide enemy capture status across all ops (preludes + regular)
        boolean moveHasEnemyCapture = false;
        for (OctiMoveOp op : ops) {
            int prevX = op.x0;
            int prevY = op.y0;
            for (OctiHop hop : op.hops) {
                int dx = hop.x - prevX;
                int dy = hop.y - prevY;
                if (edgeless) {
                    for (int d = -2; d <= 2; d++) {
                        if (((prevX + d) % w + w) % w == hop.x) { dx = d; break; }
                    }
                    for (int d = -2; d <= 2; d++) {
                        if (((prevY + d) % h + h) % h == hop.y) { dy = d; break; }
                    }
                }
                if ((Math.abs(dx) == 2 || Math.abs(dy) == 2) && hop.capture) {
                    int mx = edgeless ? (((prevX + dx / 2) % w + w) % w) : (prevX + dx / 2);
                    int my = edgeless ? (((prevY + dy / 2) % h + h) % h) : (prevY + dy / 2);
                    if (!board.isValidCoord(mx, my) || board.squareAt(mx, my).hasEnemyOf(turn)) {
                        moveHasEnemyCapture = true;
                    }
                } else if (hop.capture) {
                    moveHasEnemyCapture = true;
                }
                prevX = hop.x;
                prevY = hop.y;
            }
        }

        long[] capturedSqMask = new long[2];

        for (int opIdx = 0; opIdx < ops.size(); opIdx++) {
            OctiMoveOp op = ops.get(opIdx);
            if (!op.hops.isEmpty()) {
                boolean isInitialSegmentForPod = true;
                for (int k = 0; k < opIdx; k++) {
                    OctiMoveOp prevOp = ops.get(k);
                    if (!prevOp.hops.isEmpty()) {
                        OctiHop prevEnd = prevOp.hops.get(prevOp.hops.size() - 1);
                        if (prevEnd.x == op.x0 && prevEnd.y == op.y0) {
                            isInitialSegmentForPod = false;
                            break;
                        }
                    }
                }

                OctiHop lastHop = op.hops.get(op.hops.size() - 1);
                boolean isFinalSegmentForPod = true;
                int chainFinalX = lastHop.x;
                int chainFinalY = lastHop.y;
                for (int j = opIdx + 1; j < ops.size(); j++) {
                    OctiMoveOp nextOp = ops.get(j);
                    if (!nextOp.hops.isEmpty() && nextOp.x0 == chainFinalX && nextOp.y0 == chainFinalY) {
                        if (j == opIdx + 1 || (chainFinalX == lastHop.x && chainFinalY == lastHop.y)) {
                            isFinalSegmentForPod = false;
                        }
                        OctiHop nextEnd = nextOp.hops.get(nextOp.hops.size() - 1);
                        chainFinalX = nextEnd.x;
                        chainFinalY = nextEnd.y;
                    }
                }

                int startDist = minChebyshevToTargets(op.x0, op.y0, enemyTargets, w, h, edgeless);
                int endDist = minChebyshevToTargets(lastHop.x, lastHop.y, enemyTargets, w, h, edgeless);

                score += (startDist - endDist) * 550;

                boolean opEnemyCapture = false;
                boolean isJump = (op.hops.size() > 1);
                int prevX = op.x0;
                int prevY = op.y0;
                for (OctiHop hop : op.hops) {
                    int dx = hop.x - prevX;
                    int dy = hop.y - prevY;
                    if (edgeless) {
                        for (int d = -2; d <= 2; d++) {
                            if (((prevX + d) % w + w) % w == hop.x) { dx = d; break; }
                        }
                        for (int d = -2; d <= 2; d++) {
                            if (((prevY + d) % h + h) % h == hop.y) { dy = d; break; }
                        }
                    }
                    if (Math.abs(dx) == 2 || Math.abs(dy) == 2) {
                        isJump = true;
                        int mx = edgeless ? (((prevX + dx / 2) % w + w) % w) : (prevX + dx / 2);
                        int my = edgeless ? (((prevY + dy / 2) % h + h) % h) : (prevY + dy / 2);
                        if (board.isValidCoord(mx, my)) {
                            OctiSquare midSq = board.squareAt(mx, my);
                            if (squareHasAllyOf(midSq, mode, turn)) {
                                score += profile.moveOrderLeapfrogBonus;
                            }
                            if (hop.capture) {
                                int midBitIdx = my * 9 + mx;
                                int midWord = (midBitIdx >>> 6) & 1;
                                long midBit = 1L << (midBitIdx & 63);
                                boolean alreadyScoredMid = ((capturedSqMask[midWord] & midBit) != 0L);
                                capturedSqMask[midWord] |= midBit;

                                int capturedPodsOnMid = midSq.getNumPods();
                                int capturedProngsOnMid = 0;
                                for (int pi = 0; pi < capturedPodsOnMid; ++pi) {
                                    OctiPod pMid = midSq.getPodAt(pi);
                                    if (pMid != null) {
                                        capturedProngsOnMid += pMid.getNumProngs();
                                    }
                                }
                                if (midSq.hasEnemyOf(turn)) {
                                    opEnemyCapture = true;
                                    if (!alreadyScoredMid) {
                                        score += profile.moveOrderCaptureBonus * capturedPodsOnMid + 600 * capturedProngsOnMid;
                                    }
                                } else if (!alreadyScoredMid) {
                                    int freeProngs = board.getGame() != null ? board.getGame().getFreeProngs(turn) : 0;
                                    if (capturedProngsOnMid == 0 || mode == OctiGameMode.GAME_MODE_4) {
                                        score -= profile.moveOrderCaptureBonus;
                                    } else if (freeProngs <= 2 && capturedProngsOnMid > 0) {
                                        score += 300 * capturedProngsOnMid - 800;
                                    } else {
                                        score -= (profile.moveOrderCaptureBonus / 2);
                                    }
                                }
                            }
                        } else if (hop.capture) {
                            opEnemyCapture = true;
                            score += profile.moveOrderCaptureBonus;
                        }
                    } else if (hop.capture) {
                        opEnemyCapture = true;
                        score += profile.moveOrderCaptureBonus;
                    }
                    if (!hop.capture) {
                        score += 250;
                    }
                    prevX = hop.x;
                    prevY = hop.y;
                }

                if (op.isSuperProngBonusMove && !opEnemyCapture && endDist >= startDist && isFinalSegmentForPod) {
                    score -= 600;
                }

                OctiSquare destSq = board.squareAt(lastHop.x, lastHop.y);
                if (isFinalSegmentForPod) {
                    if (isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                            && !destSq.isCapturedByTeamOf(turn)) {
                        if (moveHasEnemyCapture) {
                            score += profile.moveOrderBaseEntryBonus;
                        } else {
                            score += (profile.moveOrderBaseEntryBonus * baseThreatScalePct) / 100;
                        }
                    }
                    if (destSq.getType() == OctiSquareType.NEUTRAL_OCTI_SQUARE && destSq.isEmpty()) {
                        score += 3000;
                    }

                    // Blunder penalty if ending square sits in enemy jump threat mask
                    int destBitIdx = lastHop.y * 9 + lastHop.x;
                    int destWord = (destBitIdx >>> 6) & 1;
                    long destBit = 1L << (destBitIdx & 63);
                    if (!moveHasEnemyCapture && enemyThreatMask != null && (enemyThreatMask[destWord] & destBit) != 0L) {
                        score -= profile.moveOrderBlunderPenalty;
                    }

                    // Avoid stacking multiple friendly pods on non-enemy-base squares
                    if (!isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                            && destSq.getNumPods() >= 1) {
                        score -= 1200 * destSq.getNumPods();
                    }

                    // Penalize premature 1-prong lone-pod rushes during development
                    if (!moveHasEnemyCapture && !isJump && earlyDevelopment && op.pod.getNumProngs() <= 1 && endDist <= 3) {
                        score -= 3200;
                    }
                }

                if (isInitialSegmentForPod) {
                    OctiSquare startSq = board.squareAt(op.x0, op.y0);
                    OctiSquare finalDestSq = board.squareAt(chainFinalX, chainFinalY);
                    if (isEnemyHomeSquareFor(startSq.getType(), mode, turn)) {
                        if (startSq.getNumPods() <= 1
                                && !isEnemyHomeSquareFor(finalDestSq.getType(), mode, turn)) {
                            score -= 22000;
                        }
                    }
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
                score += prongDirectionBonus(board, mode, turn, op.addProngDir, 1100, 750, 400);
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

    private static boolean isTacticalMoveForSearch(OctiBoard board,
                                                   OctiGameMode mode,
                                                   OctiPlayer turn,
                                                   OctiMove move) {
        if (board == null || move == null) {
            return false;
        }
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = board.getGame() != null && board.getGame().useEdgelessBoard();
        List<OctiMoveOp> ops = move.getOps();
        for (int opIdx = 0; opIdx < ops.size(); opIdx++) {
            OctiMoveOp op = ops.get(opIdx);
            int cx = op.x0;
            int cy = op.y0;
            for (OctiHop hop : op.hops) {
                int dx = hop.x - cx;
                int dy = hop.y - cy;
                if (edgeless) {
                    for (int d = -2; d <= 2; d++) {
                        if (((cx + d) % w + w) % w == hop.x) { dx = d; break; }
                    }
                    for (int d = -2; d <= 2; d++) {
                        if (((cy + d) % h + h) % h == hop.y) { dy = d; break; }
                    }
                }
                if (hop.capture && (Math.abs(dx) == 2 || Math.abs(dy) == 2)) {
                    int mx = edgeless ? (((cx + dx / 2) % w + w) % w) : (cx + dx / 2);
                    int my = edgeless ? (((cy + dy / 2) % h + h) % h) : (cy + dy / 2);
                    if (board.isValidCoord(mx, my) && board.squareAt(mx, my).hasEnemyOf(turn)) {
                        return true;
                    }
                }
                cx = hop.x;
                cy = hop.y;
            }
            if (!op.hops.isEmpty()) {
                OctiHop lastHop = op.hops.get(op.hops.size() - 1);
                boolean continuedLater = false;
                for (int j = opIdx + 1; j < ops.size(); j++) {
                    OctiMoveOp opJ = ops.get(j);
                    if (!opJ.hops.isEmpty() && opJ.x0 == lastHop.x && opJ.y0 == lastHop.y) {
                        continuedLater = true;
                        break;
                    }
                }
                if (!continuedLater && board.isValidCoord(lastHop.x, lastHop.y)) {
                    OctiSquare destSq = board.squareAt(lastHop.x, lastHop.y);
                    if (isEnemyHomeSquareFor(destSq.getType(), mode, turn)
                            && !destSq.isCapturedByTeamOf(turn)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    private static int historyBucketForMoveStr(String moveStr) {
        if (moveStr == null || moveStr.isEmpty()) {
            return 0;
        }
        return (moveStr.hashCode() & 0x7FFFFFFF) & HISTORY_MASK;
    }

    private static List<OctiMove> orderAndCapMoves(OctiBoard board,
                                                   List<OctiMove> rawMoves,
                                                   OctiPlayer turn,
                                                   String ttBestMove,
                                                   int maxBranch) {
        return orderAndCapMoves(board, rawMoves, turn, ttBestMove, maxBranch,
                defaultProfileForDifficulty(BotDifficulty.TACTICAL), true, null, 0);
    }

    private static List<OctiMove> orderAndCapMoves(OctiBoard board,
                                                   List<OctiMove> rawMoves,
                                                   OctiPlayer turn,
                                                   String ttBestMove,
                                                   int maxBranch,
                                                   BotTuningProfile profile,
                                                   boolean filterDominated) {
        return orderAndCapMoves(board, rawMoves, turn, ttBestMove, maxBranch,
                profile, filterDominated, null, 0);
    }

    private static List<OctiMove> orderAndCapMoves(OctiBoard board,
                                                   List<OctiMove> rawMoves,
                                                   OctiPlayer turn,
                                                   String ttBestMove,
                                                   int maxBranch,
                                                   BotTuningProfile profile,
                                                   boolean filterDominated,
                                                   SearchContext ctx,
                                                   int ply) {
        List<OctiMove> moves = filterDominated ? filterDominatedMoves(board, turn, rawMoves) : rawMoves;
        List<int[]> targets = targetEnemyBases(board, turn);
        long[] enemyThreatMask = computeEnemyThreatMask(board, turn, targets);
        CoalitionPhaseContext turnPhase = analyzeCoalitionPhase(board, turn);
        int baseThreatScalePct = turnPhase.baseThreatScalePct;
        OctiGameMode mode = board.getGameMode();

        String killer0 = null;
        String killer1 = null;
        boolean applyKillers = false;
        boolean applyHistory = false;
        int turnSlot = (turn != null) ? turn.getValue() : 0;
        if (ctx != null) {
            if (profile.useKillers && ply >= 0 && ply < MAX_SEARCH_PLY) {
                applyKillers = true;
                killer0 = ctx.killers[ply][0];
                killer1 = ctx.killers[ply][1];
            }
            if (profile.useHistory && turnSlot >= 0 && turnSlot < 5) {
                applyHistory = true;
            }
        }

        boolean hasTtBest = (ttBestMove != null && !ttBestMove.isEmpty());
        int n = moves.size();
        int[][] scored = new int[n][2];
        for (int i = 0; i < n; i++) {
            OctiMove m = moves.get(i);
            int s = scoreMoveHeuristic(board, m, turn, targets, profile, enemyThreatMask, baseThreatScalePct);
            String mStr = (hasTtBest || applyKillers || applyHistory) ? m.asString() : null;
            if (hasTtBest && mStr.equals(ttBestMove)) {
                s += 200000;
            } else if ((applyKillers || applyHistory) && mStr != null
                    && !isTacticalMoveForSearch(board, mode, turn, m)) {
                if (applyKillers && mStr.equals(killer0)) {
                    s += 12000;
                    ctx.killerHits++;
                } else if (applyKillers && mStr.equals(killer1)) {
                    s += 10500;
                    ctx.killerHits++;
                }
                if (applyHistory) {
                    int hv = ctx.history[turnSlot][historyBucketForMoveStr(mStr)];
                    if (hv > 0) {
                        s += Math.min(6000, hv * 8);
                    }
                }
            }
            scored[i][0] = s;
            scored[i][1] = i;
        }

        java.util.Arrays.sort(scored, (a, b) -> Integer.compare(b[0], a[0]));

        int limit = Math.min(n, maxBranch);
        List<OctiMove> result = new ArrayList<>(limit);
        if (n <= limit) {
            for (int i = 0; i < limit; i++) {
                result.add(moves.get(scored[i][1]));
            }
            return result;
        }

        Map<Long, Integer> quietProngCountByPrelude = new HashMap<>();
        Map<Long, Integer> quietStepCountByPrelude = new HashMap<>();
        Map<Long, Integer> twoPreludeQuietCount = new HashMap<>();
        List<Integer> deferredIndices = new ArrayList<>();

        for (int i = 0; i < n && result.size() < limit; i++) {
            int mIdx = scored[i][1];
            OctiMove m = moves.get(mIdx);
            List<OctiMoveOp> ops = m.getOps();
            int numPreludes = 0;
            for (OctiMoveOp op : ops) {
                if (op.isSuperProngBonusMove) {
                    numPreludes++;
                }
            }
            if (numPreludes == 0) {
                result.add(m);
                continue;
            }

            if (isTacticalMoveForSearch(board, mode, turn, m)) {
                result.add(m);
                continue;
            }

            OctiMoveOp p0 = ops.get(0);
            OctiHop p0End = p0.hops.isEmpty() ? new OctiHop(p0.x0, p0.y0, false) : p0.hops.get(p0.hops.size() - 1);
            boolean p0Cap = false;
            for (OctiHop hp : p0.hops) {
                if (hp.capture) {
                    p0Cap = true;
                    break;
                }
            }
            long pKey = ((long) (p0.x0 & 0xFF))
                    | (((long) (p0.y0 & 0xFF)) << 8)
                    | (((long) (p0End.x & 0xFF)) << 16)
                    | (((long) (p0End.y & 0xFF)) << 24)
                    | ((p0Cap ? 1L : 0L) << 32);

            if (numPreludes >= 2) {
                int cnt = twoPreludeQuietCount.getOrDefault(pKey, 0);
                if (cnt < 2) {
                    twoPreludeQuietCount.put(pKey, cnt + 1);
                    result.add(m);
                } else {
                    deferredIndices.add(mIdx);
                }
                continue;
            }

            OctiMoveOp regOp = ops.get(ops.size() - 1);
            boolean isProngOrPodReg = regOp.hops.isEmpty();
            if (isProngOrPodReg) {
                int cnt = quietProngCountByPrelude.getOrDefault(pKey, 0);
                if (cnt < 2) {
                    quietProngCountByPrelude.put(pKey, cnt + 1);
                    result.add(m);
                } else {
                    deferredIndices.add(mIdx);
                }
            } else {
                int cnt = quietStepCountByPrelude.getOrDefault(pKey, 0);
                if (cnt < 3) {
                    quietStepCountByPrelude.put(pKey, cnt + 1);
                    result.add(m);
                } else {
                    deferredIndices.add(mIdx);
                }
            }
        }

        for (int mIdx : deferredIndices) {
            if (result.size() >= limit) {
                break;
            }
            result.add(moves.get(mIdx));
        }
        return result;
    }

    /**
     * Recursive 2001 Jump-Capture & Reachability evaluation (zero-allocation bitmask).
     */
    private static void evaluateJumpCaptureRecursive(OctiBoard board,
                                                     int startX, int startY,
                                                     int x, int y,
                                                     OctiPod pod,
                                                     OctiPlayer p,
                                                     boolean edgeless,
                                                     int depth,
                                                     long[] jumpedMask,
                                                     long[] visitedMask,
                                                     long[] capturedSqMask,
                                                     List<int[]> targets,
                                                     int[] outStats) {
        // outStats[0] = reachableSquares, outStats[1] = capturableEnemyPods, outStats[2] = capturableEnemyProngs, outStats[3] = minJumpDistToTargets
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
            int dx = dir.deltaX();
            int dy = dir.deltaY();
            int jx = edgeless ? (((x + dx) % w + w) % w) : (x + dx);
            int jy = edgeless ? (((y + dy) % h + h) % h) : (y + dy);
            int lx = edgeless ? (((x + 2 * dx) % w + w) % w) : (x + 2 * dx);
            int ly = edgeless ? (((y + 2 * dy) % h + h) % h) : (y + 2 * dy);

            if (!board.isValidCoord(jx, jy) || !board.isValidCoord(lx, ly)) {
                continue;
            }

            int jumpIdx = jy * 9 + jx;
            int jumpWord = (jumpIdx >>> 6) & 1;
            long jumpBit = 1L << (jumpIdx & 63);
            if ((jumpedMask[jumpWord] & jumpBit) != 0L) {
                continue;
            }

            OctiSquare midSq = board.squareAt(jx, jy);
            OctiSquare landSq = board.squareAt(lx, ly);
            boolean canLand = landSq.isEmpty() || (lx == startX && ly == startY);
            if (midSq.isEmpty() || !canLand) {
                continue;
            }

            int destIdx = ly * 9 + lx;
            int destWord = (destIdx >>> 6) & 1;
            long destBit = 1L << (destIdx & 63);
            if ((visitedMask[destWord] & destBit) != 0L) {
                continue;
            }
            visitedMask[destWord] |= destBit;
            outStats[0]++;
            if (targets != null && !targets.isEmpty()) {
                int jd = minChebyshevToTargets(lx, ly, targets, w, h, edgeless);
                if (jd < outStats[3]) {
                    outStats[3] = jd;
                }
            }

            if (midSq.hasEnemyOf(p)) {
                int midIdx = jy * 9 + jx;
                int midWord = (midIdx >>> 6) & 1;
                long midBit = 1L << (midIdx & 63);
                if ((capturedSqMask[midWord] & midBit) == 0L) {
                    capturedSqMask[midWord] |= midBit;
                    int enemyPods = midSq.getNumPods();
                    outStats[1] += enemyPods;
                    for (int pi = 0; pi < enemyPods; pi++) {
                        OctiPod ep = midSq.getPodAt(pi);
                        if (ep != null) {
                            outStats[2] += ep.getNumProngs();
                        }
                    }
                }
            }

            long[] nextJumpedMask = new long[]{jumpedMask[0], jumpedMask[1]};
            nextJumpedMask[jumpWord] |= jumpBit;

            evaluateJumpCaptureRecursive(board, startX, startY, lx, ly, pod, p, edgeless,
                    depth + 1, nextJumpedMask, visitedMask, capturedSqMask, targets, outStats);
        }
    }

    public static int evaluatePosition(OctiBoard board, OctiPlayer perspective) {
        return evaluatePosition(board, perspective, defaultProfileForDifficulty(BotDifficulty.MASTER));
    }

    public static int evaluatePosition(OctiBoard board, OctiPlayer perspective, BotTuningProfile profile) {
        if (board == null || board.getGame() == null
                || perspective == null || perspective == OctiPlayer.PLAYER_UNDEF) {
            return 0;
        }
        if (profile == null) {
            profile = defaultProfileForDifficulty(BotDifficulty.MASTER);
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
        int allyArmedPods = 0;
        int enemyArmedPods = 0;
        int allyUnarmedHomePods = 0;
        int enemyUnarmedHomePods = 0;

        int[] playerBoardPods = new int[5];
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                int numPods = sq.getNumPods();
                if (numPods > 0) {
                    OctiPlayer sqOwner = sq.getPodAt(0).getType().toPlayer();
                    if (sqOwner != null && sqOwner.getValue() >= 0 && sqOwner.getValue() < 5) {
                        playerBoardPods[sqOwner.getValue()] += numPods;
                    }
                }
            }
        }

        CoalitionPhaseContext allyPhase = analyzeCoalitionPhase(board, perspective);
        CoalitionPhaseContext enemyPhase = analyzeCoalitionPhase(board, sampleEnemy);

        int allyBaseThreatScalePct = allyPhase.baseThreatScalePct;
        int enemyBaseThreatScalePct = enemyPhase.baseThreatScalePct;
        boolean allyBaseWinFeasible = allyPhase.feasible;
        boolean enemyBaseWinFeasible = enemyPhase.feasible;
        int allyCapturedPods = allyPhase.capturedPods;
        int enemyCapturedPods = enemyPhase.capturedPods;

        List<int[]> allyPodCoords = new ArrayList<>();
        List<int[]> enemyPodCoords = new ArrayList<>();
        long[] allyJumpThreatMask = new long[2];
        long[] enemyJumpThreatMask = new long[2];

        int allyOneStepBaseThreatRaw = 0;
        int enemyOneStepBaseThreatRaw = 0;
        int allyOneJumpBaseThreatRaw = 0;
        int enemyOneJumpBaseThreatRaw = 0;
        int allyTwoPlyBaseThreatRaw = 0;
        int enemyTwoPlyBaseThreatRaw = 0;
        int lostAlliedBasesScore = 0;

        int podZeroVal = profile.podZeroProngVal;
        int podOneVal = (mode == OctiGameMode.GAME_MODE_OFK) ? (profile.podOneProngVal * 135) / 100 : profile.podOneProngVal;
        int podTwoVal = (mode == OctiGameMode.GAME_MODE_OFK) ? (profile.podTwoProngVal * 135) / 100 : profile.podTwoProngVal;
        int podThreePlusVal = (mode == OctiGameMode.GAME_MODE_OFK) ? (profile.podThreePlusProngVal * 135) / 100 : profile.podThreePlusProngVal;

        // 1. Base Occupation & Square Stacking Assessment
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                OctiSquareType sqType = sq.getType();

                int numBases = Math.max(1, (game.getBases() != null ? game.getBases().getValue() : 1));
                if (isEnemyHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        int baseBonus = ((profile.capturedEnemyBaseBonus / numBases) * allyBaseThreatScalePct) / 100;
                        totalScore += baseBonus;
                    }
                } else if (isAlliedHomeSquareFor(sqType, mode, perspective)) {
                    if (sq.isCaptured() && !sq.isCapturedByTeamOf(perspective)) {
                        int basePen = ((profile.lostAlliedBasePenalty / numBases) * enemyBaseThreatScalePct) / 100;
                        totalScore -= basePen;
                        lostAlliedBasesScore -= basePen;
                    }
                } else if (sqType == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                    if (sq.isCapturedByTeamOf(perspective)) {
                        totalScore += profile.neutralOctiSquareBonus;
                    } else if (sq.isCaptured()) {
                        totalScore -= profile.neutralOctiSquareBonus;
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
                        if (onAlliedHome && p.getNumProngs() == 0) {
                            if (sqAlly) {
                                allyUnarmedHomePods++;
                            } else {
                                enemyUnarmedHomePods++;
                            }
                        }
                    }

                    // Penalize stacking multiple pods on a non-enemy-base square
                    if (numPods > 1 && !onCapturedEnemyBase) {
                        int stackPen = (numPods - 1) * 450 + unarmedCount * 950;
                        totalScore += sqAlly ? -stackPen : stackPen;
                    }
                }

                for (int i = 0; i < numPods; i++) {
                    OctiPod pod = sq.getPodAt(i);
                    OctiPlayer owner = pod.getType().toPlayer();
                    boolean isAlly = OctiPlayer.areAllied(mode, owner, perspective);
                    List<int[]> targets = isAlly ? allyTargets : enemyTargets;
                    int numProngs = pod.getNumProngs();

                    if (numProngs > 0) {
                        if (isAlly) {
                            allyPodCoords.add(new int[]{x, y});
                            allyArmedPods++;
                        } else {
                            enemyPodCoords.add(new int[]{x, y});
                            enemyArmedPods++;
                        }
                    }

                    int dist = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                    boolean onCapturedEnemyBase = isEnemyHomeSquareFor(sqType, mode, owner)
                            && sq.isCapturedByTeamOf(owner);

                    int scalePct = isAlly ? allyBaseThreatScalePct : enemyBaseThreatScalePct;
                    int podVal;
                    if (numProngs == 0 && !onCapturedEnemyBase
                            && sqType != OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                        podVal = podZeroVal;
                    } else if (numProngs == 1) {
                        int distBonus = (Math.max(0, (10 - dist)) * 110 * scalePct) / 100;
                        podVal = podOneVal + distBonus;
                        if (!isAlliedHomeSquareFor(sqType, mode, owner) && dist <= 4 && dist >= 2) {
                            podVal -= profile.loneOneProngRushPenalty;
                        }
                    } else if (numProngs == 2) {
                        int distBonus = (Math.max(0, (10 - dist)) * 180 * scalePct) / 100;
                        podVal = podTwoVal + distBonus;
                    } else {
                        int distBonus = (Math.max(0, (10 - dist)) * 210 * scalePct) / 100;
                        podVal = podThreePlusVal + distBonus;
                    }

                    boolean hasOrth = false;
                    boolean hasDiag = false;
                    boolean hasOneStepBaseThreat = false;
                    boolean hasOneJumpBaseThreat = false;

                    for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                        OctiDir dir = OctiDir.fromIndex(d);
                        OctiProngType pt = pod.prongAt(dir);
                        if (pt == OctiProngType.NO_PRONG) {
                            continue;
                        }
                        podVal += (pt == OctiProngType.SUPER_PRONG) ? profile.superProngVal : profile.normalProngVal;
                        if (isOrthogonalDir(dir)) hasOrth = true;
                        if (isDiagonalDir(dir)) hasDiag = true;

                        podVal += prongDirectionBonus(board, mode, owner, dir, 380, 260, 120);

                        int nx = edgeless ? (((x + dir.deltaX()) % w + w) % w) : (x + dir.deltaX());
                        int ny = edgeless ? (((y + dir.deltaY()) % h + h) % h) : (y + dir.deltaY());
                        if (board.isValidCoord(nx, ny)) {
                            int ndist = minChebyshevToTargets(nx, ny, targets, w, h, edgeless);
                            if (ndist < dist) {
                                podVal += 140;
                            }
                            OctiSquare nsq = board.squareAt(nx, ny);
                            boolean canStepBase = (mode == OctiGameMode.GAME_MODE_OFK)
                                    ? nsq.isEmpty()
                                    : !nsq.hasEnemyOf(owner);
                            if (ndist == 0 && canStepBase) {
                                hasOneStepBaseThreat = true;
                            }
                            // Leapfrog battery detection
                            if (squareHasAllyOf(nsq, mode, owner)) {
                                int jx = edgeless ? (((x + 2 * dir.deltaX()) % w + w) % w) : (x + 2 * dir.deltaX());
                                int jy = edgeless ? (((y + 2 * dir.deltaY()) % h + h) % h) : (y + 2 * dir.deltaY());
                                if (board.isValidCoord(jx, jy)) {
                                    OctiSquare destSq = board.squareAt(jx, jy);
                                    boolean canJumpBase = (mode == OctiGameMode.GAME_MODE_OFK) ? destSq.isEmpty() : !destSq.hasEnemyOf(owner);
                                    if (destSq.isEmpty() || canJumpBase) {
                                        podVal += profile.leapfrogBatteryWeight;
                                        int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                                        if (jdist < dist) {
                                            podVal += profile.leapfrogForwardBonus;
                                        }
                                    }
                                }
                            }
                        }

                        int jx = edgeless ? (((x + 2 * dir.deltaX()) % w + w) % w) : (x + 2 * dir.deltaX());
                        int jy = edgeless ? (((y + 2 * dir.deltaY()) % h + h) % h) : (y + 2 * dir.deltaY());
                        if (board.isValidCoord(nx, ny) && board.isValidCoord(jx, jy)) {
                            OctiSquare midSq = board.squareAt(nx, ny);
                            OctiSquare destSq = board.squareAt(jx, jy);
                            if (!midSq.isEmpty() && destSq.isEmpty()) {
                                int jdist = minChebyshevToTargets(jx, jy, targets, w, h, edgeless);
                                if (jdist == 0) {
                                    hasOneJumpBaseThreat = true;
                                }
                            }
                        }
                    }

                    if (hasOneStepBaseThreat) {
                        int stepBonus = (profile.oneStepBaseThreatBonus * scalePct) / 100;
                        if (isAlly) {
                            allyOneStepBaseThreatRaw += stepBonus;
                        } else {
                            enemyOneStepBaseThreatRaw += stepBonus;
                        }
                    }
                    if (hasOneJumpBaseThreat) {
                        int jumpBonus = (profile.oneJumpBaseThreatBonus * scalePct) / 100;
                        if (isAlly) {
                            allyOneJumpBaseThreatRaw += jumpBonus;
                        } else {
                            enemyOneJumpBaseThreatRaw += jumpBonus;
                        }
                    }

                    // 2-ply base threat check
                    if (dist == 1 && profile.twoPlyBaseThreatWeight > 0) {
                        int twoPlyBonus = (profile.twoPlyBaseThreatWeight * scalePct) / 100;
                        if (isAlly) {
                            allyTwoPlyBaseThreatRaw += twoPlyBonus;
                        } else {
                            enemyTwoPlyBaseThreatRaw += twoPlyBonus;
                        }
                    }

                    if (hasOrth && hasDiag) {
                        podVal += profile.orthDiagVersatilityBonus;
                    } else if (numProngs >= 2 && hasDiag && !hasOrth) {
                        podVal -= profile.diagOnlyParityPenalty;
                    }

                    // Jump mobility and threats
                    if (numProngs > 0) {
                        int[] mobStats = new int[4];
                        mobStats[3] = dist;
                        long[] visitedMask = new long[2];
                        long[] capMask = isAlly ? allyJumpThreatMask : enemyJumpThreatMask;
                        evaluateJumpCaptureRecursive(board, x, y, x, y, pod, owner, edgeless,
                                0, new long[2], visitedMask, capMask, targets, mobStats);
                        podVal += mobStats[0] * profile.jumpReachableSquareWeight;
                        podVal += mobStats[1] * profile.jumpCapturablePodWeight;
                        podVal += mobStats[2] * profile.jumpCapturableProngWeight;
                        if (mobStats[3] < dist) {
                            podVal += ((dist - mobStats[3]) * profile.jumpReachStepWeight * scalePct) / 100;
                        }
                    }

                    totalScore += isAlly ? podVal : -podVal;
                }
            }
        }

        int contestedScalePct = Math.max(0, Math.min(100, profile.contestedEnemyBaseThreatScalePct));
        boolean enemyContests = (enemyOneStepBaseThreatRaw > 0
                || enemyOneJumpBaseThreatRaw > 0
                || enemyTwoPlyBaseThreatRaw > 0
                || lostAlliedBasesScore < 0
                || enemyJumpThreatMask[0] != 0L
                || enemyJumpThreatMask[1] != 0L);
        if (contestedScalePct != 100 && enemyContests) {
            allyOneStepBaseThreatRaw = (allyOneStepBaseThreatRaw * contestedScalePct) / 100;
            allyOneJumpBaseThreatRaw = (allyOneJumpBaseThreatRaw * contestedScalePct) / 100;
            allyTwoPlyBaseThreatRaw = (allyTwoPlyBaseThreatRaw * contestedScalePct) / 100;
        }
        totalScore += (allyOneStepBaseThreatRaw - enemyOneStepBaseThreatRaw)
                + (allyOneJumpBaseThreatRaw - enemyOneJumpBaseThreatRaw)
                + (allyTwoPlyBaseThreatRaw - enemyTwoPlyBaseThreatRaw);

        // Army Development Bonuses (Armed Pods)
        if (allyArmedPods >= 1) totalScore += profile.firstArmedPodBonus;
        if (allyArmedPods >= 2) totalScore += profile.secondArmedPodBonus;
        if (allyArmedPods >= 3) totalScore += profile.thirdArmedPodBonus;

        if (enemyArmedPods >= 1) totalScore -= profile.firstArmedPodBonus;
        if (enemyArmedPods >= 2) totalScore -= profile.secondArmedPodBonus;
        if (enemyArmedPods >= 3) totalScore -= profile.thirdArmedPodBonus;

        int effMonopolyWeight = (mode == OctiGameMode.GAME_MODE_OFK && profile.ofkSinglePodMonopolyPenalty >= 0)
                ? profile.ofkSinglePodMonopolyPenalty
                : profile.singlePodMonopolyPenalty;

        // Single-Pod Monopoly & Heavy-Pod Threat Penalty
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                OctiSquare sq = board.squareAt(x, y);
                for (int i = 0; i < sq.getNumPods(); i++) {
                    OctiPod pod = sq.getPodAt(i);
                    int prongs = pod.getNumProngs();
                    if (prongs > 2) {
                        OctiPlayer podOwner = pod.getType().toPlayer();
                        boolean isAlly = OctiPlayer.areAllied(mode, podOwner, perspective);
                        int unarmedHome = isAlly ? allyUnarmedHomePods : enemyUnarmedHomePods;
                        int armedPods = isAlly ? allyArmedPods : enemyArmedPods;
                        boolean triggerMonopoly = (unarmedHome >= 2)
                                || (prongs > 3 && armedPods <= 1 && (unarmedHome + game.getFreePods(podOwner)) >= 2);
                        if (triggerMonopoly) {
                            int monPen = (prongs - 2) * effMonopolyWeight;
                            totalScore += isAlly ? -monPen : monPen;
                        }
                        if (!isAlly && prongs >= 4 && profile.enemyHeavyPodThreatPenalty != 0) {
                            totalScore -= (prongs - 3) * profile.enemyHeavyPodThreatPenalty;
                        }
                        if (prongs >= 5) {
                            int overArmPen = (prongs - 4) * 650 + (prongs >= 6 ? (prongs - 5) * 900 : 0);
                            totalScore += isAlly ? -overArmPen : overArmPen;
                        }
                        if (prongs > 3) {
                            List<int[]> targets = isAlly ? allyTargets : enemyTargets;
                            int d = minChebyshevToTargets(x, y, targets, w, h, edgeless);
                            if (d >= 4 && armedPods >= 3) {
                                int backRankPen = (prongs - 3) * 320;
                                totalScore += isAlly ? -backRankPen : backRankPen;
                            }
                        }
                    }
                }
            }
        }

        // Hanging-Pod Vulnerability Penalty
        boolean opponentTurnNext = (game.getTurn() != perspective);
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

                int prongWeight = (mode == OctiGameMode.GAME_MODE_OFK)
                        ? profile.ofkProngSeizureWeight
                        : profile.hangingProngWeight;

                if (sqAlly && (enemyJumpThreatMask[wordIdx] & bitMask) != 0L) {
                    int vulnPen = numPods * profile.hangingPodBasePenalty + prongsOnSq * prongWeight;
                    if (opponentTurnNext) {
                        vulnPen = (vulnPen * profile.opponentTurnHangingPct) / 100;
                    }
                    if (prongsOnSq > 0 && allyArmedPods == 1) {
                        vulnPen += profile.soleArmedPodDangerPenalty;
                    }
                    if (numPods >= totalAllyPodsOnBoard) {
                        vulnPen += profile.wipeoutDangerPenalty;
                    } else if (mode == OctiGameMode.GAME_MODE_4
                            && sqOwner != null && numPods >= playerBoardPods[sqOwner.getValue()]
                            && game.getFreePods(sqOwner) == 0) {
                        vulnPen += profile.wipeoutDangerPenalty / 2;
                    }
                    totalScore -= vulnPen;
                } else if (!sqAlly && (allyJumpThreatMask[wordIdx] & bitMask) != 0L) {
                    int vulnPen = numPods * profile.hangingPodBasePenalty + prongsOnSq * prongWeight;
                    if (!opponentTurnNext) {
                        vulnPen = (vulnPen * profile.opponentTurnHangingPct) / 100;
                    }
                    if (prongsOnSq > 0 && enemyArmedPods == 1) {
                        vulnPen += profile.soleArmedPodDangerPenalty;
                    }
                    if (numPods >= totalEnemyPodsOnBoard) {
                        vulnPen += profile.wipeoutDangerPenalty;
                    } else if (mode == OctiGameMode.GAME_MODE_4
                            && sqOwner != null && numPods >= playerBoardPods[sqOwner.getValue()]
                            && game.getFreePods(sqOwner) == 0) {
                        vulnPen += profile.wipeoutDangerPenalty / 2;
                    }
                    totalScore += vulnPen;
                }
            }
        }

        if (totalAllyPodsOnBoard == 0) {
            totalScore -= (profile.wipeoutDangerPenalty + profile.soleArmedPodDangerPenalty);
        }
        if (totalEnemyPodsOnBoard == 0) {
            totalScore += (profile.wipeoutDangerPenalty + profile.soleArmedPodDangerPenalty);
        }

        // 2001 Multi-Base Minimum Chebyshev Distance: closest (d_min1) and second-closest (d_min2)
        int allyDistScalePct = (allyBaseWinFeasible ? 100 : (allyCapturedPods > 0 ? 40 : 15));
        int enemyDistScalePct = (enemyBaseWinFeasible ? 100 : (enemyCapturedPods > 0 ? 40 : 15));

        allyDistScalePct = (allyDistScalePct * allyBaseThreatScalePct) / 100;
        enemyDistScalePct = (enemyDistScalePct * enemyBaseThreatScalePct) / 100;

        int allyClosestPodDistWeight = (profile.closestPodDistWeight * allyDistScalePct) / 100;
        int allySecondClosestPodDistWeight = (profile.secondClosestPodDistWeight * allyDistScalePct) / 100;
        int enemyClosestPodDistWeight = (profile.closestPodDistWeight * enemyDistScalePct) / 100;
        int enemySecondClosestPodDistWeight = (profile.secondClosestPodDistWeight * enemyDistScalePct) / 100;

        if (!allyTargets.isEmpty()) {
            for (int[] tb : allyTargets) {
                OctiSquare tsq = board.squareAt(tb[0], tb[1]);
                if (!tsq.isCapturedByTeamOf(perspective)) {
                    int d_min1 = 999;
                    int d_min2 = 999;
                    for (int[] ap : allyPodCoords) {
                        int d = Math.max(torusDist(ap[0], tb[0], w, edgeless), torusDist(ap[1], tb[1], h, edgeless));
                        if (d < d_min1) {
                            d_min2 = d_min1;
                            d_min1 = d;
                        } else if (d < d_min2) {
                            d_min2 = d;
                        }
                    }
                    if (d_min1 < 999) {
                        totalScore += Math.max(0, 10 - d_min1) * allyClosestPodDistWeight;
                    }
                    if (d_min2 < 999 && allySecondClosestPodDistWeight > 0) {
                        totalScore += Math.max(0, 10 - d_min2) * allySecondClosestPodDistWeight;
                    }
                }
            }
        }

        if (!enemyTargets.isEmpty()) {
            for (int[] tb : enemyTargets) {
                OctiSquare tsq = board.squareAt(tb[0], tb[1]);
                if (!tsq.isCapturedByTeamOf(sampleEnemy)) {
                    int d_min1 = 999;
                    int d_min2 = 999;
                    for (int[] ep : enemyPodCoords) {
                        int d = Math.max(torusDist(ep[0], tb[0], w, edgeless), torusDist(ep[1], tb[1], h, edgeless));
                        if (d < d_min1) {
                            d_min2 = d_min1;
                            d_min1 = d;
                        } else if (d < d_min2) {
                            d_min2 = d;
                        }
                    }
                    if (d_min1 < 999) {
                        totalScore -= Math.max(0, 10 - d_min1) * enemyClosestPodDistWeight;
                    }
                    if (d_min2 < 999 && enemySecondClosestPodDistWeight > 0) {
                        totalScore -= Math.max(0, 10 - d_min2) * enemySecondClosestPodDistWeight;
                    }
                }
            }
        }

        // Reserves and material bonuses
        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int pIdx = 1; pIdx <= numPlayers; pIdx++) {
            OctiPlayer p = OctiPlayer.fromValue(pIdx);
            boolean isAlly = OctiPlayer.areAllied(mode, p, perspective);
            int freePods = game.getFreePods(p);
            int capturedPodPen = (mode == OctiGameMode.GAME_MODE_OFK) ? 4800 : profile.capturedPodPenalty;
            int freeProngVal = (mode == OctiGameMode.GAME_MODE_OFK) ? 180 : profile.reserveFreeProngVal;
            int resVal = freePods * profile.reserveFreePodVal
                    - game.getCapturedPods(p) * capturedPodPen
                    + game.getFreeProngs(p) * freeProngVal
                    + game.getFreeSuperProngs(p) * 150;
            // Penalize pulling reserve pods onto the board while home pods are still unarmed
            if (mode != OctiGameMode.GAME_MODE_OFK && freePods < 4
                    && (isAlly ? (allyUnarmedHomePods >= 2 && totalAllyPodsOnBoard >= 2)
                               : (enemyUnarmedHomePods >= 2 && totalEnemyPodsOnBoard >= 2))
                    && game.getFreeProngs(p) >= 6) {
                resVal -= (4 - freePods) * 1500;
            }
            if (mode != OctiGameMode.GAME_MODE_OFK
                    && (isAlly ? (allyUnarmedHomePods >= 1 && totalAllyPodsOnBoard >= 2)
                               : (enemyUnarmedHomePods >= 1 && totalEnemyPodsOnBoard >= 2))
                    && game.getFreeProngs(p) >= 6) {
                int unarmedHome = isAlly ? allyUnarmedHomePods : enemyUnarmedHomePods;
                resVal -= unarmedHome * profile.unarmedHomePodPenalty;
            }
            if (mode == OctiGameMode.GAME_MODE_4 && game.isPlayerEliminated(p)) {
                resVal -= 20000;
            }
            if (board.canLiberate(p) && game.getCapturedPods(p) > 0) {
                resVal += profile.canLiberateBonus;
            }
            totalScore += isAlly ? resVal : -resVal;
        }

        return totalScore;
    }

    private static final class SearchContext {
        long nodesVisited = 0;
        long flushedNodes = 0;
        long deadlineMs = 0;
        volatile boolean timedOut = false;
        java.util.concurrent.atomic.AtomicBoolean stopSignal = null;
        final OctiGameSnapshot[] plySnapshots = new OctiGameSnapshot[MAX_SEARCH_PLY];
        final String[][] killers = new String[MAX_SEARCH_PLY][2];
        final int[][] history = new int[5][HISTORY_SIZE];
        long killerHits = 0;
        long killerUpdates = 0;
        long historyUpdates = 0;
        long pvsReSearches = 0;
        long lmrReductions = 0;
        long lmrReSearches = 0;
        long qsearchTtProbes = 0;
        long qsearchTtHits = 0;
        long aspirationReSearches = 0;

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

    private void recordNodeVisit(SearchContext ctx) {
        ctx.nodesVisited++;
        if ((ctx.nodesVisited & 63L) == 0L) {
            long delta = ctx.nodesVisited - ctx.flushedNodes;
            if (delta > 0L) {
                ctx.flushedNodes = ctx.nodesVisited;
                liveNodes.addAndGet(delta);
            }
        }
    }

    private void flushRemainingNodes(SearchContext ctx) {
        long delta = ctx.nodesVisited - ctx.flushedNodes;
        if (delta > 0L) {
            ctx.flushedNodes = ctx.nodesVisited;
            liveNodes.addAndGet(delta);
        }
    }

    private void storeQuiesceTt(int ttIdx,
                                Object ttLock,
                                long hashKey,
                                int storeDepth,
                                int val,
                                int ply,
                                byte flag,
                                String bestMoveStr) {
        synchronized (ttLock) {
            if (ttKeys[ttIdx] == 0L || storeDepth >= ttDepths[ttIdx]) {
                boolean sameKey = (ttKeys[ttIdx] == hashKey);
                ttKeys[ttIdx] = hashKey;
                ttDepths[ttIdx] = storeDepth;
                int storeScore = val;
                if (storeScore >= WIN_SCORE - 1000) {
                    storeScore += ply;
                } else if (storeScore <= -WIN_SCORE + 1000) {
                    storeScore -= ply;
                }
                ttScores[ttIdx] = storeScore;
                if (bestMoveStr != null && !bestMoveStr.isEmpty()) {
                    ttBestMoves[ttIdx] = bestMoveStr;
                } else if (!sameKey) {
                    ttBestMoves[ttIdx] = "";
                }
                ttFlags[ttIdx] = flag;
            }
        }
    }

    private int quiesce(OctiBoard scratchBoard,
                        int qDepth,
                        int ply,
                        int alpha,
                        int beta,
                        OctiPlayer currentPerspective,
                        SearchContext ctx) {
        recordNodeVisit(ctx);
        if ((ctx.nodesVisited & 63L) == 0L) {
            if ((ctx.stopSignal != null && ctx.stopSignal.get())
                    || steadyClockMs() >= ctx.deadlineMs
                    || Thread.currentThread().isInterrupted()) {
                ctx.timedOut = true;
                return evaluatePosition(scratchBoard, currentPerspective, profile);
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

        boolean useQsearchTt = profile.qsearchUseTt;
        int qTtDepth = Math.max(-120, -qDepth);
        long hashKey = 0L;
        int ttIdx = 0;
        Object ttLock = null;
        String ttMove = "";
        int origAlpha = alpha;

        if (useQsearchTt) {
            ctx.qsearchTtProbes++;
            hashKey = computePositionHash(scratchBoard);
            ttIdx = (int) (hashKey & TT_MASK);
            ttLock = ttLocks[ttIdx & TT_LOCK_MASK];
            synchronized (ttLock) {
                if (ttKeys[ttIdx] == hashKey && ttDepths[ttIdx] > -128) {
                    String m = ttBestMoves[ttIdx];
                    ttMove = (m != null) ? m : "";
                    if (ttDepths[ttIdx] >= qTtDepth) {
                        int ttScore = ttScores[ttIdx];
                        if (ttScore >= WIN_SCORE - 1000) {
                            ttScore -= ply;
                        } else if (ttScore <= -WIN_SCORE + 1000) {
                            ttScore += ply;
                        }
                        byte entryFlag = ttFlags[ttIdx];
                        if (entryFlag == 0) {
                            ctx.qsearchTtHits++;
                            return ttScore;
                        }
                        if (entryFlag == 1 && ttScore >= beta) {
                            ctx.qsearchTtHits++;
                            return ttScore;
                        }
                        if (entryFlag == 2 && ttScore <= alpha) {
                            ctx.qsearchTtHits++;
                            return ttScore;
                        }
                    }
                }
            }
        }

        int standPat = evaluatePosition(scratchBoard, currentPerspective, profile);
        if (standPat >= beta) {
            if (useQsearchTt && ttLock != null) {
                storeQuiesceTt(ttIdx, ttLock, hashKey, 0, standPat, ply, (byte) 1, "");
            }
            return beta;
        }
        if (standPat > alpha) {
            alpha = standPat;
        }

        if (qDepth >= profile.maxQuiesceDepth) {
            return standPat;
        }

        List<OctiMove> rawMoves = game.generateLegalMoves();
        if (rawMoves.isEmpty()) {
            return standPat;
        }

        // Filter to tactical moves only: captures and uncaptured enemy base entries
        OctiPlayer turn = game.getTurn();
        List<OctiMove> tacticalMoves = new ArrayList<>();
        for (OctiMove m : rawMoves) {
            if (isTacticalMoveForSearch(scratchBoard, mode, turn, m)) {
                tacticalMoves.add(m);
            }
        }

        if (tacticalMoves.isEmpty()) {
            if (useQsearchTt && ttLock != null) {
                storeQuiesceTt(ttIdx, ttLock, hashKey, 0, standPat, ply,
                        (byte) (standPat > origAlpha ? 0 : 2), "");
            }
            return standPat;
        }

        int qCap = (profile.quiesceBranchCap > 0)
                ? profile.quiesceBranchCap
                : tacticalMoves.size();
        List<OctiMove> ordered = orderAndCapMoves(
                scratchBoard, tacticalMoves, turn, ttMove, qCap, profile, false, null, ply);

        int bestVal = standPat;
        String bestMoveStr = "";
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
                    val = quiesce(scratchBoard, qDepth + 1, ply + 1,
                            alpha, beta, turn, ctx);
                } else {
                    OctiPlayer oppPerspective = (nextTurn != OctiPlayer.PLAYER_UNDEF) ? nextTurn :
                            ((turn == OctiPlayer.PLAYER_A || turn == OctiPlayer.PLAYER_C)
                                    ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A);
                    val = -quiesce(scratchBoard, qDepth + 1, ply + 1,
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
            if (val >= beta) {
                if (useQsearchTt && ttLock != null) {
                    storeQuiesceTt(ttIdx, ttLock, hashKey, qTtDepth, val, ply, (byte) 1, bestMoveStr);
                }
                return beta;
            }
            if (val > alpha) {
                alpha = val;
            }
        }

        if (useQsearchTt && !ctx.timedOut && ttLock != null) {
            storeQuiesceTt(ttIdx, ttLock, hashKey, qTtDepth, alpha, ply,
                    (byte) (alpha > origAlpha ? 0 : 2), bestMoveStr);
        }

        return alpha;
    }

    private int negamax(OctiBoard scratchBoard,
                        int depth,
                        int ply,
                        int alpha,
                        int beta,
                        OctiPlayer currentPerspective,
                        SearchContext ctx) {
        recordNodeVisit(ctx);
        if ((ctx.nodesVisited & 31L) == 0L) {
            if ((ctx.stopSignal != null && ctx.stopSignal.get())
                    || steadyClockMs() >= ctx.deadlineMs
                    || Thread.currentThread().isInterrupted()) {
                ctx.timedOut = true;
                return evaluatePosition(scratchBoard, currentPerspective, profile);
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
            if (profile.maxQuiesceDepth > 0) {
                return quiesce(scratchBoard, 0, ply, alpha, beta, currentPerspective, ctx);
            }
            return evaluatePosition(scratchBoard, currentPerspective, profile);
        }

        boolean useShallowTtMove = (profile.usePvs
                || profile.useKillers
                || profile.useHistory
                || profile.lmrMinDepth > 0);
        long hashKey = computePositionHash(scratchBoard);
        int ttIdx = (int) (hashKey & TT_MASK);
        Object ttLock = ttLocks[ttIdx & TT_LOCK_MASK];
        String ttMove = "";
        synchronized (ttLock) {
            if (ttKeys[ttIdx] == hashKey && ttDepths[ttIdx] > -128) {
                String m = ttBestMoves[ttIdx];
                ttMove = (m != null) ? m : "";
                if (ttDepths[ttIdx] >= depth) {
                    int entryFlag = ttFlags[ttIdx];
                    int entryScore = ttScores[ttIdx];
                    if (entryScore >= WIN_SCORE - 1000) {
                        entryScore -= ply;
                    } else if (entryScore <= -WIN_SCORE + 1000) {
                        entryScore += ply;
                    }

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
                    ttMove = (m != null) ? m : "";
                }
            }
        }

        List<OctiMove> rawMoves = game.generateLegalMoves();
        if (rawMoves.isEmpty()) {
            return evaluatePosition(scratchBoard, currentPerspective, profile);
        }

        OctiPlayer turn = game.getTurn();

        // Enumerate composite moves at root/shallow ply only to keep interior search fast
        if (ply <= profile.compositeMoveMaxPly && depth >= 2) {
            appendCompositeMoves(scratchBoard, turn, rawMoves);
        }

        int maxBranch = (profile.treeBranchCap > 0) ? profile.treeBranchCap : 24;
        List<OctiMove> ordered = orderAndCapMoves(
                scratchBoard, rawMoves, turn, ttMove, maxBranch, profile, true,
                (profile.useKillers || profile.useHistory) ? ctx : null, ply);

        int origAlpha = alpha;
        int bestVal = -INF_SCORE;
        String bestMoveStr = ordered.get(0).asString();
        OctiGameSnapshot snap = ctx.snapshotForPly(ply);
        game.captureSnapshotInto(snap);

        String killer0 = (profile.useKillers && ply >= 0 && ply < MAX_SEARCH_PLY) ? ctx.killers[ply][0] : null;
        String killer1 = (profile.useKillers && ply >= 0 && ply < MAX_SEARCH_PLY) ? ctx.killers[ply][1] : null;
        boolean lmrEnabled = (profile.lmrMinDepth > 0
                && profile.lmrMinMoveIndex > 0
                && depth >= profile.lmrMinDepth
                && (depth - 1) >= 2);
        boolean needMoveClass = (profile.useKillers || profile.useHistory || lmrEnabled);

        for (int mvIdx = 0; mvIdx < ordered.size(); mvIdx++) {
            OctiMove mv = ordered.get(mvIdx);
            boolean mvIsTactical = needMoveClass && isTacticalMoveForSearch(scratchBoard, mode, turn, mv);
            String mvStr = (lmrEnabled || (profile.useKillers && !mvIsTactical) || (profile.useHistory && !mvIsTactical))
                    ? mv.asString()
                    : null;
            boolean mvIsKiller = (profile.useKillers && mvStr != null
                    && (mvStr.equals(killer0) || mvStr.equals(killer1)));

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
                int childDepth = depth - 1;
                if (sameTeamNext) {
                    if (profile.usePvs && mvIdx > 0 && beta > alpha + 1) {
                        val = negamax(scratchBoard, childDepth, ply + 1,
                                alpha, alpha + 1, turn, ctx);
                        if (!ctx.timedOut && val > alpha) {
                            ctx.pvsReSearches++;
                            val = negamax(scratchBoard, childDepth, ply + 1,
                                    alpha, beta, turn, ctx);
                        }
                    } else {
                        val = negamax(scratchBoard, childDepth, ply + 1,
                                alpha, beta, turn, ctx);
                    }
                } else {
                    OctiPlayer oppPerspective = (nextTurn != OctiPlayer.PLAYER_UNDEF) ? nextTurn :
                            ((turn == OctiPlayer.PLAYER_A || turn == OctiPlayer.PLAYER_C)
                                    ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A);

                    int lmrRed = 0;
                    if (lmrEnabled
                            && mvIdx >= profile.lmrMinMoveIndex
                            && (ttMove.isEmpty() || !ttMove.equals(mvStr))
                            && !mvIsTactical
                            && !mvIsKiller) {
                        lmrRed = 1;
                        if (depth >= profile.lmrMinDepth + 2 && mvIdx >= profile.lmrMinMoveIndex * 3) {
                            lmrRed = 2;
                        }
                        lmrRed = Math.min(lmrRed, childDepth - 1);
                    }

                    if (lmrRed > 0) {
                        ctx.lmrReductions++;
                        val = -negamax(scratchBoard, childDepth - lmrRed, ply + 1,
                                -(alpha + 1), -alpha, oppPerspective, ctx);
                        if (!ctx.timedOut && val > alpha) {
                            ctx.lmrReSearches++;
                            if (profile.usePvs && mvIdx > 0 && beta > alpha + 1) {
                                val = -negamax(scratchBoard, childDepth, ply + 1,
                                        -(alpha + 1), -alpha, oppPerspective, ctx);
                                if (!ctx.timedOut && val > alpha) {
                                    ctx.pvsReSearches++;
                                    val = -negamax(scratchBoard, childDepth, ply + 1,
                                            -beta, -alpha, oppPerspective, ctx);
                                }
                            } else {
                                val = -negamax(scratchBoard, childDepth, ply + 1,
                                        -beta, -alpha, oppPerspective, ctx);
                            }
                        }
                    } else if (profile.usePvs && mvIdx > 0 && beta > alpha + 1) {
                        val = -negamax(scratchBoard, childDepth, ply + 1,
                                -(alpha + 1), -alpha, oppPerspective, ctx);
                        if (!ctx.timedOut && val > alpha) {
                            ctx.pvsReSearches++;
                            val = -negamax(scratchBoard, childDepth, ply + 1,
                                    -beta, -alpha, oppPerspective, ctx);
                        }
                    } else {
                        val = -negamax(scratchBoard, childDepth, ply + 1,
                                -beta, -alpha, oppPerspective, ctx);
                    }
                }
            }

            game.restoreSnapshot(snap);

            if (ctx.timedOut) {
                break;
            }

            if (val > bestVal) {
                bestVal = val;
                bestMoveStr = (mvStr != null) ? mvStr : mv.asString();
            }
            if (val > alpha) {
                alpha = val;
            }
            if (alpha >= beta) {
                if (!mvIsTactical && mvStr != null) {
                    if (profile.useKillers && ply >= 0 && ply < MAX_SEARCH_PLY) {
                        if (!mvStr.equals(ctx.killers[ply][0])) {
                            ctx.killers[ply][1] = ctx.killers[ply][0];
                            ctx.killers[ply][0] = mvStr;
                            ctx.killerUpdates++;
                        }
                    }
                    if (profile.useHistory && turn != null) {
                        int pSlot = turn.getValue();
                        if (pSlot >= 0 && pSlot < 5) {
                            int bonus = Math.min(depth * depth, 400);
                            int bucket = historyBucketForMoveStr(mvStr);
                            int hEntry = ctx.history[pSlot][bucket];
                            ctx.history[pSlot][bucket] = hEntry + bonus - (hEntry * bonus) / MAX_HISTORY_VAL;
                            ctx.historyUpdates++;
                        }
                    }
                }
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
                if (ttKeys[ttIdx] == 0L || depth >= ttDepths[ttIdx] - 1) {
                    boolean preserveExistingMove = useShallowTtMove
                            && bestVal <= origAlpha
                            && ttKeys[ttIdx] == hashKey
                            && ttBestMoves[ttIdx] != null
                            && !ttBestMoves[ttIdx].isEmpty();
                    ttKeys[ttIdx] = hashKey;
                    ttDepths[ttIdx] = depth;
                    int storeScore = bestVal;
                    if (storeScore >= WIN_SCORE - 1000) {
                        storeScore += ply;
                    } else if (storeScore <= -WIN_SCORE + 1000) {
                        storeScore -= ply;
                    }
                    ttScores[ttIdx] = storeScore;
                    if (!preserveExistingMove) {
                        ttBestMoves[ttIdx] = bestMoveStr;
                    }
                    ttFlags[ttIdx] = storeFlag;
                }
            }
        }

        return bestVal;
    }

    private boolean searchSingleRootMove(OctiBoard scratchBoard,
                                         OctiGame game,
                                         OctiGameSnapshot rootSnap,
                                         OctiGameMode mode,
                                         OctiPlayer rootTurn,
                                         OctiMove mv,
                                         int d,
                                         int mvIdx,
                                         int alpha,
                                         int beta,
                                         SearchContext ctx,
                                         int[] outScore) {
        try {
            game.move(mv);
        } catch (Throwable t) {
            game.restoreSnapshot(rootSnap);
            return false;
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
            boolean rootNullWin = (profile.usePvs
                    && profile.rootScoreJitter == 0
                    && mvIdx > 0
                    && alpha > -INF_SCORE
                    && beta > alpha + 1);
            if (sameTeamNext) {
                if (rootNullWin) {
                    score = negamax(scratchBoard, d - 1, 1,
                            alpha, alpha + 1, rootTurn, ctx);
                    if (!ctx.timedOut && score > alpha) {
                        ctx.pvsReSearches++;
                        score = negamax(scratchBoard, d - 1, 1,
                                alpha, beta, rootTurn, ctx);
                    }
                } else {
                    score = negamax(scratchBoard, d - 1, 1,
                            alpha, beta, rootTurn, ctx);
                }
            } else {
                OctiPlayer oppPerspective = (nextTurn != OctiPlayer.PLAYER_UNDEF) ? nextTurn :
                        ((rootTurn == OctiPlayer.PLAYER_A || rootTurn == OctiPlayer.PLAYER_C)
                                ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A);
                if (rootNullWin) {
                    score = -negamax(scratchBoard, d - 1, 1,
                            -(alpha + 1), -alpha, oppPerspective, ctx);
                    if (!ctx.timedOut && score > alpha) {
                        ctx.pvsReSearches++;
                        score = -negamax(scratchBoard, d - 1, 1,
                                -beta, -alpha, oppPerspective, ctx);
                    }
                } else {
                    score = -negamax(scratchBoard, d - 1, 1,
                            -beta, -alpha, oppPerspective, ctx);
                }
            }
        }

        game.restoreSnapshot(rootSnap);
        outScore[0] = score;
        return true;
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
        if (profile.compositeMoveMaxPly >= 0) {
            appendCompositeMoves(scratchBoard, rootTurn, legalMoves);
        }

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
            recordNodeVisit(ctx);
            try {
                game.move(mv);
                if (game.isGameOver()
                        && OctiPlayer.areAllied(mode, game.getWinner(), rootTurn)) {
                    game.restoreSnapshot(rootSnap);
                    flushRemainingNodes(ctx);
                    long totalNodes = liveNodes.get();
                    liveDepth.set(1);
                    liveScore.set(WIN_SCORE);
                    if (outStats != null) {
                        outStats.depthReached = 1;
                        outStats.nodesVisited = totalNodes;
                        outStats.bestScore = WIN_SCORE;
                        outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
                    }
                    return mv.asString();
                }
            } catch (Throwable ignored) {
            }
            game.restoreSnapshot(rootSnap);
        }

        // 2. Probe precomputed Zobrist opening playbook if enabled
        if (profile.useOpeningBook) {
            String[] bookMoveHolder = new String[1];
            int[] bookScoreHolder = new int[1];
            int[] bookDepthHolder = new int[1];
            if (probeOpeningBook(scratchBoard, bookMoveHolder, bookScoreHolder, bookDepthHolder)) {
                liveDepth.set(bookDepthHolder[0]);
                liveNodes.set(1L);
                liveScore.set(bookScoreHolder[0]);
                if (outStats != null) {
                    outStats.depthReached = bookDepthHolder[0];
                    outStats.nodesVisited = 1L;
                    outStats.bestScore = bookScoreHolder[0];
                    outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
                }
                return bookMoveHolder[0];
            }
        }

        int rootCap = (profile.rootBranchCap > 0) ? profile.rootBranchCap : 40;
        List<OctiMove> rootCandidates = orderAndCapMoves(
                scratchBoard, legalMoves, rootTurn, "", rootCap, profile, true);

        String bestMoveStr = rootCandidates.get(0).asString();
        int bestScore = -INF_SCORE;
        int maxTargetDepth = profile.maxSearchDepth;
        int completedDepth = 0;

        // Multi-Core Lazy SMP helper threads (if budget >= 500ms and lazySmpThreads > 1)
        ExecutorService smpExecutor = null;
        if (timeBudgetMs >= 500 && profile.lazySmpThreads > 1) {
            int numHelpers = Math.min(profile.lazySmpThreads - 1, Math.max(1, Runtime.getRuntime().availableProcessors() - 1));
            final List<OctiMove> initialCandidates = new ArrayList<>(rootCandidates);
            final boolean diversifyHelpers = profile.lazySmpDiversifyHelpers;
            smpExecutor = Executors.newFixedThreadPool(numHelpers);
            for (int h = 0; h < numHelpers; h++) {
                final int helperId = h + 1;
                final int helperDepthOffset = (h % 2 == 1) ? 1 : 2;
                smpExecutor.submit(() -> {
                    SearchContext helperCtx = new SearchContext();
                    helperCtx.deadlineMs = deadlineMs;
                    helperCtx.stopSignal = stopSignal;
                    try {
                        OctiBoard helperBoard = new OctiBoard();
                        helperBoard.suppressBoardUpdateNotification(true);
                        helperBoard.fromPositionString(rootPositionFen);
                        OctiGame hGame = helperBoard.getGame();
                        if (hGame == null || initialCandidates.isEmpty()) {
                            return;
                        }
                        hGame.setSearchMode(true);

                        if (!diversifyHelpers) {
                            for (int hd = 1 + helperDepthOffset;
                                    hd <= maxTargetDepth && !helperCtx.timedOut && !stopSignal.get();
                                    hd++) {
                                if (steadyClockMs() >= deadlineMs || stopSignal.get()) {
                                    break;
                                }
                                negamax(helperBoard, hd, 0, -INF_SCORE, INF_SCORE, rootTurn, helperCtx);
                            }
                        } else {
                            OctiGameSnapshot hRootSnap = hGame.captureSnapshot();
                            int startD = Math.min(maxTargetDepth, 1 + ((helperId - 1) % 4));
                            for (int hd = startD;
                                    hd <= maxTargetDepth && !helperCtx.timedOut && !stopSignal.get();
                                    hd++) {
                                if (steadyClockMs() >= deadlineMs || stopSignal.get()) {
                                    break;
                                }
                                int hAlpha = -INF_SCORE;
                                int hBeta = INF_SCORE;
                                helperCtx.timedOut = false;
                                for (int s = 0; s < initialCandidates.size() && !stopSignal.get(); s++) {
                                    if (steadyClockMs() >= deadlineMs) {
                                        helperCtx.timedOut = true;
                                        break;
                                    }
                                    OctiMove mv = initialCandidates.get((helperId + s) % initialCandidates.size());
                                    int[] hScore = new int[1];
                                    if (!searchSingleRootMove(helperBoard, hGame, hRootSnap, mode, rootTurn,
                                            mv, hd, s, hAlpha, hBeta, helperCtx, hScore)) {
                                        continue;
                                    }
                                    if (helperCtx.timedOut) {
                                        break;
                                    }
                                    if (hScore[0] > hAlpha) {
                                        hAlpha = hScore[0];
                                    }
                                }
                                if (helperCtx.timedOut || steadyClockMs() >= deadlineMs) {
                                    break;
                                }
                            }
                        }
                    } catch (Throwable ignored) {
                    } finally {
                        flushRemainingNodes(helperCtx);
                    }
                });
            }
        }

        try {
            int[] outScoreHolder = new int[1];
            for (int d = 1; d <= maxTargetDepth; d++) {
                ctx.timedOut = false;
                String iterBestMove = bestMoveStr;
                int iterBestScore = -INF_SCORE;

                boolean useAspiration = (d >= 2
                        && profile.aspirationWindowCp > 0
                        && profile.rootScoreJitter == 0
                        && bestScore > -WIN_SCORE + 2000
                        && bestScore < WIN_SCORE - 2000);

                if (!useAspiration) {
                    int alpha = -INF_SCORE;
                    int beta = INF_SCORE;
                    for (int mvIdx = 0; mvIdx < rootCandidates.size(); mvIdx++) {
                        OctiMove mv = rootCandidates.get(mvIdx);
                        if (!searchSingleRootMove(scratchBoard, game, rootSnap, mode, rootTurn,
                                mv, d, mvIdx, alpha, beta, ctx, outScoreHolder)) {
                            continue;
                        }
                        if (ctx.timedOut && d > 1) {
                            break;
                        }
                        int score = outScoreHolder[0];
                        if (score > alpha) {
                            if (profile.rootScoreJitter > 0 && Math.abs(score) < WIN_SCORE / 2) {
                                int half = profile.rootScoreJitter;
                                int jitter = (int) (Math.abs(nextRandom()) % (2L * half + 1L)) - half;
                                score += jitter;
                            }
                            iterBestScore = score;
                            iterBestMove = mv.asString();
                            alpha = score;
                        }
                    }
                } else {
                    int delta = profile.aspirationWindowCp;
                    int aspAlpha = Math.max(-INF_SCORE, bestScore - delta);
                    int aspBeta = Math.min(INF_SCORE, bestScore + delta);
                    int widenCount = 0;
                    List<OctiMove> aspCandidates = new ArrayList<>(rootCandidates);

                    while (true) {
                        int alpha = aspAlpha;
                        int beta = aspBeta;
                        String passBestMove = aspCandidates.get(0).asString();
                        int passBestScore = -INF_SCORE;
                        int passBestIdx = 0;

                        for (int mvIdx = 0; mvIdx < aspCandidates.size(); mvIdx++) {
                            OctiMove mv = aspCandidates.get(mvIdx);
                            if (!searchSingleRootMove(scratchBoard, game, rootSnap, mode, rootTurn,
                                    mv, d, mvIdx, alpha, beta, ctx, outScoreHolder)) {
                                continue;
                            }
                            if (ctx.timedOut && d > 1) {
                                break;
                            }
                            int score = outScoreHolder[0];
                            if (score > passBestScore) {
                                passBestScore = score;
                                passBestMove = mv.asString();
                                passBestIdx = mvIdx;
                            }
                            if (score > alpha) {
                                alpha = score;
                            }
                            if (alpha >= beta) {
                                break;
                            }
                        }

                        if (ctx.timedOut && d > 1) {
                            break;
                        }

                        if (passBestScore <= aspAlpha && aspAlpha > -INF_SCORE) {
                            ctx.aspirationReSearches++;
                            widenCount++;
                            if (widenCount >= 3 || aspAlpha - delta <= -WIN_SCORE) {
                                aspAlpha = -INF_SCORE;
                            } else {
                                aspAlpha = Math.max(-INF_SCORE, aspAlpha - delta);
                                delta *= 2;
                            }
                            continue;
                        }

                        if (passBestScore >= aspBeta && aspBeta < INF_SCORE) {
                            if (passBestIdx > 0 && passBestIdx < aspCandidates.size()) {
                                OctiMove promoted = aspCandidates.remove(passBestIdx);
                                aspCandidates.add(0, promoted);
                            }
                            ctx.aspirationReSearches++;
                            widenCount++;
                            if (widenCount >= 3 || aspBeta + delta >= WIN_SCORE) {
                                aspBeta = INF_SCORE;
                            } else {
                                aspBeta = Math.min(INF_SCORE, aspBeta + delta);
                                delta *= 2;
                            }
                            continue;
                        }

                        iterBestScore = passBestScore;
                        iterBestMove = passBestMove;
                        rootCandidates = aspCandidates;
                        break;
                    }
                }

                if (!ctx.timedOut || d == 1) {
                    bestMoveStr = iterBestMove;
                    bestScore = iterBestScore;
                    completedDepth = d;
                    flushRemainingNodes(ctx);
                    liveDepth.set(completedDepth);
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
            flushRemainingNodes(ctx);
        }

        long totalNodes = Math.max(ctx.nodesVisited, liveNodes.get());
        liveDepth.set(Math.max(1, completedDepth));
        liveNodes.set(totalNodes);
        liveScore.set(bestScore);

        if (outStats != null) {
            outStats.depthReached = Math.max(1, completedDepth);
            outStats.nodesVisited = totalNodes;
            outStats.bestScore = bestScore;
            outStats.elapsedMs = Math.max(0L, steadyClockMs() - startMs);
        }

        return bestMoveStr;
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
