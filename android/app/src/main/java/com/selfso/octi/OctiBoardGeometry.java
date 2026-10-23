/*
 * OctiBoardGeometry.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class OctiBoardGeometry {

    public enum BoardRotation {
        ROT_0(0),
        ROT_90(90),
        ROT_180(180),
        ROT_270(270);

        private final int degrees;

        BoardRotation(int degrees) {
            this.degrees = degrees;
        }

        public int degrees() {
            return degrees;
        }

        public int getDegrees() {
            return degrees;
        }
    }

    private int viewportWidth;
    private int viewportHeight;
    private int gridWidth;
    private int gridHeight;
    private BoardRotation rotation;

    private double cellSize;
    private double[] boardOuterRect = new double[4];
    private double[] gridInnerRect = new double[4];

    public OctiBoardGeometry() {
        this(800, 800, 9, 9, BoardRotation.ROT_0);
    }

    public OctiBoardGeometry(int viewportWidth, int viewportHeight, int gridWidth, int gridHeight) {
        this(viewportWidth, viewportHeight, gridWidth, gridHeight, BoardRotation.ROT_0);
    }

    public OctiBoardGeometry(int viewportWidth,
                             int viewportHeight,
                             int gridWidth,
                             int gridHeight,
                             BoardRotation rotation) {
        configure(viewportWidth, viewportHeight, gridWidth, gridHeight, rotation);
    }

    public OctiBoardGeometry(int gridWidth,
                             int gridHeight,
                             double viewportWidth,
                             double viewportHeight,
                             BoardRotation rotation) {
        configure((int) Math.round(viewportWidth), (int) Math.round(viewportHeight),
                gridWidth, gridHeight, rotation);
    }

    public void configure(int viewportWidth,
                          int viewportHeight,
                          int gridWidth,
                          int gridHeight,
                          BoardRotation rotation) {
        this.viewportWidth = Math.max(64, viewportWidth);
        this.viewportHeight = Math.max(64, viewportHeight);
        this.gridWidth = clamp(gridWidth, 1, 9);
        this.gridHeight = clamp(gridHeight, 1, 9);
        this.rotation = (rotation != null) ? rotation : BoardRotation.ROT_0;
        recomputeLayout();
    }

    public void setViewportSize(int viewportWidth, int viewportHeight) {
        this.viewportWidth = Math.max(64, viewportWidth);
        this.viewportHeight = Math.max(64, viewportHeight);
        recomputeLayout();
    }

    public void setGridDimensions(int gridWidth, int gridHeight) {
        this.gridWidth = clamp(gridWidth, 1, 9);
        this.gridHeight = clamp(gridHeight, 1, 9);
        recomputeLayout();
    }

    public void setRotation(BoardRotation rotation) {
        this.rotation = (rotation != null) ? rotation : BoardRotation.ROT_0;
        recomputeLayout();
    }

    public BoardRotation rotation() {
        return rotation;
    }

    public BoardRotation getRotation() {
        return rotation;
    }

    public int rotationDegrees() {
        return rotation.degrees();
    }

    public int getRotationDegrees() {
        return rotation.degrees();
    }

    public int viewportWidth() {
        return viewportWidth;
    }

    public int getViewportWidth() {
        return viewportWidth;
    }

    public int viewportHeight() {
        return viewportHeight;
    }

    public int getViewportHeight() {
        return viewportHeight;
    }

    public int gridWidth() {
        return gridWidth;
    }

    public int getGridWidth() {
        return gridWidth;
    }

    public int gridHeight() {
        return gridHeight;
    }

    public int getGridHeight() {
        return gridHeight;
    }

    public double cellSize() {
        return cellSize;
    }

    public double getCellSize() {
        return cellSize;
    }

    public static BoardRotation rotationForSeat(OctiGameMode mode, OctiPlayer seat) {
        if (seat == null || seat == OctiPlayer.PLAYER_A) {
            return BoardRotation.ROT_0;
        }
        if (mode == OctiGameMode.GAME_MODE_2 || mode == OctiGameMode.GAME_MODE_OFK) {
            if (seat == OctiPlayer.PLAYER_B) {
                return BoardRotation.ROT_180;
            }
            return BoardRotation.ROT_0;
        }
        if (mode == OctiGameMode.GAME_MODE_4) {
            switch (seat) {
                case PLAYER_A:
                    return BoardRotation.ROT_0;
                case PLAYER_B:
                    return BoardRotation.ROT_90;
                case PLAYER_C:
                    return BoardRotation.ROT_180;
                case PLAYER_D:
                    return BoardRotation.ROT_270;
                default:
                    return BoardRotation.ROT_0;
            }
        }
        return BoardRotation.ROT_0;
    }

    public static BoardRotation rotationForSeat(OctiGameMode mode, int seat) {
        if (seat < 1 || seat > 4) {
            return BoardRotation.ROT_0;
        }
        return rotationForSeat(mode, OctiPlayer.fromValue(seat));
    }

    public static BoardRotation nextRotationClockwise(BoardRotation r) {
        if (r == null) {
            return BoardRotation.ROT_0;
        }
        switch (r) {
            case ROT_0:
                return BoardRotation.ROT_90;
            case ROT_90:
                return BoardRotation.ROT_180;
            case ROT_180:
                return BoardRotation.ROT_270;
            case ROT_270:
                return BoardRotation.ROT_0;
            default:
                return BoardRotation.ROT_0;
        }
    }

    private int screenCols() {
        if (rotation == BoardRotation.ROT_90 || rotation == BoardRotation.ROT_270) {
            return gridHeight;
        }
        return gridWidth;
    }

    private int screenRows() {
        if (rotation == BoardRotation.ROT_90 || rotation == BoardRotation.ROT_270) {
            return gridWidth;
        }
        return gridHeight;
    }

    private void recomputeLayout() {
        int cols = Math.max(1, screenCols());
        int rows = Math.max(1, screenRows());

        double pad = Math.max(12.0, Math.min(viewportWidth, viewportHeight) * 0.045);
        double availW = Math.max(32.0, viewportWidth - 2.0 * pad);
        double availH = Math.max(32.0, viewportHeight - 2.0 * pad);

        double cellW = availW / (cols + 1.10);
        double cellH = availH / (rows + 1.10);
        cellSize = Math.max(4.0, Math.min(cellW, cellH));

        double gridW = cellSize * cols;
        double gridH = cellSize * rows;
        double gutter = cellSize * 0.55;

        double gridLeft = (viewportWidth - gridW) * 0.5;
        double gridTop = (viewportHeight - gridH) * 0.5;

        gridInnerRect = new double[]{gridLeft, gridTop, gridW, gridH};
        boardOuterRect = new double[]{
            gridLeft - gutter,
            gridTop - gutter,
            gridW + 2.0 * gutter,
            gridH + 2.0 * gutter
        };
    }

    private boolean boardToScreenGrid(int x1, int y1, int[] outColRow) {
        if (x1 < 1 || x1 > gridWidth || y1 < 1 || y1 > gridHeight) {
            return false;
        }

        switch (rotation) {
            case ROT_0:
                outColRow[0] = x1 - 1;
                outColRow[1] = gridHeight - y1;
                break;
            case ROT_90:
                outColRow[0] = gridHeight - y1;
                outColRow[1] = gridWidth - x1;
                break;
            case ROT_180:
                outColRow[0] = gridWidth - x1;
                outColRow[1] = y1 - 1;
                break;
            case ROT_270:
                outColRow[0] = y1 - 1;
                outColRow[1] = x1 - 1;
                break;
        }
        return true;
    }

    private void screenGridToBoard(int col, int row, int[] outXY1) {
        switch (rotation) {
            case ROT_0:
                outXY1[0] = col + 1;
                outXY1[1] = gridHeight - row;
                break;
            case ROT_90:
                outXY1[0] = gridWidth - row;
                outXY1[1] = gridHeight - col;
                break;
            case ROT_180:
                outXY1[0] = gridWidth - col;
                outXY1[1] = row + 1;
                break;
            case ROT_270:
                outXY1[0] = row + 1;
                outXY1[1] = col + 1;
                break;
        }
    }

    public double[] boardOuterRect() {
        return boardOuterRect.clone();
    }

    public double[] gridInnerRect() {
        return gridInnerRect.clone();
    }

    public double[] squareRect(int x1, int y1) {
        int[] colRow = new int[2];
        if (!boardToScreenGrid(x1, y1, colRow)) {
            return new double[]{0.0, 0.0, 0.0, 0.0};
        }
        double sx = gridInnerRect[0] + colRow[0] * cellSize;
        double sy = gridInnerRect[1] + colRow[1] * cellSize;
        return new double[]{sx, sy, cellSize, cellSize};
    }

    public double[] squareRect0(int x0, int y0) {
        return squareRect(x0 + 1, y0 + 1);
    }

    public double[] squareCenter(int x1, int y1) {
        double[] r = squareRect(x1, y1);
        if (r[2] <= 0.0 || r[3] <= 0.0) {
            return new double[]{0.0, 0.0};
        }
        return new double[]{r[0] + r[2] * 0.5, r[1] + r[3] * 0.5};
    }

    public double[] squareCenter0(int x0, int y0) {
        return squareCenter(x0 + 1, y0 + 1);
    }

    public boolean screenToSquare(double px, double py, int[] outXY1) {
        if (outXY1 == null || outXY1.length < 2) {
            return false;
        }
        if (px < gridInnerRect[0] || px >= gridInnerRect[0] + gridInnerRect[2]
                || py < gridInnerRect[1] || py >= gridInnerRect[1] + gridInnerRect[3]) {
            return false;
        }

        int cols = screenCols();
        int rows = screenRows();
        int col = (int) Math.floor((px - gridInnerRect[0]) / cellSize);
        int row = (int) Math.floor((py - gridInnerRect[1]) / cellSize);
        if (col < 0 || col >= cols || row < 0 || row >= rows) {
            return false;
        }

        screenGridToBoard(col, row, outXY1);
        return (outXY1[0] >= 1 && outXY1[0] <= gridWidth
                && outXY1[1] >= 1 && outXY1[1] <= gridHeight);
    }

    public boolean screenToSquare0(double px, double py, int[] outXY0) {
        if (outXY0 == null || outXY0.length < 2) {
            return false;
        }
        int[] xy1 = new int[2];
        if (!screenToSquare(px, py, xy1)) {
            return false;
        }
        outXY0[0] = xy1[0] - 1;
        outXY0[1] = xy1[1] - 1;
        return true;
    }

    public double[] dirUnitVectorScreen(OctiDir dir) {
        if (dir == null || dir == OctiDir.DIR_UNDEF) {
            return new double[]{0.0, 0.0};
        }

        double dx = dir.deltaX();
        double dy = dir.deltaY();
        double sx = 0.0;
        double sy = 0.0;

        switch (rotation) {
            case ROT_0:
                sx = dx;
                sy = -dy;
                break;
            case ROT_90:
                sx = -dy;
                sy = -dx;
                break;
            case ROT_180:
                sx = -dx;
                sy = dy;
                break;
            case ROT_270:
                sx = dy;
                sy = dx;
                break;
        }

        double len = Math.hypot(sx, sy);
        if (len <= 1e-9) {
            return new double[]{0.0, 0.0};
        }
        return new double[]{sx / len, sy / len};
    }

    public OctiDir hitTestCompassSector(double podCenterX,
                                        double podCenterY,
                                        double innerRadius,
                                        double outerRadius,
                                        double cursorX,
                                        double cursorY) {
        if (innerRadius < 0.0 || outerRadius <= innerRadius) {
            return OctiDir.DIR_UNDEF;
        }

        double vx = cursorX - podCenterX;
        double vy = cursorY - podCenterY;
        double dist = Math.hypot(vx, vy);
        if (dist < innerRadius || dist > outerRadius || dist < 1e-9) {
            return OctiDir.DIR_UNDEF;
        }

        double ux = vx / dist;
        double uy = vy / dist;

        OctiDir bestDir = OctiDir.DIR_UNDEF;
        double bestDot = -2.0;

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            double[] du = dirUnitVectorScreen(dir);
            double dot = ux * du[0] + uy * du[1];
            if (dot > bestDot) {
                bestDot = dot;
                bestDir = dir;
            }
        }
        return bestDir;
    }

    public OctiDir hitTestCompassSector(double[] podCenter,
                                        double innerRadius,
                                        double outerRadius,
                                        double cursorX,
                                        double cursorY) {
        if (podCenter == null || podCenter.length < 2) {
            return OctiDir.DIR_UNDEF;
        }
        return hitTestCompassSector(
                podCenter[0], podCenter[1], innerRadius, outerRadius, cursorX, cursorY);
    }

    public double[] stackFanOverlayRect(double[] sqRect, int numPods) {
        if (sqRect == null || sqRect.length < 4 || sqRect[2] <= 0.0 || sqRect[3] <= 0.0 || numPods <= 0) {
            return new double[]{0.0, 0.0, 0.0, 0.0};
        }

        int clampedPods = clamp(numPods, 1, 8);
        double slotW = sqRect[2] * 0.78;
        double slotH = sqRect[3] * 0.78;
        double pad = sqRect[2] * 0.08;
        double totalW = clampedPods * slotW + (clampedPods + 1) * pad;
        double totalH = slotH + 2.0 * pad;

        double cx = sqRect[0] + sqRect[2] * 0.5;
        double minLeft = boardOuterRect[0] + 4.0;
        double maxLeft = Math.max(minLeft, boardOuterRect[0] + boardOuterRect[2] - totalW - 4.0);
        double left = clampDouble(cx - totalW * 0.5, minLeft, maxLeft);

        double top = sqRect[1] - totalH - 6.0;
        if (top < boardOuterRect[1] + 4.0) {
            top = sqRect[1] + sqRect[3] + 6.0;
        }
        return new double[]{left, top, totalW, totalH};
    }

    private double[] resolveSquareRectFlexible(int x, int y) {
        if (x >= 1 && x <= gridWidth && y >= 1 && y <= gridHeight) {
            return squareRect(x, y);
        }
        if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight) {
            return squareRect0(x, y);
        }
        return new double[]{0.0, 0.0, 0.0, 0.0};
    }

    public double[] stackFanOverlayRect(int x, int y, int numPods) {
        return stackFanOverlayRect(resolveSquareRectFlexible(x, y), numPods);
    }

    public double[] stackFanOverlayRect0(int x0, int y0, int numPods) {
        return stackFanOverlayRect(squareRect0(x0, y0), numPods);
    }

    public double[] stackFanSlotRect(double[] sqRect, int numPods, int slotIndex) {
        if (sqRect == null || sqRect.length < 4 || sqRect[2] <= 0.0 || sqRect[3] <= 0.0
                || numPods <= 0 || slotIndex < 0 || slotIndex >= numPods) {
            return new double[]{0.0, 0.0, 0.0, 0.0};
        }

        double[] overlay = stackFanOverlayRect(sqRect, numPods);
        if (overlay[2] <= 0.0 || overlay[3] <= 0.0) {
            return new double[]{0.0, 0.0, 0.0, 0.0};
        }

        double slotW = sqRect[2] * 0.78;
        double slotH = sqRect[3] * 0.78;
        double pad = sqRect[2] * 0.08;
        double sx = overlay[0] + pad + slotIndex * (slotW + pad);
        double sy = overlay[1] + pad;
        return new double[]{sx, sy, slotW, slotH};
    }

    public double[] stackFanSlotRect(int x, int y, int numPods, int slotIndex) {
        return stackFanSlotRect(resolveSquareRectFlexible(x, y), numPods, slotIndex);
    }

    public double[] stackFanSlotRect0(int x0, int y0, int numPods, int slotIndex) {
        return stackFanSlotRect(squareRect0(x0, y0), numPods, slotIndex);
    }

    public int hitTestStackFanSlot(double[] sqRect, int numPods, double cursorX, double cursorY) {
        if (sqRect == null || sqRect.length < 4 || sqRect[2] <= 0.0 || sqRect[3] <= 0.0
                || numPods <= 0) {
            return -1;
        }
        double[] overlay = stackFanOverlayRect(sqRect, numPods);
        if (!rectContains(overlay, cursorX, cursorY)) {
            return -1;
        }

        for (int i = 0; i < numPods; i++) {
            double[] slot = stackFanSlotRect(sqRect, numPods, i);
            if (rectContains(slot, cursorX, cursorY)) {
                return i;
            }
        }
        return -1;
    }

    public int hitTestStackFanSlot(int x, int y, int numPods, double cursorX, double cursorY) {
        return hitTestStackFanSlot(resolveSquareRectFlexible(x, y), numPods, cursorX, cursorY);
    }

    public int hitTestStackFanSlot0(int x0, int y0, int numPods, double cursorX, double cursorY) {
        return hitTestStackFanSlot(squareRect0(x0, y0), numPods, cursorX, cursorY);
    }

    private static boolean rectContains(double[] r, double px, double py) {
        return r != null && r.length >= 4
                && px >= r[0] && px <= r[0] + r[2]
                && py >= r[1] && py <= r[1] + r[3];
    }

    private static int clamp(int val, int min, int max) {
        return Math.max(min, Math.min(max, val));
    }

    private static double clampDouble(double val, double min, double max) {
        return Math.max(min, Math.min(max, val));
    }
}
