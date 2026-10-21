/*
 * OctiPlayer.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiPlayer {
    PLAYER_UNDEF(0),
    PLAYER_A(1),
    PLAYER_B(2),
    PLAYER_C(3),
    PLAYER_D(4);

    private final int value;

    OctiPlayer(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiPlayer fromValue(int val) {
        switch (val) {
            case 1:
                return PLAYER_A;
            case 2:
                return PLAYER_B;
            case 3:
                return PLAYER_C;
            case 4:
                return PLAYER_D;
            default:
                return PLAYER_UNDEF;
        }
    }

    public OctiPodType toPodType() {
        switch (this) {
            case PLAYER_A:
                return OctiPodType.A_POD;
            case PLAYER_B:
                return OctiPodType.B_POD;
            case PLAYER_C:
                return OctiPodType.C_POD;
            case PLAYER_D:
                return OctiPodType.D_POD;
            default:
                return OctiPodType.POD_UNDEF;
        }
    }

    public static boolean areAllied(OctiGameMode mode, OctiPlayer p1, OctiPlayer p2) {
        if (p1 == null || p2 == null || p1 == PLAYER_UNDEF || p2 == PLAYER_UNDEF) {
            return false;
        }
        if (p1 == p2) {
            return true;
        }
        if (mode == OctiGameMode.GAME_MODE_4) {
            if ((p1 == PLAYER_A && p2 == PLAYER_C) || (p1 == PLAYER_C && p2 == PLAYER_A)) {
                return true;
            }
            if ((p1 == PLAYER_B && p2 == PLAYER_D) || (p1 == PLAYER_D && p2 == PLAYER_B)) {
                return true;
            }
        }
        return false;
    }
}
