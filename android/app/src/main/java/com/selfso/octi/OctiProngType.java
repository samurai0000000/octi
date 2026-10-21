/*
 * OctiProngType.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiProngType {
    NO_PRONG(0),
    NORMAL_PRONG(1),
    SUPER_PRONG(2);

    private final int value;

    OctiProngType(int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }

    public static OctiProngType fromValue(int val) {
        switch (val) {
            case 1:
                return NORMAL_PRONG;
            case 2:
                return SUPER_PRONG;
            default:
                return NO_PRONG;
        }
    }
}
