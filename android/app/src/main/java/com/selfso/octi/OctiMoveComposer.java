/*
 * OctiMoveComposer.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

public class OctiMoveComposer {

    public enum DragKind {
        NONE,
        ADD_POD_FROM_RESERVE,
        LIBERATE_POD_FROM_RESERVE,
        ADD_PRONG_FROM_RESERVE,
        ADD_SUPERPRONG_FROM_RESERVE,
        REPOSITION_PRONG_ON_POD,
        MOVE_OR_JUMP_POD
    }

    public static final class CaptureChoice {
        public int jumpedX;
        public int jumpedY;
        public boolean capture;
        public boolean mandatory;

        public CaptureChoice(int jumpedX, int jumpedY, boolean capture, boolean mandatory) {
            this.jumpedX = jumpedX;
            this.jumpedY = jumpedY;
            this.capture = capture;
            this.mandatory = mandatory;
        }
    }

    public interface MoveCommitListener {
        void onMoveCommitted(String moveNotation);
    }

    private static final class HopCaptureLink {
        final int opIndex;
        final int hopIndex;
        final int jumpedX; // 1-based
        final int jumpedY; // 1-based

        HopCaptureLink(int opIndex, int hopIndex, int jumpedX, int jumpedY) {
            this.opIndex = opIndex;
            this.hopIndex = hopIndex;
            this.jumpedX = jumpedX;
            this.jumpedY = jumpedY;
        }
    }

    private static final class IntPair {
        final int first;
        final int second;

        IntPair(int first, int second) {
            this.first = first;
            this.second = second;
        }

        @Override
        public boolean equals(Object obj) {
            if (this == obj) {
                return true;
            }
            if (!(obj instanceof IntPair)) {
                return false;
            }
            IntPair other = (IntPair) obj;
            return first == other.first && second == other.second;
        }

        @Override
        public int hashCode() {
            return Objects.hash(first, second);
        }
    }

    private OctiBoard board;
    private boolean confirmAllMoves = true;
    private boolean moveEntireMatchingStack = false;
    private MoveCommitListener onMoveCommitted = null;

    private final List<OctiMove> cachedLegalMoves = new ArrayList<>();
    private final Map<IntPair, Integer> selectedStackPods = new HashMap<>();
    private int defaultSelectedStackPodIndex = -1;
    private int activeStackSquareX = -1;
    private int activeStackSquareY = -1;

    private DragKind activeDragKind = DragKind.NONE;
    private int dragStartX = -1;
    private int dragStartY = -1;
    private int dragPodIndex = -1;
    private OctiDir dragSrcDir = OctiDir.DIR_UNDEF;
    private boolean dragMoveAllMatching = false;

    private boolean awaitingConfirmation = false;
    private boolean awaitingSuperProngBonusMove = false;
    private final List<OctiMoveOp> stagedOps = new ArrayList<>();
    private final List<IntPair> claimedStartPods = new ArrayList<>(); // (bx * 10 + by, podIndex)
    private final List<CaptureChoice> captureChoices = new ArrayList<>();
    private final List<HopCaptureLink> hopCaptureLinks = new ArrayList<>();

    private boolean activeJumpHasChain = false;
    private int activeJumpStartBx = -1;
    private int activeJumpStartBy = -1;
    private int activeJumpCurBx = -1;
    private int activeJumpCurBy = -1;
    private List<IntPair> activeJumpedCoords = new ArrayList<>(); // 0-based (jx, jy)

    private boolean hasEighthProngBonusHop = false;
    private int eighthProngTargetBx = -1;
    private int eighthProngTargetBy = -1;
    private String eighthProngBonusNotation = "";

    // Guided step / jump chain state (0-based internally)
    private boolean guidedMoveActive = false;
    private boolean guidedMidHop = false;
    private int guidedOriginBx = -1;
    private int guidedOriginBy = -1;
    private int guidedCurBx = -1;
    private int guidedCurBy = -1;
    private int guidedPodIndex = -1;
    private boolean guidedMoveAllStack = false;

    public OctiMoveComposer() {
        this(null);
    }

    public OctiMoveComposer(OctiBoard board) {
        this.board = board;
        refreshLegalMoves();
    }

    public void attachBoard(OctiBoard board) {
        this.board = board;
        this.selectedStackPods.clear();
        this.defaultSelectedStackPodIndex = -1;
        this.activeStackSquareX = -1;
        this.activeStackSquareY = -1;
        cancelAndRollback();
    }

    public void setBoard(OctiBoard board) {
        attachBoard(board);
    }

    public void openStackFan(int x, int y) {
        this.activeStackSquareX = x;
        this.activeStackSquareY = y;
    }

    public void closeStackFan() {
        this.activeStackSquareX = -1;
        this.activeStackSquareY = -1;
    }

    public boolean isStackFanOpen() {
        return activeStackSquareX >= 1 && activeStackSquareY >= 1;
    }

    public int getStackFanX() {
        return activeStackSquareX;
    }

    public int getStackFanY() {
        return activeStackSquareY;
    }

    public OctiBoard board() {
        return board;
    }

    public OctiBoard getBoard() {
        return board;
    }

    public void setConfirmAllMoves(boolean confirmAll) {
        this.confirmAllMoves = confirmAll;
    }

    public boolean confirmAllMoves() {
        return confirmAllMoves;
    }

    public boolean isConfirmAllMoves() {
        return confirmAllMoves;
    }

    public void setMoveCommitListener(MoveCommitListener cb) {
        this.onMoveCommitted = cb;
    }

    public void setMoveCommittedCallback(MoveCommitListener cb) {
        this.onMoveCommitted = cb;
    }

    public void refreshLegalMoves() {
        cachedLegalMoves.clear();
        if (board != null && board.getGame() != null
                && board.getGame().isGameStarted()
                && !board.getGame().isGameOver()) {
            board.getGame().generateLegalMoves(cachedLegalMoves);
        }
    }

    public void selectStackPodIndex(int x, int y, int podIndex) {
        if (board == null || !board.isValidCoord(x - 1, y - 1)) {
            return;
        }
        OctiSquare sq = board.squareAt(x - 1, y - 1);
        IntPair key = new IntPair(x, y);
        if (podIndex < 0 || podIndex >= sq.getNumPods()) {
            selectedStackPods.remove(key);
            defaultSelectedStackPodIndex = -1;
            return;
        }
        selectedStackPods.put(key, podIndex);
        defaultSelectedStackPodIndex = podIndex;
    }

    public void selectStackPodIndex(int podIndex) {
        defaultSelectedStackPodIndex = podIndex;
        if (activeStackSquareX >= 1 && activeStackSquareY >= 1) {
            selectStackPodIndex(activeStackSquareX, activeStackSquareY, podIndex);
        }
    }

    public int selectedStackPodIndex(int x, int y) {
        if (board == null || !board.isValidCoord(x - 1, y - 1)) {
            return -1;
        }
        OctiSquare sq = board.squareAt(x - 1, y - 1);
        if (sq.isEmpty()) {
            return -1;
        }
        Integer sel = selectedStackPods.get(new IntPair(x, y));
        if (sel != null && sel >= 0 && sel < sq.getNumPods()) {
            return sel;
        }
        if (defaultSelectedStackPodIndex >= 0 && defaultSelectedStackPodIndex < sq.getNumPods()) {
            return defaultSelectedStackPodIndex;
        }
        return sq.getNumPods() - 1;
    }

    public int selectedStackPodIndex() {
        return defaultSelectedStackPodIndex;
    }

    public int getSelectedStackPodIndex() {
        return defaultSelectedStackPodIndex;
    }

    public void setMoveEntireMatchingStack(boolean moveAll) {
        this.moveEntireMatchingStack = moveAll;
    }

    public boolean moveEntireMatchingStack() {
        return moveEntireMatchingStack;
    }

    public boolean isMoveEntireMatchingStack() {
        return moveEntireMatchingStack;
    }

    public DragKind activeDragKind() {
        return activeDragKind;
    }

    public DragKind getDragKind() {
        return activeDragKind;
    }

    public boolean beginAddPodFromReserve() {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }
        if (game.getFreePods(game.getTurn()) <= 0) {
            return false;
        }
        activeDragKind = DragKind.ADD_POD_FROM_RESERVE;
        dragStartX = -1;
        dragStartY = -1;
        dragPodIndex = -1;
        dragSrcDir = OctiDir.DIR_UNDEF;
        return true;
    }

    public boolean beginLiberatePodFromReserve() {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }
        OctiPlayer turn = game.getTurn();
        if (game.getCapturedPods(turn) <= 0 || !board.canLiberate(turn)) {
            return false;
        }
        activeDragKind = DragKind.LIBERATE_POD_FROM_RESERVE;
        dragStartX = -1;
        dragStartY = -1;
        dragPodIndex = -1;
        dragSrcDir = OctiDir.DIR_UNDEF;
        return true;
    }

    public boolean beginAddProngFromReserve(boolean superProng) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }
        OctiPlayer turn = game.getTurn();
        if (superProng) {
            if (!game.useSuperProngs() || game.getFreeSuperProngs(turn) <= 0) {
                return false;
            }
            activeDragKind = DragKind.ADD_SUPERPRONG_FROM_RESERVE;
        } else {
            if (game.getFreeProngs(turn) <= 0) {
                return false;
            }
            activeDragKind = DragKind.ADD_PRONG_FROM_RESERVE;
        }
        dragStartX = -1;
        dragStartY = -1;
        dragPodIndex = -1;
        dragSrcDir = OctiDir.DIR_UNDEF;
        return true;
    }

    public boolean beginRepositionProng(int x, int y, int podIndex, OctiDir srcDir) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        if (!board.isValidCoord(x - 1, y - 1) || srcDir == null || srcDir == OctiDir.DIR_UNDEF) {
            return false;
        }
        activeDragKind = DragKind.REPOSITION_PRONG_ON_POD;
        dragStartX = x;
        dragStartY = y;
        dragPodIndex = podIndex;
        dragSrcDir = srcDir;
        return true;
    }

    public boolean beginMoveOrJumpPod(int x, int y, int podIndex, boolean moveAllMatching) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        if (!board.isValidCoord(x - 1, y - 1)) {
            return false;
        }
        activeDragKind = DragKind.MOVE_OR_JUMP_POD;
        dragStartX = x;
        dragStartY = y;
        dragPodIndex = podIndex;
        dragMoveAllMatching = moveAllMatching;
        return true;
    }

    public boolean beginMoveOrJumpPod(int x, int y, int podIndex) {
        return beginMoveOrJumpPod(x, y, podIndex, moveEntireMatchingStack);
    }

    public boolean dropOnSquare(int targetX, int targetY) {
        DragKind kind = activeDragKind;
        activeDragKind = DragKind.NONE;

        if (kind == DragKind.ADD_POD_FROM_RESERVE) {
            return stageOrCommitAddPod(targetX, targetY, false);
        }
        if (kind == DragKind.LIBERATE_POD_FROM_RESERVE) {
            return stageOrCommitAddPod(targetX, targetY, true);
        }
        if (kind == DragKind.MOVE_OR_JUMP_POD) {
            return stageOrCommitPodMoveOrJump(
                    dragStartX, dragStartY, dragPodIndex,
                    targetX, targetY, dragMoveAllMatching);
        }
        if (activeJumpHasChain && activeJumpCurBx >= 0 && activeJumpCurBy >= 0) {
            return stageOrCommitPodMoveOrJump(
                    activeJumpCurBx + 1, activeJumpCurBy + 1, -1,
                    targetX, targetY, false);
        }
        if (awaitingSuperProngBonusMove && stagedOps.size() == 1) {
            return stageOrCommitPodMoveOrJump(
                    stagedOps.get(0).x0 + 1, stagedOps.get(0).y0 + 1, -1,
                    targetX, targetY, false);
        }
        return false;
    }

    public boolean dropOnCompassSector(int x, int y, int podIndex, OctiDir dir) {
        DragKind kind = activeDragKind;
        activeDragKind = DragKind.NONE;

        if (kind == DragKind.REPOSITION_PRONG_ON_POD) {
            int sx = (dragStartX >= 1) ? dragStartX : x;
            int sy = (dragStartY >= 1) ? dragStartY : y;
            int pIdx = (dragPodIndex >= 0) ? dragPodIndex : podIndex;
            return stageOrCommitRepositionProng(sx, sy, pIdx, dragSrcDir, dir);
        }
        if (kind == DragKind.ADD_SUPERPRONG_FROM_RESERVE) {
            return stageOrCommitAddProng(x, y, podIndex, dir, true);
        }
        return stageOrCommitAddProng(x, y, podIndex, dir, false);
    }

    private static boolean isOfkBackwardDirection(OctiPlayer turn, OctiDir dir) {
        if (dir == null || dir == OctiDir.DIR_UNDEF) {
            return false;
        }
        if (turn == OctiPlayer.PLAYER_A) {
            return dir.deltaY() < 0;
        }
        if (turn == OctiPlayer.PLAYER_B) {
            return dir.deltaY() > 0;
        }
        return false;
    }

    private static boolean computeDisplacement(int cx, int cy, int tx, int ty,
                                               boolean edgeless, int width, int height,
                                               int[] outDxDy) {
        if (!edgeless) {
            outDxDy[0] = tx - cx;
            outDxDy[1] = ty - cy;
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

        outDxDy[0] = bestDx;
        outDxDy[1] = bestDy;
        return true;
    }

    private static final class BfsNode {
        final int x;
        final int y;
        final List<OctiHop> hops;
        final List<IntPair> jumped;

        BfsNode(int x, int y, List<OctiHop> hops, List<IntPair> jumped) {
            this.x = x;
            this.y = y;
            this.hops = hops;
            this.jumped = jumped;
        }
    }

    private static boolean findJumpPathBfs(OctiBoard board,
                                           boolean edgeless,
                                           OctiPlayer turn,
                                           int startX, int startY,
                                           int curX, int curY,
                                           int targetX, int targetY,
                                           OctiPod pod,
                                           List<OctiPod> ghostPods,
                                           List<IntPair> initialJumped,
                                           List<OctiHop> outHops,
                                           List<IntPair> outJumpedCoords) {
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();

        List<BfsNode> queue = new ArrayList<>();
        queue.add(new BfsNode(curX, curY, new ArrayList<>(), new ArrayList<>(initialJumped)));

        for (int head = 0; head < queue.size() && queue.size() < 256; head++) {
            BfsNode cur = queue.get(head);
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    continue;
                }
                int jdx = dir.deltaX();
                int jdy = dir.deltaY();
                int dx = jdx * 2;
                int dy = jdy * 2;

                int jx = edgeless ? (((cur.x + jdx) % w + w) % w) : (cur.x + jdx);
                int jy = edgeless ? (((cur.y + jdy) % h + h) % h) : (cur.y + jdy);
                int lx = edgeless ? (((cur.x + dx) % w + w) % w) : (cur.x + dx);
                int ly = edgeless ? (((cur.y + dy) % h + h) % h) : (cur.y + dy);

                if (!edgeless) {
                    if (jx < 0 || jx >= w || jy < 0 || jy >= h) {
                        continue;
                    }
                    if (lx < 0 || lx >= w || ly < 0 || ly >= h) {
                        continue;
                    }
                }

                IntPair jCoord = new IntPair(jx, jy);
                if (cur.jumped.contains(jCoord)) {
                    continue;
                }

                OctiSquare jumpedSq = board.squareAt(jx, jy);
                boolean jumpedEmpty = (jx == startX && jy == startY)
                        ? jumpedSq.isEmptyWithout(ghostPods)
                        : jumpedSq.isEmpty();
                if (jumpedEmpty) {
                    continue;
                }

                OctiSquare landSq = board.squareAt(lx, ly);
                boolean landEmpty = (lx == startX && ly == startY)
                        ? landSq.isEmptyWithout(ghostPods)
                        : landSq.isEmpty();
                if (!landEmpty) {
                    continue;
                }

                OctiHop hop = new OctiHop(lx, ly, jumpedSq.hasEnemyOf(turn));
                List<OctiHop> nextHops = new ArrayList<>(cur.hops);
                nextHops.add(hop);
                List<IntPair> nextJumped = new ArrayList<>(cur.jumped);
                nextJumped.add(jCoord);

                if (lx == targetX && ly == targetY) {
                    outHops.clear();
                    outHops.addAll(nextHops);
                    outJumpedCoords.clear();
                    outJumpedCoords.addAll(nextJumped);
                    return true;
                }

                queue.add(new BfsNode(lx, ly, nextHops, nextJumped));
            }
        }
        return false;
    }

    private boolean isPodClaimed(int bx, int by, int podIdx) {
        int key = bx * 10 + by;
        return claimedStartPods.contains(new IntPair(key, podIdx));
    }

    public boolean isSquarePodClaimed(int x, int y, int podIndex) {
        if (board == null || !board.isValidCoord(x - 1, y - 1)) {
            return false;
        }
        return isPodClaimed(x - 1, y - 1, podIndex);
    }

    public int firstUnclaimedFriendlyPodIndex(int x, int y) {
        if (board == null || board.getGame() == null || !board.isValidCoord(x - 1, y - 1)) {
            return -1;
        }
        OctiSquare sq = board.squareAt(x - 1, y - 1);
        OctiPodType desiredType = board.getGame().getTurn().toPodType();
        for (int i = sq.getNumPods() - 1; i >= 0; i--) {
            if (!isPodClaimed(x - 1, y - 1, i) && sq.getPodAt(i).getType() == desiredType) {
                return i;
            }
        }
        return -1;
    }

    private int resolvePodIndex(OctiSquare sq,
                                int x, int y,
                                int requestedIndex,
                                OctiPlayer turn,
                                boolean excludeClaimed) {
        int bx = x - 1;
        int by = y - 1;
        OctiPodType desiredType = turn.toPodType();

        if (requestedIndex >= 0 && requestedIndex < sq.getNumPods()) {
            if (sq.getPodAt(requestedIndex).getType() == desiredType
                    && (!excludeClaimed || !isPodClaimed(bx, by, requestedIndex))) {
                return requestedIndex;
            }
            return -1;
        }

        Integer sel = selectedStackPods.get(new IntPair(x, y));
        if (sel != null && sel >= 0 && sel < sq.getNumPods()
                && sq.getPodAt(sel).getType() == desiredType
                && (!excludeClaimed || !isPodClaimed(bx, by, sel))) {
            return sel;
        }

        if (defaultSelectedStackPodIndex >= 0 && defaultSelectedStackPodIndex < sq.getNumPods()
                && sq.getPodAt(defaultSelectedStackPodIndex).getType() == desiredType
                && (!excludeClaimed || !isPodClaimed(bx, by, defaultSelectedStackPodIndex))) {
            return defaultSelectedStackPodIndex;
        }

        for (int i = sq.getNumPods() - 1; i >= 0; i--) {
            if (sq.getPodAt(i).getType() == desiredType
                    && (!excludeClaimed || !isPodClaimed(bx, by, i))) {
                return i;
            }
        }
        return -1;
    }

    private boolean validateOpsOnScratch(List<OctiMoveOp> ops) {
        if (board == null || board.getGame() == null || ops == null || ops.isEmpty()) {
            return false;
        }
        try {
            OctiMove mv = new OctiMove(ops);
            OctiBoard scratch = new OctiBoard();
            scratch.suppressBoardUpdateNotification(true);
            scratch.fromPositionString(board.toPositionString());
            scratch.getGame().move(mv);
            return true;
        } catch (Throwable t) {
            return false;
        }
    }

    private boolean hasContinuationJumpFrom(int bx, int by,
                                            OctiPod pod,
                                            int startBx, int startBy,
                                            List<OctiPod> ghostPods,
                                            List<IntPair> alreadyJumped) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = board.getGame().useEdgelessBoard();

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                continue;
            }
            int jdx = dir.deltaX();
            int jdy = dir.deltaY();
            int dx = jdx * 2;
            int dy = jdy * 2;

            int jx = edgeless ? (((bx + jdx) % w + w) % w) : (bx + jdx);
            int jy = edgeless ? (((by + jdy) % h + h) % h) : (by + jdy);
            int lx = edgeless ? (((bx + dx) % w + w) % w) : (bx + dx);
            int ly = edgeless ? (((by + dy) % h + h) % h) : (by + dy);

            if (!edgeless) {
                if (jx < 0 || jx >= w || jy < 0 || jy >= h) {
                    continue;
                }
                if (lx < 0 || lx >= w || ly < 0 || ly >= h) {
                    continue;
                }
            }

            if (alreadyJumped.contains(new IntPair(jx, jy))) {
                continue;
            }

            OctiSquare jumpedSq = board.squareAt(jx, jy);
            boolean jumpedEmpty = (jx == startBx && jy == startBy)
                    ? jumpedSq.isEmptyWithout(ghostPods)
                    : jumpedSq.isEmpty();
            if (jumpedEmpty) {
                continue;
            }

            OctiSquare landSq = board.squareAt(lx, ly);
            boolean landEmpty = (lx == startBx && ly == startBy)
                    ? landSq.isEmptyWithout(ghostPods)
                    : landSq.isEmpty();
            if (!landEmpty) {
                continue;
            }

            return true;
        }
        return false;
    }

    private boolean hasOtherMovableFriendlyPods(int bx, int by, OctiPlayer turn) {
        if (board == null || !board.isValidCoord(bx, by)) {
            return false;
        }
        OctiSquare sq = board.squareAt(bx, by);
        OctiPodType desiredType = turn.toPodType();

        for (int i = 0; i < sq.getNumPods(); i++) {
            if (isPodClaimed(bx, by, i)) {
                continue;
            }
            OctiPod pod = sq.getPodAt(i);
            if (pod.getType() != desiredType) {
                continue;
            }
            if (pod.getNumProngs() > 0) {
                return true;
            }
        }
        return false;
    }

    private void syncPreparedStateToGame() {
        if (board == null || board.getGame() == null) {
            return;
        }
        OctiGame game = board.getGame();
        game.rollbackPreparedMove();

        for (OctiMoveOp op : stagedOps) {
            if (op.addPod) {
                game.prepareAddPod(op.x0, op.y0);
            } else if (op.addProngDir != OctiDir.DIR_UNDEF
                    && op.rmProngDir == OctiDir.DIR_UNDEF) {
                game.prepareAddProng(op.x0, op.y0, op.pod, op.addProngDir, op.superProng);
            } else if (op.addProngDir != OctiDir.DIR_UNDEF
                    && op.rmProngDir != OctiDir.DIR_UNDEF) {
                game.prepareReposition(op.x0, op.y0, op.pod, op.rmProngDir, op.addProngDir);
            } else if (!op.hops.isEmpty()) {
                game.prepareHops(op.x0, op.y0, op.pod, op.hops);
            }
        }
    }

    public boolean stageOrCommitAddPod(int targetX, int targetY, boolean liberate) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }

        int bx = targetX - 1;
        int by = targetY - 1;
        if (!board.isValidCoord(bx, by)) {
            return false;
        }

        if (!stagedOps.isEmpty()) {
            return false;
        }

        OctiPlayer turn = game.getTurn();
        if (liberate) {
            if (game.getCapturedPods(turn) <= 0 || !board.canLiberate(turn)) {
                return false;
            }
        } else {
            if (game.getFreePods(turn) <= 0) {
                return false;
            }
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = bx;
        op.y0 = by;
        op.addPod = true;
        op.liberate = liberate;

        if (!validateOpsOnScratch(Collections.singletonList(op))) {
            return false;
        }

        stagedOps.clear();
        stagedOps.add(op);
        guidedMoveActive = false;
        guidedMidHop = false;
        awaitingConfirmation = true;
        syncPreparedStateToGame();

        if (!confirmAllMoves) {
            return commitStagedMove();
        }
        return true;
    }

    public boolean stageOrCommitAddProng(int x, int y,
                                         int podIndex,
                                         OctiDir dir,
                                         boolean superProng) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }
        if (dir == null || dir == OctiDir.DIR_UNDEF) {
            return false;
        }

        int bx = x - 1;
        int by = y - 1;
        if (!board.isValidCoord(bx, by)) {
            return false;
        }

        OctiPlayer turn = game.getTurn();
        if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) {
            if (superProng || isOfkBackwardDirection(turn, dir)) {
                return false;
            }
        }

        if (!stagedOps.isEmpty()) {
            return false;
        }

        OctiSquare sq = board.squareAt(bx, by);
        int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
        if (resolvedIdx < 0) {
            return false;
        }

        OctiPod pod = sq.getPodAt(resolvedIdx);
        if (pod.prongAt(dir) != OctiProngType.NO_PRONG) {
            return false;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = bx;
        op.y0 = by;
        op.pod = pod.copy();
        op.addProngDir = dir;
        op.superProng = superProng;

        if (!validateOpsOnScratch(Collections.singletonList(op))) {
            return false;
        }

        stagedOps.clear();
        stagedOps.add(op);
        guidedMoveActive = false;
        guidedMidHop = false;
        awaitingSuperProngBonusMove = (superProng && pod.getNumProngs() == 7);
        awaitingConfirmation = true;
        syncPreparedStateToGame();

        if (!confirmAllMoves && !awaitingSuperProngBonusMove) {
            return commitStagedMove();
        }
        return true;
    }

    public boolean stageOrCommitRepositionProng(int x, int y,
                                                int podIndex,
                                                OctiDir srcDir,
                                                OctiDir tgtDir) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }
        if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) {
            return false;
        }
        if (srcDir == null || srcDir == OctiDir.DIR_UNDEF
                || tgtDir == null || tgtDir == OctiDir.DIR_UNDEF
                || srcDir == tgtDir) {
            return false;
        }
        if (!stagedOps.isEmpty()) {
            return false;
        }

        int bx = x - 1;
        int by = y - 1;
        if (!board.isValidCoord(bx, by)) {
            return false;
        }

        OctiPlayer turn = game.getTurn();
        OctiSquare sq = board.squareAt(bx, by);
        int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
        if (resolvedIdx < 0) {
            return false;
        }

        OctiPod pod = sq.getPodAt(resolvedIdx);
        if (pod.prongAt(srcDir) != OctiProngType.NORMAL_PRONG
                || pod.prongAt(tgtDir) != OctiProngType.NO_PRONG) {
            return false;
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = bx;
        op.y0 = by;
        op.pod = pod.copy();
        op.rmProngDir = srcDir;
        op.addProngDir = tgtDir;
        op.superProng = false;

        if (!validateOpsOnScratch(Collections.singletonList(op))) {
            return false;
        }

        stagedOps.clear();
        stagedOps.add(op);
        guidedMoveActive = false;
        guidedMidHop = false;
        awaitingConfirmation = true;
        syncPreparedStateToGame();

        if (!confirmAllMoves) {
            return commitStagedMove();
        }
        return true;
    }

    public boolean stageOrCommitPodMoveOrJump(int fromX, int fromY,
                                              int podIndex,
                                              int toX, int toY,
                                              boolean moveAllMatchingStack) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }

        int fbx = fromX - 1;
        int fby = fromY - 1;
        int tbx = toX - 1;
        int tby = toY - 1;
        if (!board.isValidCoord(fbx, fby) || !board.isValidCoord(tbx, tby)) {
            return false;
        }
        if (fbx == tbx && fby == tby && !activeJumpHasChain) {
            return false;
        }

        OctiPlayer turn = game.getTurn();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = game.useEdgelessBoard();

        // Case A: 8th-prong superprong bonus move follow-up
        if (awaitingSuperProngBonusMove && stagedOps.size() == 1
                && stagedOps.get(0).addProngDir != OctiDir.DIR_UNDEF
                && fbx == stagedOps.get(0).x0 && fby == stagedOps.get(0).y0) {
            OctiPod fullPod = stagedOps.get(0).pod.copy();
            fullPod.addProng(stagedOps.get(0).addProngDir, true);

            int[] dxDy = new int[2];
            if (computeDisplacement(fbx, fby, tbx, tby, edgeless, w, h, dxDy)) {
                int dx = dxDy[0];
                int dy = dxDy[1];
                boolean isDist1 = (Math.max(Math.abs(dx), Math.abs(dy)) == 1
                        && (Math.abs(dx) == 1 || Math.abs(dy) == 1));
                boolean isDist2 = ((Math.abs(dx) == 0 || Math.abs(dx) == 2)
                        && (Math.abs(dy) == 0 || Math.abs(dy) == 2)
                        && (Math.abs(dx) == 2 || Math.abs(dy) == 2)
                        && (Math.abs(dx) == 0 || Math.abs(dy) == 0 || Math.abs(dx) == Math.abs(dy)));
                OctiDir moveDir = isDist1 ? OctiDir.deltaToDir(dx, dy) :
                                  isDist2 ? OctiDir.deltaToDir(dx / 2, dy / 2) : OctiDir.DIR_UNDEF;
                if (moveDir != OctiDir.DIR_UNDEF
                        && fullPod.prongAt(moveDir) != OctiProngType.NO_PRONG
                        && !board.squareAt(tbx, tby).hasEnemyOf(turn)) {
                    hasEighthProngBonusHop = true;
                    eighthProngTargetBx = tbx;
                    eighthProngTargetBy = tby;
                    eighthProngBonusNotation =
                            OctiMove.formatOps(stagedOps) + ","
                            + (char) ('1' + fbx) + (char) ('1' + fby) + "-"
                            + (char) ('1' + tbx) + (char) ('1' + tby);
                    awaitingConfirmation = true;
                    return true;
                }
            }
            return false;
        }

        // Case B: Appending a continuation hop to an active staged jump chain
        if (activeJumpHasChain && !stagedOps.isEmpty()
                && fbx == activeJumpCurBx && fby == activeJumpCurBy) {
            OctiMoveOp activeOp = stagedOps.get(stagedOps.size() - 1);
            OctiSquare startSq = board.squareAt(activeJumpStartBx, activeJumpStartBy);
            List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());

            List<OctiHop> extraHops = new ArrayList<>();
            List<IntPair> nextJumped = new ArrayList<>();
            if (!findJumpPathBfs(board, edgeless, turn,
                    activeJumpStartBx, activeJumpStartBy,
                    activeJumpCurBx, activeJumpCurBy,
                    tbx, tby, activeOp.pod, ghostPods,
                    activeJumpedCoords, extraHops, nextJumped)) {
                return false;
            }

            List<OctiMoveOp> candidateOps = new ArrayList<>();
            for (OctiMoveOp op : stagedOps) {
                candidateOps.add(new OctiMoveOp(op));
            }
            OctiMoveOp lastCandidate = candidateOps.get(candidateOps.size() - 1);
            for (OctiHop eh : extraHops) {
                lastCandidate.hops.add(new OctiHop(eh));
            }
            if (!validateOpsOnScratch(candidateOps)) {
                return false;
            }

            int opIdx = stagedOps.size() - 1;
            int prevJumpedCount = activeJumpedCoords.size();
            for (int i = 0; i < extraHops.size(); i++) {
                int hopIdx = activeOp.hops.size();
                activeOp.hops.add(new OctiHop(extraHops.get(i)));
                IntPair jc = nextJumped.get(prevJumpedCount + i);
                OctiSquare jsq = board.squareAt(jc.first, jc.second);
                if (jsq.hasEnemyOf(turn)) {
                    int jx1 = jc.first + 1;
                    int jy1 = jc.second + 1;
                    hopCaptureLinks.add(new HopCaptureLink(opIdx, hopIdx, jx1, jy1));
                    boolean alreadyListed = false;
                    for (CaptureChoice cc : captureChoices) {
                        if (cc.jumpedX == jx1 && cc.jumpedY == jy1) {
                            alreadyListed = true;
                            break;
                        }
                    }
                    if (!alreadyListed) {
                        captureChoices.add(new CaptureChoice(
                                jx1, jy1, true, board.getGameMode() == OctiGameMode.GAME_MODE_OFK));
                    }
                }
            }

            activeJumpedCoords = nextJumped;
            activeJumpCurBx = tbx;
            activeJumpCurBy = tby;
            awaitingConfirmation = true;
            syncPreparedStateToGame();
            return true;
        }

        // Case C: Starting a new move/jump op (or adding a composite pod from the same start square)
        if (!stagedOps.isEmpty()) {
            for (OctiMoveOp existingOp : stagedOps) {
                if (existingOp.hops.isEmpty()
                        || existingOp.x0 != fbx || existingOp.y0 != fby) {
                    return false;
                }
            }
        }

        OctiSquare startSq = board.squareAt(fbx, fby);
        int resolvedIdx = resolvePodIndex(startSq, fromX, fromY, podIndex, turn, true);
        if (resolvedIdx < 0) {
            return false;
        }

        OctiPod movingPod = startSq.getPodAt(resolvedIdx);
        List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());

        int[] dxDy = new int[2];
        boolean hasDirectDelta = computeDisplacement(fbx, fby, tbx, tby, edgeless, w, h, dxDy);
        int dx = dxDy[0];
        int dy = dxDy[1];
        boolean isDist1 = hasDirectDelta
                && (Math.max(Math.abs(dx), Math.abs(dy)) == 1
                && (Math.abs(dx) == 1 || Math.abs(dy) == 1));

        List<OctiHop> chosenHops = new ArrayList<>();
        List<IntPair> chosenJumped = new ArrayList<>();

        if (isDist1) {
            OctiDir dir = OctiDir.deltaToDir(dx, dy);
            if (dir == OctiDir.DIR_UNDEF || movingPod.prongAt(dir) == OctiProngType.NO_PRONG) {
                return false;
            }
            chosenHops.add(new OctiHop(tbx, tby, false));
        } else {
            if (!findJumpPathBfs(board, edgeless, turn,
                    fbx, fby, fbx, fby, tbx, tby,
                    movingPod, ghostPods, Collections.emptyList(),
                    chosenHops, chosenJumped)) {
                return false;
            }
        }

        boolean moveStack = moveAllMatchingStack || moveEntireMatchingStack;
        List<Integer> podIndicesToMove = new ArrayList<>();
        podIndicesToMove.add(resolvedIdx);

        if (moveStack && board.getGameMode() != OctiGameMode.GAME_MODE_OFK) {
            for (int i = 0; i < startSq.getNumPods(); i++) {
                if (i == resolvedIdx || isPodClaimed(fbx, fby, i)) {
                    continue;
                }
                OctiPod otherPod = startSq.getPodAt(i);
                if (otherPod.getType() != turn.toPodType()) {
                    continue;
                }
                boolean canFollowSamePath = true;
                int cx = fbx;
                int cy = fby;
                for (OctiHop hop : chosenHops) {
                    int[] hDxDy = new int[2];
                    if (!computeDisplacement(cx, cy, hop.x, hop.y, edgeless, w, h, hDxDy)) {
                        canFollowSamePath = false;
                        break;
                    }
                    int hdx = hDxDy[0];
                    int hdy = hDxDy[1];
                    OctiDir reqDir = (Math.max(Math.abs(hdx), Math.abs(hdy)) == 1)
                            ? OctiDir.deltaToDir(hdx, hdy)
                            : OctiDir.deltaToDir(hdx / 2, hdy / 2);
                    if (reqDir == OctiDir.DIR_UNDEF
                            || otherPod.prongAt(reqDir) == OctiProngType.NO_PRONG) {
                        canFollowSamePath = false;
                        break;
                    }
                    cx = hop.x;
                    cy = hop.y;
                }
                if (canFollowSamePath) {
                    podIndicesToMove.add(i);
                }
            }
        }

        List<OctiMoveOp> candidateOps = new ArrayList<>();
        for (OctiMoveOp op : stagedOps) {
            candidateOps.add(new OctiMoveOp(op));
        }
        for (int pIdx : podIndicesToMove) {
            OctiMoveOp op = new OctiMoveOp();
            op.x0 = fbx;
            op.y0 = fby;
            op.pod = startSq.getPodAt(pIdx).copy();
            for (OctiHop hopItem : chosenHops) {
                op.hops.add(new OctiHop(hopItem));
            }
            candidateOps.add(op);
        }

        if (!validateOpsOnScratch(candidateOps)) {
            return false;
        }

        int startKey = fbx * 10 + fby;
        for (int pIdx : podIndicesToMove) {
            int opIdx = stagedOps.size();
            OctiMoveOp op = new OctiMoveOp();
            op.x0 = fbx;
            op.y0 = fby;
            op.pod = startSq.getPodAt(pIdx).copy();
            for (OctiHop hopItem : chosenHops) {
                op.hops.add(new OctiHop(hopItem));
            }
            stagedOps.add(op);
            claimedStartPods.add(new IntPair(startKey, pIdx));

            if (!isDist1) {
                for (int hi = 0; hi < chosenJumped.size(); hi++) {
                    IntPair jc = chosenJumped.get(hi);
                    OctiSquare jsq = board.squareAt(jc.first, jc.second);
                    if (jsq.hasEnemyOf(turn)) {
                        int jx1 = jc.first + 1;
                        int jy1 = jc.second + 1;
                        hopCaptureLinks.add(new HopCaptureLink(opIdx, hi, jx1, jy1));
                        boolean alreadyListed = false;
                        for (CaptureChoice cc : captureChoices) {
                            if (cc.jumpedX == jx1 && cc.jumpedY == jy1) {
                                alreadyListed = true;
                                break;
                            }
                        }
                        if (!alreadyListed) {
                            captureChoices.add(new CaptureChoice(
                                    jx1, jy1, true, board.getGameMode() == OctiGameMode.GAME_MODE_OFK));
                        }
                    }
                }
            }
        }

        activeJumpHasChain = !isDist1;
        activeJumpStartBx = fbx;
        activeJumpStartBy = fby;
        activeJumpCurBx = tbx;
        activeJumpCurBy = tby;
        activeJumpedCoords = chosenJumped;

        boolean hasMorePodsOnStart = hasOtherMovableFriendlyPods(fbx, fby, turn);
        boolean hasContinuation = !isDist1
                && hasContinuationJumpFrom(tbx, tby, movingPod, fbx, fby, ghostPods, chosenJumped);
        boolean hasOptionalCapture = false;
        for (CaptureChoice cc : captureChoices) {
            if (!cc.mandatory) {
                hasOptionalCapture = true;
                break;
            }
        }

        boolean shouldPause = confirmAllMoves
                || hasMorePodsOnStart
                || hasContinuation
                || hasOptionalCapture
                || awaitingSuperProngBonusMove;

        awaitingConfirmation = true;
        syncPreparedStateToGame();

        if (!shouldPause) {
            return commitStagedMove();
        }
        return true;
    }

    public void setCaptureAtSquare(int jumpedX, int jumpedY, boolean capture) {
        if (stagedOps.isEmpty()) {
            return;
        }

        int targetIdx = -1;
        for (int i = 0; i < captureChoices.size(); i++) {
            CaptureChoice cc = captureChoices.get(i);
            if (cc.jumpedX == jumpedX && cc.jumpedY == jumpedY) {
                targetIdx = i;
                break;
            }
        }
        if (targetIdx < 0) {
            return;
        }
        CaptureChoice choice = captureChoices.get(targetIdx);
        if (choice.mandatory) {
            return;
        }

        choice.capture = capture;

        for (HopCaptureLink link : hopCaptureLinks) {
            if (link.jumpedX == jumpedX && link.jumpedY == jumpedY
                    && link.opIndex < stagedOps.size()
                    && link.hopIndex < stagedOps.get(link.opIndex).hops.size()) {
                stagedOps.get(link.opIndex).hops.get(link.hopIndex).capture = capture;
            }
        }

        syncPreparedStateToGame();
    }

    public boolean toggleCaptureAtSquare(int jumpedX, int jumpedY) {
        if (stagedOps.isEmpty()) {
            return false;
        }

        int targetIdx = -1;
        for (int i = 0; i < captureChoices.size(); i++) {
            CaptureChoice cc = captureChoices.get(i);
            if (cc.jumpedX == jumpedX && cc.jumpedY == jumpedY) {
                targetIdx = i;
                break;
            }
        }
        if (targetIdx < 0) {
            return false;
        }
        CaptureChoice choice = captureChoices.get(targetIdx);
        if (choice.mandatory) {
            return false;
        }

        boolean newCap = !choice.capture;
        choice.capture = newCap;

        for (HopCaptureLink link : hopCaptureLinks) {
            if (link.jumpedX == jumpedX && link.jumpedY == jumpedY
                    && link.opIndex < stagedOps.size()
                    && link.hopIndex < stagedOps.get(link.opIndex).hops.size()) {
                stagedOps.get(link.opIndex).hops.get(link.hopIndex).capture = newCap;
            }
        }

        syncPreparedStateToGame();
        return true;
    }

    public boolean toggleHopCapture(int hopIndex) {
        if (hopIndex < 0 || hopIndex >= captureChoices.size()) {
            return false;
        }
        CaptureChoice cc = captureChoices.get(hopIndex);
        return toggleCaptureAtSquare(cc.jumpedX, cc.jumpedY);
    }

    public boolean canCommitStagedMove() {
        return !stagedOps.isEmpty() && validateOpsOnScratch(stagedOps);
    }

    public boolean isAwaitingConfirmation() {
        return awaitingConfirmation && !stagedOps.isEmpty();
    }

    public OctiBoard buildPreviewBoard() {
        if (board == null) {
            return null;
        }
        if (!isAwaitingConfirmation() || stagedOps.isEmpty()) {
            return board;
        }
        try {
            OctiBoard preview = new OctiBoard();
            preview.suppressBoardUpdateNotification(true);
            preview.fromPositionString(board.toPositionString());
            preview.getGame().move(new OctiMove(stagedOps));
            if (hasEighthProngBonusHop
                    && preview.isValidCoord(eighthProngTargetBx, eighthProngTargetBy)
                    && stagedOps.size() == 1) {
                int sx = stagedOps.get(0).x0;
                int sy = stagedOps.get(0).y0;
                OctiSquare srcSq = preview.squareAt(sx, sy);
                OctiSquare dstSq = preview.squareAt(eighthProngTargetBx, eighthProngTargetBy);
                if (!srcSq.isEmpty()) {
                    OctiPod movedPod = srcSq.getPodAt(srcSq.getNumPods() - 1).copy();
                    srcSq.removePod(movedPod);
                    dstSq.addPod(movedPod);
                }
            }
            return preview;
        } catch (Throwable t) {
            return board;
        }
    }

    public boolean shouldPauseForConfirmation() {
        return isAwaitingConfirmation();
    }

    public boolean isAwaitingSuperProngBonusMove() {
        return awaitingSuperProngBonusMove;
    }

    public boolean hasUnlockedSuperProngBonus() {
        return awaitingSuperProngBonusMove && !hasEighthProngBonusHop;
    }

    public int[] eighthProngOriginSquare() {
        if (awaitingSuperProngBonusMove && stagedOps.size() == 1) {
            return new int[]{stagedOps.get(0).x0 + 1, stagedOps.get(0).y0 + 1};
        }
        return new int[]{0, 0};
    }

    public String stagedMoveNotation() {
        if (stagedOps.isEmpty()) {
            return "";
        }
        if (hasEighthProngBonusHop && !eighthProngBonusNotation.isEmpty()) {
            return eighthProngBonusNotation;
        }
        return OctiMove.formatOps(stagedOps);
    }

    public String getStagedMoveNotation() {
        return stagedMoveNotation();
    }

    public List<CaptureChoice> captureChoices() {
        return Collections.unmodifiableList(captureChoices);
    }

    // Guided Step / Multi-Hop Jump Helpers
    public boolean beginGuidedPodMove(int x, int y, int podIndex, boolean moveAllMatchingStack) {
        if (board == null || board.getGame() == null) {
            return false;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return false;
        }

        int bx = x - 1;
        int by = y - 1;
        if (!board.isValidCoord(bx, by)) {
            return false;
        }

        boolean isSuperProngBonusStart = awaitingSuperProngBonusMove
                && !hasEighthProngBonusHop
                && stagedOps.size() == 1
                && stagedOps.get(0).addProngDir != OctiDir.DIR_UNDEF
                && bx == stagedOps.get(0).x0
                && by == stagedOps.get(0).y0;

        if (!stagedOps.isEmpty() && !isSuperProngBonusStart) {
            for (OctiMoveOp op : stagedOps) {
                if (op.hops.isEmpty() || op.x0 != bx || op.y0 != by) {
                    return false;
                }
            }
        }

        OctiPlayer turn = game.getTurn();
        OctiSquare sq = board.squareAt(bx, by);
        int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, !isSuperProngBonusStart);
        if (resolvedIdx < 0 || resolvedIdx >= sq.getNumPods()) {
            return false;
        }

        guidedOriginBx = bx;
        guidedOriginBy = by;
        guidedCurBx = bx;
        guidedCurBy = by;
        guidedPodIndex = resolvedIdx;
        guidedMoveAllStack = moveAllMatchingStack;
        guidedMidHop = false;
        guidedMoveActive = true;

        if (guidedNextSquares().isEmpty()) {
            guidedMoveActive = false;
            return false;
        }

        return true;
    }

    public boolean isGuidedMoveActive() {
        return guidedMoveActive;
    }

    public boolean isGuidedMidHop() {
        return guidedMidHop;
    }

    public int[] guidedCurrentSquare() {
        if (!guidedMoveActive) {
            return new int[]{0, 0};
        }
        return new int[]{guidedCurBx + 1, guidedCurBy + 1};
    }

    public int[] guidedOriginSquare() {
        if (guidedOriginBx >= 0 && guidedOriginBy >= 0) {
            return new int[]{guidedOriginBx + 1, guidedOriginBy + 1};
        }
        if (!stagedOps.isEmpty()) {
            return new int[]{stagedOps.get(0).x0 + 1, stagedOps.get(0).y0 + 1};
        }
        return new int[]{0, 0};
    }

    public List<int[]> guidedNextSquares() {
        List<int[]> result = new ArrayList<>();
        if (!guidedMoveActive || board == null || board.getGame() == null) {
            return result;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return result;
        }

        if (awaitingSuperProngBonusMove && !hasEighthProngBonusHop
                && stagedOps.size() == 1
                && stagedOps.get(0).addProngDir != OctiDir.DIR_UNDEF) {
            return legalDestinationsFrom(guidedOriginBx + 1, guidedOriginBy + 1, guidedPodIndex);
        }

        OctiPlayer turn = game.getTurn();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = game.useEdgelessBoard();
        Set<IntPair> seen = new HashSet<>();

        if (!guidedMidHop) {
            if (!board.isValidCoord(guidedOriginBx, guidedOriginBy)) {
                return result;
            }
            OctiSquare startSq = board.squareAt(guidedOriginBx, guidedOriginBy);
            if (guidedPodIndex < 0 || guidedPodIndex >= startSq.getNumPods()) {
                return result;
            }
            OctiPod pod = startSq.getPodAt(guidedPodIndex);
            if (pod.getType() != turn.toPodType()) {
                return result;
            }
            List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());

            // 1. Distance-1 steps
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    continue;
                }
                if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                    continue;
                }
                int jdx = dir.deltaX();
                int jdy = dir.deltaY();
                int sx = edgeless ? (((guidedOriginBx + jdx) % w + w) % w) : (guidedOriginBx + jdx);
                int sy = edgeless ? (((guidedOriginBy + jdy) % h + h) % h) : (guidedOriginBy + jdy);
                if (!board.isValidCoord(sx, sy)) {
                    continue;
                }
                OctiSquare destSq = board.squareAt(sx, sy);
                if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) {
                    if (!destSq.isEmpty()) {
                        continue;
                    }
                } else {
                    if (destSq.hasEnemyOf(turn)) {
                        continue;
                    }
                    if (destSq.getNumPods() >= 7) {
                        continue;
                    }
                }
                OctiHop hop = new OctiHop(sx, sy, false);
                OctiMoveOp op = new OctiMoveOp();
                op.x0 = guidedOriginBx;
                op.y0 = guidedOriginBy;
                op.pod = pod.copy();
                op.hops.add(hop);
                List<OctiMoveOp> cand = new ArrayList<>();
                for (OctiMoveOp sop : stagedOps) {
                    cand.add(new OctiMoveOp(sop));
                }
                cand.add(op);
                if (validateOpsOnScratch(cand)) {
                    IntPair pair = new IntPair(sx + 1, sy + 1);
                    if (seen.add(pair)) {
                        result.add(new int[]{sx + 1, sy + 1});
                    }
                }
            }

            // 2. Distance-2 jumps
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    continue;
                }
                if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                    continue;
                }
                int jdx = dir.deltaX();
                int jdy = dir.deltaY();
                int dx = jdx * 2;
                int dy = jdy * 2;
                int jx = edgeless ? (((guidedOriginBx + jdx) % w + w) % w) : (guidedOriginBx + jdx);
                int jy = edgeless ? (((guidedOriginBy + jdy) % h + h) % h) : (guidedOriginBy + jdy);
                int lx = edgeless ? (((guidedOriginBx + dx) % w + w) % w) : (guidedOriginBx + dx);
                int ly = edgeless ? (((guidedOriginBy + dy) % h + h) % h) : (guidedOriginBy + dy);
                if (!board.isValidCoord(jx, jy) || !board.isValidCoord(lx, ly)) {
                    continue;
                }
                OctiSquare jumpedSq = board.squareAt(jx, jy);
                boolean jumpedEmpty = (jx == guidedOriginBx && jy == guidedOriginBy)
                        ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
                if (jumpedEmpty) {
                    continue;
                }

                OctiSquare landSq = board.squareAt(lx, ly);
                boolean landEmpty = (lx == guidedOriginBx && ly == guidedOriginBy)
                        ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
                if (!landEmpty) {
                    continue;
                }

                OctiHop hop = new OctiHop(lx, ly, (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) || jumpedSq.hasEnemyOf(turn));
                OctiMoveOp op = new OctiMoveOp();
                op.x0 = guidedOriginBx;
                op.y0 = guidedOriginBy;
                op.pod = pod.copy();
                op.hops.add(hop);
                List<OctiMoveOp> cand = new ArrayList<>();
                for (OctiMoveOp sop : stagedOps) {
                    cand.add(new OctiMoveOp(sop));
                }
                cand.add(op);
                if (validateOpsOnScratch(cand)) {
                    IntPair pair = new IntPair(lx + 1, ly + 1);
                    if (seen.add(pair)) {
                        result.add(new int[]{lx + 1, ly + 1});
                    }
                }
            }
        } else {
            // Mid-jump continuation from (guidedCurBx, guidedCurBy)
            if (stagedOps.isEmpty()) {
                return result;
            }
            OctiMoveOp curOp = stagedOps.get(stagedOps.size() - 1);
            OctiPod pod = curOp.pod;
            OctiSquare startSq = board.squareAt(guidedOriginBx, guidedOriginBy);
            List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());

            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                if (pod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    continue;
                }
                if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK && isOfkBackwardDirection(turn, dir)) {
                    continue;
                }
                int jdx = dir.deltaX();
                int jdy = dir.deltaY();
                int dx = jdx * 2;
                int dy = jdy * 2;
                int jx = edgeless ? (((guidedCurBx + jdx) % w + w) % w) : (guidedCurBx + jdx);
                int jy = edgeless ? (((guidedCurBy + jdy) % h + h) % h) : (guidedCurBy + jdy);
                int lx = edgeless ? (((guidedCurBx + dx) % w + w) % w) : (guidedCurBx + dx);
                int ly = edgeless ? (((guidedCurBy + dy) % h + h) % h) : (guidedCurBy + dy);
                if (!board.isValidCoord(jx, jy) || !board.isValidCoord(lx, ly)) {
                    continue;
                }
                if (activeJumpedCoords.contains(new IntPair(jx, jy))) {
                    continue;
                }
                OctiSquare jumpedSq = board.squareAt(jx, jy);
                boolean jumpedEmpty = (jx == guidedOriginBx && jy == guidedOriginBy)
                        ? jumpedSq.isEmptyWithout(ghostPods) : jumpedSq.isEmpty();
                if (jumpedEmpty) {
                    continue;
                }

                OctiSquare landSq = board.squareAt(lx, ly);
                boolean landEmpty = (lx == guidedOriginBx && ly == guidedOriginBy)
                        ? landSq.isEmptyWithout(ghostPods) : landSq.isEmpty();
                if (!landEmpty) {
                    continue;
                }

                List<OctiMoveOp> cand = new ArrayList<>();
                for (OctiMoveOp sop : stagedOps) {
                    cand.add(new OctiMoveOp(sop));
                }
                OctiHop hop = new OctiHop(lx, ly, (board.getGameMode() == OctiGameMode.GAME_MODE_OFK) || jumpedSq.hasEnemyOf(turn));
                cand.get(cand.size() - 1).hops.add(hop);
                if (validateOpsOnScratch(cand)) {
                    IntPair pair = new IntPair(lx + 1, ly + 1);
                    if (seen.add(pair)) {
                        result.add(new int[]{lx + 1, ly + 1});
                    }
                }
            }
        }

        return result;
    }

    public boolean isGuidedLegalDestination(int x, int y) {
        if (!guidedMoveActive) {
            return false;
        }
        for (int[] sq : guidedNextSquares()) {
            if (sq[0] == x && sq[1] == y) {
                return true;
            }
        }
        return false;
    }

    public boolean selectGuidedDestination(int toX, int toY) {
        if (!guidedMoveActive || board == null || board.getGame() == null) {
            return false;
        }
        List<int[]> legal = guidedNextSquares();
        boolean found = false;
        for (int[] sq : legal) {
            if (sq[0] == toX && sq[1] == toY) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }

        int tbx = toX - 1;
        int tby = toY - 1;
        int fromBx = guidedMidHop ? guidedCurBx : guidedOriginBx;
        int fromBy = guidedMidHop ? guidedCurBy : guidedOriginBy;

        if (awaitingSuperProngBonusMove && !hasEighthProngBonusHop) {
            if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, guidedPodIndex,
                    toX, toY, false)) {
                return false;
            }
            guidedCurBx = tbx;
            guidedCurBy = tby;
            guidedMoveActive = false;
            awaitingConfirmation = true;
            return true;
        }

        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = board.getGame().useEdgelessBoard();
        int[] dxy = new int[2];
        computeDisplacement(fromBx, fromBy, tbx, tby, edgeless, w, h, dxy);
        boolean isStep = (Math.max(Math.abs(dxy[0]), Math.abs(dxy[1])) == 1);

        if (!guidedMidHop) {
            if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, guidedPodIndex,
                    toX, toY, guidedMoveAllStack)) {
                return false;
            }
            guidedCurBx = tbx;
            guidedCurBy = tby;
            if (isStep) {
                guidedMoveActive = false;
            } else {
                guidedMidHop = true;
                if (guidedNextSquares().isEmpty()) {
                    guidedMoveActive = false;
                }
            }
            return true;
        } else {
            if (!stageOrCommitPodMoveOrJump(fromBx + 1, fromBy + 1, guidedPodIndex,
                    toX, toY, guidedMoveAllStack)) {
                return false;
            }
            guidedCurBx = tbx;
            guidedCurBy = tby;
            if (guidedNextSquares().isEmpty()) {
                guidedMoveActive = false;
            }
            return true;
        }
    }

    public boolean finishGuidedMoveHere() {
        if (!guidedMoveActive) {
            return false;
        }
        if (guidedMidHop) {
            guidedMoveActive = false;
            activeJumpHasChain = false;
            return true;
        }
        guidedMoveActive = false;
        return false;
    }

    public boolean hasContinuationJump() {
        return guidedMoveActive && guidedMidHop && !guidedNextSquares().isEmpty();
    }

    public boolean canAddMoreMoves() {
        return (awaitingSuperProngBonusMove && !hasEighthProngBonusHop)
                || hasOtherMovablePodAtOrigin()
                || hasContinuationJump();
    }

    public boolean hasOtherMovablePodAtOrigin() {
        if (stagedOps.isEmpty() || stagedOps.get(0).hops.isEmpty()
                || board == null || board.getGame() == null) {
            return false;
        }
        int origBx = stagedOps.get(0).x0;
        int origBy = stagedOps.get(0).y0;
        OctiPlayer turn = board.getGame().getTurn();
        OctiSquare sq = board.squareAt(origBx, origBy);
        OctiPodType desiredType = turn.toPodType();

        for (int i = 0; i < sq.getNumPods(); i++) {
            if (isPodClaimed(origBx, origBy, i)) {
                continue;
            }
            OctiPod pod = sq.getPodAt(i);
            if (pod.getType() != desiredType) {
                continue;
            }
            if (pod.getNumProngs() > 0) {
                List<int[]> dests = legalDestinationsFrom(origBx + 1, origBy + 1, i);
                if (!dests.isEmpty()) {
                    return true;
                }
            }
        }
        return false;
    }

    public List<int[]> legalDestinationsFrom(int x, int y) {
        return legalDestinationsFrom(x, y, -1);
    }

    public List<int[]> legalDestinationsFrom(int x, int y, int podIndex) {
        List<int[]> result = new ArrayList<>();
        if (board == null || board.getGame() == null) {
            return result;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return result;
        }

        int bx = x - 1;
        int by = y - 1;
        if (!board.isValidCoord(bx, by)) {
            return result;
        }

        OctiPlayer turn = game.getTurn();
        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        boolean edgeless = game.useEdgelessBoard();

        if (activeJumpHasChain && !stagedOps.isEmpty()
                && bx == activeJumpCurBx && by == activeJumpCurBy) {
            OctiPod pod = stagedOps.get(stagedOps.size() - 1).pod;
            OctiSquare startSq = board.squareAt(activeJumpStartBx, activeJumpStartBy);
            List<OctiPod> ghostPods = new ArrayList<>(startSq.pods());

            for (int ty = 0; ty < h; ty++) {
                for (int tx = 0; tx < w; tx++) {
                    List<OctiHop> extraHops = new ArrayList<>();
                    List<IntPair> nextJumped = new ArrayList<>();
                    if (findJumpPathBfs(board, edgeless, turn,
                            activeJumpStartBx, activeJumpStartBy,
                            activeJumpCurBx, activeJumpCurBy,
                            tx, ty, pod, ghostPods,
                            activeJumpedCoords, extraHops, nextJumped)) {
                        result.add(new int[]{tx + 1, ty + 1});
                    }
                }
            }
            return result;
        }

        if (awaitingSuperProngBonusMove && stagedOps.size() == 1
                && stagedOps.get(0).x0 == bx && stagedOps.get(0).y0 == by) {
            OctiPod fullPod = stagedOps.get(0).pod.copy();
            fullPod.addProng(stagedOps.get(0).addProngDir, true);
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                if (fullPod.prongAt(dir) == OctiProngType.NO_PRONG) {
                    continue;
                }
                int sx = edgeless ? (((bx + dir.deltaX()) % w + w) % w) : (bx + dir.deltaX());
                int sy = edgeless ? (((by + dir.deltaY()) % h + h) % h) : (by + dir.deltaY());
                if (board.isValidCoord(sx, sy) && !board.squareAt(sx, sy).hasEnemyOf(turn)) {
                    result.add(new int[]{sx + 1, sy + 1});
                }
                int jx = edgeless ? (((bx + 2 * dir.deltaX()) % w + w) % w) : (bx + 2 * dir.deltaX());
                int jy = edgeless ? (((by + 2 * dir.deltaY()) % h + h) % h) : (by + 2 * dir.deltaY());
                if (board.isValidCoord(sx, sy) && board.isValidCoord(jx, jy)
                        && !board.squareAt(sx, sy).isEmpty()
                        && board.squareAt(jx, jy).isEmpty()) {
                    result.add(new int[]{jx + 1, jy + 1});
                }
            }
            return result;
        }

        OctiSquare sq = board.squareAt(bx, by);
        int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, true);
        boolean filterByPod = (podIndex >= 0 && resolvedIdx >= 0);
        int targetPodConf = filterByPod
                ? (sq.getPodAt(resolvedIdx).rawConfig() & 0xFFFF)
                : 0;

        Set<IntPair> seen = new LinkedHashSet<>();
        List<OctiMove> legal = game.generateLegalMoves();
        for (OctiMove mv : legal) {
            List<OctiMoveOp> ops = mv.getOps();
            if (ops.size() != 1 || ops.get(0).hops.isEmpty()) {
                continue;
            }
            OctiMoveOp op = ops.get(0);
            if (op.x0 != bx || op.y0 != by) {
                continue;
            }
            if (filterByPod && (op.pod.rawConfig() & 0xFFFF) != targetPodConf) {
                continue;
            }
            OctiHop lastHop = op.hops.get(op.hops.size() - 1);
            IntPair dest = new IntPair(lastHop.x + 1, lastHop.y + 1);
            if (seen.add(dest)) {
                result.add(new int[]{dest.first, dest.second});
            }
        }
        return result;
    }

    public List<OctiDir> legalProngAdditionsAt(int x, int y, int podIndex, boolean superProng) {
        List<OctiDir> result = new ArrayList<>();
        if (board == null || board.getGame() == null) {
            return result;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return result;
        }

        int bx = x - 1;
        int by = y - 1;
        if (!board.isValidCoord(bx, by)) {
            return result;
        }

        OctiPlayer turn = game.getTurn();
        if (superProng) {
            if (!game.useSuperProngs() || game.getFreeSuperProngs(turn) <= 0) {
                return result;
            }
        } else {
            if (game.getFreeProngs(turn) <= 0) {
                return result;
            }
        }

        OctiSquare sq = board.squareAt(bx, by);
        int resolvedIdx = resolvePodIndex(sq, x, y, podIndex, turn, false);
        if (resolvedIdx < 0) {
            return result;
        }

        OctiPod pod = sq.getPodAt(resolvedIdx);
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            if (pod.prongAt(dir) != OctiProngType.NO_PRONG) {
                continue;
            }
            if (board.getGameMode() == OctiGameMode.GAME_MODE_OFK
                    && isOfkBackwardDirection(turn, dir)) {
                continue;
            }
            result.add(dir);
        }
        return result;
    }

    public List<int[]> legalPodDropSquares(boolean liberate) {
        List<int[]> result = new ArrayList<>();
        if (board == null || board.getGame() == null) {
            return result;
        }
        OctiGame game = board.getGame();
        if (!game.isGameStarted() || game.isGameOver()) {
            return result;
        }

        OctiPlayer turn = game.getTurn();
        if (liberate) {
            if (game.getCapturedPods(turn) <= 0 || !board.canLiberate(turn)) {
                return result;
            }
        } else {
            if (game.getFreePods(turn) <= 0) {
                return result;
            }
        }

        int w = board.getBoardWidth();
        int h = board.getBoardHeight();
        for (int by = 0; by < h; by++) {
            for (int bx = 0; bx < w; bx++) {
                OctiMoveOp op = new OctiMoveOp();
                op.x0 = bx;
                op.y0 = by;
                op.addPod = true;
                op.liberate = liberate;
                if (validateOpsOnScratch(Collections.singletonList(op))) {
                    result.add(new int[]{bx + 1, by + 1});
                }
            }
        }
        return result;
    }

    public boolean commitMove() {
        return commitStagedMove();
    }

    public boolean commitStagedMove() {
        if (board == null || board.getGame() == null || stagedOps.isEmpty()) {
            return false;
        }
        OctiGame game = board.getGame();
        List<OctiMoveOp> opsToCommit = new ArrayList<>();
        for (OctiMoveOp op : stagedOps) {
            opsToCommit.add(new OctiMoveOp(op));
        }
        boolean hadBonusHop = hasEighthProngBonusHop;
        int bonusTx = eighthProngTargetBx;
        int bonusTy = eighthProngTargetBy;
        String committedNotation = stagedMoveNotation();

        game.rollbackPreparedMove();

        try {
            OctiMove mv = new OctiMove(opsToCommit);
            game.move(mv);
            if (hadBonusHop && board.isValidCoord(bonusTx, bonusTy) && opsToCommit.size() == 1) {
                int sx = opsToCommit.get(0).x0;
                int sy = opsToCommit.get(0).y0;
                OctiSquare srcSq = board.squareAt(sx, sy);
                OctiSquare dstSq = board.squareAt(bonusTx, bonusTy);
                if (!srcSq.isEmpty()) {
                    OctiPod movedPod = srcSq.getPodAt(srcSq.getNumPods() - 1).copy();
                    srcSq.removePod(movedPod);
                    dstSq.addPod(movedPod);
                }
            }
            if (!hadBonusHop) {
                committedNotation = mv.asString();
            }
        } catch (Throwable t) {
            cancelAndRollback();
            return false;
        }

        stagedOps.clear();
        claimedStartPods.clear();
        captureChoices.clear();
        hopCaptureLinks.clear();
        awaitingConfirmation = false;
        awaitingSuperProngBonusMove = false;
        activeJumpHasChain = false;
        hasEighthProngBonusHop = false;
        eighthProngBonusNotation = "";
        activeDragKind = DragKind.NONE;

        guidedMoveActive = false;
        guidedMidHop = false;
        guidedOriginBx = -1;
        guidedOriginBy = -1;
        guidedCurBx = -1;
        guidedCurBy = -1;
        guidedPodIndex = -1;
        guidedMoveAllStack = false;

        refreshLegalMoves();

        if (onMoveCommitted != null) {
            onMoveCommitted.onMoveCommitted(committedNotation);
        }
        return true;
    }

    public void rollbackMove() {
        cancelAndRollback();
    }

    public void cancelAndRollback() {
        if (board != null && board.getGame() != null) {
            board.getGame().rollbackPreparedMove();
        }
        stagedOps.clear();
        claimedStartPods.clear();
        captureChoices.clear();
        hopCaptureLinks.clear();
        awaitingConfirmation = false;
        awaitingSuperProngBonusMove = false;
        activeJumpHasChain = false;
        activeJumpStartBx = -1;
        activeJumpStartBy = -1;
        activeJumpCurBx = -1;
        activeJumpCurBy = -1;
        activeJumpedCoords.clear();
        hasEighthProngBonusHop = false;
        eighthProngTargetBx = -1;
        eighthProngTargetBy = -1;
        eighthProngBonusNotation = "";
        activeDragKind = DragKind.NONE;

        guidedMoveActive = false;
        guidedMidHop = false;
        guidedOriginBx = -1;
        guidedOriginBy = -1;
        guidedCurBx = -1;
        guidedCurBy = -1;
        guidedPodIndex = -1;
        guidedMoveAllStack = false;

        refreshLegalMoves();
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
