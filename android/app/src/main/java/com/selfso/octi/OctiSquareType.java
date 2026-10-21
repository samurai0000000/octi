/*
 * OctiSquareType.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiSquareType {
    NEUTRAL_SQUARE(0),
    A_HOME_SQUARE(1),
    B_HOME_SQUARE(2),
    C_HOME_SQUARE(3),
    D_HOME_SQUARE(4),
    NEUTRAL_OCTI_SQUARE(5);

    private final int value;

    OctiSquareType(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiSquareType fromValue(int val) {
        switch (val) {
            case 1:
                return A_HOME_SQUARE;
            case 2:
                return B_HOME_SQUARE;
            case 3:
                return C_HOME_SQUARE;
            case 4:
                return D_HOME_SQUARE;
            case 5:
                return NEUTRAL_OCTI_SQUARE;
            default:
                return NEUTRAL_SQUARE;
        }
    }
}
