/*
 * OctiOpeningBookData.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public final class OctiOpeningBookData {

    private OctiOpeningBookData() {
    }

    public static final OctiBotEngine.OpeningBookEntry[] ENTRIES =
            new OctiBotEngine.OpeningBookEntry[] {
        // 2P 1-Base ply 1 (after 33+b / 73+h)
        new OctiBotEngine.OpeningBookEntry(0x0B45FE3410182A4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 12),
            new OctiBotEngine.OpeningBookCandidate("77+f", 8),
            new OctiBotEngine.OpeningBookCandidate("37+d", 8)),
        // 2P 3-Base ply 1 (after 33+b / 73+h)
        new OctiBotEngine.OpeningBookEntry(0x1636F05EC19C04C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 12),
            new OctiBotEngine.OpeningBookCandidate("77+f", 8),
            new OctiBotEngine.OpeningBookCandidate("37+d", 8)),
        // 2P 1-Base ply 0 (A to move)
        new OctiBotEngine.OpeningBookEntry(0x26BE80163B6578BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 55),
            new OctiBotEngine.OpeningBookCandidate("33+b", 48),
            new OctiBotEngine.OpeningBookCandidate("73+h", 48)),
        // 2P 3-Base ply 0 (A to move)
        new OctiBotEngine.OpeningBookEntry(0x3BCD8E7CEAE1563EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 45),
            new OctiBotEngine.OpeningBookCandidate("33+b", 40),
            new OctiBotEngine.OpeningBookCandidate("73+h", 40)),
        // OFK ply 1 (after 22+b / 52+h)
        new OctiBotEngine.OpeningBookEntry(0x3E7C56E08875ECA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", 12),
            new OctiBotEngine.OpeningBookCandidate("36+e", 12),
            new OctiBotEngine.OpeningBookCandidate("56+f", 8)),
        // 2P 3-Base ply 1 (after 53+b / 53+h)
        new OctiBotEngine.OpeningBookEntry(0x42F7FBED36E9148FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 12),
            new OctiBotEngine.OpeningBookCandidate("37+d", 8),
            new OctiBotEngine.OpeningBookCandidate("77+f", 8)),
        // 4P ply 0 (A to move)
        new OctiBotEngine.OpeningBookEntry(0x5C3D5244B85BC3A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 40),
            new OctiBotEngine.OpeningBookCandidate("33+b", 35),
            new OctiBotEngine.OpeningBookCandidate("33+c", 25)),
        // 2P 3-Base ply 1 (after 33+a / 73+a)
        new OctiBotEngine.OpeningBookEntry(0x62F499414419B482L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 15),
            new OctiBotEngine.OpeningBookCandidate("77+e", 10),
            new OctiBotEngine.OpeningBookCandidate("37+d", 8)),
        // OFK ply 1 (after 32+a / 42+a)
        new OctiBotEngine.OpeningBookEntry(0x97220033605ECA2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", 10),
            new OctiBotEngine.OpeningBookCandidate("36+e", 10),
            new OctiBotEngine.OpeningBookCandidate("56+f", 5)),
        // OFK ply 0 (A to move)
        new OctiBotEngine.OpeningBookEntry(0xBF20205531084330L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 50),
            new OctiBotEngine.OpeningBookCandidate("42+a", 50),
            new OctiBotEngine.OpeningBookCandidate("22+b", 42)),
        // 2P 3-Base ply 1 (after 53+a)
        new OctiBotEngine.OpeningBookEntry(0xE61535559B2C9569L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 10),
            new OctiBotEngine.OpeningBookCandidate("37+d", 5),
            new OctiBotEngine.OpeningBookCandidate("77+f", 5)),
        // 2P 1-Base ply 1 (after 53+a)
        new OctiBotEngine.OpeningBookEntry(0xFB663B3F4AA8BBECL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", 10),
            new OctiBotEngine.OpeningBookCandidate("37+d", 5),
            new OctiBotEngine.OpeningBookCandidate("77+f", 5))
    };
}
