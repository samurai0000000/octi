/*
 * OctiGameBases.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiGameBases {
    GAME_BASES_1(1),
    GAME_BASES_3(3);

    private final int value;

    OctiGameBases(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiGameBases fromValue(int val) {
        switch (val) {
            case 1:
                return GAME_BASES_1;
            case 3:
                return GAME_BASES_3;
            default:
                return GAME_BASES_3;
        }
    }
}
