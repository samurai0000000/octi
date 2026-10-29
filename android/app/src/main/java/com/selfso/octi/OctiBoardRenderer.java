/*
 * OctiBoardRenderer.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public final class OctiBoardRenderer {

    public static final int LAST_MOVE_INDICATOR_COLOR = 0xFFFF8C00;

    public static final class RenderConfig {
        public List<int[]> lastMoveModifiedSquares = new ArrayList<>();
        public float lastMoveAnimPhase = 0.0f;
        public List<int[]> stagedPaths = new ArrayList<>();
    }

    private int stackFanX = -1;
    private int stackFanY = -1;
    private int hoverSquareX = -1;
    private int hoverSquareY = -1;
    private OctiDir hoverProngDir = OctiDir.DIR_UNDEF;

    public OctiBoardRenderer() {
    }

    private static void addUniqueSquare(List<int[]> list, int x1, int y1) {
        for (int[] sq : list) {
            if (sq != null && sq.length >= 2 && sq[0] == x1 && sq[1] == y1) {
                return;
            }
        }
        list.add(new int[]{x1, y1});
    }

    public static List<int[]> extractModifiedSquares(OctiMove move, boolean edgeless) {
        List<int[]> result = new ArrayList<>();
        if (move == null || move.getOps() == null) {
            return result;
        }
        for (OctiMoveOp op : move.getOps()) {
            addUniqueSquare(result, op.x0 + 1, op.y0 + 1);
            if (op.hops != null && !op.hops.isEmpty()) {
                int cx = op.x0;
                int cy = op.y0;
                for (OctiHop h : op.hops) {
                    int dx = h.x - cx;
                    int dy = h.y - cy;
                    if (edgeless) {
                        if (dx > 4) dx -= 9;
                        else if (dx < -4) dx += 9;
                        if (dy > 4) dy -= 9;
                        else if (dy < -4) dy += 9;
                    }
                    if (Math.max(Math.abs(dx), Math.abs(dy)) == 2) {
                        int jx = edgeless ? (cx + dx / 2 + 9) % 9 : (cx + dx / 2);
                        int jy = edgeless ? (cy + dy / 2 + 9) % 9 : (cy + dy / 2);
                        addUniqueSquare(result, jx + 1, jy + 1);
                    }
                    addUniqueSquare(result, h.x + 1, h.y + 1);
                    cx = h.x;
                    cy = h.y;
                }
            }
        }
        return result;
    }

    public static List<int[]> extractModifiedSquares(String moveNotation, boolean edgeless) {
        if (moveNotation == null || moveNotation.trim().isEmpty()) {
            return new ArrayList<>();
        }
        try {
            return extractModifiedSquares(new OctiMove(moveNotation.trim()), edgeless);
        } catch (Throwable t) {
            return new ArrayList<>();
        }
    }

    public static void paintClockwiseOrangeSquareSegment(int[] pixels, int w, int h,
                                                         double[] sqRect, double phase) {
        if (sqRect == null || sqRect.length < 4 || sqRect[2] <= 3.0 || sqRect[3] <= 3.0) {
            return;
        }
        double left = sqRect[0] + 1.5;
        double top = sqRect[1] + 1.5;
        double right = sqRect[0] + sqRect[2] - 1.5;
        double bottom = sqRect[1] + sqRect[3] - 1.5;
        double thickness = Math.max(3.0, sqRect[2] * 0.055);

        double normPhase = phase - Math.floor(phase);
        double s0 = normPhase * 4.0;
        double segLen = 1.2;
        double cur = s0;
        double end = s0 + segLen;
        while (cur < end - 1e-6) {
            double edgeBase = Math.floor(cur);
            double nextBoundary = Math.min(end, edgeBase + 1.0);
            int edgeIdx = ((int) edgeBase) % 4;
            if (edgeIdx < 0) edgeIdx += 4;
            double t0 = cur - edgeBase;
            double t1 = nextBoundary - edgeBase;
            double x1, y1, x2, y2;
            switch (edgeIdx) {
                case 0: // Top: left -> right
                    x1 = left + t0 * (right - left);
                    y1 = top;
                    x2 = left + t1 * (right - left);
                    y2 = top;
                    break;
                case 1: // Right: top -> bottom
                    x1 = right;
                    y1 = top + t0 * (bottom - top);
                    x2 = right;
                    y2 = top + t1 * (bottom - top);
                    break;
                case 2: // Bottom: right -> left
                    x1 = right - t0 * (right - left);
                    y1 = bottom;
                    x2 = right - t1 * (right - left);
                    y2 = bottom;
                    break;
                default: // Left: bottom -> top
                    x1 = left;
                    y1 = bottom - t0 * (bottom - top);
                    x2 = left;
                    y2 = bottom - t1 * (bottom - top);
                    break;
            }
            OctiAppIconRenderer.drawLineSegment(pixels, w, h, x1, y1, x2, y2, thickness, LAST_MOVE_INDICATOR_COLOR);
            cur = nextBoundary;
        }
    }

    public void openStackFanAt(int x1, int y1) {
        this.stackFanX = x1;
        this.stackFanY = y1;
    }

    public void closeStackFan() {
        this.stackFanX = -1;
        this.stackFanY = -1;
    }

    public void setHoverSquare(int x1, int y1, OctiDir prongDir) {
        this.hoverSquareX = x1;
        this.hoverSquareY = y1;
        this.hoverProngDir = (prongDir != null) ? prongDir : OctiDir.DIR_UNDEF;
    }

    public static int[] renderBoardPixels(OctiBoard board,
                                          OctiMoveComposer composer,
                                          OctiBoardGeometry geo,
                                          OctiThemeId theme) {
        return new OctiBoardRenderer().render(board, composer, geo, theme, null);
    }

    public static int[] renderBoardPixels(OctiBoard board,
                                          OctiMoveComposer composer,
                                          OctiBoardGeometry geo,
                                          OctiThemeId theme,
                                          RenderConfig config) {
        return new OctiBoardRenderer().render(board, composer, geo, theme, config);
    }

    public static int[] paintBoard(OctiBoard board,
                                   OctiMoveComposer composer,
                                   OctiBoardGeometry geo,
                                   OctiThemeId theme,
                                   RenderConfig config) {
        return new OctiBoardRenderer().render(board, composer, geo, theme, config);
    }

    public static int[] renderBoardPixels(OctiBoard board,
                                          OctiMoveComposer composer,
                                          OctiThemeId theme,
                                          OctiBoardGeometry.BoardRotation rotation,
                                          int viewportWidth,
                                          int viewportHeight) {
        OctiBoardGeometry geo = new OctiBoardGeometry(
                viewportWidth,
                viewportHeight,
                board != null ? board.getBoardWidth() : 9,
                board != null ? board.getBoardHeight() : 9,
                rotation);
        return new OctiBoardRenderer().render(board, composer, geo, theme, null);
    }

    public int[] render(OctiBoard board,
                        OctiMoveComposer composer,
                        OctiBoardGeometry geo,
                        OctiThemeId theme) {
        return render(board, composer, geo, theme, null);
    }

    public int[] render(OctiBoard board,
                        OctiMoveComposer composer,
                        OctiBoardGeometry geo,
                        OctiThemeId theme,
                        RenderConfig config) {
        OctiBoardGeometry g = (geo != null)
                ? geo
                : new OctiBoardGeometry(
                        800, 800,
                        board != null ? board.getBoardWidth() : 9,
                        board != null ? board.getBoardHeight() : 9,
                        OctiBoardGeometry.BoardRotation.ROT_0);
        int w = Math.max(64, g.viewportWidth());
        int h = Math.max(64, g.viewportHeight());
        OctiPalette pal = OctiPalette.forTheme(theme);

        int[] pixels = new int[w * h];
        Arrays.fill(pixels, pal.windowBg);

        // 1. Grid inner rect spanning borderless full-width canvas
        double[] outer = g.boardOuterRect();
        double[] grid = g.gridInnerRect();
        int gw = g.gridWidth();
        int gh = g.gridHeight();
        double cell = g.cellSize();

        // 2. Corner compass rose showing North ('a') inside top-right corner
        double roseSize = Math.max(18.0, cell * 0.44);
        double roseCx = grid[0] + grid[2] - roseSize * 0.5 - 4.0;
        double roseCy = grid[1] + roseSize * 0.5 + 4.0;
        paintCompassRose(pixels, w, h, roseCx, roseCy, roseSize * 0.44, g, pal);

        // 4. Edgeless board portal glow & chevrons
        if (board != null && board.getGame() != null && board.getGame().useEdgelessBoard()) {
            drawRectOutline(pixels, w, h,
                    grid[0] - 3.0, grid[1] - 3.0,
                    grid[2] + 6.0, grid[3] + 6.0,
                    2.2, pal.edgelessPortalGlow);
            for (int x = 1; x <= gw; x++) {
                double[] rBot = g.squareRect(x, 1);
                double[] rTop = g.squareRect(x, gh);
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                        rBot[0] + rBot[2] * 0.5, grid[1] + grid[3] + cell * 0.10,
                        Math.max(2.0, cell * 0.045), pal.edgelessPortalGlow);
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                        rTop[0] + rTop[2] * 0.5, grid[1] - cell * 0.10,
                        Math.max(2.0, cell * 0.045), pal.edgelessPortalGlow);
            }
            for (int y = 1; y <= gh; y++) {
                double[] rLeft = g.squareRect(1, y);
                double[] rRight = g.squareRect(gw, y);
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                        grid[0] - cell * 0.10, rLeft[1] + rLeft[3] * 0.5,
                        Math.max(2.0, cell * 0.045), pal.edgelessPortalGlow);
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                        grid[0] + grid[2] + cell * 0.10, rRight[1] + rRight[3] * 0.5,
                        Math.max(2.0, cell * 0.045), pal.edgelessPortalGlow);
            }
        }

        // 5. Grid squares, recessed sockets, home base shields, and neutral OCTI medallions
        for (int y = 1; y <= gh; y++) {
            for (int x = 1; x <= gw; x++) {
                double[] sqRect = g.squareRect(x, y);
                boolean isDark = ((x + y) % 2 == 0);
                fillRectSolid(pixels, w, h,
                        sqRect[0], sqRect[1], sqRect[2], sqRect[3],
                        isDark ? pal.gridCellDark : pal.gridCellLight);
                drawRectOutline(pixels, w, h,
                        sqRect[0], sqRect[1], sqRect[2], sqRect[3],
                        1.0, pal.gridLine);

                double cx = sqRect[0] + sqRect[2] * 0.5;
                double cy = sqRect[1] + sqRect[3] * 0.5;
                OctiAppIconRenderer.drawCircleStroke(pixels, w, h,
                        cx, cy, sqRect[2] * 0.34, 1.1, pal.gridSocketShadow);

                if (board != null && board.isValidCoord(x - 1, y - 1)) {
                    OctiSquare sq = board.squareAt(x - 1, y - 1);
                    OctiSquareType st = sq.getType();
                    if (st == OctiSquareType.A_HOME_SQUARE
                            || st == OctiSquareType.B_HOME_SQUARE
                            || st == OctiSquareType.C_HOME_SQUARE
                            || st == OctiSquareType.D_HOME_SQUARE) {
                        paintHomeSquareBadge(pixels, w, h, sqRect, sq, board, g, pal);
                    } else if (st == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                        paintNeutralOctiMedallion(pixels, w, h, sqRect, pal);
                    }
                }

                // Crisp vector coordinates inside perimeter square corners
                if (sqRect[1] + sqRect[3] >= grid[1] + grid[3] - 1.5) {
                    drawVectorString(pixels, w, h, sqRect[0] + 3.0, sqRect[1] + sqRect[3] - 11.0, 8.5, Integer.toString(x), pal.coordLabel, 1.3);
                }
                if (sqRect[0] <= grid[0] + 1.5) {
                    drawVectorString(pixels, w, h, sqRect[0] + 3.0, sqRect[1] + 3.0, 8.5, Integer.toString(y), pal.coordLabel, 1.3);
                }
                if (sqRect[1] <= grid[1] + 1.5) {
                    drawVectorString(pixels, w, h, sqRect[0] + sqRect[2] - 9.0, sqRect[1] + 3.0, 8.5, Integer.toString(x), pal.coordLabel, 1.3);
                }
                if (sqRect[0] + sqRect[2] >= grid[0] + grid[2] - 1.5) {
                    drawVectorString(pixels, w, h, sqRect[0] + sqRect[2] - 9.0, sqRect[1] + sqRect[3] - 11.0, 8.5, Integer.toString(y), pal.coordLabel, 1.3);
                }
            }
        }

        // 6. Legal destination guides
        if (composer != null) {
            List<int[]> guides = null;
            if (composer.isGuidedMoveActive()) {
                guides = composer.guidedNextSquares();
            } else if (composer.activeDragKind() == OctiMoveComposer.DragKind.ADD_POD_FROM_RESERVE) {
                guides = composer.legalPodDropSquares(false);
            } else if (composer.activeDragKind() == OctiMoveComposer.DragKind.LIBERATE_POD_FROM_RESERVE) {
                guides = composer.legalPodDropSquares(true);
            }
            if (guides != null) {
                for (int[] dest : guides) {
                    double[] r = g.squareRect(dest[0], dest[1]);
                    double cx = r[0] + r[2] * 0.5;
                    double cy = r[1] + r[3] * 0.5;
                    double rad = r[2] * 0.16;
                    OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                            cx, cy, rad, (pal.legalStepGuide & 0x00FFFFFF) | 0x60000000);
                    OctiAppIconRenderer.drawCircleStroke(pixels, w, h,
                            cx, cy, rad, 2.0, pal.legalStepGuide);
                }
            }
        }

        // 6b. Highlighted dashed indicator border around origin square if remaining movable pods exist
        if (composer != null && composer.isAwaitingConfirmation() && composer.hasOtherMovablePodAtOrigin()) {
            int[] orig = composer.guidedOriginSquare();
            if (orig != null && orig.length >= 2 && orig[0] >= 1 && orig[1] >= 1) {
                double[] sqRect = g.squareRect(orig[0], orig[1]);
                if (sqRect[2] > 0.0) {
                    drawDashedRectOutline(pixels, w, h, sqRect[0] + 1.0, sqRect[1] + 1.0,
                            sqRect[2] - 2.0, sqRect[3] - 2.0, 2.0, pal.accentBrass, 6.0, 4.0);
                }
            }
        }

        // 7. Pods, prongs, superprongs, and layered 3D-offset multi-pod stacks
        if (board != null) {
            for (int y = 1; y <= gh; y++) {
                for (int x = 1; x <= gw; x++) {
                    if (!board.isValidCoord(x - 1, y - 1)) {
                        continue;
                    }
                    OctiSquare sq = board.squareAt(x - 1, y - 1);
                    int numPods = sq.getNumPods();
                    if (numPods <= 0) {
                        continue;
                    }

                    double[] sqRect = g.squareRect(x, y);
                    int[] sorted = sortedPodIndicesMostProngsFirst(sq);
                    int maxLayers = Math.min(numPods, 4);

                    for (int layer = 0; layer < maxLayers; layer++) {
                        int podIdx = sorted[Math.min(sorted.length - 1, layer)];
                        OctiPod pod = sq.getPodAt(podIdx);
                        double offset = (maxLayers - 1 - layer) * 2.5;
                        double[] layerRect = {
                            sqRect[0] - offset,
                            sqRect[1] + offset,
                            sqRect[2],
                            sqRect[3]
                        };
                        boolean hi = (x == hoverSquareX && y == hoverSquareY && layer == maxLayers - 1);
                        String centerLabel = (numPods >= 2 && layer == maxLayers - 1) ? ("x" + numPods) : null;
                        paintOctagonalPod(pixels, w, h, layerRect, pod.getType(), pod, g, pal, hi, centerLabel);
                    }
                }
            }
        }

        // 9. Clockwise orange line segment around squares modified during the last committed move
        if (config != null
                && config.lastMoveModifiedSquares != null
                && !config.lastMoveModifiedSquares.isEmpty()
                && (config.stagedPaths == null || config.stagedPaths.isEmpty())
                && (composer == null || !composer.isAwaitingConfirmation())) {
            for (int[] sq : config.lastMoveModifiedSquares) {
                if (sq == null || sq.length < 2) {
                    continue;
                }
                int sx = sq[0];
                int sy = sq[1];
                if (sx < 1 || sx > gw || sy < 1 || sy > gh) {
                    continue;
                }
                double[] sqRect = g.squareRect(sx, sy);
                paintClockwiseOrangeSquareSegment(pixels, w, h, sqRect, config.lastMoveAnimPhase);
            }
        }

        // 10. Floating Capture [x] / Keep [ ] pills
        if (composer != null && !composer.captureChoices().isEmpty()) {
            for (OctiMoveComposer.CaptureChoice cc : composer.captureChoices()) {
                double[] sqRect = g.squareRect(cc.jumpedX, cc.jumpedY);
                if (sqRect[2] <= 0.0) {
                    continue;
                }
                double px = sqRect[0] + 2.0;
                double py = sqRect[1] + 2.0;
                double pw = sqRect[2] - 4.0;
                double ph = Math.max(12.0, sqRect[3] * 0.26);
                int bg = cc.capture ? pal.legalCaptureGlow : pal.panelCardBg;
                fillRectSolid(pixels, w, h, px, py, pw, ph, bg);
                drawRectOutline(pixels, w, h, px, py, pw, ph, 1.2, pal.prongSuperCore);
            }
        }

        return pixels;
    }

    public static void writeBoardPng(OctiBoard board,
                                     OctiMoveComposer composer,
                                     OctiBoardGeometry geo,
                                     OctiThemeId theme,
                                     File outFile) throws IOException {
        OctiBoardGeometry g = (geo != null)
                ? geo
                : new OctiBoardGeometry(
                        960, 960,
                        board != null ? board.getBoardWidth() : 9,
                        board != null ? board.getBoardHeight() : 9,
                        OctiBoardGeometry.BoardRotation.ROT_0);
        int[] pixels = renderBoardPixels(board, composer, g, theme);
        OctiAppIconRenderer.writeArgbPng(g.viewportWidth(), g.viewportHeight(), pixels, outFile);
    }

    public static int[] sortedPodIndicesMostProngsFirst(OctiSquare sq) {
        if (sq == null || sq.getNumPods() <= 0) {
            return new int[0];
        }
        int n = sq.getNumPods();
        Integer[] indices = new Integer[n];
        for (int i = 0; i < n; i++) {
            indices[i] = i;
        }
        Arrays.sort(indices, (a, b) -> {
            OctiPod podA = sq.getPodAt(a);
            OctiPod podB = sq.getPodAt(b);
            int prongsA = (podA != null) ? podA.getNumProngs() : 0;
            int prongsB = (podB != null) ? podB.getNumProngs() : 0;
            if (prongsA != prongsB) {
                return Integer.compare(prongsB, prongsA);
            }
            int superA = (podA != null) ? podA.getNumSuperProngs() : 0;
            int superB = (podB != null) ? podB.getNumSuperProngs() : 0;
            if (superA != superB) {
                return Integer.compare(superB, superA);
            }
            return Integer.compare(b, a);
        });
        int[] result = new int[n];
        for (int i = 0; i < n; i++) {
            result[i] = indices[i];
        }
        return result;
    }

    public static void drawDashedLineSegment(int[] pixels, int w, int h,
                                             double x1, double y1, double x2, double y2,
                                             double thickness, int color,
                                             double dashLen, double gapLen) {
        double dx = x2 - x1;
        double dy = y2 - y1;
        double dist = Math.hypot(dx, dy);
        if (dist <= 0.001) {
            return;
        }
        double ux = dx / dist;
        double uy = dy / dist;
        double step = dashLen + gapLen;
        for (double t = 0.0; t < dist; t += step) {
            double segEnd = Math.min(dist, t + dashLen);
            OctiAppIconRenderer.drawLineSegment(pixels, w, h,
                    x1 + ux * t, y1 + uy * t,
                    x1 + ux * segEnd, y1 + uy * segEnd,
                    thickness, color);
        }
    }

    public static void drawDashedRectOutline(int[] pixels, int w, int h,
                                             double rx, double ry, double rw, double rh,
                                             double thickness, int color,
                                             double dashLen, double gapLen) {
        drawDashedLineSegment(pixels, w, h, rx, ry, rx + rw, ry, thickness, color, dashLen, gapLen);
        drawDashedLineSegment(pixels, w, h, rx + rw, ry, rx + rw, ry + rh, thickness, color, dashLen, gapLen);
        drawDashedLineSegment(pixels, w, h, rx + rw, ry + rh, rx, ry + rh, thickness, color, dashLen, gapLen);
        drawDashedLineSegment(pixels, w, h, rx, ry + rh, rx, ry, thickness, color, dashLen, gapLen);
    }

    private static void paintHomeSquareBadge(int[] pixels, int w, int h,
                                             double[] sqRect, OctiSquare sq,
                                             OctiBoard board, OctiBoardGeometry g,
                                             OctiPalette pal) {
        OctiPlayer owner = OctiPlayer.PLAYER_A;
        switch (sq.getType()) {
            case A_HOME_SQUARE:
                owner = OctiPlayer.PLAYER_A;
                break;
            case B_HOME_SQUARE:
                owner = OctiPlayer.PLAYER_B;
                break;
            case C_HOME_SQUARE:
                owner = OctiPlayer.PLAYER_C;
                break;
            case D_HOME_SQUARE:
                owner = OctiPlayer.PLAYER_D;
                break;
            default:
                break;
        }

        int darkBg = (pal.themeId == OctiThemeId.ROSEWOOD_IVORY) ? 0xFF140E0B : 0xFF0A0E16;
        int innerBg = 0xFF070A10;
        fillRectSolid(pixels, w, h, sqRect[0] + 1.0, sqRect[1] + 1.0, sqRect[2] - 2.0, sqRect[3] - 2.0, darkBg);

        double ix = sqRect[0] + 3.5;
        double iy = sqRect[1] + 3.5;
        double iw = Math.max(2.0, sqRect[2] - 7.0);
        double ih = Math.max(2.0, sqRect[3] - 7.0);
        fillRectSolid(pixels, w, h, ix, iy, iw, ih, innerBg);

        int ringCol = sq.isCaptured() ? pal.legalCaptureGlow : pal.boardBevelDark;
        double ringThick = sq.isCaptured() ? 2.4 : 1.2;
        drawRectOutline(pixels, w, h, ix, iy, iw, ih, ringThick, ringCol);

        OctiGameMode mode = (board != null && board.getGame() != null)
                ? board.getGame().getMode()
                : OctiGameMode.GAME_MODE_2;
        int seatRotDeg = OctiBoardGeometry.rotationForSeat(mode, owner).degrees();
        int rotDeg = (g != null) ? g.rotationDegrees() : 0;
        int textAngleCwDeg = ((seatRotDeg - rotDeg) % 360 + 360) % 360;

        if (sq.isEmpty()) {
            double textH = Math.max(10.0, sqRect[3] * 0.22);
            double cx = sqRect[0] + sqRect[2] * 0.5;
            double cy = sqRect[1] + sqRect[3] * 0.5;
            drawVectorStringRotated(pixels, w, h, cx, cy, textH, "OCTI", pal.coordLabel, 1.5, textAngleCwDeg);
        }
    }

    private static void paintHomeSquareBadge(int[] pixels, int w, int h,
                                             double[] sqRect, OctiSquare sq,
                                             OctiBoard board, OctiPalette pal) {
        paintHomeSquareBadge(pixels, w, h, sqRect, sq, board, null, pal);
    }

    private static void paintNeutralOctiMedallion(int[] pixels, int w, int h,
                                                  double[] sqRect, OctiPalette pal) {
        double ix = sqRect[0] + 4.0;
        double iy = sqRect[1] + 4.0;
        double iw = Math.max(2.0, sqRect[2] - 8.0);
        double ih = Math.max(2.0, sqRect[3] - 8.0);
        fillRectAlpha(pixels, w, h, ix, iy, iw, ih, pal.neutralOctiFill, 0.65);
        drawRectOutline(pixels, w, h, ix, iy, iw, ih, 1.8, pal.neutralOctiRim);
    }

    private static void paintOctagonalPod(int[] pixels, int w, int h,
                                          double[] rect,
                                          OctiPodType type,
                                          OctiPod pod,
                                          OctiBoardGeometry geo,
                                          OctiPalette pal,
                                          boolean highlighted,
                                          String centerLabelOverride) {
        OctiPalette.PodFactionPalette fp = pal.factionForPodType(type);
        double cx = rect[0] + rect[2] * 0.5;
        double cy = rect[1] + rect[3] * 0.5;
        double radius = Math.min(rect[2], rect[3]) * 0.40;

        double prongInnerR = radius * 0.60;
        double prongOuterR = radius * 1.46;
        double superOuterR = radius * 1.56;

        // 1. Prongs
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            OctiProngType pt = pod.prongAt(dir);
            if (pt == OctiProngType.NO_PRONG) {
                continue;
            }
            double[] u = geo.dirUnitVectorScreen(dir);
            double px = -u[1];
            double py = u[0];

            if (pt == OctiProngType.NORMAL_PRONG) {
                double halfW = Math.max(2.8, radius * 0.20);
                double[] vx = {
                    cx + u[0] * prongInnerR - px * halfW,
                    cx + u[0] * (prongOuterR * 0.86) - px * (halfW * 0.85),
                    cx + u[0] * prongOuterR,
                    cx + u[0] * (prongOuterR * 0.86) + px * (halfW * 0.85),
                    cx + u[0] * prongInnerR + px * halfW
                };
                double[] vy = {
                    cy + u[1] * prongInnerR - py * halfW,
                    cy + u[1] * (prongOuterR * 0.86) - py * (halfW * 0.85),
                    cy + u[1] * prongOuterR,
                    cy + u[1] * (prongOuterR * 0.86) + py * (halfW * 0.85),
                    cy + u[1] * prongInnerR + py * halfW
                };
                OctiAppIconRenderer.fillPolygonShaded(pixels, w, h, vx, vy,
                        0xFFFFFFFF, pal.prongNormalTip, pal.prongNormalMetal);
                OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, vx, vy, 1.4, 0xFF0A0F18);
                OctiAppIconRenderer.drawLineSegment(pixels, w, h,
                        cx + u[0] * (prongInnerR * 1.05), cy + u[1] * (prongInnerR * 1.05),
                        cx + u[0] * (prongOuterR * 0.88), cy + u[1] * (prongOuterR * 0.88),
                        1.2, 0xCCFFFFFF);
            } else if (pt == OctiProngType.SUPER_PRONG) {
                double halfW = Math.max(3.6, radius * 0.26);
                double[] vx = {
                    cx + u[0] * prongInnerR - px * (halfW * 0.75),
                    cx + u[0] * (superOuterR * 0.72) - px * (halfW * 1.25),
                    cx + u[0] * superOuterR,
                    cx + u[0] * (superOuterR * 0.72) + px * (halfW * 1.25),
                    cx + u[0] * prongInnerR + px * (halfW * 0.75)
                };
                double[] vy = {
                    cy + u[1] * prongInnerR - py * (halfW * 0.75),
                    cy + u[1] * (superOuterR * 0.72) - py * (halfW * 1.25),
                    cy + u[1] * superOuterR,
                    cy + u[1] * (superOuterR * 0.72) + py * (halfW * 1.25),
                    cy + u[1] * prongInnerR + py * (halfW * 0.75)
                };
                OctiAppIconRenderer.fillPolygonShaded(pixels, w, h, vx, vy,
                        0xFFFEF08A, pal.prongSuperGold, 0xFFD97706);
                OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, vx, vy, 1.6, pal.prongSuperCore);
                OctiAppIconRenderer.drawLineSegment(pixels, w, h,
                        cx + u[0] * (prongInnerR * 1.05), cy + u[1] * (prongInnerR * 1.05),
                        cx + u[0] * (superOuterR * 0.90), cy + u[1] * (superOuterR * 0.90),
                        1.4, 0xFFFEF08A);
            }
        }

        // 2. Drop shadow
        OctiAppIconRenderer.fillOctagonFlat(pixels, w, h,
                cx + radius * 0.07, cy + radius * 0.10, radius * 1.02, 0x7D000000);

        // 3. Outer bevel octagon
        OctiAppIconRenderer.fillOctagonGradient(pixels, w, h,
                cx, cy, radius, fp.highlight, fp.rim, fp.shadow);
        OctiAppIconRenderer.drawOctagonStroke(pixels, w, h,
                cx, cy, radius, highlighted ? 2.2 : 1.2,
                highlighted ? pal.prongSuperGold : pal.accentBrass);

        // 4. Inner gem dome octagon
        OctiAppIconRenderer.fillOctagonGradient(pixels, w, h,
                cx, cy, radius * 0.78, fp.highlight, fp.core, fp.shadow);
        OctiAppIconRenderer.drawOctagonStroke(pixels, w, h,
                cx, cy, radius * 0.78, 0.9, fp.highlight);

        // 5. Recessed 8-direction prong sockets
        double socketDist = radius * 0.86;
        double socketR = Math.max(1.2, radius * 0.085);
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            double[] u = geo.dirUnitVectorScreen(dir);
            double sx = cx + u[0] * socketDist;
            double sy = cy + u[1] * socketDist;
            OctiProngType pt = pod.prongAt(dir);
            int sCol = (pt == OctiProngType.NO_PRONG) ? 0xB40C1018
                    : (pt == OctiProngType.SUPER_PRONG) ? pal.prongSuperGold : pal.prongNormalMetal;
            OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, sx, sy, socketR, sCol);
        }

        // 6. Central emblem or count label
        if (centerLabelOverride != null && !centerLabelOverride.isEmpty()) {
            double textH = Math.max(9.0, radius * 0.50);
            drawVectorStringRotated(pixels, w, h, cx, cy, textH, centerLabelOverride, pal.prongSuperGold, 1.6, 0);
        } else {
            String factionLetter = (type != null) ? type.toPlayer().name().substring(7) : "A";
            double textH = Math.max(8.0, radius * 0.44);
            drawVectorStringRotated(pixels, w, h, cx, cy, textH, factionLetter, fp.emblem, 1.5, 0);
        }
    }

    private static void paintOctagonalPod(int[] pixels, int w, int h,
                                          double[] rect,
                                          OctiPodType type,
                                          OctiPod pod,
                                          OctiBoardGeometry geo,
                                          OctiPalette pal,
                                          boolean highlighted) {
        paintOctagonalPod(pixels, w, h, rect, type, pod, geo, pal, highlighted, null);
    }

    private static void paintCompassRose(int[] pixels, int w, int h,
                                         double cx, double cy, double r,
                                         OctiBoardGeometry geo, OctiPalette pal) {
        OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, cx, cy, r, pal.panelCardBg);
        OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy, r, 1.4, pal.accentBrass);

        double[] northU = geo.dirUnitVectorScreen(OctiDir.DIR_N);
        double px = -northU[1];
        double py = northU[0];

        double[] nx = {
            cx + northU[0] * (r * 0.82),
            cx + px * (r * 0.22),
            cx - px * (r * 0.22)
        };
        double[] ny = {
            cy + northU[1] * (r * 0.82),
            cy + py * (r * 0.22),
            cy - py * (r * 0.22)
        };
        OctiAppIconRenderer.fillPolygonShaded(pixels, w, h, nx, ny,
                pal.prongSuperCore, pal.legalCaptureGlow, pal.legalCaptureGlow);
    }

    private static void paintDigitPip(int[] pixels, int w, int h,
                                      double cx, double cy, double r,
                                      int val, int color) {
        OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, cx, cy, Math.max(1.5, r), color);
        if (val % 2 == 0) {
            OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy,
                    Math.max(2.2, r * 1.4), 0.9, color);
        }
    }

    private static void fillRectSolid(int[] pixels, int w, int h,
                                      double left, double top, double rw, double rh, int color) {
        int x0 = Math.max(0, (int) Math.floor(left));
        int x1 = Math.min(w - 1, (int) Math.ceil(left + rw) - 1);
        int y0 = Math.max(0, (int) Math.floor(top));
        int y1 = Math.min(h - 1, (int) Math.ceil(top + rh) - 1);
        int solid = color | 0xFF000000;
        for (int y = y0; y <= y1; y++) {
            int row = y * w;
            for (int x = x0; x <= x1; x++) {
                pixels[row + x] = solid;
            }
        }
    }

    private static void fillRectAlpha(int[] pixels, int w, int h,
                                      double left, double top, double rw, double rh,
                                      int color, double alpha) {
        int x0 = Math.max(0, (int) Math.floor(left));
        int x1 = Math.min(w - 1, (int) Math.ceil(left + rw) - 1);
        int y0 = Math.max(0, (int) Math.floor(top));
        int y1 = Math.min(h - 1, (int) Math.ceil(top + rh) - 1);
        for (int y = y0; y <= y1; y++) {
            int row = y * w;
            for (int x = x0; x <= x1; x++) {
                pixels[row + x] = OctiAppIconRenderer.blendColor(pixels[row + x], color, alpha);
            }
        }
    }

    private static void fillRectGradient(int[] pixels, int w, int h,
                                         double left, double top, double rw, double rh,
                                         int cTop, int cMid, int cBot) {
        int x0 = Math.max(0, (int) Math.floor(left));
        int x1 = Math.min(w - 1, (int) Math.ceil(left + rw) - 1);
        int y0 = Math.max(0, (int) Math.floor(top));
        int y1 = Math.min(h - 1, (int) Math.ceil(top + rh) - 1);
        double span = Math.max(1.0, rw + rh);
        for (int y = y0; y <= y1; y++) {
            int row = y * w;
            for (int x = x0; x <= x1; x++) {
                double t = Math.max(0.0, Math.min(1.0, ((x - left) + (y - top)) / span));
                int col = (t < 0.5)
                        ? OctiAppIconRenderer.lerpColor(cTop, cMid, t * 2.0)
                        : OctiAppIconRenderer.lerpColor(cMid, cBot, (t - 0.5) * 2.0);
                pixels[row + x] = col;
            }
        }
    }

    private static void drawRectOutline(int[] pixels, int w, int h,
                                        double left, double top, double rw, double rh,
                                        double thickness, int color) {
        double right = left + rw;
        double bottom = top + rh;
        OctiAppIconRenderer.drawLineSegment(pixels, w, h, left, top, right, top, thickness, color);
        OctiAppIconRenderer.drawLineSegment(pixels, w, h, right, top, right, bottom, thickness, color);
        OctiAppIconRenderer.drawLineSegment(pixels, w, h, right, bottom, left, bottom, thickness, color);
        OctiAppIconRenderer.drawLineSegment(pixels, w, h, left, bottom, left, top, thickness, color);
    }

    @FunctionalInterface
    public interface VectorStrokeConsumer {
        void drawLine(double x1, double y1, double x2, double y2);
        default void drawDot(double cx, double cy, double r) {
            drawLine(cx - r * 0.5, cy, cx + r * 0.5, cy);
        }
    }

    public static void emitVectorGlyphStrokes(VectorStrokeConsumer consumer,
                                              double x, double y, double height,
                                              char ch) {
        double gw = height * 0.60;
        char c = Character.toUpperCase(ch);
        switch (c) {
            case '0':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x + gw * 0.8, y + height * 0.2, x + gw * 0.2, y + height * 0.8);
                break;
            case '1':
                consumer.drawLine(x + gw * 0.2, y + height * 0.25, x + gw * 0.55, y);
                consumer.drawLine(x + gw * 0.55, y, x + gw * 0.55, y + height);
                consumer.drawLine(x + gw * 0.1, y + height, x + gw, y + height);
                break;
            case '2':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x, y + height * 0.5);
                consumer.drawLine(x, y + height * 0.5, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                break;
            case '3':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                consumer.drawLine(x + gw * 0.3, y + height * 0.5, x + gw, y + height * 0.5);
                break;
            case '4':
                consumer.drawLine(x, y, x, y + height * 0.5);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw * 0.75, y, x + gw * 0.75, y + height);
                break;
            case '5':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height * 0.5);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                break;
            case '6':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x, y + height * 0.5);
                break;
            case '7':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw * 0.4, y + height);
                break;
            case '8':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                break;
            case '9':
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x, y, x, y + height * 0.5);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                break;
            case 'X':
            case '×':
                consumer.drawLine(x + gw * 0.1, y + height * 0.1, x + gw * 0.9, y + height * 0.9);
                consumer.drawLine(x + gw * 0.9, y + height * 0.1, x + gw * 0.1, y + height * 0.9);
                break;
            case 'A':
                consumer.drawLine(x, y + height, x + gw * 0.5, y);
                consumer.drawLine(x + gw * 0.5, y, x + gw, y + height);
                consumer.drawLine(x + gw * 0.25, y + height * 0.55, x + gw * 0.75, y + height * 0.55);
                break;
            case 'B':
                consumer.drawLine(x, y, x + gw * 0.8, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height * 0.5, x + gw * 0.8, y + height * 0.5);
                consumer.drawLine(x, y + height, x + gw * 0.8, y + height);
                consumer.drawLine(x + gw * 0.8, y, x + gw, y + height * 0.25);
                consumer.drawLine(x + gw, y + height * 0.25, x + gw * 0.8, y + height * 0.5);
                consumer.drawLine(x + gw * 0.8, y + height * 0.5, x + gw, y + height * 0.75);
                consumer.drawLine(x + gw, y + height * 0.75, x + gw * 0.8, y + height);
                break;
            case 'C':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                break;
            case 'D':
                consumer.drawLine(x, y, x + gw * 0.7, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw * 0.7, y + height);
                consumer.drawLine(x + gw * 0.7, y, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x + gw * 0.7, y + height);
                break;
            case 'E':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height * 0.5, x + gw * 0.8, y + height * 0.5);
                consumer.drawLine(x, y + height, x + gw, y + height);
                break;
            case 'F':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height * 0.5, x + gw * 0.8, y + height * 0.5);
                break;
            case 'G':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x + gw * 0.5, y + height * 0.5);
                break;
            case 'H':
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                break;
            case 'M':
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x, y, x + gw * 0.5, y + height * 0.6);
                consumer.drawLine(x + gw * 0.5, y + height * 0.6, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                break;
            case 'O':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                consumer.drawLine(x, y + height, x, y);
                break;
            case 'I':
                consumer.drawLine(x + gw * 0.15, y, x + gw * 0.85, y);
                consumer.drawLine(x + gw * 0.5, y, x + gw * 0.5, y + height);
                consumer.drawLine(x + gw * 0.15, y + height, x + gw * 0.85, y + height);
                break;
            case 'K':
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height * 0.5, x + gw, y);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height);
                break;
            case 'L':
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                break;
            case 'N':
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x, y, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x + gw, y);
                break;
            case 'P':
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x, y + height * 0.5);
                break;
            case 'R':
                consumer.drawLine(x, y + height, x, y);
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw, y, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x, y + height * 0.5);
                consumer.drawLine(x + gw * 0.4, y + height * 0.5, x + gw, y + height);
                break;
            case 'S':
                consumer.drawLine(x + gw, y, x, y);
                consumer.drawLine(x, y, x, y + height * 0.5);
                consumer.drawLine(x, y + height * 0.5, x + gw, y + height * 0.5);
                consumer.drawLine(x + gw, y + height * 0.5, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x, y + height);
                break;
            case 'T':
                consumer.drawLine(x, y, x + gw, y);
                consumer.drawLine(x + gw * 0.5, y, x + gw * 0.5, y + height);
                break;
            case 'U':
                consumer.drawLine(x, y, x, y + height);
                consumer.drawLine(x, y + height, x + gw, y + height);
                consumer.drawLine(x + gw, y + height, x + gw, y);
                break;
            case 'V':
                consumer.drawLine(x, y, x + gw * 0.5, y + height);
                consumer.drawLine(x + gw * 0.5, y + height, x + gw, y);
                break;
            case 'W':
                consumer.drawLine(x, y, x + gw * 0.25, y + height);
                consumer.drawLine(x + gw * 0.25, y + height, x + gw * 0.5, y + height * 0.45);
                consumer.drawLine(x + gw * 0.5, y + height * 0.45, x + gw * 0.75, y + height);
                consumer.drawLine(x + gw * 0.75, y + height, x + gw, y);
                break;
            case 'Y':
                consumer.drawLine(x, y, x + gw * 0.5, y + height * 0.5);
                consumer.drawLine(x + gw, y, x + gw * 0.5, y + height * 0.5);
                consumer.drawLine(x + gw * 0.5, y + height * 0.5, x + gw * 0.5, y + height);
                break;
            case '-':
                consumer.drawLine(x + gw * 0.1, y + height * 0.5, x + gw * 0.9, y + height * 0.5);
                break;
            case '.':
                consumer.drawDot(x + gw * 0.5, y + height * 0.85, 1.2);
                break;
            case '•':
                consumer.drawDot(x + gw * 0.5, y + height * 0.5, 1.2);
                break;
            case ':':
                consumer.drawDot(x + gw * 0.5, y + height * 0.3, 1.2);
                consumer.drawDot(x + gw * 0.5, y + height * 0.75, 1.2);
                break;
            case '!':
                consumer.drawLine(x + gw * 0.5, y, x + gw * 0.5, y + height * 0.7);
                consumer.drawDot(x + gw * 0.5, y + height * 0.92, 1.2);
                break;
            case ',':
                consumer.drawLine(x + gw * 0.5, y + height * 0.75, x + gw * 0.3, y + height * 1.05);
                break;
            default:
                break;
        }
    }


    public static void drawVectorGlyph(int[] pixels, int w, int h,
                                       double x, double y, double height,
                                       char ch, int color, double thickness) {
        emitVectorGlyphStrokes(new VectorStrokeConsumer() {
            @Override
            public void drawLine(double x1, double y1, double x2, double y2) {
                OctiAppIconRenderer.drawLineSegment(pixels, w, h, x1, y1, x2, y2, thickness, color);
            }
            @Override
            public void drawDot(double cx, double cy, double r) {
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, cx, cy, r, color);
            }
        }, x, y, height, ch);
    }
    public static void drawVectorString(int[] pixels, int w, int h,
                                        double x, double y, double height,
                                        String str, int color, double thickness) {
        if (str == null) return;
        double curX = x;
        double gw = height * 0.60;
        double spacing = height * 0.20;
        for (int i = 0; i < str.length(); i++) {
            char ch = str.charAt(i);
            if (ch == ' ') {
                curX += gw * 0.7;
            } else {
                drawVectorGlyph(pixels, w, h, curX, y, height, ch, color, thickness);
                curX += gw + spacing;
            }
        }
    }

    public static void drawVectorStringRotated(int[] pixels, int w, int h,
                                               double cx, double cy, double height,
                                               String str, int color, double thickness,
                                               int angleCwDeg) {
        if (str == null || str.isEmpty()) {
            return;
        }
        double gw = height * 0.60;
        double spacing = height * 0.20;
        double totalW = 0.0;
        for (int i = 0; i < str.length(); i++) {
            char ch = str.charAt(i);
            if (ch == ' ') {
                totalW += gw * 0.7;
            } else {
                totalW += (i == 0 ? gw : (gw + spacing));
            }
        }
        double startX = cx - totalW * 0.5;
        double startY = cy - height * 0.5;

        int normAngle = ((angleCwDeg % 360) + 360) % 360;
        if (normAngle == 0) {
            drawVectorString(pixels, w, h, startX, startY, height, str, color, thickness);
            return;
        }

        double rad = Math.toRadians(normAngle);
        double cos = Math.cos(rad);
        double sin = Math.sin(rad);

        VectorStrokeConsumer consumer = new VectorStrokeConsumer() {
            @Override
            public void drawLine(double x1, double y1, double x2, double y2) {
                double dx1 = x1 - cx;
                double dy1 = y1 - cy;
                double rx1 = cx + dx1 * cos - dy1 * sin;
                double ry1 = cy + dx1 * sin + dy1 * cos;

                double dx2 = x2 - cx;
                double dy2 = y2 - cy;
                double rx2 = cx + dx2 * cos - dy2 * sin;
                double ry2 = cy + dx2 * sin + dy2 * cos;

                OctiAppIconRenderer.drawLineSegment(pixels, w, h, rx1, ry1, rx2, ry2, thickness, color);
            }

            @Override
            public void drawDot(double dotCx, double dotCy, double r) {
                double dx = dotCx - cx;
                double dy = dotCy - cy;
                double rx = cx + dx * cos - dy * sin;
                double ry = cy + dx * sin + dy * cos;
                OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, rx, ry, r, color);
            }
        };

        double curX = startX;
        for (int i = 0; i < str.length(); i++) {
            char ch = str.charAt(i);
            if (ch == ' ') {
                curX += gw * 0.7;
            } else {
                emitVectorGlyphStrokes(consumer, curX, startY, height, ch);
                curX += gw + spacing;
            }
        }
    }

    private static void paintZoomedPodBodyAndProngs(int[] targetPixels,
                                                    int s,
                                                    double podCx,
                                                    double podCy,
                                                    double podR,
                                                    OctiPod pod,
                                                    OctiBoardGeometry geo,
                                                    OctiPalette pal,
                                                    boolean drawHighlightRim) {
        OctiPalette.PodFactionPalette fp = pal.factionForPodType(pod.getType());
        double prongInnerR = podR * 0.60;
        double prongOuterR = podR * 1.48;
        double superOuterR = podR * 1.55;

        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            OctiProngType pt = pod.prongAt(dir);
            if (pt == OctiProngType.NO_PRONG) {
                continue;
            }
            double[] u = geo.dirUnitVectorScreen(dir);
            double u0 = u[0];
            double u1 = u[1];
            double px = -u1;
            double py = u0;

            if (pt == OctiProngType.NORMAL_PRONG) {
                double halfW = Math.max(8.0, podR * 0.145);
                double[] vx = {
                    podCx + u0 * prongInnerR - px * halfW,
                    podCx + u0 * (prongOuterR * 0.86) - px * (halfW * 0.85),
                    podCx + u0 * prongOuterR,
                    podCx + u0 * (prongOuterR * 0.86) + px * (halfW * 0.85),
                    podCx + u0 * prongInnerR + px * halfW
                };
                double[] vy = {
                    podCy + u1 * prongInnerR - py * halfW,
                    podCy + u1 * (prongOuterR * 0.86) - py * (halfW * 0.85),
                    podCy + u1 * prongOuterR,
                    podCy + u1 * (prongOuterR * 0.86) + py * (halfW * 0.85),
                    podCy + u1 * prongInnerR + py * halfW
                };
                OctiAppIconRenderer.fillPolygonShaded(targetPixels, s, s, vx, vy,
                        0xFFFFFFFF, pal.prongNormalTip, pal.prongNormalMetal);
                OctiAppIconRenderer.drawPolygonOutline(targetPixels, s, s, vx, vy, 1.4, 0xFF05070B);
                OctiAppIconRenderer.drawLineSegment(targetPixels, s, s,
                        podCx + u0 * (prongInnerR * 1.05), podCy + u1 * (prongInnerR * 1.05),
                        podCx + u0 * (prongOuterR * 0.88), podCy + u1 * (prongOuterR * 0.88),
                        1.4, 0xCCFFFFFF);
            } else if (pt == OctiProngType.SUPER_PRONG) {
                double halfW = Math.max(10.0, podR * 0.185);
                double[] vx = {
                    podCx + u0 * prongInnerR - px * (halfW * 0.75),
                    podCx + u0 * (superOuterR * 0.72) - px * (halfW * 1.25),
                    podCx + u0 * superOuterR,
                    podCx + u0 * (superOuterR * 0.72) + px * (halfW * 1.25),
                    podCx + u0 * prongInnerR + px * (halfW * 0.75)
                };
                double[] vy = {
                    podCy + u1 * prongInnerR - py * (halfW * 0.75),
                    podCy + u1 * (superOuterR * 0.72) - py * (halfW * 1.25),
                    podCy + u1 * superOuterR,
                    podCy + u1 * (superOuterR * 0.72) + py * (halfW * 1.25),
                    podCy + u1 * prongInnerR + py * (halfW * 0.75)
                };
                OctiAppIconRenderer.fillPolygonShaded(targetPixels, s, s, vx, vy,
                        0xFFFEF08A, pal.prongSuperGold, 0xFFD97706);
                OctiAppIconRenderer.drawPolygonOutline(targetPixels, s, s, vx, vy, 1.8, pal.prongSuperCore);
                OctiAppIconRenderer.drawLineSegment(targetPixels, s, s,
                        podCx + u0 * (prongInnerR * 1.05), podCy + u1 * (prongInnerR * 1.05),
                        podCx + u0 * (superOuterR * 0.90), podCy + u1 * (superOuterR * 0.90),
                        1.6, 0xFFFEF08A);
            }
        }

        OctiAppIconRenderer.fillOctagonFlat(targetPixels, s, s,
                podCx + podR * 0.05, podCy + podR * 0.08, podR * 1.02, 0x7D000000);
        OctiAppIconRenderer.fillOctagonGradient(targetPixels, s, s,
                podCx, podCy, podR, fp.highlight, fp.rim, fp.shadow);
        OctiAppIconRenderer.drawOctagonStroke(targetPixels, s, s,
                podCx, podCy, podR, drawHighlightRim ? 2.4 : 1.6,
                drawHighlightRim ? pal.prongSuperGold : fp.rim);

        OctiAppIconRenderer.fillOctagonGradient(targetPixels, s, s,
                podCx, podCy, podR * 0.78, fp.highlight, fp.core, fp.shadow);
        OctiAppIconRenderer.drawOctagonStroke(targetPixels, s, s,
                podCx, podCy, podR * 0.78, 1.2, fp.highlight);

        OctiAppIconRenderer.fillCircleAlpha(targetPixels, s, s, podCx, podCy, podR * 0.18, fp.emblem);
        OctiAppIconRenderer.drawCircleStroke(targetPixels, s, s, podCx, podCy, podR * 0.18, 1.4, pal.accentBrass);
    }

    public static int[] renderZoomedPodPixels(OctiBoard board,
                                              int x,
                                              int y,
                                              int podIndex,
                                              int rotationDeg,
                                              OctiThemeId theme,
                                              int sizePx,
                                              int hoverDir) {
        int s = Math.max(128, sizePx);
        int[] pixels = new int[s * s];
        OctiPalette pal = OctiPalette.forTheme(theme != null ? theme : OctiThemeId.OBSIDIAN_BRASS);
        double cx = s * 0.5;
        double cy = s * 0.5;
        double podR = s * 0.30;
        double socketR = s * 0.046;
        double socketDist = podR * 0.76;

        for (int py = 0; py < s; py++) {
            for (int px = 0; px < s; px++) {
                double dist = Math.hypot(px - cx, py - cy) / (s * 0.5);
                if (dist <= 1.0) {
                    int c = OctiAppIconRenderer.lerpColor(0xFF141E2E, 0xEE0B0F17, dist);
                    pixels[py * s + px] = c;
                } else {
                    pixels[py * s + px] = 0x00000000;
                }
            }
        }
        OctiAppIconRenderer.drawCircleStroke(pixels, s, s, cx, cy, s * 0.49, 1.8, pal.accentBrass);

        boolean hasSquare = (board != null && board.isValidCoord(x - 1, y - 1));
        OctiSquare sq = hasSquare ? board.squareAt(x - 1, y - 1) : null;
        boolean hasPod = (sq != null && !sq.isEmpty());

        OctiBoardGeometry geo = new OctiBoardGeometry(s, s, 9, 9,
                OctiBoardGeometry.BoardRotation.fromDegrees(rotationDeg));

        if (hasPod) {
            int numPods = sq.getNumPods();
            int pIdx = Math.max(0, Math.min(numPods - 1, podIndex));

            // Translucent offset underlay of other pods in stack
            if (numPods >= 2) {
                int[] scratch = new int[s * s];
                int[] sorted = sortedPodIndicesMostProngsFirst(sq);
                int activeRank = 0;
                for (int r = 0; r < sorted.length; r++) {
                    if (sorted[r] == pIdx) {
                        activeRank = r;
                        break;
                    }
                }
                for (int r = sorted.length - 1; r >= 0; r--) {
                    int otherIdx = sorted[r];
                    if (otherIdx == pIdx) {
                        continue;
                    }
                    int layerStep = activeRank - r;
                    double offX = -layerStep * (s * 0.035);
                    double offY = layerStep * (s * 0.035);
                    Arrays.fill(scratch, 0);
                    paintZoomedPodBodyAndProngs(scratch, s, cx + offX, cy + offY, podR,
                            sq.getPodAt(otherIdx), geo, pal, false);
                    for (int idx = 0; idx < s * s; idx++) {
                        if ((scratch[idx] & 0xFF000000) != 0) {
                            pixels[idx] = OctiAppIconRenderer.blendColor(pixels[idx], scratch[idx], 0.28);
                        }
                    }
                }
            }

            // Render active pod at full opacity at center
            OctiPod pod = sq.getPodAt(pIdx);
            paintZoomedPodBodyAndProngs(pixels, s, cx, cy, podR, pod, geo, pal, true);

            // Render 8 interactive sockets ('a'..'h')
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                double[] u = geo.dirUnitVectorScreen(dir);
                double u0 = u[0];
                double u1 = u[1];
                double scx = cx + u0 * socketDist;
                double scy = cy + u1 * socketDist;

                boolean isHover = (d == hoverDir);
                OctiProngType pt = pod.prongAt(dir);

                int sFill = isHover ? pal.legalStepGuide
                        : (pt == OctiProngType.NO_PRONG) ? 0xDD0E1420
                        : (pt == OctiProngType.SUPER_PRONG) ? pal.prongSuperGold
                        : pal.prongNormalMetal;
                int sStroke = isHover ? pal.prongSuperCore
                        : (pt == OctiProngType.SUPER_PRONG) ? pal.prongSuperCore
                        : (pt == OctiProngType.NORMAL_PRONG) ? pal.prongNormalTip
                        : pal.accentBrass;

                OctiAppIconRenderer.fillCircleAlpha(pixels, s, s, scx, scy, socketR, sFill);
                OctiAppIconRenderer.drawCircleStroke(pixels, s, s, scx, scy, socketR, isHover ? 2.4 : 1.4, sStroke);

                char letter = (char) ('a' + d);
                int letterColor = isHover ? pal.prongSuperGold : (pt == OctiProngType.NO_PRONG ? pal.accentBrass : 0xFF0B0F17);
                drawVectorGlyph(pixels, s, s, scx - socketR * 0.35, scy - socketR * 0.45, socketR * 0.90, letter, letterColor, 1.4);
            }

            // Draw large, high-contrast zoomed prong blades AFTER socket circles
            for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                OctiDir dir = OctiDir.fromIndex(d);
                OctiProngType pt = pod.prongAt(dir);
                boolean isHoverEmpty = (pt == OctiProngType.NO_PRONG && d == hoverDir);
                if (pt == OctiProngType.NO_PRONG && !isHoverEmpty) {
                    continue;
                }
                double[] u = geo.dirUnitVectorScreen(dir);
                double u0 = u[0];
                double u1 = u[1];
                double px = -u1;
                double py = u0;

                double bladeInnerR = socketDist + socketR * 0.55;
                boolean isSuperProng = (pt == OctiProngType.SUPER_PRONG || (isHoverEmpty && pod.getNumProngs() == 7));

                if (!isSuperProng) {
                    double bladeOuterR = podR * 1.48;
                    double halfW = Math.max(9.0, podR * 0.145);
                    double[] vx = {
                        cx + u0 * bladeInnerR - px * halfW,
                        cx + u0 * (bladeOuterR * 0.86) - px * (halfW * 0.85),
                        cx + u0 * bladeOuterR,
                        cx + u0 * (bladeOuterR * 0.86) + px * (halfW * 0.85),
                        cx + u0 * bladeInnerR + px * halfW
                    };
                    double[] vy = {
                        cy + u1 * bladeInnerR - py * halfW,
                        cy + u1 * (bladeOuterR * 0.86) - py * (halfW * 0.85),
                        cy + u1 * bladeOuterR,
                        cy + u1 * (bladeOuterR * 0.86) + py * (halfW * 0.85),
                        cy + u1 * bladeInnerR + py * halfW
                    };
                    if (isHoverEmpty) {
                        OctiAppIconRenderer.fillPolygonShaded(pixels, s, s, vx, vy,
                                0xD0FFFFFF, pal.prongNormalTip, pal.prongNormalMetal);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 2.2, pal.legalStepGuide);
                    } else {
                        OctiAppIconRenderer.fillPolygonShaded(pixels, s, s, vx, vy,
                                0xFFFFFFFF, 0xFFE2E8F0, 0xFFCBD5E1);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 1.8, 0xFF05070B);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 1.0, pal.accentBrass);
                    }
                    OctiAppIconRenderer.drawLineSegment(pixels, s, s,
                            cx + u0 * bladeInnerR, cy + u1 * bladeInnerR,
                            cx + u0 * (bladeOuterR * 0.90), cy + u1 * (bladeOuterR * 0.90),
                            1.8, 0xFFFFFFFF);
                } else {
                    double bladeOuterR = podR * 1.55;
                    double halfW = Math.max(12.0, podR * 0.185);
                    double[] vx = {
                        cx + u0 * bladeInnerR - px * (halfW * 0.80),
                        cx + u0 * (bladeOuterR * 0.75) - px * (halfW * 1.20),
                        cx + u0 * bladeOuterR,
                        cx + u0 * (bladeOuterR * 0.75) + px * (halfW * 1.20),
                        cx + u0 * bladeInnerR + px * (halfW * 0.80)
                    };
                    double[] vy = {
                        cy + u1 * bladeInnerR - py * (halfW * 0.80),
                        cy + u1 * (bladeOuterR * 0.75) - py * (halfW * 1.20),
                        cy + u1 * bladeOuterR,
                        cy + u1 * (bladeOuterR * 0.75) + py * (halfW * 1.20),
                        cy + u1 * bladeInnerR + py * (halfW * 0.80)
                    };
                    if (isHoverEmpty) {
                        OctiAppIconRenderer.fillPolygonShaded(pixels, s, s, vx, vy,
                                0xD0FEF08A, pal.prongSuperGold, 0xFFD97706);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 2.2, pal.prongSuperGold);
                    } else {
                        OctiAppIconRenderer.fillPolygonShaded(pixels, s, s, vx, vy,
                                0xFFFEF08A, 0xFFFBBF24, 0xFFD97706);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 2.0, pal.prongSuperCore);
                        OctiAppIconRenderer.drawPolygonOutline(pixels, s, s, vx, vy, 1.0, pal.accentBrass);
                    }
                    OctiAppIconRenderer.drawLineSegment(pixels, s, s,
                            cx + u0 * bladeInnerR, cy + u1 * bladeInnerR,
                            cx + u0 * (bladeOuterR * 0.92), cy + u1 * (bladeOuterR * 0.92),
                            2.0, 0xFFFEF08A);
                }
            }
        } else {
            OctiAppIconRenderer.drawOctagonStroke(pixels, s, s, cx, cy, podR * 0.85, 2.0, pal.accentBrass);
            drawVectorString(pixels, s, s, cx - 40.0, cy - 8.0, 16.0, "HOME", pal.accentBrass, 1.8);
        }

        return pixels;
    }

    public static int[] renderSplashPixels(int width, int height, double progressZeroToOne) {
        return renderSplashPixels(width, height, OctiThemeId.OBSIDIAN_BRASS, progressZeroToOne, "dev-local");
    }

    public static int[] renderSplashPixels(int width, int height, OctiThemeId themeId, double progressZeroToOne) {
        return renderSplashPixels(width, height, themeId, progressZeroToOne, "dev-local");
    }

    public static int[] renderSplashPixels(int width, int height, OctiThemeId themeId, double progressZeroToOne, String sha256Prefix) {
        int w = Math.max(128, width);
        int h = Math.max(128, height);
        int[] pixels = new int[w * h];
        double progress = Math.max(0.0, Math.min(1.0, progressZeroToOne));

        OctiPalette pal = OctiPalette.forTheme(themeId != null ? themeId : OctiThemeId.OBSIDIAN_BRASS);
        double cx = w * 0.5;
        double cy = h * 0.40;

        double maxR = Math.hypot(w, h) * 0.65;
        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                double dist = Math.hypot(x - cx, y - cy) / maxR;
                int col;
                if (dist < 0.55) {
                    col = OctiAppIconRenderer.lerpColor(0xFF182234, 0xFF0D121B, dist / 0.55);
                } else {
                    col = OctiAppIconRenderer.lerpColor(0xFF0D121B, 0xFF05070B, Math.min(1.0, (dist - 0.55) / 0.45));
                }
                pixels[y * w + x] = col;
            }
        }

        double vpX = w * 0.5;
        double vpY = h * 0.12;
        int numLines = 14;
        for (int i = -numLines; i <= numLines; i++) {
            double botX = w * 0.5 + i * (w * 0.065);
            OctiAppIconRenderer.drawLineSegment(pixels, w, h, vpX, vpY, botX, h, 1.0, 0x22C8A253);
        }
        double scrollY = (progress * 180.0) % 36.0;
        for (double y = h * 0.35 + scrollY; y < h; y += 36.0) {
            double factor = (y - vpY) / Math.max(1.0, h - vpY);
            int alpha = (int) (0x35 * factor);
            int lineColor = (alpha << 24) | 0x00C8A253;
            OctiAppIconRenderer.drawLineSegment(pixels, w, h, 0, y, w, y, 1.0, lineColor);
        }

        double outerR = Math.min(w, h) * 0.28;
        double ringAngleDeg = progress * 360.0 * 0.4;
        OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy, outerR, 2.2, pal.boardOuterFrame);
        OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy, outerR * 0.85, 1.2, 0x66C8A253);

        for (int deg = 0; deg < 360; deg += 15) {
            double r = Math.toRadians(deg + ringAngleDeg);
            double r1 = (deg % 45 == 0) ? (outerR * 0.80) : (outerR * 0.86);
            double r2 = outerR * 0.98;
            double p1x = cx + Math.cos(r) * r1;
            double p1y = cy + Math.sin(r) * r1;
            double p2x = cx + Math.cos(r) * r2;
            double p2y = cy + Math.sin(r) * r2;
            int tickColor = (deg % 90 == 0) ? pal.accentBrass : 0x88C8A253;
            OctiAppIconRenderer.drawLineSegment(pixels, w, h, p1x, p1y, p2x, p2y, 1.2, tickColor);
        }

        double podScale = 0.5 + 0.5 * Math.min(1.0, progress * 1.6);
        double podR = Math.min(w, h) * 0.16 * podScale;

        OctiPod splashPod = new OctiPod(OctiPodType.A_POD);
        if (progress >= 0.25) splashPod.addProng(OctiDir.DIR_E, false);
        if (progress >= 0.40) splashPod.addProng(OctiDir.DIR_W, false);
        if (progress >= 0.55) splashPod.addProng(OctiDir.DIR_NE, false);
        if (progress >= 0.70) splashPod.addProng(OctiDir.DIR_SW, false);
        if (progress >= 0.82) splashPod.addProng(OctiDir.DIR_N, true);

        if (progress >= 0.25 && progress < 0.45) {
            double waveR = podR * (1.0 + (progress - 0.25) * 4.0);
            int waveAlpha = (int) (0x60 * (1.0 - (progress - 0.25) / 0.20));
            OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy, waveR, 1.8, (waveAlpha << 24) | 0x00E4C070);
        }
        if (progress >= 0.82 && progress < 1.0) {
            double waveR = podR * (1.0 + (progress - 0.82) * 4.5);
            int waveAlpha = (int) (0x80 * (1.0 - (progress - 0.82) / 0.18));
            OctiAppIconRenderer.drawCircleStroke(pixels, w, h, cx, cy, waveR, 2.4, (waveAlpha << 24) | 0x00FEF08A);
        }

        double[] podRect = {cx - podR, cy - podR, podR * 2.0, podR * 2.0};
        OctiBoardGeometry tempGeo = new OctiBoardGeometry(w, h, 9, 9, OctiBoardGeometry.BoardRotation.ROT_0);
        paintOctagonalPod(pixels, w, h, podRect, OctiPodType.A_POD, splashPod, tempGeo, pal, true);

        int particleCount = 24;
        for (int i = 0; i < particleCount; i++) {
            double pAngle = Math.toRadians((i * (360.0 / particleCount)) + progress * 720.0);
            double pDist = outerR * (0.6 + 0.4 * Math.sin(pAngle * 2.0 + i));
            double px = cx + Math.cos(pAngle) * pDist;
            double py = cy + Math.sin(pAngle) * pDist;
            int pCol = (i % 2 == 0) ? pal.prongSuperGold : pal.accentBrass;
            OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, px, py, 2.2, pCol);
        }

        String titleStr = "OCTI";
        double titleH = Math.min(h * 0.055, w * 0.09);
        double titleW = titleStr.length() * (titleH * 0.80);
        drawVectorString(pixels, w, h, Math.max(10.0, (w - titleW) * 0.5), h * 0.055, titleH, titleStr, pal.prongSuperGold, 3.6);

        String prefix = (sha256Prefix != null && !sha256Prefix.isEmpty()) ? sha256Prefix : "dev-local";
        String verStr = "Version " + OctiDefs.OCTI_VERSION_STRING + "  •  SHA-256: " + prefix;
        double textH = Math.max(11.0, Math.min(h * 0.020, (w - 56.0) / Math.max(1.0, verStr.length() * 0.78)));
        double strW = verStr.length() * (textH * 0.76);
        double tx = Math.max(16.0, (w - strW) * 0.5);
        double ty = h * 0.78;
        drawVectorString(pixels, w, h, tx, ty, textH, verStr, 0xFFFFFFFF, 2.0);

        String authStr = "Author: Charles Chiou (selfso.com)";
        double subH = textH * 0.82;
        double authW = authStr.length() * (subH * 0.76);
        drawVectorString(pixels, w, h, Math.max(16.0, (w - authW) * 0.5), ty + textH * 2.0, subH, authStr, pal.textSecondary, 1.5);

        String copyStr = "Copyright (C) 2026, Charles Chiou";
        double copyW = copyStr.length() * (subH * 0.76);
        drawVectorString(pixels, w, h, Math.max(16.0, (w - copyW) * 0.5), ty + textH * 3.8, subH, copyStr, pal.accentBrass, 1.6);

        return pixels;
    }

    public static void paintPlayerReserveCard(int[] pixels, int w, int h,
                                              double rx, double ry, double rw, double rh,
                                              OctiPlayer player, boolean isTurn,
                                              int freePods, int capPods, int freeProngs, int freeSuper,
                                              boolean useSuperProngs, OctiPalette pal) {
        fillRectSolid(pixels, w, h, rx, ry, rw, rh, pal.panelCardBg);
        drawRectOutline(pixels, w, h, rx, ry, rw, rh, isTurn ? 2.2 : 1.2, isTurn ? pal.accentBrass : pal.panelCardBorder);

        OctiPalette.PodFactionPalette fp = pal.factionForPlayer(player);
        OctiPlayer opponent = (player == OctiPlayer.PLAYER_A) ? OctiPlayer.PLAYER_B : OctiPlayer.PLAYER_A;
        OctiPalette.PodFactionPalette oppFp = pal.factionForPlayer(opponent);

        String name = (player == OctiPlayer.PLAYER_A) ? "PLAYER A" : "PLAYER B";
        drawVectorString(pixels, w, h, rx + 14.0, ry + 12.0, 15.0, name, isTurn ? pal.accentBrass : pal.textSecondary, 1.6);

        double curX = rx + 16.0;
        double itemCy = ry + 48.0;

        // 1. Pod icon + freePods count
        double podR = 10.0;
        double podCx = curX + podR;
        OctiAppIconRenderer.fillOctagonGradient(pixels, w, h, podCx, itemCy, podR, fp.highlight, fp.core, fp.shadow);
        OctiAppIconRenderer.drawOctagonStroke(pixels, w, h, podCx, itemCy, podR, 1.2, fp.rim);
        drawVectorString(pixels, w, h, podCx - 3.2, itemCy - 4.5, 9.0, player.name().substring(7), fp.emblem, 1.2);
        curX += podR * 2.0 + 5.0;
        drawVectorString(pixels, w, h, curX, ry + 40.0, 15.0, Integer.toString(freePods), pal.textPrimary, 1.6);
        curX += 28.0;

        // 2. Prong blade icon + freeProngs count
        double prW = 12.0;
        double prH = 18.0;
        double prCx = curX + prW * 0.5;
        double prTop = itemCy - prH * 0.5;
        double[] pvx = { prCx, prCx + prW * 0.45, prCx + prW * 0.35, prCx - prW * 0.35, prCx - prW * 0.45 };
        double[] pvy = { prTop, prTop + prH * 0.65, prTop + prH, prTop + prH, prTop + prH * 0.65 };
        OctiAppIconRenderer.fillPolygonShaded(pixels, w, h, pvx, pvy, 0xFFFFFFFF, pal.prongNormalTip, pal.prongNormalMetal);
        OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, pvx, pvy, 1.2, 0xFF0A0F18);
        OctiAppIconRenderer.drawLineSegment(pixels, w, h, prCx, prTop + 2.0, prCx, prTop + prH - 2.0, 1.0, 0xCCFFFFFF);
        curX += prW + 5.0;
        drawVectorString(pixels, w, h, curX, ry + 40.0, 15.0, Integer.toString(freeProngs), pal.textPrimary, 1.6);
        curX += 34.0;

        // 3. Superprong icon + freeSuper count (if useSuperProngs)
        if (useSuperProngs) {
            double spW = 15.0;
            double spH = 18.0;
            double spCx = curX + spW * 0.5;
            double spTop = itemCy - spH * 0.5;
            double[] svx = { spCx, spCx + spW * 0.5, spCx + spW * 0.3, spCx - spW * 0.3, spCx - spW * 0.5 };
            double[] svy = { spTop, spTop + spH * 0.55, spTop + spH, spTop + spH, spTop + spH * 0.55 };
            OctiAppIconRenderer.fillPolygonShaded(pixels, w, h, svx, svy, 0xFFFEF08A, pal.prongSuperGold, 0xFFD97706);
            OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, svx, svy, 1.3, pal.prongSuperCore);
            curX += spW + 5.0;
            drawVectorString(pixels, w, h, curX, ry + 40.0, 15.0, Integer.toString(freeSuper), pal.textPrimary, 1.6);
            curX += 28.0;
        }

        // 4. Captured Pod icon + capPods count (only when capPods > 0)
        if (capPods > 0) {
            double capCx = curX + podR;
            OctiAppIconRenderer.fillOctagonGradient(pixels, w, h, capCx, itemCy, podR, oppFp.highlight, oppFp.core, oppFp.shadow);
            OctiAppIconRenderer.drawCircleStroke(pixels, w, h, capCx, itemCy, podR * 1.05, 1.5, pal.legalCaptureGlow);
            OctiAppIconRenderer.drawLineSegment(pixels, w, h, capCx - podR * 0.7, itemCy - podR * 0.7, capCx + podR * 0.7, itemCy + podR * 0.7, 1.5, pal.legalCaptureGlow);
            curX += podR * 2.0 + 5.0;
            drawVectorString(pixels, w, h, curX, ry + 40.0, 15.0, Integer.toString(capPods), pal.textPrimary, 1.6);
        }
    }

    public static int[] renderPlyConfirmScreenPixels(OctiBoard previewBoard,
                                                     OctiMoveComposer composer,
                                                     OctiThemeId theme) {
        int w = 1080;
        int h = 1920;
        OctiPalette pal = OctiPalette.forTheme(theme != null ? theme : OctiThemeId.OBSIDIAN_BRASS);
        int[] pixels = new int[w * h];
        Arrays.fill(pixels, pal.windowBg);

        // 1. Decluttered TopAppBar (PCE-A08)
        fillRectSolid(pixels, w, h, 0, 0, w, 128, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 0, 0, w, 128, 1.5, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 36, 42, 42, "OCTI", pal.accentBrass, 3.6);
        drawVectorString(pixels, w, h, 220, 52, 22, "VS BOT TACTICAL", pal.textSecondary, 2.0);
        drawRectOutline(pixels, w, h, 680, 32, 210, 64, 2.0, pal.accentBrass);
        drawVectorString(pixels, w, h, 706, 52, 22, "NEW GAME", pal.accentBrass, 2.2);
        drawVectorString(pixels, w, h, 920, 52, 22, "MENU", pal.accentBrass, 2.2);

        // 2. 1080x1080 Board Canvas
        OctiBoard boardToRender = (composer != null && composer.isAwaitingConfirmation())
                ? composer.buildPreviewBoard()
                : previewBoard;
        OctiBoardGeometry geo = new OctiBoardGeometry(1080, 1080, 9, 9,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPx = renderBoardPixels(boardToRender, composer, geo, theme);
        for (int by = 0; by < 1080; by++) {
            System.arraycopy(boardPx, by * 1080, pixels, (136 + by) * w, 1080);
        }

        // 3. Player A & Player B Reserve Cards
        OctiGame game = boardToRender != null ? boardToRender.getGame() : null;
        int freePodsA = game != null ? game.getFreePods(OctiPlayer.PLAYER_A) : 4;
        int capPodsA = game != null ? game.getCapturedPods(OctiPlayer.PLAYER_A) : 0;
        int freeProngsA = game != null ? game.getFreeProngs(OctiPlayer.PLAYER_A) : 25;
        int freeSuperA = game != null ? game.getFreeSuperProngs(OctiPlayer.PLAYER_A) : 0;
        int freePodsB = game != null ? game.getFreePods(OctiPlayer.PLAYER_B) : 4;
        int capPodsB = game != null ? game.getCapturedPods(OctiPlayer.PLAYER_B) : 0;
        int freeProngsB = game != null ? game.getFreeProngs(OctiPlayer.PLAYER_B) : 25;
        int freeSuperB = game != null ? game.getFreeSuperProngs(OctiPlayer.PLAYER_B) : 0;
        boolean useSuper = game != null && game.useSuperProngs();
        OctiPlayer turn = game != null ? game.getTurn() : OctiPlayer.PLAYER_A;

        paintPlayerReserveCard(pixels, w, h, 28, 1224, 496, 76,
                OctiPlayer.PLAYER_A, turn == OctiPlayer.PLAYER_A,
                freePodsA, capPodsA, freeProngsA, freeSuperA, useSuper, pal);
        paintPlayerReserveCard(pixels, w, h, 556, 1224, 496, 76,
                OctiPlayer.PLAYER_B, turn == OctiPlayer.PLAYER_B,
                freePodsB, capPodsB, freeProngsB, freeSuperB, useSuper, pal);

        drawRectOutline(pixels, w, h, 48, 1312, 220, 56, 1.8, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 68, 1329, 20, "MOVE STACK", pal.textPrimary, 2.0);
        drawRectOutline(pixels, w, h, 288, 1312, 160, 56, 1.8, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 326, 1329, 20, "UNDO", pal.textPrimary, 2.0);
        drawRectOutline(pixels, w, h, 468, 1312, 180, 56, 1.8, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 496, 1329, 20, "RESIGN", pal.textPrimary, 2.0);

        // 4. Bottom-anchored borderless PlyConfirmDialog (PCE-A01, PCE-A05)
        int dlgY = 1500;
        fillRectSolid(pixels, w, h, 28, dlgY, 1024, 220, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 28, dlgY, 1024, 220, 3.2, pal.accentBrass);
        String notation = (composer != null) ? composer.stagedMoveNotation() : "33A-34";
        drawVectorString(pixels, w, h, 60, dlgY + 32, 26, "STAGED PLY: " + notation, pal.legalStepGuide, 2.6);

        // Cancel button
        drawRectOutline(pixels, w, h, 60, dlgY + 104, 460, 84, 2.4, pal.legalCaptureGlow);
        drawVectorString(pixels, w, h, 220, dlgY + 134, 24, "CANCEL", pal.legalCaptureGlow, 2.4);

        // Commit button
        fillRectSolid(pixels, w, h, 560, dlgY + 104, 460, 84, pal.accentBrass);
        drawVectorString(pixels, w, h, 730, dlgY + 134, 24, "COMMIT", pal.windowBg, 2.8);

        // 5. Bottom Navigation Bar
        fillRectSolid(pixels, w, h, 0, 1760, w, 160, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 0, 1760, w, 160, 1.5, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 90, 1826, 22, "BOARD", pal.accentBrass, 2.4);
        drawVectorString(pixels, w, h, 450, 1826, 22, "LOBBY", pal.textSecondary, 2.0);
        drawVectorString(pixels, w, h, 790, 1826, 22, "ARCHIVE", pal.textSecondary, 2.0);

        return pixels;
    }

    public static int[] renderGameOverScreenPixels(OctiBoard board,
                                                   OctiMoveComposer composer,
                                                   OctiThemeId theme) {
        int w = 1080;
        int h = 1920;
        OctiPalette pal = OctiPalette.forTheme(theme != null ? theme : OctiThemeId.OBSIDIAN_BRASS);
        int[] pixels = new int[w * h];
        Arrays.fill(pixels, pal.windowBg);

        // 1. TopAppBar
        fillRectSolid(pixels, w, h, 0, 0, w, 128, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 0, 0, w, 128, 1.5, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 36, 42, 42, "OCTI", pal.accentBrass, 3.6);
        drawVectorString(pixels, w, h, 220, 52, 22, "GAME OVER: PLAYER A", pal.textSecondary, 2.0);
        drawRectOutline(pixels, w, h, 680, 32, 210, 64, 2.0, pal.accentBrass);
        drawVectorString(pixels, w, h, 706, 52, 22, "NEW GAME", pal.accentBrass, 2.2);
        drawVectorString(pixels, w, h, 920, 52, 22, "MENU", pal.accentBrass, 2.2);

        // 2. Persistent Top Victory Banner above board (PCE-A07)
        fillRectSolid(pixels, w, h, 20, 140, 1040, 88, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 20, 140, 1040, 88, 2.8, pal.accentBrass);
        drawVectorString(pixels, w, h, 48, 170, 26, "GAME OVER - WINNER: PLAYER A", pal.accentBrass, 2.6);
        fillRectSolid(pixels, w, h, 810, 154, 220, 60, pal.accentBrass);
        drawVectorString(pixels, w, h, 838, 172, 22, "NEW GAME", pal.windowBg, 2.4);

        // 3. Board Canvas
        OctiBoardGeometry geo = new OctiBoardGeometry(1080, 1080, 9, 9,
                OctiBoardGeometry.BoardRotation.ROT_0);
        int[] boardPx = renderBoardPixels(board, composer, geo, theme);
        for (int by = 0; by < 1080; by++) {
            System.arraycopy(boardPx, by * 1080, pixels, (240 + by) * w, 1080);
        }

        // 3b. Reserve HUD + Bottom Navigation Bar under Scrim
        OctiGame gme = board != null ? board.getGame() : null;
        int freePodsA_go = gme != null ? gme.getFreePods(OctiPlayer.PLAYER_A) : 4;
        int capPodsA_go = gme != null ? gme.getCapturedPods(OctiPlayer.PLAYER_A) : 0;
        int freeProngsA_go = gme != null ? gme.getFreeProngs(OctiPlayer.PLAYER_A) : 25;
        int freeSuperA_go = gme != null ? gme.getFreeSuperProngs(OctiPlayer.PLAYER_A) : 0;
        int freePodsB_go = gme != null ? gme.getFreePods(OctiPlayer.PLAYER_B) : 4;
        int capPodsB_go = gme != null ? gme.getCapturedPods(OctiPlayer.PLAYER_B) : 0;
        int freeProngsB_go = gme != null ? gme.getFreeProngs(OctiPlayer.PLAYER_B) : 25;
        int freeSuperB_go = gme != null ? gme.getFreeSuperProngs(OctiPlayer.PLAYER_B) : 0;
        boolean useSuper_go = gme != null && gme.useSuperProngs();

        paintPlayerReserveCard(pixels, w, h, 28, 1336, 496, 76,
                OctiPlayer.PLAYER_A, false,
                freePodsA_go, capPodsA_go, freeProngsA_go, freeSuperA_go, useSuper_go, pal);
        paintPlayerReserveCard(pixels, w, h, 556, 1336, 496, 76,
                OctiPlayer.PLAYER_B, false,
                freePodsB_go, capPodsB_go, freeProngsB_go, freeSuperB_go, useSuper_go, pal);
        fillRectSolid(pixels, w, h, 0, 1760, w, 160, pal.panelCardBg);
        drawRectOutline(pixels, w, h, 0, 1760, w, 160, 1.5, pal.panelCardBorder);
        drawVectorString(pixels, w, h, 90, 1826, 22, "BOARD", pal.accentBrass, 2.4);
        drawVectorString(pixels, w, h, 450, 1826, 22, "LOBBY", pal.textSecondary, 2.0);
        drawVectorString(pixels, w, h, 790, 1826, 22, "ARCHIVE", pal.textSecondary, 2.0);

        // 4. Scrim + Centered GameOverDialog Modal (PCE-A07)
        fillRectAlpha(pixels, w, h, 0, 128, w, 1632, 0xFF000000, 0.58);
        int mx = 84;
        int my = 660;
        int mw = 912;
        int mh = 440;
        fillRectSolid(pixels, w, h, mx, my, mw, mh, pal.panelCardBg);
        drawRectOutline(pixels, w, h, mx, my, mw, mh, 3.8, pal.accentBrass);

        drawVectorString(pixels, w, h, mx + 86, my + 56, 34, "VICTORY - PLAYER A WINS!", pal.accentBrass, 3.2);
        drawVectorString(pixels, w, h, mx + 120, my + 140, 24, "MATCH CONCLUDED BY RESIGNATION", pal.textPrimary, 2.2);
        drawVectorString(pixels, w, h, mx + 250, my + 200, 22, "LOCAL VS BOT TACTICAL", pal.textSecondary, 2.0);

        // Review Board & New Game buttons
        drawRectOutline(pixels, w, h, mx + 56, my + 294, 380, 92, 2.6, pal.accentBrass);
        drawVectorString(pixels, w, h, mx + 104, my + 328, 24, "REVIEW BOARD", pal.accentBrass, 2.4);

        fillRectSolid(pixels, w, h, mx + 476, my + 294, 380, 92, pal.accentBrass);
        drawVectorString(pixels, w, h, mx + 566, my + 328, 24, "NEW GAME", pal.windowBg, 2.8);

        return pixels;
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
