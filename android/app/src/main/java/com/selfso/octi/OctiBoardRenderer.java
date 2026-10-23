/*
 * OctiBoardRenderer.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.List;

public final class OctiBoardRenderer {

    private int stackFanX = -1;
    private int stackFanY = -1;
    private int hoverSquareX = -1;
    private int hoverSquareY = -1;
    private OctiDir hoverProngDir = OctiDir.DIR_UNDEF;

    public OctiBoardRenderer() {
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
        return new OctiBoardRenderer().render(board, composer, geo, theme);
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
        return new OctiBoardRenderer().render(board, composer, geo, theme);
    }

    public int[] render(OctiBoard board,
                        OctiMoveComposer composer,
                        OctiBoardGeometry geo,
                        OctiThemeId theme) {
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

        // 1. Luxury board outer frame with 3D brass bevel
        double[] outer = g.boardOuterRect();
        double[] grid = g.gridInnerRect();

        fillRectGradient(pixels, w, h, outer[0], outer[1], outer[2], outer[3],
                OctiAppIconRenderer.lerpColor(pal.boardOuterFrame, 0xFFFFFFFF, 0.12),
                pal.boardOuterFrame,
                OctiAppIconRenderer.lerpColor(pal.boardOuterFrame, 0xFF000000, 0.25));
        drawRectOutline(pixels, w, h, outer[0], outer[1], outer[2], outer[3], 2.6, pal.boardBevelLight);
        drawRectOutline(pixels, w, h,
                outer[0] + 3.0, outer[1] + 3.0,
                Math.max(2.0, outer[2] - 6.0), Math.max(2.0, outer[3] - 6.0),
                1.6, pal.boardBevelDark);

        int gw = g.gridWidth();
        int gh = g.gridHeight();
        double cell = g.cellSize();
        double gutter = (outer[2] - grid[2]) * 0.5;

        // 2. Corner compass rose showing North ('a')
        double roseSize = Math.max(16.0, gutter * 0.90);
        double roseCx = outer[0] + outer[2] - roseSize * 0.5 - 3.0;
        double roseCy = outer[1] + roseSize * 0.5 + 3.0;
        paintCompassRose(pixels, w, h, roseCx, roseCy, roseSize * 0.44, g, pal);

        // 3. Coordinate gutter ticks/markers along all 4 sides
        for (int x = 1; x <= gw; x++) {
            double[] rBot = g.squareRect(x, 1);
            double[] rTop = g.squareRect(x, gh);
            if (g.rotation() == OctiBoardGeometry.BoardRotation.ROT_0
                    || g.rotation() == OctiBoardGeometry.BoardRotation.ROT_180) {
                paintDigitPip(pixels, w, h,
                        rBot[0] + rBot[2] * 0.5, grid[1] + grid[3] + gutter * 0.5,
                        cell * 0.10, x, pal.coordLabel);
                paintDigitPip(pixels, w, h,
                        rTop[0] + rTop[2] * 0.5, outer[1] + gutter * 0.5,
                        cell * 0.10, x, pal.coordLabel);
            } else {
                paintDigitPip(pixels, w, h,
                        outer[0] + gutter * 0.5, rBot[1] + rBot[3] * 0.5,
                        cell * 0.10, x, pal.coordLabel);
                paintDigitPip(pixels, w, h,
                        grid[0] + grid[2] + gutter * 0.5, rTop[1] + rTop[3] * 0.5,
                        cell * 0.10, x, pal.coordLabel);
            }
        }
        for (int y = 1; y <= gh; y++) {
            double[] rLeft = g.squareRect(1, y);
            double[] rRight = g.squareRect(gw, y);
            if (g.rotation() == OctiBoardGeometry.BoardRotation.ROT_0
                    || g.rotation() == OctiBoardGeometry.BoardRotation.ROT_180) {
                paintDigitPip(pixels, w, h,
                        outer[0] + gutter * 0.5, rLeft[1] + rLeft[3] * 0.5,
                        cell * 0.10, y, pal.coordLabel);
                paintDigitPip(pixels, w, h,
                        grid[0] + grid[2] + gutter * 0.5, rRight[1] + rRight[3] * 0.5,
                        cell * 0.10, y, pal.coordLabel);
            } else {
                paintDigitPip(pixels, w, h,
                        rLeft[0] + rLeft[2] * 0.5, grid[1] + grid[3] + gutter * 0.5,
                        cell * 0.10, y, pal.coordLabel);
                paintDigitPip(pixels, w, h,
                        rRight[0] + rRight[2] * 0.5, outer[1] + gutter * 0.5,
                        cell * 0.10, y, pal.coordLabel);
            }
        }

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
                        paintHomeSquareBadge(pixels, w, h, sqRect, sq, board, pal);
                    } else if (st == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                        paintNeutralOctiMedallion(pixels, w, h, sqRect, pal);
                    }
                }
            }
        }

        // 6. Legal destination guides
        if (composer != null) {
            List<int[]> guides = null;
            if (composer.activeDragKind() == OctiMoveComposer.DragKind.ADD_POD_FROM_RESERVE) {
                guides = composer.legalPodDropSquares(false);
            } else if (composer.activeDragKind() == OctiMoveComposer.DragKind.LIBERATE_POD_FROM_RESERVE) {
                guides = composer.legalPodDropSquares(true);
            } else if (hoverSquareX >= 1 && hoverSquareY >= 1) {
                guides = composer.legalDestinationsFrom(hoverSquareX, hoverSquareY);
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

        // 7. Pods, prongs, superprongs, and layered 3D-offset multi-pod stacks with count badges
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
                    int selIdx = (composer != null)
                            ? composer.selectedStackPodIndex(x, y)
                            : (numPods - 1);
                    int maxLayers = Math.min(numPods, 4);

                    for (int layer = 0; layer < maxLayers; layer++) {
                        int podIdx = (layer == maxLayers - 1) ? selIdx : layer;
                        podIdx = Math.max(0, Math.min(numPods - 1, podIdx));
                        OctiPod pod = sq.getPodAt(podIdx);
                        double offset = (maxLayers - 1 - layer) * 2.5;
                        double[] layerRect = {
                            sqRect[0] - offset,
                            sqRect[1] + offset,
                            sqRect[2],
                            sqRect[3]
                        };
                        boolean hi = (x == hoverSquareX && y == hoverSquareY && layer == maxLayers - 1);
                        paintOctagonalPod(pixels, w, h, layerRect, pod.getType(), pod, g, pal, hi);
                    }

                    if (numPods >= 2) {
                        double bw = Math.max(16.0, sqRect[2] * 0.32);
                        double bh = Math.max(12.0, sqRect[3] * 0.24);
                        double bx = sqRect[0] + sqRect[2] - bw - 2.0;
                        double by = sqRect[1] + 2.0;
                        fillRectSolid(pixels, w, h, bx, by, bw, bh, pal.panelCardBg);
                        drawRectOutline(pixels, w, h, bx, by, bw, bh, 1.3, pal.accentBrass);
                        paintDigitPip(pixels, w, h, bx + bw * 0.5, by + bh * 0.5,
                                bh * 0.28, numPods, pal.prongSuperGold);
                    }
                }
            }
        }

        // 8. Radial Prong Wheel when hovering a friendly pod
        if (composer != null && hoverSquareX >= 1 && hoverSquareY >= 1
                && board != null && board.isValidCoord(hoverSquareX - 1, hoverSquareY - 1)
                && !board.squareAt(hoverSquareX - 1, hoverSquareY - 1).isEmpty()) {
            int selPod = composer.selectedStackPodIndex(hoverSquareX, hoverSquareY);
            List<OctiDir> legalDirs = composer.legalProngAdditionsAt(
                    hoverSquareX, hoverSquareY, selPod, false);
            if (!legalDirs.isEmpty()) {
                double[] center = g.squareCenter(hoverSquareX, hoverSquareY);
                double slotDist = cell * 0.36;
                OctiAppIconRenderer.drawCircleStroke(pixels, w, h,
                        center[0], center[1], slotDist, 1.2, pal.accentBrass);
                for (int d = 0; d < OctiDir.DIR_MAX; d++) {
                    OctiDir dir = OctiDir.fromIndex(d);
                    double[] u = g.dirUnitVectorScreen(dir);
                    double sx = center[0] + u[0] * slotDist;
                    double sy = center[1] + u[1] * slotDist;
                    boolean isLegal = legalDirs.contains(dir);
                    boolean isHover = (dir == hoverProngDir);
                    double dotR = isHover ? cell * 0.10 : cell * 0.08;
                    int fill = (isHover && isLegal) ? pal.legalStepGuide
                            : isLegal ? 0xD8162A3E : 0x70141A24;
                    int stroke = (isHover && isLegal) ? pal.prongSuperCore
                            : isLegal ? pal.accentBrass : pal.textSecondary;
                    OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, sx, sy, dotR, fill);
                    OctiAppIconRenderer.drawCircleStroke(pixels, w, h, sx, sy, dotR, 1.2, stroke);
                }
            }
        }

        // 9. Floating Stack Fan Pop-over strip
        if (stackFanX >= 1 && stackFanY >= 1 && board != null
                && board.isValidCoord(stackFanX - 1, stackFanY - 1)) {
            OctiSquare sq = board.squareAt(stackFanX - 1, stackFanY - 1);
            int n = sq.getNumPods();
            if (n >= 2) {
                double[] overlay = g.stackFanOverlayRect(stackFanX, stackFanY, n);
                fillRectSolid(pixels, w, h, overlay[0], overlay[1], overlay[2], overlay[3], 0xEB0F1622);
                drawRectOutline(pixels, w, h, overlay[0], overlay[1], overlay[2], overlay[3],
                        1.8, pal.accentBrass);
                int selIdx = (composer != null)
                        ? composer.selectedStackPodIndex(stackFanX, stackFanY)
                        : (n - 1);
                for (int i = 0; i < n; i++) {
                    double[] slot = g.stackFanSlotRect(stackFanX, stackFanY, n, i);
                    boolean selected = (i == selIdx);
                    if (selected) {
                        drawRectOutline(pixels, w, h, slot[0], slot[1], slot[2], slot[3],
                                2.0, pal.prongSuperGold);
                    }
                    OctiPod pod = sq.getPodAt(i);
                    paintOctagonalPod(pixels, w, h, slot, pod.getType(), pod, g, pal, selected);
                }
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

        // 11. Floating Commit Move / Rollback confirmation banner
        if (composer != null && composer.isAwaitingConfirmation()) {
            double barW = Math.min(outer[2] * 0.88, 520.0);
            double barH = Math.max(28.0, outer[3] * 0.055);
            double barX = outer[0] + outer[2] * 0.5 - barW * 0.5;
            double barY = outer[1] + outer[3] - barH - 6.0;
            fillRectSolid(pixels, w, h, barX, barY, barW, barH, 0xEE0E1420);
            drawRectOutline(pixels, w, h, barX, barY, barW, barH, 1.8, pal.accentBrass);

            double btnW = barW * 0.24;
            double btnH = barH - 8.0;
            double commitX = barX + barW - btnW * 2.0 - 14.0;
            double rollbackX = barX + barW - btnW - 6.0;
            double btnY = barY + 4.0;

            fillRectSolid(pixels, w, h, commitX, btnY, btnW, btnH,
                    OctiAppIconRenderer.lerpColor(pal.legalStepGuide, 0xFF000000, 0.25));
            drawRectOutline(pixels, w, h, commitX, btnY, btnW, btnH, 1.2, pal.prongSuperCore);

            fillRectSolid(pixels, w, h, rollbackX, btnY, btnW, btnH,
                    OctiAppIconRenderer.lerpColor(pal.legalCaptureGlow, 0xFF000000, 0.30));
            drawRectOutline(pixels, w, h, rollbackX, btnY, btnW, btnH, 1.2, pal.accentBrass);
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

    private static void paintHomeSquareBadge(int[] pixels, int w, int h,
                                             double[] sqRect, OctiSquare sq,
                                             OctiBoard board, OctiPalette pal) {
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
        OctiPalette.PodFactionPalette fp = pal.factionForPlayer(owner);
        double ix = sqRect[0] + 3.5;
        double iy = sqRect[1] + 3.5;
        double iw = Math.max(2.0, sqRect[2] - 7.0);
        double ih = Math.max(2.0, sqRect[3] - 7.0);

        fillRectAlpha(pixels, w, h, ix, iy, iw, ih, fp.core, 0.20);
        int ringCol = sq.isCaptured() ? pal.legalCaptureGlow : fp.core;
        double ringThick = sq.isCaptured() ? 2.6 : 1.8;
        drawRectOutline(pixels, w, h, ix, iy, iw, ih, ringThick, ringCol);

        int numPips = (board != null && board.getGame() != null
                && board.getGame().getBases() == OctiGameBases.GAME_BASES_1) ? 1 : 3;
        double pipR = Math.max(1.5, sqRect[2] * 0.035);
        double startX = (sqRect[0] + sqRect[2] * 0.5) - (numPips - 1) * pipR * 1.4;
        double pipY = iy + ih - pipR * 1.8;
        for (int i = 0; i < numPips; i++) {
            OctiAppIconRenderer.fillCircleAlpha(pixels, w, h,
                    startX + i * pipR * 2.8, pipY, pipR, fp.highlight);
        }
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
                                          boolean highlighted) {
        OctiPalette.PodFactionPalette fp = pal.factionForPodType(type);
        double cx = rect[0] + rect[2] * 0.5;
        double cy = rect[1] + rect[3] * 0.5;
        double radius = Math.min(rect[2], rect[3]) * 0.36;

        double prongInnerR = radius * 0.65;
        double prongOuterR = radius * 1.34;
        double superOuterR = radius * 1.44;

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
                double halfW = Math.max(1.8, radius * 0.15);
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
                        pal.prongNormalTip, pal.prongNormalMetal, 0xFF64748B);
                OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, vx, vy, 1.0, 0xFF141A24);
            } else if (pt == OctiProngType.SUPER_PRONG) {
                double halfW = Math.max(2.4, radius * 0.22);
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
                        pal.prongSuperCore, pal.prongSuperGold, 0xFFD97706);
                OctiAppIconRenderer.drawPolygonOutline(pixels, w, h, vx, vy, 1.2, pal.prongSuperCore);
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

        // 6. Central emblem pip
        OctiAppIconRenderer.fillCircleAlpha(pixels, w, h, cx, cy,
                Math.max(1.6, radius * 0.18), fp.emblem);
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
}
