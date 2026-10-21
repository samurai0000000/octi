/*
 * OctiPodType.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiPodType {
    POD_UNDEF(0),
    A_POD(1),
    B_POD(2),
    C_POD(3),
    D_POD(4);

    private final int value;

    OctiPodType(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiPodType fromValue(int val) {
        switch (val) {
            case 1:
                return A_POD;
            case 2:
                return B_POD;
            case 3:
                return C_POD;
            case 4:
                return D_POD;
            default:
                return POD_UNDEF;
        }
    }

    public OctiPlayer toPlayer() {
        switch (this) {
            case A_POD:
                return OctiPlayer.PLAYER_A;
            case B_POD:
                return OctiPlayer.PLAYER_B;
            case C_POD:
                return OctiPlayer.PLAYER_C;
            case D_POD:
                return OctiPlayer.PLAYER_D;
            default:
                return OctiPlayer.PLAYER_UNDEF;
        }
    }

    public static boolean areAllied(OctiGameMode mode, OctiPodType t1, OctiPodType t2) {
        if (t1 == null || t2 == null) {
            return false;
        }
        return OctiPlayer.areAllied(mode, t1.toPlayer(), t2.toPlayer());
    }
}
