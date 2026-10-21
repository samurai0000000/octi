/*
 * OctiGameMode.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiGameMode {
    GAME_MODE_UNDEF(0),
    GAME_MODE_2(1),
    GAME_MODE_4(2),
    GAME_MODE_OFK(3);

    private final int value;

    OctiGameMode(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiGameMode fromValue(int val) {
        switch (val) {
            case 1:
                return GAME_MODE_2;
            case 2:
                return GAME_MODE_4;
            case 3:
                return GAME_MODE_OFK;
            default:
                return GAME_MODE_UNDEF;
        }
    }
}
