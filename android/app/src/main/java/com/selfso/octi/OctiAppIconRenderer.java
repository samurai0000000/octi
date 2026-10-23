/*
 * OctiAppIconRenderer.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.zip.CRC32;
import java.util.zip.Deflater;

public final class OctiAppIconRenderer {

    private static final double PI = Math.PI;

    public OctiAppIconRenderer() {
    }

    public static int[] renderPixels(int size) {
        int s = Math.max(16, Math.min(2048, size));
        int[] pixels = new int[s * s];

        double cx = s * 0.5;
        double cy = s * 0.5;
        double half = s * 0.5;

        // 1. Background: Obsidian & Brass radial slate gradient + recessed 3x3 grid + corner squircle
        double cornerR = s * 0.18;
        for (int y = 0; y < s; y++) {
            for (int x = 0; x < s; x++) {
                double px = x + 0.5;
                double py = y + 0.5;
                if (!insideRoundedRect(px, py, 0.0, 0.0, s, s, cornerR)) {
                    pixels[y * s + x] = 0x00000000;
                    continue;
                }

                double dist = Math.hypot(px - cx, py - cy) / (half * 1.42);
                int bgCol;
                if (dist < 0.58) {
                    bgCol = lerpColor(0xFF131C2E, 0xFF0B0F17, dist / 0.58);
                } else {
                    bgCol = lerpColor(0xFF0B0F17, 0xFF05070B, Math.min(1.0, (dist - 0.58) / 0.42));
                }

                // Subtle 4x4 grid cell alternation (lines at 0.25, 0.50, 0.75)
                int gx = Math.min(3, (int) ((px / s) * 4.0));
                int gy = Math.min(3, (int) ((py / s) * 4.0));
                int cellTint = ((gx + gy) % 2 == 0) ? 0xFF1D2940 : 0xFF151E30;
                bgCol = blendColor(bgCol, cellTint, 0.26);

                // Brass grid hairlines at 1/4, 1/2, 3/4
                double lineThresh = Math.max(0.85, s * 0.0045);
                boolean onGridLine =
                        Math.abs(px - s * 0.25) < lineThresh
                        || Math.abs(px - s * 0.50) < lineThresh
                        || Math.abs(px - s * 0.75) < lineThresh
                        || Math.abs(py - s * 0.25) < lineThresh
                        || Math.abs(py - s * 0.50) < lineThresh
                        || Math.abs(py - s * 0.75) < lineThresh;
                if (onGridLine) {
                    bgCol = blendColor(bgCol, 0xFFC8A253, 0.34);
                }

                pixels[y * s + x] = bgCol;
            }
        }

        // 2. Engraved background octagonal home-base ring
        drawOctagonStroke(pixels, s, s, cx, cy, s * 0.40, Math.max(1.2, s * 0.008), 0x66C8A253);

        // 3. Ambient drop shadow beneath outer octagonal crest
        fillOctagonFlat(pixels, s, s, cx + s * 0.012, cy + s * 0.022, s * 0.365, 0x88000000);

        // 4. Outer 3D Faceted Imperial Brass & Gold Octagonal Bezel
        double rOuter = s * 0.355;
        double rDial = rOuter * 0.84;
        fillOctagonGradient(pixels, s, s, cx, cy, rOuter, 0xFFFEF08A, 0xFFE4C070, 0xFF8C6523);
        drawOctagonStroke(pixels, s, s, cx, cy, rOuter, Math.max(1.2, s * 0.009), 0xFFFEF08A);

        // 5. Obsidian-Sapphire Starburst Dial inside outer bezel
        fillOctagonRadial(pixels, s, s, cx, cy, rDial, 0xFF1E2C48, 0xFF0F172A, 0xFF060A13);
        drawOctagonStroke(pixels, s, s, cx, cy, rDial, Math.max(1.0, s * 0.008), 0xFFD97706);
        drawOctagonStroke(pixels, s, s, cx, cy, rDial * 0.85, Math.max(0.8, s * 0.005), 0x66FDE047);

        // 6. Directional Prongs (Normal silver-steel in NE, E, SE, SW, W, NW & Superprong in N)
        double rPod = s * 0.215;
        double rProngBase = rPod * 0.70;
        double rNormalTip = rDial * 0.90;
        double rSuperTip = rOuter * 0.96;

        OctiDir[] normalDirs = {
            OctiDir.DIR_NE, OctiDir.DIR_E, OctiDir.DIR_SE,
            OctiDir.DIR_SW, OctiDir.DIR_W, OctiDir.DIR_NW
        };
        for (OctiDir dir : normalDirs) {
            double ux = dir.deltaX();
            double uy = -dir.deltaY();
            double len = Math.hypot(ux, uy);
            ux /= len;
            uy /= len;
            double px = -uy;
            double py = ux;

            double hwBase = Math.max(1.8, s * 0.024);
            double hwShoulder = Math.max(2.2, s * 0.030);
            double shDist = rNormalTip * 0.78;

            double[] polyX = {
                cx + ux * rProngBase - px * hwBase,
                cx + ux * shDist - px * hwShoulder,
                cx + ux * rNormalTip,
                cx + ux * shDist + px * hwShoulder,
                cx + ux * rProngBase + px * hwBase
            };
            double[] polyY = {
                cy + uy * rProngBase - py * hwBase,
                cy + uy * shDist - py * hwShoulder,
                cy + uy * rNormalTip,
                cy + uy * shDist + py * hwShoulder,
                cy + uy * rProngBase + py * hwBase
            };
            fillPolygonShaded(pixels, s, s, polyX, polyY, 0xFFF8FAFC, 0xFFCBD5E1, 0xFF64748B);
            drawPolygonOutline(pixels, s, s, polyX, polyY, Math.max(1.0, s * 0.006), 0xFF0F172A);
        }

        // Crowned Golden-Diamond Superprong (!) Protruding North (DIR_N: ux = 0, uy = -1)
        {
            // Outer starburst aura
            fillCircleAlpha(pixels, s, s, cx, cy - rSuperTip * 0.78, s * 0.095, 0x44FDE047);

            double yBase = cy - rProngBase;
            double yWing1 = cy - rSuperTip * 0.62;
            double yNotch = cy - rSuperTip * 0.70;
            double yWing2 = cy - rSuperTip * 0.80;
            double yTip = cy - rSuperTip;

            double wBase = s * 0.030;
            double wWing1 = s * 0.064;
            double wNotch = s * 0.036;
            double wWing2 = s * 0.056;

            double[] spX = {
                cx - wBase,
                cx - wWing1,
                cx - wNotch,
                cx - wWing2,
                cx,
                cx + wWing2,
                cx + wNotch,
                cx + wWing1,
                cx + wBase
            };
            double[] spY = {
                yBase,
                yWing1,
                yNotch,
                yWing2,
                yTip,
                yWing2,
                yNotch,
                yWing1,
                yBase
            };
            fillPolygonShaded(pixels, s, s, spX, spY, 0xFFFEF9C3, 0xFFFDE047, 0xFFD97706);
            drawPolygonOutline(pixels, s, s, spX, spY, Math.max(1.1, s * 0.007), 0xFFFFFBEB);
        }

        // 7. Central Sculpted Emerald Jade Octagonal Pod
        fillOctagonFlat(pixels, s, s, cx + s * 0.010, cy + s * 0.016, rPod * 1.05, 0x99000000);
        fillOctagonGradient(pixels, s, s, cx, cy, rPod, 0xFFFEF08A, 0xFFE4C070, 0xFF8C6523);
        drawOctagonStroke(pixels, s, s, cx, cy, rPod, Math.max(1.2, s * 0.010), 0xFFFDE047);

        double rPodInner = rPod * 0.82;
        fillOctagonRadial(pixels, s, s, cx - rPod * 0.16, cy - rPod * 0.16, rPodInner,
                0xFF6EE7B7, 0xFF10B981, 0xFF047857);
        drawOctagonStroke(pixels, s, s, cx, cy, rPodInner, Math.max(1.0, s * 0.007), 0xFF6EE7B7);

        // 8. 8 Recessed Prong Collar Sockets around Pod Rim
        double sockDist = rPod * 0.88;
        double sockR = Math.max(1.4, s * 0.016);
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            double ux = dir.deltaX();
            double uy = -dir.deltaY();
            double len = Math.hypot(ux, uy);
            ux /= len;
            uy /= len;
            double sx = cx + ux * sockDist;
            double sy = cy + uy * sockDist;
            int fillCol = (dir == OctiDir.DIR_N) ? 0xFFFDE047
                    : (dir == OctiDir.DIR_S) ? 0xFF0B0F17 : 0xFFE2E8F0;
            fillCircleAlpha(pixels, s, s, sx, sy, sockR, fillCol);
            drawCircleStroke(pixels, s, s, sx, sy, sockR, Math.max(0.8, s * 0.004), 0xFF064E3B);
        }

        // 9. Inner Specular Jewel Bevel & Central Brass Compass Rose Medallion
        drawOctagonStroke(pixels, s, s, cx, cy, rPod * 0.56, Math.max(0.9, s * 0.006), 0xAA6EE7B7);

        double starOuter = rPod * 0.46;
        double starInner = rPod * 0.16;
        double[] starX = {
            cx, cx + starInner, cx + starOuter, cx + starInner,
            cx, cx - starInner, cx - starOuter, cx - starInner
        };
        double[] starY = {
            cy - starOuter, cy - starInner, cy, cy + starInner,
            cy + starOuter, cy + starInner, cy, cy - starInner
        };
        fillPolygonShaded(pixels, s, s, starX, starY, 0xFFFFFBEB, 0xFFFDE68A, 0xFFD97706);
        drawPolygonOutline(pixels, s, s, starX, starY, Math.max(0.8, s * 0.005), 0xFF78350F);
        fillCircleAlpha(pixels, s, s, cx, cy, Math.max(1.5, rPod * 0.10), 0xFFFFFBEB);

        return pixels;
    }

    public int[] renderIconPixels(int size) {
        return renderPixels(size);
    }

    public static void writePng(int size, File outFile) throws IOException {
        int s = Math.max(16, Math.min(2048, size));
        int[] pixels = renderPixels(s);
        writeArgbPng(s, s, pixels, outFile);
    }

    public static void writeArgbPng(int width, int height, int[] argbPixels, File outFile)
            throws IOException {
        if (outFile == null) {
            throw new IllegalArgumentException("outFile must not be null");
        }
        File parent = outFile.getParentFile();
        if (parent != null && !parent.exists()) {
            parent.mkdirs();
        }
        byte[] pngBytes = encodePng(width, height, argbPixels);
        try (FileOutputStream fos = new FileOutputStream(outFile)) {
            fos.write(pngBytes);
            fos.flush();
        }
    }

    public static byte[] encodePng(int width, int height, int[] argbPixels) throws IOException {
        if (width <= 0 || height <= 0 || argbPixels == null || argbPixels.length < width * height) {
            throw new IllegalArgumentException("Invalid image dimensions or pixel buffer");
        }

        ByteArrayOutputStream rawScanlines = new ByteArrayOutputStream(height * (width * 4 + 1));
        for (int y = 0; y < height; y++) {
            rawScanlines.write(0); // Filter type 0 (None)
            int rowOffset = y * width;
            for (int x = 0; x < width; x++) {
                int argb = argbPixels[rowOffset + x];
                rawScanlines.write((argb >>> 16) & 0xFF); // R
                rawScanlines.write((argb >>> 8) & 0xFF);  // G
                rawScanlines.write(argb & 0xFF);          // B
                rawScanlines.write((argb >>> 24) & 0xFF); // A
            }
        }

        byte[] uncompressed = rawScanlines.toByteArray();
        Deflater deflater = new Deflater(Deflater.DEFAULT_COMPRESSION);
        deflater.setInput(uncompressed);
        deflater.finish();

        ByteArrayOutputStream compressed = new ByteArrayOutputStream(uncompressed.length / 2 + 64);
        byte[] buf = new byte[8192];
        while (!deflater.finished()) {
            int n = deflater.deflate(buf);
            if (n > 0) {
                compressed.write(buf, 0, n);
            } else {
                break;
            }
        }
        deflater.end();

        ByteArrayOutputStream png = new ByteArrayOutputStream(compressed.size() + 128);
        // PNG 8-byte signature
        png.write(new byte[]{
            (byte) 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
        });

        // IHDR chunk
        ByteArrayOutputStream ihdr = new ByteArrayOutputStream(13);
        DataOutputStream ihdrOut = new DataOutputStream(ihdr);
        ihdrOut.writeInt(width);
        ihdrOut.writeInt(height);
        ihdrOut.writeByte(8); // bit depth
        ihdrOut.writeByte(6); // color type 6 = RGBA
        ihdrOut.writeByte(0); // compression
        ihdrOut.writeByte(0); // filter
        ihdrOut.writeByte(0); // interlace
        ihdrOut.flush();
        writePngChunk(png, "IHDR", ihdr.toByteArray());

        // IDAT chunk
        writePngChunk(png, "IDAT", compressed.toByteArray());

        // IEND chunk
        writePngChunk(png, "IEND", new byte[0]);

        return png.toByteArray();
    }

    private static void writePngChunk(ByteArrayOutputStream out, String type, byte[] data)
            throws IOException {
        byte[] typeBytes = type.getBytes(StandardCharsets.US_ASCII);
        DataOutputStream dos = new DataOutputStream(out);
        dos.writeInt(data.length);
        dos.write(typeBytes);
        dos.write(data);

        CRC32 crc = new CRC32();
        crc.update(typeBytes);
        crc.update(data);
        dos.writeInt((int) crc.getValue());
        dos.flush();
    }

    static boolean insideRoundedRect(double px, double py,
                                     double left, double top, double w, double h, double r) {
        if (px < left || px > left + w || py < top || py > top + h) {
            return false;
        }
        double right = left + w;
        double bottom = top + h;
        double cr = Math.max(0.0, Math.min(r, Math.min(w, h) * 0.5));
        if (px < left + cr && py < top + cr) {
            return Math.hypot(px - (left + cr), py - (top + cr)) <= cr;
        }
        if (px > right - cr && py < top + cr) {
            return Math.hypot(px - (right - cr), py - (top + cr)) <= cr;
        }
        if (px < left + cr && py > bottom - cr) {
            return Math.hypot(px - (left + cr), py - (bottom - cr)) <= cr;
        }
        if (px > right - cr && py > bottom - cr) {
            return Math.hypot(px - (right - cr), py - (bottom - cr)) <= cr;
        }
        return true;
    }

    static boolean insideRegularOctagon(double px, double py, double cx, double cy, double radius) {
        double dx = Math.abs(px - cx);
        double dy = Math.abs(py - cy);
        double apothem = radius * Math.cos(PI / 8.0);
        if (dx > apothem || dy > apothem) {
            return false;
        }
        return (dx + dy) <= apothem * 1.41421356237;
    }

    static void fillOctagonFlat(int[] pixels, int w, int h,
                                double cx, double cy, double radius, int argb) {
        int minX = Math.max(0, (int) Math.floor(cx - radius - 1));
        int maxX = Math.min(w - 1, (int) Math.ceil(cx + radius + 1));
        int minY = Math.max(0, (int) Math.floor(cy - radius - 1));
        int maxY = Math.min(h - 1, (int) Math.ceil(cy + radius + 1));
        double alpha = ((argb >>> 24) & 0xFF) / 255.0;
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                if (insideRegularOctagon(x + 0.5, y + 0.5, cx, cy, radius)) {
                    pixels[y * w + x] = blendColor(pixels[y * w + x], argb | 0xFF000000, alpha);
                }
            }
        }
    }

    static void fillOctagonGradient(int[] pixels, int w, int h,
                                    double cx, double cy, double radius,
                                    int cTopLeft, int cMid, int cBotRight) {
        int minX = Math.max(0, (int) Math.floor(cx - radius - 1));
        int maxX = Math.min(w - 1, (int) Math.ceil(cx + radius + 1));
        int minY = Math.max(0, (int) Math.floor(cy - radius - 1));
        int maxY = Math.min(h - 1, (int) Math.ceil(cy + radius + 1));
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double px = x + 0.5;
                double py = y + 0.5;
                if (insideRegularOctagon(px, py, cx, cy, radius)) {
                    double t = ((px - (cx - radius)) + (py - (cy - radius))) / (4.0 * radius);
                    t = Math.max(0.0, Math.min(1.0, t));
                    int col = (t < 0.45)
                            ? lerpColor(cTopLeft, cMid, t / 0.45)
                            : lerpColor(cMid, cBotRight, (t - 0.45) / 0.55);
                    pixels[y * w + x] = col;
                }
            }
        }
    }

    static void fillOctagonRadial(int[] pixels, int w, int h,
                                  double focalX, double focalY, double radius,
                                  int cCenter, int cMid, int cEdge) {
        double cx = w * 0.5;
        double cy = h * 0.5;
        int minX = Math.max(0, (int) Math.floor(cx - radius - 2));
        int maxX = Math.min(w - 1, (int) Math.ceil(cx + radius + 2));
        int minY = Math.max(0, (int) Math.floor(cy - radius - 2));
        int maxY = Math.min(h - 1, (int) Math.ceil(cy + radius + 2));
        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double px = x + 0.5;
                double py = y + 0.5;
                if (insideRegularOctagon(px, py, cx, cy, radius)) {
                    double t = Math.min(1.0, Math.hypot(px - focalX, py - focalY) / (radius * 1.25));
                    int col = (t < 0.50)
                            ? lerpColor(cCenter, cMid, t / 0.50)
                            : lerpColor(cMid, cEdge, (t - 0.50) / 0.50);
                    pixels[y * w + x] = col;
                }
            }
        }
    }

    static void drawOctagonStroke(int[] pixels, int w, int h,
                                  double cx, double cy, double radius,
                                  double strokeWidth, int argb) {
        double[] vx = new double[8];
        double[] vy = new double[8];
        for (int i = 0; i < 8; i++) {
            double ang = (i * 45.0 + 22.5) * PI / 180.0;
            vx[i] = cx + radius * Math.cos(ang);
            vy[i] = cy + radius * Math.sin(ang);
        }
        drawPolygonOutline(pixels, w, h, vx, vy, strokeWidth, argb);
    }

    static boolean pointInPolygon(double px, double py, double[] vx, double[] vy) {
        boolean inside = false;
        int n = vx.length;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            if (((vy[i] > py) != (vy[j] > py))
                    && (px < (vx[j] - vx[i]) * (py - vy[i]) / (vy[j] - vy[i] + 1e-12) + vx[i])) {
                inside = !inside;
            }
        }
        return inside;
    }

    static void fillPolygonShaded(int[] pixels, int w, int h,
                                  double[] vx, double[] vy,
                                  int cLight, int cMid, int cDark) {
        double minX = vx[0], maxX = vx[0], minY = vy[0], maxY = vy[0];
        for (int i = 1; i < vx.length; i++) {
            minX = Math.min(minX, vx[i]);
            maxX = Math.max(maxX, vx[i]);
            minY = Math.min(minY, vy[i]);
            maxY = Math.max(maxY, vy[i]);
        }
        int x0 = Math.max(0, (int) Math.floor(minX));
        int x1 = Math.min(w - 1, (int) Math.ceil(maxX));
        int y0 = Math.max(0, (int) Math.floor(minY));
        int y1 = Math.min(h - 1, (int) Math.ceil(maxY));
        double span = Math.max(1.0, (maxX - minX) + (maxY - minY));

        for (int y = y0; y <= y1; y++) {
            for (int x = x0; x <= x1; x++) {
                double px = x + 0.5;
                double py = y + 0.5;
                if (pointInPolygon(px, py, vx, vy)) {
                    double t = Math.max(0.0, Math.min(1.0, ((px - minX) + (py - minY)) / span));
                    int col = (t < 0.5)
                            ? lerpColor(cLight, cMid, t * 2.0)
                            : lerpColor(cMid, cDark, (t - 0.5) * 2.0);
                    pixels[y * w + x] = col;
                }
            }
        }
    }

    static void drawPolygonOutline(int[] pixels, int w, int h,
                                   double[] vx, double[] vy,
                                   double thickness, int argb) {
        for (int i = 0; i < vx.length; i++) {
            int next = (i + 1) % vx.length;
            drawLineSegment(pixels, w, h, vx[i], vy[i], vx[next], vy[next], thickness, argb);
        }
    }

    static void drawLineSegment(int[] pixels, int w, int h,
                                double x0, double y0, double x1, double y1,
                                double thickness, int argb) {
        double halfT = Math.max(0.5, thickness * 0.5);
        int minX = Math.max(0, (int) Math.floor(Math.min(x0, x1) - halfT - 1));
        int maxX = Math.min(w - 1, (int) Math.ceil(Math.max(x0, x1) + halfT + 1));
        int minY = Math.max(0, (int) Math.floor(Math.min(y0, y1) - halfT - 1));
        int maxY = Math.min(h - 1, (int) Math.ceil(Math.max(y0, y1) + halfT + 1));

        double dx = x1 - x0;
        double dy = y1 - y0;
        double lenSq = dx * dx + dy * dy;
        double baseAlpha = ((argb >>> 24) & 0xFF) / 255.0;
        int solid = argb | 0xFF000000;

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double px = x + 0.5;
                double py = y + 0.5;
                double t = (lenSq > 1e-9)
                        ? Math.max(0.0, Math.min(1.0, ((px - x0) * dx + (py - y0) * dy) / lenSq))
                        : 0.0;
                double dist = Math.hypot(px - (x0 + t * dx), py - (y0 + t * dy));
                if (dist <= halfT + 0.5) {
                    double cov = Math.max(0.0, Math.min(1.0, halfT + 0.5 - dist));
                    pixels[y * w + x] = blendColor(pixels[y * w + x], solid, baseAlpha * cov);
                }
            }
        }
    }

    static void fillCircleAlpha(int[] pixels, int w, int h,
                                double cx, double cy, double r, int argb) {
        int minX = Math.max(0, (int) Math.floor(cx - r - 1));
        int maxX = Math.min(w - 1, (int) Math.ceil(cx + r + 1));
        int minY = Math.max(0, (int) Math.floor(cy - r - 1));
        int maxY = Math.min(h - 1, (int) Math.ceil(cy + r + 1));
        double baseAlpha = ((argb >>> 24) & 0xFF) / 255.0;
        int solid = argb | 0xFF000000;

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double dist = Math.hypot((x + 0.5) - cx, (y + 0.5) - cy);
                if (dist <= r + 0.5) {
                    double cov = Math.max(0.0, Math.min(1.0, r + 0.5 - dist));
                    pixels[y * w + x] = blendColor(pixels[y * w + x], solid, baseAlpha * cov);
                }
            }
        }
    }

    static void drawCircleStroke(int[] pixels, int w, int h,
                                 double cx, double cy, double r,
                                 double thickness, int argb) {
        double halfT = Math.max(0.5, thickness * 0.5);
        int minX = Math.max(0, (int) Math.floor(cx - r - halfT - 1));
        int maxX = Math.min(w - 1, (int) Math.ceil(cx + r + halfT + 1));
        int minY = Math.max(0, (int) Math.floor(cy - r - halfT - 1));
        int maxY = Math.min(h - 1, (int) Math.ceil(cy + r + halfT + 1));
        double baseAlpha = ((argb >>> 24) & 0xFF) / 255.0;
        int solid = argb | 0xFF000000;

        for (int y = minY; y <= maxY; y++) {
            for (int x = minX; x <= maxX; x++) {
                double dist = Math.abs(Math.hypot((x + 0.5) - cx, (y + 0.5) - cy) - r);
                if (dist <= halfT + 0.5) {
                    double cov = Math.max(0.0, Math.min(1.0, halfT + 0.5 - dist));
                    pixels[y * w + x] = blendColor(pixels[y * w + x], solid, baseAlpha * cov);
                }
            }
        }
    }

    static int lerpColor(int c0, int c1, double t) {
        double u = Math.max(0.0, Math.min(1.0, t));
        int a0 = (c0 >>> 24) & 0xFF;
        int r0 = (c0 >>> 16) & 0xFF;
        int g0 = (c0 >>> 8) & 0xFF;
        int b0 = c0 & 0xFF;

        int a1 = (c1 >>> 24) & 0xFF;
        int r1 = (c1 >>> 16) & 0xFF;
        int g1 = (c1 >>> 8) & 0xFF;
        int b1 = c1 & 0xFF;

        int a = (int) Math.round(a0 + (a1 - a0) * u);
        int r = (int) Math.round(r0 + (r1 - r0) * u);
        int g = (int) Math.round(g0 + (g1 - g0) * u);
        int b = (int) Math.round(b0 + (b1 - b0) * u);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    static int blendColor(int dst, int srcRgb, double alpha) {
        double a = Math.max(0.0, Math.min(1.0, alpha));
        int r0 = (dst >>> 16) & 0xFF;
        int g0 = (dst >>> 8) & 0xFF;
        int b0 = dst & 0xFF;

        int r1 = (srcRgb >>> 16) & 0xFF;
        int g1 = (srcRgb >>> 8) & 0xFF;
        int b1 = srcRgb & 0xFF;

        int r = (int) Math.round(r0 * (1.0 - a) + r1 * a);
        int g = (int) Math.round(g0 * (1.0 - a) + g1 * a);
        int b = (int) Math.round(b0 * (1.0 - a) + b1 * a);
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }
}
