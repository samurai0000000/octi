/*
 * OctiDir.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiDir {
    DIR_UNDEF(-1),
    DIR_N(0),
    DIR_NE(1),
    DIR_E(2),
    DIR_SE(3),
    DIR_S(4),
    DIR_SW(5),
    DIR_W(6),
    DIR_NW(7);

    public static final int DIR_MAX = 8;

    private final int index;

    OctiDir(int index) {
        this.index = index;
    }

    public int getIndex() {
        return index;
    }

    public static OctiDir fromIndex(int idx) {
        switch (idx) {
            case 0:
                return DIR_N;
            case 1:
                return DIR_NE;
            case 2:
                return DIR_E;
            case 3:
                return DIR_SE;
            case 4:
                return DIR_S;
            case 5:
                return DIR_SW;
            case 6:
                return DIR_W;
            case 7:
                return DIR_NW;
            default:
                return DIR_UNDEF;
        }
    }

    public static OctiDir charToDir(char c) {
        if (c >= 'A' && c <= 'H') {
            return fromIndex(c - 'A');
        }
        if (c >= 'a' && c <= 'h') {
            return fromIndex(c - 'a');
        }
        return DIR_UNDEF;
    }

    public char toChar() {
        if (index >= 0 && index < DIR_MAX) {
            return (char) ('a' + index);
        }
        return '?';
    }

    public int deltaX() {
        switch (this) {
            case DIR_NE:
            case DIR_E:
            case DIR_SE:
                return 1;
            case DIR_SW:
            case DIR_W:
            case DIR_NW:
                return -1;
            case DIR_N:
            case DIR_S:
            default:
                return 0;
        }
    }

    public int deltaY() {
        switch (this) {
            case DIR_NW:
            case DIR_N:
            case DIR_NE:
                return 1;
            case DIR_SE:
            case DIR_S:
            case DIR_SW:
                return -1;
            case DIR_E:
            case DIR_W:
            default:
                return 0;
        }
    }

    public static OctiDir deltaToDir(int dx, int dy) {
        if (dx == 0 && dy == 1) {
            return DIR_N;
        }
        if (dx == 1 && dy == 1) {
            return DIR_NE;
        }
        if (dx == 1 && dy == 0) {
            return DIR_E;
        }
        if (dx == 1 && dy == -1) {
            return DIR_SE;
        }
        if (dx == 0 && dy == -1) {
            return DIR_S;
        }
        if (dx == -1 && dy == -1) {
            return DIR_SW;
        }
        if (dx == -1 && dy == 0) {
            return DIR_W;
        }
        if (dx == -1 && dy == 1) {
            return DIR_NW;
        }
        return DIR_UNDEF;
    }
}
