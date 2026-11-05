/*
 * OctiOpeningBookData.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public final class OctiOpeningBookData {

    private OctiOpeningBookData() {
    }

    public static final OctiBotEngine.OpeningBookEntry[] ENTRIES = buildEntries();

    private static OctiBotEngine.OpeningBookEntry[] buildEntries() {
        OctiBotEngine.OpeningBookEntry[] c0 = chunk0();
        OctiBotEngine.OpeningBookEntry[] c1 = chunk1();
        OctiBotEngine.OpeningBookEntry[] c2 = chunk2();
        OctiBotEngine.OpeningBookEntry[] c3 = chunk3();
        OctiBotEngine.OpeningBookEntry[] all =
                new OctiBotEngine.OpeningBookEntry[c0.length + c1.length + c2.length + c3.length];
        int pos = 0;
        System.arraycopy(c0, 0, all, pos, c0.length);
        pos += c0.length;
        System.arraycopy(c1, 0, all, pos, c1.length);
        pos += c1.length;
        System.arraycopy(c2, 0, all, pos, c2.length);
        pos += c2.length;
        System.arraycopy(c3, 0, all, pos, c3.length);
        pos += c3.length;
        return all;
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk0() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // OFK ply 3 (after 22+b 56+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0x00172E8C050D20EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -4483),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("36+f", -5052)),
        // OFK ply 4 (after 32+a 26+d 22+c 26d-35)
        new OctiBotEngine.OpeningBookEntry(0x002920C091653AD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", 443),
            new OctiBotEngine.OpeningBookCandidate("32+b", -239),
            new OctiBotEngine.OpeningBookCandidate("32+a", -980)),
        // OFK ply 2 (after 32+b 46+f)
        new OctiBotEngine.OpeningBookEntry(0x004C97ACE487F4A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -28),
            new OctiBotEngine.OpeningBookCandidate("42+a", -28),
            new OctiBotEngine.OpeningBookCandidate("32b+e", -180)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x005F3329D7449A0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x00697FA1E33C8D2EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x007F3607973EA85CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -610)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x00880FB151FECD8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x00A4D38453FED25EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+b 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x00CA09C78AFE19B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -400)),
        // 2P 1-Base ply 3 (after 33+b 57+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x00E070DC835A080BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2980),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x00F5D335026032DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 3-Base ply 4 (after 33+a 37+d 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x01012D3ED6339499L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x0120B090FBB8397EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x0125C4D38C928CA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -1090)),
        // OFK ply 4 (after 32+a 46+f 22+c 46f+d)
        new OctiBotEngine.OpeningBookEntry(0x0133151A7F82671EL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 39),
            new OctiBotEngine.OpeningBookCandidate("32+a", 39),
            new OctiBotEngine.OpeningBookCandidate("42a+h", 39)),
        // 4P ply 4 (after 33+d 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0147C7CCD1438649L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 3 (after 53+a 57+c 33+b)
        new OctiBotEngine.OpeningBookEntry(0x014C92CB7EAFBC8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -2900),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000)),
        // 4P ply 4 (after 33+g 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x014D09532C5A8006L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 2 (after 33+h 37+b)
        new OctiBotEngine.OpeningBookEntry(0x017504839F2D16FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // OFK ply 4 (after 32+b 56+f 42+a 36+g)
        new OctiBotEngine.OpeningBookEntry(0x01A62CFEC767626DL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 2440),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 2320),
            new OctiBotEngine.OpeningBookCandidate("52+g", 1931)),
        // 2P 3-Base ply 3 (after 33+h 77+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x01D6C93A22AC5CE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x020378A0DE391EFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -1050)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x02210714DFF08D51L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -730)),
        // 2P 1-Base ply 4 (after 53+b 77+f 53b-64 57+f)
        new OctiBotEngine.OpeningBookEntry(0x022953EEBF673C16L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 270),
            new OctiBotEngine.OpeningBookCandidate("64b+a", -470),
            new OctiBotEngine.OpeningBookCandidate("33+a", -470)),
        // 4P ply 1 (after 33+d)
        new OctiBotEngine.OpeningBookEntry(0x0257AC58B9AAA876L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1116),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1116)),
        // OFK ply 3 (after 22+b 46+f 52+h)
        new OctiBotEngine.OpeningBookEntry(0x0259A90DB5523446L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3382),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3382),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -3382)),
        // OFK ply 4 (after 32+b 36+e 32b-43 26+a)
        new OctiBotEngine.OpeningBookEntry(0x026C8E2E3D43CC70L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 1101),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 1101),
            new OctiBotEngine.OpeningBookCandidate("22+b", 260)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+a 77+d)
        new OctiBotEngine.OpeningBookEntry(0x02774F7C47B2791BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 462),
            new OctiBotEngine.OpeningBookCandidate("73+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 462)),
        // OFK ply 4 (after 32+b 46+f 32b+a 46f-35)
        new OctiBotEngine.OpeningBookEntry(0x0290A14530580774L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -688),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -688),
            new OctiBotEngine.OpeningBookCandidate("32ab+h", -1308)),
        // 2P 1-Base ply 4 (after 53+b 57+e 33+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x02B4B49D0EBEEEEEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -630)),
        // OFK ply 3 (after 32+b 46+f 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x02B8C043D5753CC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4483),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -4483)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x02BEE95E70EBF5DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x02C30BE9965279BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 3 (after 32+a 56+f 42+c)
        new OctiBotEngine.OpeningBookEntry(0x02EAB3D9C48784AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("46+f", -4663)),
        // 2P 3-Base ply 4 (after 53+b 77+e 53b+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x03468309F7C436A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // OFK ply 4 (after 32+a 46+e 42+e 56+f)
        new OctiBotEngine.OpeningBookEntry(0x034E54324475DD27L, 8,
            new OctiBotEngine.OpeningBookCandidate("42e+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("22+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("22+c", -3107)),
        // 2P 3-Base ply 3 (after 33+h 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0355E97F5BD7727CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 4P ply 3 (after 33+h 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x037E9EEA20C56F45L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 32+b 46+e 32b+c 56+f)
        new OctiBotEngine.OpeningBookEntry(0x0388F8D5EB971353L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -3398),
            new OctiBotEngine.OpeningBookCandidate("32+b", -3398),
            new OctiBotEngine.OpeningBookCandidate("22+b", -3782)),
        // 2P 1-Base ply 3 (after 53+c 57+c 73+h)
        new OctiBotEngine.OpeningBookEntry(0x03918249B07C3A9AL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("57c+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2480)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x039A9FACBF8DF8F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 1-Base ply 3 (after 53+b 57+f 53)
        new OctiBotEngine.OpeningBookEntry(0x039B09895876D2BFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2630),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2630),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2630)),
        // 4P ply 4 (after 33+d 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x03B9904D87FF6677L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x03C5A29871D995BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x03D61B0527E11558L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x03DFDFA634A22CC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", -590),
            new OctiBotEngine.OpeningBookCandidate("33+b", -740),
            new OctiBotEngine.OpeningBookCandidate("33+a", -1020)),
        // 2P 1-Base ply 3 (after 53+b 77+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x03E4B31BD03809FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+e", -3090)),
        // OFK ply 4 (after 22+b 46+f 42+h 46f+c)
        new OctiBotEngine.OpeningBookEntry(0x03E8AB7F773876C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 2418),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 2418),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 2418)),
        // OFK ply 4 (after 22+b 46+f 32+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x04128976B567D329L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 640),
            new OctiBotEngine.OpeningBookCandidate("42+b", -2741),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3515)),
        // 2P 1-Base ply 4 (after 53+b 57+f 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x043D09917B2DA6EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -210),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -470)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x044E141885672FCDL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0)),
        // 2P 1-Base ply 3 (after 53+a 57+e 33+b)
        new OctiBotEngine.OpeningBookEntry(0x04527352171E2C87L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2480)),
        // 2P 3-Base ply 3 (after 33+b 77+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x0454E50B4C76A717L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2024)),
        // 2P 3-Base ply 3 (after 33+a 57+d 33a+b)
        new OctiBotEngine.OpeningBookEntry(0x0473EEFC3A958160L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x04835EC9CEA89AF8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x04A643D630066E16L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+a 77+e 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x04B7ECA3EE733C69L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 2P 3-Base ply 2 (after 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x04E083C8E83B4C36L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // OFK ply 4 (after 22+b 46+f 22b+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x04EB190AD2D0322CL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+b", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+g", -3579)),
        // OFK ply 3 (after 32+a 46+e 42+e)
        new OctiBotEngine.OpeningBookEntry(0x04EBD775A8CC9A5DL, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // 4P ply 3 (after 33+g 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x04F34FB6C1541313L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+c 37+e)
        new OctiBotEngine.OpeningBookEntry(0x0504B1C0BD1296ADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -400)),
        // 2P 1-Base ply 3 (after 53+b 77+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x05111EA2658B8F48L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3930),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3930),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3930)),
        // OFK ply 3 (after 32+b 26+d 32b+e)
        new OctiBotEngine.OpeningBookEntry(0x055892E82B1AB81EL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // OFK ply 4 (after 32+b 46+e 32b-43 36+g)
        new OctiBotEngine.OpeningBookEntry(0x05718DBBCC76AF31L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 1024),
            new OctiBotEngine.OpeningBookCandidate("52+h", 1024),
            new OctiBotEngine.OpeningBookCandidate("52+g", 992)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x05B2BD47A788746AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 62)),
        // 2P 1-Base ply 3 (after 53+c 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x05B8CA0566580C40L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2080)),
        // 2P 3-Base ply 3 (after 33+h 37+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x05D30689666F8876L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2176)),
        // 4P ply 3 (after 33+d 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x05DDD26B93DEEBE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -996),
            new OctiBotEngine.OpeningBookCandidate("73+e", -996),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1116)),
        // 2P 1-Base ply 4 (after 53+b 57+d 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x05E1AE9791B694D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120)),
        // 4P ply 3 (after 33+c 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x05FAF58C429264C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+c 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x06596697A845F2E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 1-Base ply 3 (after 33+b 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0667EC41F1FA77B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3490),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3490),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -3750)),
        // 4P ply 4 (after 33+h 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0674AA5FE4E5881DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 3 (after 53+c 57+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x068F63D0D9CDAA97L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x06A630444831673FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 3-Base ply 3 (after 33+b 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x06AB8A3D836CB2AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282)),
        // 2P 1-Base ply 3 (after 53+c 57+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x06BC09C2353A0DB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2830),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2840),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2950)),
        // 4P ply 4 (after 33+d 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x06CA33205027098DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+h 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x06E56D50C2D49525L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120)),
        // 4P ply 3 (after 33+d 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x070A449400099313L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1256)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+h 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x07300EF53B0C9E81L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // 2P 1-Base ply 4 (after 33+b 77+f 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x0739CA56284FA71AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 180),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0750DCAD2B838535L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x075E953844CA1B98L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 57+c 53b+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x07768DBBDCC4C11AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 2P 3-Base ply 4 (after 33+b 77+e 33b+a 77e+f)
        new OctiBotEngine.OpeningBookEntry(0x07B32F64BD113D9CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+h 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x07BEC4A14E367B7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 2P 3-Base ply 4 (after 33+h 77+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x07D3D4B66322C1C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -310)),
        // 2P 1-Base ply 3 (after 53+b 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x0800DCAB8E29F26CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f+d", -3500),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3500),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3600)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x08157F420F13C8BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x08322B730FB11B54L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62)),
        // 4P ply 4 (after 33+h 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x083B7CE279582DEDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x084E3BC6CF91D5E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 2P 3-Base ply 3 (after 53+a 57+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x086484BC5AE95F06L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -1686)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+h 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x086AD22B9530CDDEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73h+a", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -520)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+h 37+f)
        new OctiBotEngine.OpeningBookEntry(0x0875640C7BC7752EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+a 57c+f)
        new OctiBotEngine.OpeningBookEntry(0x08825DBABD0710FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 390)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0885E59DCD9AAC7DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 4 (after 33+c 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x08A2C27A1CD62359L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+c 77+f)
        new OctiBotEngine.OpeningBookEntry(0x08B563CD4620511AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1010),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1050),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -1550)),
        // 2P 1-Base ply 4 (after 53+b 57+d 33+a 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x08BC1CCC69B857CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x08D17D1DDBF575B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2402)),
        // 4P ply 3 (after 33+g 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x08DF7DA4E610BA5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+h 57+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x08E590B18D6F26BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // OFK ply 2 (after 32+b 36+d)
        new OctiBotEngine.OpeningBookEntry(0x08EE9C0681127B36L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -620)),
        // 2P 3-Base ply 3 (after 33+b 77+e 33b+a)
        new OctiBotEngine.OpeningBookEntry(0x09622CF5D137FC44L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2086)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x09B3016E08814A3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x09DA63262299475AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 4P ply 4 (after 33+g 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x09F137ACB37E2D19L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0A07A8D7A8FBC63AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+g 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x0A08EB5B75C7C2AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // OFK ply 3 (after 32+b 36+d 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x0A1ACBE9B0E0B357L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3801),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -3801),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4243)),
        // OFK ply 2 (after 32+b 36+e)
        new OctiBotEngine.OpeningBookEntry(0x0A4892853E575C29L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -28),
            new OctiBotEngine.OpeningBookCandidate("42+a", -28),
            new OctiBotEngine.OpeningBookCandidate("32b+e", -180)),
        // 2P 3-Base ply 3 (after 33+a 77+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0A525D58A28E5B20L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2086)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0A7554858F015BACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+d 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0A7BB21C9B264C43L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0A81DCF8C03E5936L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1180)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+b 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x0AB7C2A95BE34BCEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 2P 3-Base ply 3 (after 53+a 37+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0x0AD17CC92493F83DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086)),
        // 2P 1-Base ply 3 (after 53+b 77+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x0AD8B86A37D0C1A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+e", -3090)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x0ADA987C00BC4468L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -860),
            new OctiBotEngine.OpeningBookCandidate("33+b", -860),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1010)),
        // 2P 3-Base ply 4 (after 33+h 57+e 33h+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0B025ADE7C08BC30L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+b", -62)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x0B250EEF7CAA6FD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x0B36B7722A92EF3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 1-Base ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0x0B45FE3410182A4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3100),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3350),
            new OctiBotEngine.OpeningBookCandidate("57+e", -3350)),
        // 2P 3-Base ply 4 (after 33+a 37+d 33a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0B4AF8019CF3D416L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -138),
            new OctiBotEngine.OpeningBookCandidate("53+b", -138),
            new OctiBotEngine.OpeningBookCandidate("53+h", -138)),
        // 2P 3-Base ply 3 (after 53+a 37+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x0B570CBCEB751323L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2886),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2886),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2886)),
        // 2P 1-Base ply 4 (after 53+c 37+d 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x0B5D7BFE2AA56B09L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0B6346F99D215007L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0B6DF3F6EA10F261L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0B817B4FFA151F2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+b", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // OFK ply 3 (after 22+b 46+e 32+a)
        new OctiBotEngine.OpeningBookEntry(0x0B8A8D7BA198BE7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -4725),
            new OctiBotEngine.OpeningBookCandidate("36+a", -6570),
            new OctiBotEngine.OpeningBookCandidate("56+f", -6570)),
        // 2P 1-Base ply 2 (after 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x0B9772580FE53ECFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x0BF4AF592976E801L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // OFK ply 2 (after 32+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0x0BFBD718BD817E6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32+b", 0),
            new OctiBotEngine.OpeningBookCandidate("32+e", -578)),
        // 4P ply 4 (after 33+d 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0C4FA69C5994D1BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x0C593C7C035B3913L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 462),
            new OctiBotEngine.OpeningBookCandidate("33+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 462)),
        // 4P ply 4 (after 33+b 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0C62E0E80EAFD192L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 3 (after 53+a 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0x0C637E934FB15138L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0C6CA97D61E64F3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x0C7D02CB03B6FA63L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0CADE0F76130DA36L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 4P ply 2 (after 33+d 37+f)
        new OctiBotEngine.OpeningBookEntry(0x0CB0723E4A3D21C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0x0CC4B6EF930DEA18L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2200)),
        // 2P 1-Base ply 3 (after 53+a 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x0CE6C95B92C479B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2360),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2480)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0D0102E09CC56EB7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -690),
            new OctiBotEngine.OpeningBookCandidate("33+b", -740),
            new OctiBotEngine.OpeningBookCandidate("53b+h", -930)),
        // 2P 1-Base ply 3 (after 53+b 37+d 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x0D10451319016960L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2480),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2480)),
        // 2P 3-Base ply 3 (after 33+a 37+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x0D2013633D24F2F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2382),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2396),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402)),
        // 4P ply 3 (after 33+h 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0D314857BD78CAB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 3 (after 32+a 56+f 22+c)
        new OctiBotEngine.OpeningBookEntry(0x0D3A8CA49E89E320L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // 2P 3-Base ply 3 (after 53+b 77+e 53b+a)
        new OctiBotEngine.OpeningBookEntry(0x0DA332F2BB3951EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2396),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402)),
        // OFK ply 4 (after 32+b 46+f 32b-43 46f-35)
        new OctiBotEngine.OpeningBookEntry(0x0DDAA3F79FDC54BFL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -267),
            new OctiBotEngine.OpeningBookCandidate("43b+a", -688),
            new OctiBotEngine.OpeningBookCandidate("43b+h", -1012)),
        // 2P 3-Base ply 3 (after 33+b 37+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x0E1062CE4E9D5593L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2396)),
        // OFK ply 4 (after 22+b 26+d 32+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x0E168C5F6FB77BA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -179),
            new OctiBotEngine.OpeningBookCandidate("22b-33", -620),
            new OctiBotEngine.OpeningBookCandidate("22b+h", -1020)),
        // 2P 1-Base ply 3 (after 53+a 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x0E19A66D5DDE6C08L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2480),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2480)),
        // 2P 1-Base ply 3 (after 53+c 57+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0E1CD22E2AF4D9D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2080),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 2P 3-Base ply 4 (after 53+b 57+g 53b+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x0E4A86CA3B2C0942L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 4P ply 4 (after 33+h 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0E7CE6F052EF514FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+d 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0E983063CA43A94EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 2P 1-Base ply 3 (after 33+b 77+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0E9A87FCDBC3CF15L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2690),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2970)),
        // 4P ply 4 (after 33+c 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0EBF17841B0F266AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 77+f 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x0EEC621BDEC9C24AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -610)),
        // OFK ply 3 (after 32+a 36+d 42+e)
        new OctiBotEngine.OpeningBookEntry(0x0EEFD25C721C32D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -3463)),
        // 2P 3-Base ply 4 (after 33+a 57+e 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0F07956D38CB68AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0F08C79770E0DAA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", -138)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0F3378C16E513BA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0F6D754C01A59CB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -30),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -30),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53b-64 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x0F74E1B54769FF0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x0F84B4FCBED6CBB3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+h", -62)),
        // 2P 3-Base ply 4 (after 53+a 37+d 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0F8BE606F6FD79B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+a 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0F9A36067A63A41CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x0FA923303252DBC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400)),
        // 4P ply 4 (after 33+g 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0FB6ADBE98C951BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x10011103B30BC910L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62)),
        // 2P 3-Base ply 2 (after 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x101EDCB755B8D169L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 4P ply 4 (after 33+h 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x102042A61C42A375L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 4 (after 32+b 46+e 42+a 36+g)
        new OctiBotEngine.OpeningBookEntry(0x1030B70DADF5EF3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 1370),
            new OctiBotEngine.OpeningBookCandidate("42h+g", 1370),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 1024)),
        // OFK ply 3 (after 32+b 46+f 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x104D7BE69B24A152L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -4725),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -4725),
            new OctiBotEngine.OpeningBookCandidate("56+f", -6570)),
        // OFK ply 3 (after 32+b 46+e 32b+g)
        new OctiBotEngine.OpeningBookEntry(0x10A797A28D9FC067L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // OFK ply 4 (after 32+b 56+f 52+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x10C68BC8EE0C20E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("52a+h", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3602),
            new OctiBotEngine.OpeningBookCandidate("32b+g", -3782)),
        // 2P 1-Base ply 3 (after 53+b 77+e 53b+a)
        new OctiBotEngine.OpeningBookEntry(0x10D03C986ABD7F6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -3270),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3270),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3350)),
        // OFK ply 4 (after 32+a 26+d 22+c 46+d)
        new OctiBotEngine.OpeningBookEntry(0x10DE9518D98D02F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -160),
            new OctiBotEngine.OpeningBookCandidate("32a+b", -201),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1733)),
        // 4P ply 2 (after 33+h 37+g)
        new OctiBotEngine.OpeningBookEntry(0x10DF96040FEB530DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // OFK ply 3 (after 32+b 26+d 52+h)
        new OctiBotEngine.OpeningBookEntry(0x10DFD3BD3E5C3A31L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("26d+a", -6783)),
        // 2P 1-Base ply 3 (after 53+a 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0x111070F99E357FBDL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2480)),
        // 2P 3-Base ply 3 (after 33+h 37+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x1116E6A0C55DF42DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x112E17648CE2D068L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+a 77+d)
        new OctiBotEngine.OpeningBookEntry(0x115C5CFBD35693D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 462),
            new OctiBotEngine.OpeningBookCandidate("73+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 462)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+h 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x116E60FF9E5FFE68L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("53bg+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1180)),
        // 2P 1-Base ply 2 (after 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x117321D534CC98D4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // OFK ply 3 (after 22+b 36+e 42+h)
        new OctiBotEngine.OpeningBookEntry(0x118DFF6EDDAD10F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435)),
        // 2P 3-Base ply 3 (after 53+a 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x1195C73143405730L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2024)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x11AEAB8CA7DA720DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -138)),
        // 4P ply 4 (after 33+b 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x11E4407DB92D3D2CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x11EA09E8D664A381L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x11F5EF0867586F53L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 2 (after 33+h 37+c)
        new OctiBotEngine.OpeningBookEntry(0x120800FB9C3C2BF8L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x121C9693CDE148A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x12300D2B2F4BBD05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x124076ABBFD51524L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -460),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -510)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x12608C58FE3CC2DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+a 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x12720D5EB227CDBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+f 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x127BC9FDA164F420L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -690)),
        // OFK ply 4 (after 32+b 46+e 52+g 46e+f)
        new OctiBotEngine.OpeningBookEntry(0x12A408B93EA38888L, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1020),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020)),
        // OFK ply 4 (after 22+b 26+d 22b+c 36+e)
        new OctiBotEngine.OpeningBookEntry(0x12A81FD943AEB344L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3979),
            new OctiBotEngine.OpeningBookCandidate("42+g", -3979),
            new OctiBotEngine.OpeningBookCandidate("42+a", -4000)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x12DE15274AFE434BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 3 (after 32+b 46+e 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x12EB75652461864DL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435)),
        // 2P 1-Base ply 4 (after 53+a 37+d 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x12F8E86C2779573DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 120),
            new OctiBotEngine.OpeningBookCandidate("73+a", 120)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1312513B563F77FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+a", -1180),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1200)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x13147BD9E02DB02DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 3 (after 33+b 57+c 73+h)
        new OctiBotEngine.OpeningBookEntry(0x1325EFEA78D1248DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3630),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3630),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -3630)),
        // OFK ply 3 (after 32+b 46+f 52+h)
        new OctiBotEngine.OpeningBookEntry(0x132BF1ED62E837E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("36d-45", -4725),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4725),
            new OctiBotEngine.OpeningBookCandidate("56+f", -6570)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x133988A0EAA827C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1339B711AC8DCAD4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x135226A0448259FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -400)),
        // 2P 3-Base ply 3 (after 53+a 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x136AA8078C5A428DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37d+e", -2086)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+g)
        new OctiBotEngine.OpeningBookEntry(0x136FDC44FB70F757L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -1686)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x137878BDF83BDE9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+a 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x13A76EFB4BFFC61FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -210)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x13C6E1C24CF95F0FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 3 (after 22+b 46+f 32+a)
        new OctiBotEngine.OpeningBookEntry(0x13CA98A302CF3F6DL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("36d+g", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+e", -6435)),
        // 2P 3-Base ply 3 (after 33+a 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x13E989960A47E190L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37d+e", -2086)),
        // 2P 3-Base ply 4 (after 33+b 77+f 73+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x140B8C4ABFE3AF85L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62)),
        // 2P 1-Base ply 3 (after 53+a 57+c 33+a)
        new OctiBotEngine.OpeningBookEntry(0x140E9160F784EFB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3000),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -3020)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x141645D64A7B723AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33+h", 280)),
        // OFK ply 4 (after 32+b 46+f 32b+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x1432FD45812CFA41L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1287),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -1287),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1747)),
        // 2P 3-Base ply 4 (after 33+a 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x14432E955F18C7A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1477973904FCC1A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 2 (after 33+d 37+e)
        new OctiBotEngine.OpeningBookEntry(0x14AB4C7A2F27AF5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x14C00F04D90564BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x14C3C0D94D274678L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x14E192AAF48EC959L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120)),
        // 2P 1-Base ply 3 (after 53+a 57+d 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x152152E68C6CF82CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -3350),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -3350)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x152462EFAB54CFE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 3-Base ply 3 (after 33+a 37+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x153B2D27583E7C6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2502)),
        // 4P ply 3 (after 33+g 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x1559DD31519256E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 32+a 26+d 42+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x1581B8D802FAD802L, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -1287),
            new OctiBotEngine.OpeningBookCandidate("32a+g", -2107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -2348)),
        // 2P 3-Base ply 4 (after 33+a 77+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x158362F4AB90C771L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 3-Base ply 4 (after 53+b 37+c 53b+a 57+c)
        new OctiBotEngine.OpeningBookEntry(0x159CFEF085F15B2AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 800),
            new OctiBotEngine.OpeningBookCandidate("73+a", 800),
            new OctiBotEngine.OpeningBookCandidate("73+h", 800)),
        // 2P 1-Base ply 3 (after 53+b 57+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0x15A273770A715B31L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2690),
            new OctiBotEngine.OpeningBookCandidate("57d-66", -3100)),
        // OFK ply 4 (after 32+b 46+e 42+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0x15AB5B9BFF69CC5DL, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", -59),
            new OctiBotEngine.OpeningBookCandidate("52+h", -179),
            new OctiBotEngine.OpeningBookCandidate("52+b", -1020)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x15AB66C072309BADL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+d 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x15DB343C4B205A3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x15ED78B47F584D1FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 1-Base ply 2 (after 53+c 77+f)
        new OctiBotEngine.OpeningBookEntry(0x15F73C0A3B55AEA0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 4 (after 33+h 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x16145626DEF03E8DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // OFK ply 4 (after 22+b 36+d 42+a 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x161464CEC2E85916L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1747),
            new OctiBotEngine.OpeningBookCandidate("42a+b", -2348),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2348)),
        // 2P 3-Base ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0x1636F05EC19C04C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x164CC7392D33C9F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53bc-64", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // OFK ply 3 (after 32+a 36+d 22+c)
        new OctiBotEngine.OpeningBookEntry(0x168B4C00735A6F13L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46f+a", -3463)),
        // OFK ply 4 (after 22+b 26+d 42+a 46+a)
        new OctiBotEngine.OpeningBookEntry(0x16A0A8C47F2E89FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -580),
            new OctiBotEngine.OpeningBookCandidate("42+a", -580),
            new OctiBotEngine.OpeningBookCandidate("52h-43", -580)),
        // 4P ply 3 (after 33+d 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x16A0D61390CFD6E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x16B33CA2017FC13FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 400)),
        // 2P 3-Base ply 2 (after 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x16E47C32DE61104AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x16FBB18638D20833L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -710),
            new OctiBotEngine.OpeningBookCandidate("53h+b", -710),
            new OctiBotEngine.OpeningBookCandidate("73+a", -744)),
        // 4P ply 4 (after 33+d 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x170CA2C3D8F722C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // OFK ply 3 (after 22+b 46+e 22b+c)
        new OctiBotEngine.OpeningBookEntry(0x17341EFD8D81769CL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // OFK ply 3 (after 22+b 26+d 22b-33)
        new OctiBotEngine.OpeningBookEntry(0x1759477B4CC7D185L, 8,
            new OctiBotEngine.OpeningBookCandidate("56f+d", -3595),
            new OctiBotEngine.OpeningBookCandidate("56f-45", -3595),
            new OctiBotEngine.OpeningBookCandidate("36+e", -5061)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x176304BF56DBDE6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -610),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1850)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x178108421F792A84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+g 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x178E4BCEC2452E10L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+h 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x17B8DC764A47ABFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -62)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x17C25A9AD10FEEDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+c", -730)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x17C4CCC38A67654BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // OFK ply 4 (after 22+b 36+e 42+h 46+f)
        new OctiBotEngine.OpeningBookEntry(0x17E5DC1A4F36D8F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 640),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 640),
            new OctiBotEngine.OpeningBookCandidate("52h+b", -1020)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x17F3F4103883B712L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x183B745A43E1395DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+b 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x1850E710359748F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120)),
        // 2P 3-Base ply 3 (after 33+b 37+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x186210C8B40FA1CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396)),
        // 4P ply 3 (after 33+c 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x187C5519F510887CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x188303CA15641DA7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 32+b 46+e 52+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0x18A00D90E4732003L, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -1020)),
        // OFK ply 4 (after 22+b 26+d 22b+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x18AC1AF0997E1BCFL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3979),
            new OctiBotEngine.OpeningBookCandidate("32+a", -4002),
            new OctiBotEngine.OpeningBookCandidate("42+c", -4182)),
        // OFK ply 3 (after 22+b 46+e 42+c)
        new OctiBotEngine.OpeningBookEntry(0x18E42180D78F1113L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("56+g", -3963)),
        // OFK ply 3 (after 32+b 36+d 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x18EF704CFEB12EC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("46f+c", -6435)),
        // 4P ply 4 (after 33+a 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1920E343878482A8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 3 (after 53+a 57+e 33+b)
        new OctiBotEngine.OpeningBookEntry(0x19217D38C69A0202L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086)),
        // 2P 1-Base ply 3 (after 33+b 77+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x1927EB619DF28992L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2360),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x193D1A7254E30148L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x196CC2CD6E062A11L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400)),
        // 4P ply 3 (after 33+g 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x1975EF2376D6FFADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0x1986B5441A26B922L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200)),
        // 4P ply 3 (after 33+h 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x199BF72F136301E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+b 57+e 33b+a)
        new OctiBotEngine.OpeningBookEntry(0x19A25CA94087A11FL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+b 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x19F70ACBD7FA0E0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x1A0583D10D40EF9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // OFK ply 3 (after 32+a 36+e 42+a)
        new OctiBotEngine.OpeningBookEntry(0x1A19BCD51605B551L, 8,
            new OctiBotEngine.OpeningBookCandidate("46e+f", -4450),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4450),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x1A2E5A4AB1D7BFA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1180),
            new OctiBotEngine.OpeningBookCandidate("53bg-64", -1200)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x1A2F76E7888ECF8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // OFK ply 4 (after 32+b 36+d 42+a 36d+g)
        new OctiBotEngine.OpeningBookEntry(0x1A34B224772547B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 2418),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 2418),
            new OctiBotEngine.OpeningBookCandidate("22+c", 2418)),
        // 2P 3-Base ply 4 (after 33+b 77+f 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x1A4AC43CF9CB899FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b-64 57+e)
        new OctiBotEngine.OpeningBookEntry(0x1A5ACBBE133781CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("44h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // OFK ply 4 (after 22+b 26+d 22b-33 26d+f)
        new OctiBotEngine.OpeningBookEntry(0x1A6570C7B0EEC96EL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -548),
            new OctiBotEngine.OpeningBookCandidate("42+b", -692),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -1400)),
        // OFK ply 2 (after 22+b 26+d)
        new OctiBotEngine.OpeningBookEntry(0x1A68E6B60A1C7541L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -620),
            new OctiBotEngine.OpeningBookCandidate("42+h", -620),
            new OctiBotEngine.OpeningBookCandidate("22b-33", -620)),
        // 2P 1-Base ply 4 (after 53+c 77+f 53c+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x1A6E2DD1A36A91A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 2P 3-Base ply 3 (after 33+h 37+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x1A93E135A080379AL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // OFK ply 1 (after 32+b)
        new OctiBotEngine.OpeningBookEntry(0x1A9B50BEC8C890F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -4143),
            new OctiBotEngine.OpeningBookCandidate("56+f", -4143),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4143)),
        // OFK ply 4 (after 32+b 36+e 42+a 26+a)
        new OctiBotEngine.OpeningBookEntry(0x1AC4F02B6B5A4241L, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", 1101),
            new OctiBotEngine.OpeningBookCandidate("32b+a", 1101),
            new OctiBotEngine.OpeningBookCandidate("52+h", 260)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1AD67C389C01698BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1AD835ADF348F726L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+a 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x1AEB209BBB7988F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280)),
        // OFK ply 4 (after 32+b 26+d 42+a 26d+a)
        new OctiBotEngine.OpeningBookEntry(0x1B04967CD787CF76L, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", 1160),
            new OctiBotEngine.OpeningBookCandidate("22+b", 1160),
            new OctiBotEngine.OpeningBookCandidate("42h+a", 1127)),
        // 4P ply 3 (after 33+h 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x1B65A0AE45DFE1DDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 3 (after 22+b 36+d 32+a)
        new OctiBotEngine.OpeningBookEntry(0x1B689309675AB0F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("46f-35", -4725),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570)),
        // OFK ply 4 (after 32+b 26+d 52+g 46+a)
        new OctiBotEngine.OpeningBookEntry(0x1B6C63A2848D42B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 0),
            new OctiBotEngine.OpeningBookCandidate("22c+b", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1B75B9794B7EE4E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -710)),
        // OFK ply 4 (after 32+b 46+f 32b-43 36+e)
        new OctiBotEngine.OpeningBookEntry(0x1B78FFF72EA8A98AL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("43b+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1068)),
        // OFK ply 3 (after 22+b 46+f 42+h)
        new OctiBotEngine.OpeningBookEntry(0x1B89FA47077DB87CL, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("46f+c", -6435)),
        // 4P ply 4 (after 33+d 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1BA2AE09E2E5E8EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 2P 3-Base ply 4 (after 33+h 37+f 33h+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x1BC3E6B37E06D08AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 1-Base ply 3 (after 33+b 77+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x1BD8845752E89C2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x1BDE9CDC14C31B26L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1BDFC6021FC71E5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // OFK ply 4 (after 32+b 26+d 32b+e 46+f)
        new OctiBotEngine.OpeningBookEntry(0x1BEF28A4870554B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("42eh-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("42eh+b", -1887)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x1C53BEFA2A3C17FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 1-Base ply 1 (after 53+c)
        new OctiBotEngine.OpeningBookEntry(0x1C59CC8CD1FD5C7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2700),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2700),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2950)),
        // OFK ply 3 (after 22+b 26+d 22b+c)
        new OctiBotEngine.OpeningBookEntry(0x1C67C02BA690F42CL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -3283),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3283),
            new OctiBotEngine.OpeningBookCandidate("56f-45", -3283)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1C72235407B7BA1CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // OFK ply 3 (after 32+a 46+e 22+c)
        new OctiBotEngine.OpeningBookEntry(0x1C8F4929A98AC798L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463)),
        // OFK ply 4 (after 32+b 36+e 32b+a 36e+d)
        new OctiBotEngine.OpeningBookEntry(0x1C90F6EFE4B975D5L, 8,
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -88),
            new OctiBotEngine.OpeningBookCandidate("42+a", -88),
            new OctiBotEngine.OpeningBookCandidate("52+h", -640)),
        // 2P 1-Base ply 3 (after 53+b 77+e 33+h)
        new OctiBotEngine.OpeningBookEntry(0x1CA5C750F328726DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3030),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3230),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -3230)),
        // 4P ply 4 (after 33+g 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1CCBA9C69BD86CB8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x1CD09F6BAC21B4E6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+h 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1D01E28851FE6C4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 2P 1-Base ply 2 (after 53+c 57+c)
        new OctiBotEngine.OpeningBookEntry(0x1D0A57B7116C1602L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53g+h", 0)),
        // OFK ply 3 (after 32+b 26+d 52+g)
        new OctiBotEngine.OpeningBookEntry(0x1D3C0CB42A5CE5DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // 2P 1-Base ply 3 (after 53+b 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x1D42DF000702A156L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f+d", -3500),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3500),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3600)),
        // OFK ply 4 (after 22+b 36+e 42+c 26+d)
        new OctiBotEngine.OpeningBookEntry(0x1D7820A419A0D4CBL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -3553)),
        // 2P 3-Base ply 3 (after 33+b 57+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x1D937EB652DE268EL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2382)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+h 57c+f)
        new OctiBotEngine.OpeningBookEntry(0x1DC05E11342C43C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 280),
            new OctiBotEngine.OpeningBookCandidate("53+b", 280),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 280)),
        // OFK ply 4 (after 32+b 36+e 52+h 46+a)
        new OctiBotEngine.OpeningBookEntry(0x1DDC627DBB9C1FE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 544),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 544),
            new OctiBotEngine.OpeningBookCandidate("42h+a", 187)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1DE5341BC952944DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+h 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x1DF2C5789339DA92L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", 0)),
        // 2P 3-Base ply 3 (after 53+a 37+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x1E150F17625E4019L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2086)),
        // 4P ply 4 (after 33+g 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1E1C3F39080F144DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // OFK ply 3 (after 32+a 46+f 22+c)
        new OctiBotEngine.OpeningBookEntry(0x1E2947AA16CFE087L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3980),
            new OctiBotEngine.OpeningBookCandidate("36d+f", -4523),
            new OctiBotEngine.OpeningBookCandidate("36d+e", -4843)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1E2FF05D633BA15BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -258),
            new OctiBotEngine.OpeningBookCandidate("53+b", -258),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -258)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x1E358D6326401826L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // OFK ply 4 (after 32+a 46+e 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x1E8CD6321AB68F77L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("32g+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("32g+a", -2500)),
        // 2P 3-Base ply 3 (after 33+a 37+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x1E97BD7101BC277BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2144)),
        // 4P ply 4 (after 33+d 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1ED10D64353D8715L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // OFK ply 4 (after 22+b 56+f 42+a 36+f)
        new OctiBotEngine.OpeningBookEntry(0x1F0D7C3C6C40A774L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 1020),
            new OctiBotEngine.OpeningBookCandidate("42a+h", 1020),
            new OctiBotEngine.OpeningBookCandidate("32+b", 140)),
        // 2P 3-Base ply 3 (after 33+a 37+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x1F105EF32BA5081AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -2086)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1F45AB7C21D09500L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 3 (after 53+b 57+c 33+a)
        new OctiBotEngine.OpeningBookEntry(0x1F56DDBDCC6D5781L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2486)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1F70CC48CADB01AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // OFK ply 4 (after 32+a 26+d 42+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x1F85BDF1D82A7089L, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -1287),
            new OctiBotEngine.OpeningBookCandidate("32a+g", -2107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -2348)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1FC3DF5349150A0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", -610),
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1850)),
        // 4P ply 4 (after 33+h 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1FD67477C22914B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x1FF3575B89A09364L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -258),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -258),
            new OctiBotEngine.OpeningBookCandidate("53+c", -462)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+a 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x1FF5C102D2C818F4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x20068D8108C7B8E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x200F49221B848173L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -690),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -690),
            new OctiBotEngine.OpeningBookCandidate("73+h", -740)),
        // OFK ply 4 (after 22+b 46+e 42+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x2037E5690753889EL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -761),
            new OctiBotEngine.OpeningBookCandidate("32+a", -761),
            new OctiBotEngine.OpeningBookCandidate("32+b", -940)),
        // OFK ply 4 (after 32+b 26+d 32b+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x203E0525686EB3AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1287),
            new OctiBotEngine.OpeningBookCandidate("42ah-33", -1287),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1615)),
        // 4P ply 4 (after 33+c 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2067FC2FBC5AE860L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+e 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x208C68B39D99226EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x2098658C7B42C64AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x20A2F8218E78C81CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x20A83BDC025E0B85L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280)),
        // 4P ply 4 (after 33+a 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x20E4B0376FF3F7C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x21024F92A4C33494L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120)),
        // 2P 3-Base ply 2 (after 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x21128F1A7C1A6B78L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x21138112B9D0E046L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 1-Base ply 4 (after 53+c 37+d 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x2126A67FFE622CA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -650),
            new OctiBotEngine.OpeningBookCandidate("33+a", -650),
            new OctiBotEngine.OpeningBookCandidate("33+b", -730)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x213EC766EEEBE06FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x214B9E260B20D904L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33+c", -62)),
        // 2P 1-Base ply 4 (after 53+b 37+d 53b-64 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2175B6015E7780B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("64b+a", 0)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x21805E195A2961FFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 32+b 36+d 42+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x21A7A3FB162B85B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 640),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 640),
            new OctiBotEngine.OpeningBookCandidate("22+h", -1020)),
        // OFK ply 3 (after 22+b 56+f 42+e)
        new OctiBotEngine.OpeningBookEntry(0x21BF94B66BA7C442L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3960)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x21C191B50E9F75B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 3 (after 53+b 37+c 33+a)
        new OctiBotEngine.OpeningBookEntry(0x21C8620AA5C4215AL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486)),
        // 2P 3-Base ply 3 (after 33+a 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x21D5AB96505CFCE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+h 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x22068C55F7A13561L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 3 (after 53+b 77+e 33+b)
        new OctiBotEngine.OpeningBookEntry(0x2260323588D18692L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3990),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3990),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -3990)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2279213C6D823002L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -1010)),
        // 2P 1-Base ply 4 (after 53+a 37+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x227C557F1AA885D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 1-Base ply 3 (after 53+c 57+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x228E497155932A29L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2080)),
        // OFK ply 4 (after 22+b 46+e 42+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x2291EBEAB816AF81L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1747),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2348),
            new OctiBotEngine.OpeningBookCandidate("42a+b", -2527)),
        // OFK ply 4 (after 32+a 36+e 42+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x22CA783CC6D92CDCL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("42c+b", -2108)),
        // 4P ply 3 (after 33+g 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x22CDD50ED54B0997L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x22F0597755F9ABFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", -780)),
        // OFK ply 4 (after 32+b 26+d 32b-43 46+a)
        new OctiBotEngine.OpeningBookEntry(0x2301213CBD94DFA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 544),
            new OctiBotEngine.OpeningBookCandidate("52+h", 544),
            new OctiBotEngine.OpeningBookCandidate("43b+a", 187)),
        // 4P ply 4 (after 33+g 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x23122F4C74761CE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2336FC2C7B7E1F8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 2P 3-Base ply 3 (after 33+h 57+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x23568BD32927D271L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2276),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2276),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2282)),
        // 2P 3-Base ply 3 (after 33+a 57+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x2362667F7FBD817EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x23B5DDBDFD63BC91L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+a 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x23BC8A2C8B8A8A28L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+b 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x23C417ED2A0798B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 2P 3-Base ply 3 (after 53+a 57+e 33+h)
        new OctiBotEngine.OpeningBookEntry(0x23E147EEF9A02263L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+g 37+e)
        new OctiBotEngine.OpeningBookEntry(0x240177AE1D9FB6D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -400)),
        // 4P ply 4 (after 33+h 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x241DC0C888B103D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 77+e 53b+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x2437AF9E1A9E3F79L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x246CEB1ADA1C2227L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -62)),
        // 2P 3-Base ply 3 (after 33+b 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x24708C504865216DL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2382)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x248063A3CA19CF6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62)),
        // 2P 3-Base ply 2 (after 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x2490FCED64816519L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // OFK ply 4 (after 22+b 36+e 42+g 36e+d)
        new OctiBotEngine.OpeningBookEntry(0x249BDCDBF79E62D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42g+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2110),
            new OctiBotEngine.OpeningBookCandidate("42g+b", -2168)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x249DAA3F3F8112D4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x24A73792CABB1C82L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -310)),
        // 4P ply 3 (after 33+c 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x24A7A664D2A9FC5EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 2 (after 22+b 36+d)
        new OctiBotEngine.OpeningBookEntry(0x24AB91F2A43A88FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -28),
            new OctiBotEngine.OpeningBookCandidate("32+b", -28),
            new OctiBotEngine.OpeningBookCandidate("32+e", -180)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x24B48E0F9C839C64L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+a 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x24CB236606A09907L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -520)),
        // 4P ply 4 (after 33+c 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x250BD2B49A910872L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2539B63A47D70EF1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // OFK ply 4 (after 22+b 26+d 32+a 46+c)
        new OctiBotEngine.OpeningBookEntry(0x25412AA087DA1E31L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 2440),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 1931),
            new OctiBotEngine.OpeningBookCandidate("32+c", 891)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x254E51954FE30D9AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338)),
        // 4P ply 3 (after 33+g 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x25766D6A60579170L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x2592892AF428B263L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // 4P ply 3 (after 33+b 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x25A055CB3E261522L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 3 (after 22+b 26+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0x25ABE44DC97C4D46L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3690),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3690),
            new OctiBotEngine.OpeningBookCandidate("46+a", -4663)),
        // 2P 3-Base ply 3 (after 33+h 37+e 33h+a)
        new OctiBotEngine.OpeningBookEntry(0x25D06425149F287BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // OFK ply 2 (after 32+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x25DABE2ED5658D56L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -28),
            new OctiBotEngine.OpeningBookCandidate("22+b", -28),
            new OctiBotEngine.OpeningBookCandidate("42+e", -578)),
        // 4P ply 4 (after 33+d 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x25F484677AE59DA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x25FB103EA03DF68AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x260D8F45BBB81DA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 1-Base ply 2 (after 53+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x26690582F0F3FB34L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 1-Base ply 4 (after 33+b 57+d 73+h 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x269512754E6EBC0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120)),
        // 4P ply 2 (after 33+c 37+e)
        new OctiBotEngine.OpeningBookEntry(0x26AC3C0D6D4185E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 4P ply 3 (after 33+a 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x26B6BE994FAA6D70L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x26BE80163B6578BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -250)),
        // 2P 1-Base ply 3 (after 53+c 57+f 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x26BF6B6E47CAADB8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2170),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2950),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2950)),
        // 2P 1-Base ply 3 (after 53+c 57+d 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x26FF1CF5557783B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2170),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2290),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2290)),
        // OFK ply 4 (after 32+a 46+f 22+c 46f+e)
        new OctiBotEngine.OpeningBookEntry(0x271E42684607E3D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 162),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -878),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1243)),
        // 4P ply 4 (after 33+d 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x27231298E932E55CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 3 (after 33+b 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x275E024A689AF51CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+h 57+e 33h+a)
        new OctiBotEngine.OpeningBookEntry(0x2767A9CC3B7E55E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x2784FFA2EF3A3B00L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // OFK ply 4 (after 32+b 56+f 52+g 36+f)
        new OctiBotEngine.OpeningBookEntry(0x279A94C58F600FE9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32b+a", -2828),
            new OctiBotEngine.OpeningBookCandidate("32b+d", -2828),
            new OctiBotEngine.OpeningBookCandidate("42+c", -2843)),
        // 4P ply 3 (after 33+g 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x27A1FB95F380E985L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x27DC444B09467087L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+c 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x27EA08C33D3E67A4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x27F75D71CDD60675L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 462),
            new OctiBotEngine.OpeningBookCandidate("73+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 462)),
        // 2P 3-Base ply 4 (after 33+a 57+d 33a+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x28161DEE7DE368B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x281816140A6BC1C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 4 (after 33+g 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x282E5A9C3E13D6EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280)),
        // 2P 3-Base ply 4 (after 33+b 77+f 73+b 77f+e)
        new OctiBotEngine.OpeningBookEntry(0x283116190B004EC7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x283E44E3432BFCCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x28779557ECC8115CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // OFK ply 3 (after 32+a 26+d 22+c)
        new OctiBotEngine.OpeningBookEntry(0x2886466552F88E8BL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -3980),
            new OctiBotEngine.OpeningBookCandidate("26d-35", -3980),
            new OctiBotEngine.OpeningBookCandidate("46+d", -4523)),
        // 4P ply 4 (after 33+b 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x28D1A343A509CA72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 3 (after 33+a 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x28D21FB1A905D5B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086)),
        // 2P 1-Base ply 4 (after 33+b 37+d 33b-44 77+f)
        new OctiBotEngine.OpeningBookEntry(0x28F63BC58B8053E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33", 270),
            new OctiBotEngine.OpeningBookCandidate("53+a", -3390),
            new OctiBotEngine.OpeningBookCandidate("44b+c", -3390)),
        // 4P ply 3 (after 33+a 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x28F96824D217C880L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 22+b 36+d 32+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0x290C481D1C833116L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 2P 1-Base ply 4 (after 53+b 77+f 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x29334253FC6C492BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -510)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x29391F908239521EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+h 77+d)
        new OctiBotEngine.OpeningBookEntry(0x295BB3EC338B8035L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 2P 3-Base ply 3 (after 53+a 57+d 33+c)
        new OctiBotEngine.OpeningBookEntry(0x296142D2FA647F7DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -1686)),
        // 2P 3-Base ply 3 (after 33+b 77+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x29614E59A3C75C49L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2382),
            new OctiBotEngine.OpeningBookCandidate("57+c", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2768)),
        // 4P ply 4 (after 33+g 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x298837068CD64B3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+c 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x29A5DE7EA083C254L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+h 77+d)
        new OctiBotEngine.OpeningBookEntry(0x29D8927DB5962328L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x29D8B6755DA134E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+e 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x29DAE16FED02E423L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400)),
        // 2P 3-Base ply 3 (after 53+b 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x29FDDC37D2EDC225L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+d", -2486)),
        // 4P ply 4 (after 33+b 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2A0635BC36DEB287L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 3 (after 53+b 77+f 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x2A069F95E413DCCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600)),
        // 2P 3-Base ply 3 (after 33+a 77+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x2A513FF4D07EFB2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2396)),
        // 4P ply 3 (after 33+d 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x2A7B256EB776A2C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x2A8473BD5702371CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+g 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2ACF80EB99BCB93DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x2AD9FAD0724CE465L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -1010)),
        // 2P 1-Base ply 4 (after 53+c 57+e 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2AEA90C29EBB4347L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -400)),
        // 4P ply 2 (after 33+a 37+b)
        new OctiBotEngine.OpeningBookEntry(0x2AF2F24D6DFFB13EL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // OFK ply 4 (after 22+b 46+e 42+c 56+g)
        new OctiBotEngine.OpeningBookEntry(0x2B16CCA8C48820C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1040),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1040),
            new OctiBotEngine.OpeningBookCandidate("22b+c", -1040)),
        // 2P 1-Base ply 3 (after 33+b 57+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x2B536BBE2712FCC1L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2360),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480)),
        // OFK ply 4 (after 32+b 56+f 52+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x2B5A36AEAB4CCC3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("22a+b", -3579),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3602),
            new OctiBotEngine.OpeningBookCandidate("42h+g", -3782)),
        // 2P 3-Base ply 3 (after 33+b 77+f 33b+a)
        new OctiBotEngine.OpeningBookEntry(0x2BD6DC76FA67D44CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2086)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2BD8C5E725963DC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2BEE896F11EE2AEBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 2P 3-Base ply 2 (after 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x2C034D1397B8165CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -252)),
        // OFK ply 2 (after 22+b 46+f)
        new OctiBotEngine.OpeningBookEntry(0x2C099A58C1AF076AL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32+b", -620),
            new OctiBotEngine.OpeningBookCandidate("52h+g", -1220)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2C1BE0423107B7B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2C39326FBCE4DCBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 3-Base ply 4 (after 33+h 77+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x2C479BE5C7C8E6E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 2P 3-Base ply 4 (after 33+a 77+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x2C54227891F06603L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+a 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x2C6BF88A196E4D60L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -520)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x2C725AE4A80BC5C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280),
            new OctiBotEngine.OpeningBookCandidate("73+g", -188)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x2C8583CEC42CE1DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -460),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -460)),
        // 2P 3-Base ply 3 (after 53+b 57+f 33+b)
        new OctiBotEngine.OpeningBookEntry(0x2C9020274516DB0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2402)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x2CAE825B13C07A3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53ab-64", -340),
            new OctiBotEngine.OpeningBookCandidate("73+h", -340),
            new OctiBotEngine.OpeningBookCandidate("33+a", -340)),
        // 4P ply 2 (after 33+b 37+c)
        new OctiBotEngine.OpeningBookEntry(0x2CCDF59EE7C5DF07L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2CD67D13775BADAEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 3 (after 33+a 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x2CE33DAEBB5C5228L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2564)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2CE4199DAA1DAB2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+b 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2DBD8DD883C22A60L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2DD18EBC9BD444D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 3 (after 33+b 57+c 73+b)
        new OctiBotEngine.OpeningBookEntry(0x2DE01A8F0328D072L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2760),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2760),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2DF35C9116372FD8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 2 (after 33+b 37+g)
        new OctiBotEngine.OpeningBookEntry(0x2E1A63617412A7F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // 2P 1-Base ply 3 (after 53+c 57+f 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x2E1FB082580479DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2170),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2290),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2290)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2E338F6239CAD3D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // OFK ply 2 (after 32+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x2E471DD971F013C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -620),
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("32+a", -820)),
        // 2P 1-Base ply 3 (after 53+c 57+d 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x2E5FC7194AB957DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2820),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2950),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2950)),
        // 4P ply 3 (after 33+g 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2E7C54321E4A4C59L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 2P 1-Base ply 2 (after 53+a 57+c)
        new OctiBotEngine.OpeningBookEntry(0x2E946E3FDACA4396L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x2EA41FB416C1C1B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // OFK ply 2 (after 22+b 46+e)
        new OctiBotEngine.OpeningBookEntry(0x2EAF94DB7EEA2075L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -28),
            new OctiBotEngine.OpeningBookCandidate("32+b", -28),
            new OctiBotEngine.OpeningBookCandidate("32+e", -698)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2EE5B7C367BB578AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 2 (after 53+c 57+e)
        new OctiBotEngine.OpeningBookEntry(0x2EFFC03F74E03DABL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2F21B518C2D4C9D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x2F2DD3F1E9394613L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // OFK ply 4 (after 32+a 46+e 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x2F3F02C4201A7A4BL, 8,
            new OctiBotEngine.OpeningBookCandidate("42e+h", 0),
            new OctiBotEngine.OpeningBookCandidate("22+b", 0),
            new OctiBotEngine.OpeningBookCandidate("22+c", 0)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x2F6453D5E249C167L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62)),
        // OFK ply 4 (after 32+a 36+d 22+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x2FBC49C223926C4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1343),
            new OctiBotEngine.OpeningBookCandidate("42+c", -2667),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -3107)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2FCFAD14A761379DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2FE2446C8B34BEF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+a 57g+d)
        new OctiBotEngine.OpeningBookEntry(0x2FF8C1BB93635F0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 4P ply 1 (after 33+c)
        new OctiBotEngine.OpeningBookEntry(0x3050DC2FFBCC82CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1236),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1236)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3053AB10FB1E4244L, 8,
            new OctiBotEngine.OpeningBookCandidate("33", 550),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 400)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x30596A12E611BCE3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53b-64 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x305DD33105874945L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("64b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+b 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x3077F87F64D96BAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -1050)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x30B913952916A5B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 3 (after 33+a 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x30E25660B70D4618L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 3 (after 33+h 37+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x311417FB5252905FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2024)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+g 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x3118F6E0C8EA63E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -462)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x3134958F164CC860L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -510)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x315EADA04459F7A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73+h", 338)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+b 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x318920CD8F8BCA3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -520),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -520),
            new OctiBotEngine.OpeningBookCandidate("73+a", -520)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x31930942E9CCC5A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 3 (after 53+a 37+f 33+h)
        new OctiBotEngine.OpeningBookEntry(0x3197366AD44F3342L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x319D4128159F4B68L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -1970)),
        // OFK ply 4 (after 32+b 26+d 52+h 36+e)
        new OctiBotEngine.OpeningBookEntry(0x31AD348BDFF3B884L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1020)),
        // 4P ply 4 (after 33+c 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x31BEE03AC5994CCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // OFK ply 4 (after 22+b 46+e 42+c 56+f)
        new OctiBotEngine.OpeningBookEntry(0x31DC90D8030D79E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("32g+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("42+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("52h+e", -3500)),
        // 2P 3-Base ply 4 (after 53+b 57+c 53b+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x31DD8C31C24454BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 338),
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73+a", 338)),
        // 2P 1-Base ply 3 (after 33+b 57+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x31E32E4D9492F58FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("57d+f", -3500)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x3201438AE8F41EE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2080)),
        // 4P ply 3 (after 33+a 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x321C01E1E1B1A626L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x3270FC052528A9C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x32A7158D7556256DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 22+b 56+f 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x32C0EF4C0F0111EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -179),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -179),
            new OctiBotEngine.OpeningBookCandidate("22b+c", -1548)),
        // OFK ply 4 (after 32+b 46+e 32b+g 36+d)
        new OctiBotEngine.OpeningBookEntry(0x32C90F00603C2ADAL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // OFK ply 4 (after 32+b 36+e 32b-43 26+d)
        new OctiBotEngine.OpeningBookEntry(0x32CBBE80263F2E3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -1020)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+h 37e+d)
        new OctiBotEngine.OpeningBookEntry(0x32F24E6346CACEABL, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x333AEA0C985887CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3340B7BB9325ACF2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3351440C3672F9F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x336D5EC3BF702599L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+a 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x33D711DEC745EF30L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 3 (after 33+b 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x33F4BD32C6813E4AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 22+b 26+d 22b+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x34218D96AEB15CBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3602),
            new OctiBotEngine.OpeningBookCandidate("32+g", -3602)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x34337E4B0AF44FA6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 400)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x34404C392DE867AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk1() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // OFK ply 3 (after 22+b 36+e 42+c)
        new OctiBotEngine.OpeningBookEntry(0x344091FCCD22BC3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x3444299DBBC7ADC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1970)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x345182B930FBB37CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+d 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x345E16E0EA23D85FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+h 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x347237CC201ABF31L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -1020)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x3474A1957B7234A1L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -830),
            new OctiBotEngine.OpeningBookCandidate("53+h", -830),
            new OctiBotEngine.OpeningBookCandidate("53+b", -862)),
        // 4P ply 4 (after 33+c 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x34A140330A574D84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x34C2FE5F4E0A00CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 62)),
        // 4P ply 4 (after 33+c 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x34CD435712412336L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x34FBE30F632DDFC4L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200)),
        // 4P ply 3 (after 33+c 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x350D34E3426FB9A8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // OFK ply 4 (after 22+b 56+f 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x3531909D3312EFF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42e+h", 0),
            new OctiBotEngine.OpeningBookCandidate("22b+c", -497)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x354D4A8992AFD249L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // OFK ply 3 (after 22+b 26+d 42+h)
        new OctiBotEngine.OpeningBookEntry(0x3563197CF22D579AL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+g", -6435)),
        // 4P ply 4 (after 33+g 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x35708B3DB8A920AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 2P 1-Base ply 3 (after 53+c 57+c 33+b)
        new OctiBotEngine.OpeningBookEntry(0x357AF458BC41813EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2820)),
        // OFK ply 3 (after 32+b 36+e 42+a)
        new OctiBotEngine.OpeningBookEntry(0x358B907EFD37642EL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("26+a", -6783)),
        // 2P 3-Base ply 3 (after 53+a 37+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x3592F9D9908CE7DCL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2024)),
        // 4P ply 4 (after 33+b 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x35A6B39CE6D8A4F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 4 (after 22+b 56+f 22b+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x35B2DB9B309AF9F0L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107),
            new OctiBotEngine.OpeningBookCandidate("42+b", -3747)),
        // 2P 3-Base ply 4 (after 33+b 77+e 33b+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x35C7AFBB07F148CFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // OFK ply 4 (after 22+b 36+e 22b-33 46+f)
        new OctiBotEngine.OpeningBookEntry(0x35DF821DF1DC5EE9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+h", -3603),
            new OctiBotEngine.OpeningBookCandidate("32+c", -3603),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3603)),
        // 4P ply 3 (after 33+g 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x360B69126346AC73L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 3-Base ply 3 (after 33+h 57+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x362065D4F696D244L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+h 57c+f)
        new OctiBotEngine.OpeningBookEntry(0x364AD1D628E9DC20L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 400)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x364FA5955FC369FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0)),
        // 4P ply 3 (after 33+g 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x36676A767B50C2C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x36861446A32CCB89L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x368BCEA3B468211CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+b 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x36F61227E7F6DE4AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0)),
        // 4P ply 4 (after 33+b 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x36FE898702A1D912L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 3 (after 53+b 77+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0x3722319E01FAD5A8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -3350),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3990)),
        // OFK ply 3 (after 32+b 36+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0x372D9EFD42724331L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("36d+g", -6435)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x37317B4D5ACF5BDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x375769A55EF31323L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120)),
        // OFK ply 4 (after 22+b 36+e 22b-33 46+e)
        new OctiBotEngine.OpeningBookEntry(0x37798C9E4E9979F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1242),
            new OctiBotEngine.OpeningBookCandidate("43h+a", -1242),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3891)),
        // 4P ply 4 (after 33+g 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x37A71DC22B7E585FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 4P ply 3 (after 33+c 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x37DAA21CD1B8C15DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x37FBF40BF2562A64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+g", -120)),
        // 4P ply 3 (after 33+d 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x37FD85FB00F44E79L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+c 57+f)
        new OctiBotEngine.OpeningBookEntry(0x3823B10B64EC018BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1010),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -1010),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -2250)),
        // 4P ply 3 (after 33+g 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x3828BCCBE6ED6731L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x3862A6D1A3B13FFBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x387224F2CD677117L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x387CC26BD94066F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 3 (after 32+b 56+f 52+g)
        new OctiBotEngine.OpeningBookEntry(0x3880C675E62D8870L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -3980),
            new OctiBotEngine.OpeningBookCandidate("36+f", -4542),
            new OctiBotEngine.OpeningBookCandidate("46+f", -5281)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x389051A6D73DDE27L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x38938D1712FF17E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x38B15F3A9F1C7CE2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x38B9C665EB624684L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 4 (after 53+a 37+d 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x38C34276E1033E9DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x38FF8E730AE9795AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 3 (after 33+b 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x3903823A99E10FE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2980),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x391FE5700B980CA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -340)),
        // 2P 1-Base ply 4 (after 53+c 57+e 73+h 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x3929FB2190451E5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 2P 3-Base ply 4 (after 33+b 77+f 33b+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x393E78DE2613A145L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x3944A1F4CB1A11FCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -340),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", -340),
            new OctiBotEngine.OpeningBookCandidate("33+a", -340)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3959E3E9B82CE48EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 3 (after 32+b 36+e 32b+e)
        new OctiBotEngine.OpeningBookEntry(0x3969EDC23A1EDBD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // 4P ply 4 (after 33+b 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x397B31C435CF8F84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 2 (after 33+a 37+c)
        new OctiBotEngine.OpeningBookEntry(0x398FF6356EEE8C3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x399B605D3F33EF67L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x39C780654D07B2E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -340),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -390)),
        // OFK ply 3 (after 22+b 56+f 22b+c)
        new OctiBotEngine.OpeningBookEntry(0x39DB0AEA6AE19987L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // 2P 1-Base ply 2 (after 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x39E3F287B5054B9CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 3-Base ply 4 (after 53+b 37+c 53b+a 37c+d)
        new OctiBotEngine.OpeningBookEntry(0x39E8320CB56DB7C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x3A2D38E6C3271FAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // OFK ply 4 (after 32+a 26+d 42+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x3A397730145B1D22L, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -267),
            new OctiBotEngine.OpeningBookCandidate("22+b", -761),
            new OctiBotEngine.OpeningBookCandidate("52+h", -819)),
        // OFK ply 4 (after 22+b 56+f 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x3A62E4E66A949E7FL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("22b+e", -3500)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3A8295EA8FFC86C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 3 (after 33+h 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x3A91106E378F53E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3AA5B20D5EB009E2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 3-Base ply 3 (after 53+b 37+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x3AD6CFB0460928EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2486)),
        // 4P ply 3 (after 33+g 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x3AD6EB4AB051870FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3B091108116C9279L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 2 (after 53+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3B1A0BE82177D5B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 2 (after 33+a 37+g)
        new OctiBotEngine.OpeningBookEntry(0x3B5860CAFD39F4C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3B65126C097AFCCBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 3-Base ply 2 (after 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3B992A79A76A76ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+a 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3BA7B468EE9004B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 3-Base ply 4 (after 33+h 57+e 33h+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3BC117E8E909B4F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62)),
        // 2P 3-Base ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x3BCD8E7CEAE1563EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 4 (after 33+h 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3C06FE8CEDAB8D41L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 22+b 36+e 22b+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x3C470BCEA4CE5C5FL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+g", -2170)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x3C514FA1BD337389L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x3C7141F875471A11L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3C7B1C3B0B120124L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 1-Base ply 4 (after 53+c 57+e 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x3C98E2C46429B719L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3C9F4014882583F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x3CAAAE6F8C907B08L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 150),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 150),
            new OctiBotEngine.OpeningBookCandidate("33+b", -250)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3CB2A96CA4700A9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3CB867F359690CD1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x3CE4F271A40979AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -730),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -1970)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x3CF34CC4CA03C924L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 370),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 370),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 280)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3D22887E22CD8069L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+g 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3D4E8B1A3ADBEEDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x3D7C4748CA00AFF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // OFK ply 4 (after 22+b 36+e 22b-33 36e+d)
        new OctiBotEngine.OpeningBookEntry(0x3D7D89B79449D17DL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3863),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3863),
            new OctiBotEngine.OpeningBookCandidate("43h+g", -3863)),
        // 2P 1-Base ply 3 (after 53+c 77+f 33+b)
        new OctiBotEngine.OpeningBookEntry(0x3D879FE59678399CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 2P 3-Base ply 3 (after 33+b 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x3D901C1A202E8683L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2086)),
        // 4P ply 4 (after 33+a 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3D93A0E82C229948L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 3 (after 33+b 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x3DBB6B8F5B3C9BBAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 4 (after 53+b 37+d 33+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x3DF9F0801775877BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -510)),
        // 2P 3-Base ply 3 (after 53+a 37+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x3E17FE4CF551246BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2086)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x3E2C900E80E23C68L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+h", 62)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3E48D6EB1BF2FB00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 4 (after 33+g 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3E653F9337A7726BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // OFK ply 1 (after 22+b)
        new OctiBotEngine.OpeningBookEntry(0x3E7C56E08875ECA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4143),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4143),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4143)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3E9AC64CACBD6EF2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3EAC8AC498C579D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 2 (after 33+c 37+f)
        new OctiBotEngine.OpeningBookEntry(0x3EB70249085B0B78L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 2P 3-Base ply 3 (after 33+h 37+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x3EBCDBE9AAECDBAAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2402),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2402)),
        // OFK ply 4 (after 22+b 36+e 22b+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x3EE1054D1B8B7B40L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3979),
            new OctiBotEngine.OpeningBookCandidate("32+c", -3979),
            new OctiBotEngine.OpeningBookCandidate("32+a", -4000)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+g 37+e)
        new OctiBotEngine.OpeningBookEntry(0x3F0A2F56BC061E87L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x3F0F5B15CB2CAB5DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 2P 3-Base ply 3 (after 33+b 77+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x3F133C5F5955A817L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2382),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2402),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2402)),
        // 4P ply 4 (after 33+a 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3F443617BFF5E1BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x3F8C7A844D310840L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // OFK ply 3 (after 32+b 46+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0x3F8F955727E7CCA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -4725),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570)),
        // OFK ply 4 (after 32+b 26+d 52+h 26d+a)
        new OctiBotEngine.OpeningBookEntry(0x3F90B3E8A8311C5EL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 1160),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 1160),
            new OctiBotEngine.OpeningBookCandidate("32b+a", 1127)),
        // 4P ply 2 (after 33+b 37+b)
        new OctiBotEngine.OpeningBookEntry(0x3FB0F1E6E4D4E204L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // 2P 3-Base ply 4 (after 33+a 57+e 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3FC4D85BADCA6066L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x3FC54BAC4835A419L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // OFK ply 2 (after 22+b 56+f)
        new OctiBotEngine.OpeningBookEntry(0x3FD42C77C66D18EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42+h", 0),
            new OctiBotEngine.OpeningBookCandidate("42+e", -698)),
        // OFK ply 2 (after 32+b 46+e)
        new OctiBotEngine.OpeningBookEntry(0x3FDDCC3BA95023DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -620),
            new OctiBotEngine.OpeningBookCandidate("32+a", -620),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -620)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x3FE33B43C810AFF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -400)),
        // OFK ply 4 (after 32+b 26+d 32b+a 36+c)
        new OctiBotEngine.OpeningBookEntry(0x3FE651FDA99863C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 741),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", 741),
            new OctiBotEngine.OpeningBookCandidate("52+h", 741)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x40009740CB8FB086L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 2P 1-Base ply 4 (after 33+b 57+f 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x4020991903FBD91EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -210)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x402AE0A5BAA4ABC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x40398A10E6A86F8EL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4050DC0814955FA0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280)),
        // 4P ply 4 (after 33+d 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4056FA6E897921B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x405A1D0A099AA107L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // 2P 1-Base ply 4 (after 53+b 57+e 33+h 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x40748F678B52764BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // OFK ply 3 (after 32+a 46+e 42+c)
        new OctiBotEngine.OpeningBookEntry(0x40A9709E46569B37L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("56+g", -3963)),
        // 4P ply 4 (after 33+c 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x40A9ACBD690DB46BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+c 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x40C5AFD9711BDAD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x40C93A2512957416L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -520),
            new OctiBotEngine.OpeningBookCandidate("73+h", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -860)),
        // 2P 3-Base ply 4 (after 33+h 77+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x40CA87F94896BFC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62)),
        // 2P 3-Base ply 4 (after 33+h 37+f 33h+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x40D93C9B8F87021AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -258),
            new OctiBotEngine.OpeningBookCandidate("53+h", -258),
            new OctiBotEngine.OpeningBookCandidate("73+h", -258)),
        // OFK ply 4 (after 32+a 46+f 22+c 26+d)
        new OctiBotEngine.OpeningBookEntry(0x40E14BEE08A791EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -3795),
            new OctiBotEngine.OpeningBookCandidate("42+h", -3840),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -4002)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x40E863113DBBA020L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+h 77+d)
        new OctiBotEngine.OpeningBookEntry(0x40EDD137D91D5115L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x410FC127DBC829BFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140)),
        // OFK ply 4 (after 32+b 36+d 32b-43 36d+g)
        new OctiBotEngine.OpeningBookEntry(0x411F69311F305B29L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 2418),
            new OctiBotEngine.OpeningBookCandidate("22+b", 2418),
            new OctiBotEngine.OpeningBookCandidate("22+c", 2418)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4122285FF79DA0D4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // OFK ply 4 (after 32+a 36+d 22+c 36d+a)
        new OctiBotEngine.OpeningBookEntry(0x418EDC73C52CD4B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 0),
            new OctiBotEngine.OpeningBookCandidate("42+e", 0),
            new OctiBotEngine.OpeningBookCandidate("22c+b", 0)),
        // 2P 3-Base ply 3 (after 33+h 37+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x419441723BC42EA6L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2402),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2402)),
        // 2P 1-Base ply 3 (after 53+c 57+c 33+a)
        new OctiBotEngine.OpeningBookEntry(0x41B89D4739C43174L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2940)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x41C25C769D9433A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 2 (after 33+c 37+c)
        new OctiBotEngine.OpeningBookEntry(0x41D9D4FB0D0A410CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x41DF68DEC8308D25L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 462),
            new OctiBotEngine.OpeningBookCandidate("33+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 462)),
        // 2P 1-Base ply 3 (after 53+a 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x41E059557B19E86BL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -3350),
            new OctiBotEngine.OpeningBookCandidate("77+e", -3350)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x41FC3E1FE960EB21L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 3 (after 33+g 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x4203859C001C559CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 3-Base ply 2 (after 33+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x4224E476CCEBC472L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 3-Base ply 4 (after 33+a 77+e 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4235B709F1CD5FE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4249A7BC31ED9156L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -258),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -258),
            new OctiBotEngine.OpeningBookCandidate("53+h", -378)),
        // OFK ply 3 (after 22+b 46+e 42+a)
        new OctiBotEngine.OpeningBookEntry(0x4254EDCB87DC3F75L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4483),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4706)),
        // 4P ply 3 (after 33+g 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x426F86F8180A3B2EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4287474A324C3195L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 150),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 60)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+a 57c+f)
        new OctiBotEngine.OpeningBookEntry(0x4288B8C9AD6C6C6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x428DCC8ADA46D9B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 2P 1-Base ply 3 (after 53+a 57+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x42C391655E98CFE9L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2360),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2360)),
        // 4P ply 4 (after 33+a 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x42D4B724EC184BF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 3 (after 53+b 57+d 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x42E9BA2B3FC6ED03L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57d+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("77+e", -3350)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x42F6650961FB20FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 3-Base ply 1 (after 53+b)
        new OctiBotEngine.OpeningBookEntry(0x42F7FBED36E9148FL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+c", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2486)),
        // 4P ply 2 (after 33+c 37+g)
        new OctiBotEngine.OpeningBookEntry(0x430E42049EDD39F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+c)
        new OctiBotEngine.OpeningBookEntry(0x431F8776FC773BB3L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1770),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1770),
            new OctiBotEngine.OpeningBookCandidate("57+d", -1800)),
        // 2P 1-Base ply 3 (after 53+c 57+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0x433D49D7B5CA6E7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2080)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+b 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x43574757AED351C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // OFK ply 4 (after 22+b 36+d 22b+c 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x437497F8C8B510FFL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107),
            new OctiBotEngine.OpeningBookCandidate("42+e", -3257)),
        // 2P 3-Base ply 4 (after 33+a 37+d 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x439500BADB76A369L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -258),
            new OctiBotEngine.OpeningBookCandidate("53+h", -258),
            new OctiBotEngine.OpeningBookCandidate("53+b", -258)),
        // 2P 3-Base ply 2 (after 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x43A29403030D2F6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x43DC7FDEA12140EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // OFK ply 4 (after 22+b 56+f 42+e 46+e)
        new OctiBotEngine.OpeningBookEntry(0x43DC9C14EB2873A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("32e+h", -2105),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3107)),
        // 2P 3-Base ply 2 (after 53+b 57+c)
        new OctiBotEngine.OpeningBookEntry(0x43E4174DE4C570F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400)),
        // OFK ply 4 (after 32+b 56+f 52+a 56f+g)
        new OctiBotEngine.OpeningBookEntry(0x43F160C1C2C181C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -180),
            new OctiBotEngine.OpeningBookCandidate("52a+h", -180),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1200)),
        // 4P ply 4 (after 33+c 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x43F196A68D74C981L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x44222010A9B5DDE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // OFK ply 3 (after 22+b 36+e 42+g)
        new OctiBotEngine.OpeningBookEntry(0x445EDAFAC854F222L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -4103),
            new OctiBotEngine.OpeningBookCandidate("36e+d", -4103),
            new OctiBotEngine.OpeningBookCandidate("56+f", -4720)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+c 57+e)
        new OctiBotEngine.OpeningBookEntry(0x447269EA1250C284L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -800),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -800),
            new OctiBotEngine.OpeningBookCandidate("33+a", -800)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x44A484C60C62EF6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -610),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -650),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730)),
        // 4P ply 4 (after 33+b 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x44B1FF1B4A4C5C5FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x44C962DAEBC14EC4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x44DCF328CB44D684L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4501C52655091DFAL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53+b", 62)),
        // 2P 1-Base ply 3 (after 53+b 57+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x45214752111745C5L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -3260)),
        // OFK ply 4 (after 32+a 36+e 42+c 26+d)
        new OctiBotEngine.OpeningBookEntry(0x453571BA88795EEFL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x4548ABC8264C443DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x454E3D917D24CFADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120)),
        // 2P 1-Base ply 3 (after 33+b 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x45525ADBEF5DCCE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3230),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3230),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3650)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x456C42257CED5C00L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -520),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -520)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4582E4B7D314BEE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 62)),
        // 2P 1-Base ply 2 (after 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x45B22A66C3B98893L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 3 (after 33+g 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x45B83DF8B500CD7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x45C582264FC65479L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x45CB64BF5BE14396L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x461EF42578163631L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 2P 1-Base ply 3 (after 33+b 77+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x4625849542754430L, 8,
            new OctiBotEngine.OpeningBookCandidate("77", -3030),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090)),
        // 2P 1-Base ply 3 (after 53+a 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x46C65ED61A5B1B77L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2360),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2360)),
        // 4P ply 2 (after 33+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x46E120876BBFC649L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x46F5B6EF3A62A513L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x471214D9DC112C8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // OFK ply 4 (after 32+b 46+f 52+g 46f+a)
        new OctiBotEngine.OpeningBookEntry(0x4726DADD39EF2418L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 0),
            new OctiBotEngine.OpeningBookCandidate("52g+h", 0)),
        // 2P 3-Base ply 3 (after 33+a 37+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x47288225FD32F1D7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2144)),
        // OFK ply 4 (after 32+b 56+f 32b-43 36+g)
        new OctiBotEngine.OpeningBookEntry(0x47324EFC4EA7B0A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 2320),
            new OctiBotEngine.OpeningBookCandidate("52+h", 2320),
            new OctiBotEngine.OpeningBookCandidate("52+g", 1931)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+c 57+f)
        new OctiBotEngine.OpeningBookEntry(0x474C9CF560E7D579L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1010),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1010),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -2250)),
        // 2P 3-Base ply 3 (after 33+b 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x47607F3C6B886C6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 4P ply 3 (after 33+g 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x476FAB0726D7B58EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+b 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x479052D8BDCDA917L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 53+c 37+d 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x47932D13B3F749BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -400)),
        // 2P 3-Base ply 3 (after 53+a 37+d 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x47ABA3B47B2F52CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2206),
            new OctiBotEngine.OpeningBookCandidate("37d+e", -2396)),
        // OFK ply 4 (after 22+b 26+d 42+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x47C8ECEF49F3FAADL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3498)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x47F147B00E9E0E1BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x47F883131DDD3780L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 400)),
        // 4P ply 1 (after 33+a)
        new OctiBotEngine.OpeningBookEntry(0x4806FEE198284FFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
        // 4P ply 4 (after 33+h 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x480E12028EF174AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x4810072FBEAB60DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53+b", 62)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x481FF3D1705E9679L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4837EC4462AF0760L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73+b", 338)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4869E1C90D5BA071L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -730)),
        // OFK ply 4 (after 22+b 36+e 42+h 46+c)
        new OctiBotEngine.OpeningBookEntry(0x487DE6133C2C6518L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 1370),
            new OctiBotEngine.OpeningBookCandidate("22b-33", 1370),
            new OctiBotEngine.OpeningBookCandidate("42h+g", 1370)),
        // 4P ply 3 (after 33+c 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x48B474AED4E98B29L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x48C0356E0E882379L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 60),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0)),
        // 4P ply 4 (after 33+g 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x48E00A0EEB448AE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x491C9268E4131146L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -340),
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -590)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x492A64F041977986L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // OFK ply 3 (after 32+b 56+f 52+a)
        new OctiBotEngine.OpeningBookEntry(0x49393E0C2BC37BD6L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -3323),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("26d+c", -3683)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4946679459811734L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 3 (after 33+b 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x49527505A5AB36C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2282)),
        // 2P 3-Base ply 4 (after 33+b 37+e 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x4985CEC727750B26L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -120)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+c)
        new OctiBotEngine.OpeningBookEntry(0x49A92D0E2C1AB230L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1770),
            new OctiBotEngine.OpeningBookCandidate("57+d", -1770),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1800)),
        // 4P ply 3 (after 33+b 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x49B3870138666255L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // OFK ply 3 (after 32+b 46+e 42+a)
        new OctiBotEngine.OpeningBookEntry(0x49C0AE704C749AD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435)),
        // OFK ply 3 (after 22+b 56+f 32+a)
        new OctiBotEngine.OpeningBookEntry(0x49C94E3C2349A1E2L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("56f+a", -6783)),
        // OFK ply 4 (after 22+b 56+f 42+e 36+d)
        new OctiBotEngine.OpeningBookEntry(0x49D8993D31F8DB22L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("32e+h", -1887),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3107)),
        // 4P ply 4 (after 33+a 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x49E8C2F4A67D81FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x49F9DE72E755C591L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -830),
            new OctiBotEngine.OpeningBookCandidate("53+b", -830),
            new OctiBotEngine.OpeningBookCandidate("73b+h", -830)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4A1E5D8FBDF86ADEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 3-Base ply 3 (after 33+a 37+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x4A3942D387B9B1CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37f+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 4P ply 3 (after 33+c 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x4A4A232F82556B17L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 3 (after 33+a 37+d 33a+b)
        new OctiBotEngine.OpeningBookEntry(0x4A56B6C2F6C9373CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 2P 3-Base ply 3 (after 33+h 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x4A71BD35802A114BL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2276),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2276),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2282)),
        // 2P 3-Base ply 4 (after 33+h 77+e 33h+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4A810D0074170AD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4AA606F702F42CA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4AC8652EE389EE8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+b 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4AFE29A6D7F1F9AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4B06B144284FD02CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+c)
        new OctiBotEngine.OpeningBookEntry(0x4B0C4416ED6513C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("77+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -1686)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4B169575F0C161C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+a 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x4B21525C4D951FBDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 2P 3-Base ply 4 (after 33+a 77+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4B4E139BC8B4B80AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // OFK ply 4 (after 32+b 46+f 42+a 46f-35)
        new OctiBotEngine.OpeningBookEntry(0x4B4EC1F5161C867BL, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", -267),
            new OctiBotEngine.OpeningBookCandidate("32b+a", -688),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1012)),
        // 2P 1-Base ply 3 (after 53+c 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x4B9D923BAA04BA1CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2950),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2950),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2950)),
        // 4P ply 3 (after 33+h 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x4BAD81196426E288L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x4BB81D68878E1BEBL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4BCD320A4EA91B17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4C07A077531F7E4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 4 (after 33+g 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4C2A490F7F4AF726L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4C3223F7266CD3FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338)),
        // OFK ply 4 (after 32+a 56+f 22+c 36+a)
        new OctiBotEngine.OpeningBookEntry(0x4C919A6960E9AED0L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 0),
            new OctiBotEngine.OpeningBookCandidate("52g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32+e", 0)),
        // 4P ply 4 (after 33+a 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4C9B619971A5EE07L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 4 (after 22+b 36+d 42+c 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x4CA4A88592BB7770L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("22b+e", -3500)),
        // 2P 3-Base ply 4 (after 53+b 77+e 53b+a 77e+f)
        new OctiBotEngine.OpeningBookEntry(0x4CBF7C31D103B2C0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4CD5B0D0E450EBBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4CF762FD69B380B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // OFK ply 4 (after 32+b 36+e 52+g 46+a)
        new OctiBotEngine.OpeningBookEntry(0x4D22DFF4E33F8C93L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 0),
            new OctiBotEngine.OpeningBookCandidate("52g+h", 0)),
        // 4P ply 3 (after 33+a 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x4D5B162D218B7499L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 4 (after 33+b 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4D73DD4A5695766BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 4 (after 22+b 26+d 42+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x4DCCE9C693235226L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+b", -3498)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4DDB470F321DFFD8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+d 53+b 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x4DF3433BEBBDA304L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62)),
        // 2P 3-Base ply 3 (after 53+b 37+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x4E1489541ADA398FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2264)),
        // 4P ply 2 (after 33+g 37+d)
        new OctiBotEngine.OpeningBookEntry(0x4E2BCA2C3A5FE760L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
        // 2P 3-Base ply 3 (after 33+h 37+f 33h+a)
        new OctiBotEngine.OpeningBookEntry(0x4E3C8D60C37A6553L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // OFK ply 4 (after 32+a 36+d 42+c 36d+a)
        new OctiBotEngine.OpeningBookEntry(0x4E5EE30E9F22B337L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1040),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1040),
            new OctiBotEngine.OpeningBookCandidate("22+c", -1040)),
        // 2P 1-Base ply 3 (after 53+b 57+e 33+h)
        new OctiBotEngine.OpeningBookEntry(0x4E913E9CC7AF1102L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2360),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4ED03688C0C806B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4EEF51F946A2AFE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73h+a", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -520)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4EF7116F1184899CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4EFDDFF0EC9D8FD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+g 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4F01FD8672366B96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x4F0C2C0D24A7B6B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 1-Base ply 3 (after 53+b 37+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0x4F198FE4A59D8C68L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090)),
        // 2P 3-Base ply 3 (after 53+b 57+g 53b+a)
        new OctiBotEngine.OpeningBookEntry(0x4F56C8095116EA68L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2486)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4F6DFEE26A200524L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 3 (after 33+a 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x4F8C80D2B25C0C6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 3 (after 53+b 57+g 33+a)
        new OctiBotEngine.OpeningBookEntry(0x4FD5E998D70B4975L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -2486)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4FEFAB779BE1215EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5043C68DDC224EC0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 62)),
        // 2P 3-Base ply 2 (after 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x505295F2E104D553L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 3 (after 33+c 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x50AF4AEAB1F305B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x50B2E54FCDE09127L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2396),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2402)),
        // OFK ply 4 (after 32+b 56+f 52+g 46+f)
        new OctiBotEngine.OpeningBookEntry(0x50E7CBFEBC729710L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -227),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -227),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -657)),
        // 4P ply 3 (after 33+g 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x511282801B1B062DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5146976995D906C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 4P ply 4 (after 33+g 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x515D59D03C9B99ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+c 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x516F3D5EE1DD9F2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 4 (after 33+b 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x518B617162EA1DFEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x51D87F64E65C7861L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+h 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x51E2E2C913667637L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 4P ply 4 (after 33+a 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x51F3FCB0C3670F65L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x520563CBD8E2E446L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 1-Base ply 3 (after 53+c 57+e 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x520E1FF83C0594D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2950)),
        // 2P 1-Base ply 3 (after 53+a 57+c 53a+h)
        new OctiBotEngine.OpeningBookEntry(0x5225C6638092C4EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2880),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3000),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -3750)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x5239A12912EBC7A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 280),
            new OctiBotEngine.OpeningBookCandidate("53+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280)),
        // 2P 3-Base ply 3 (after 33+b 37+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x526A818E7419A2EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2382),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2402)),
        // OFK ply 4 (after 32+b 36+d 52+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x5285488F6CDD3F7FL, 8,
            new OctiBotEngine.OpeningBookCandidate("52a+h", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3602),
            new OctiBotEngine.OpeningBookCandidate("32b+g", -3782)),
        // 2P 3-Base ply 3 (after 53+a 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x52A15D0664BE9C48L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 4P ply 2 (after 33+c 37+b)
        new OctiBotEngine.OpeningBookEntry(0x52A4D0830E1B7C0FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 120),
            new OctiBotEngine.OpeningBookCandidate("77+a", 120),
            new OctiBotEngine.OpeningBookCandidate("77+g", 0)),
        // 2P 1-Base ply 2 (after 53+c 57+f)
        new OctiBotEngine.OpeningBookEntry(0x52AE18DE025CFEA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -1180)),
        // OFK ply 3 (after 22+b 36+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0x52B6F3B9411E31F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4483),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -4483)),
        // 4P ply 3 (after 33+a 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x52D2517334E6FA2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 53+b 57+g 73+b)
        new OctiBotEngine.OpeningBookEntry(0x52F91EDC3B597B14L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424)),
        // OFK ply 4 (after 32+b 46+e 32b-43 56+f)
        new OctiBotEngine.OpeningBookEntry(0x533F399976A91E99L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -59),
            new OctiBotEngine.OpeningBookCandidate("52+h", -227),
            new OctiBotEngine.OpeningBookCandidate("52+b", -1020)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x535CF78EF13D650BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5371B1FAA6066522L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x538EE7294672F0F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 2 (after 33+b 37+f)
        new OctiBotEngine.OpeningBookEntry(0x53A3232CE2949573L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+c 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x53B8ABA1720AE7DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // OFK ply 3 (after 22+b 56+f 42+c)
        new OctiBotEngine.OpeningBookEntry(0x53BF987A8382CE08L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("46+f", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4663)),
        // 4P ply 3 (after 33+g 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x53C5147F88CC7ED8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 3 (after 33+h 57+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x53E230F6162B3F87L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2024)),
        // 2P 1-Base ply 3 (after 53+a 37+d 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x53E4A6AF4D43B417L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2690)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+h 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x540B02747D8B09FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53bg-64", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 2P 1-Base ply 3 (after 33+b 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x54217B6F742F184CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -3230)),
        // 2P 3-Base ply 4 (after 33+b 57+e 33b+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x5447C50F0B5FABBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x546F10A1DF00118FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 3-Base ply 4 (after 33+b 37+d 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x546F9C0235973FC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 57g+d)
        new OctiBotEngine.OpeningBookEntry(0x54725BC812DD7D06L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280)),
        // 4P ply 4 (after 33+a 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x54805FDD14BF609FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 4 (after 22+b 36+e 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x548597DDF2E82CCFL, 8,
            new OctiBotEngine.OpeningBookCandidate("32g+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("22+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3553)),
        // OFK ply 3 (after 22+b 26+d 22b+a)
        new OctiBotEngine.OpeningBookEntry(0x54E48BB7917BCC4FL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -3323),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("56f-45", -3395)),
        // OFK ply 4 (after 32+a 56+f 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x54F5043561AFF315L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32e+b", 0),
            new OctiBotEngine.OpeningBookCandidate("52+g", 0)),
        // 2P 3-Base ply 3 (after 53+b 37+c 33+b)
        new OctiBotEngine.OpeningBookEntry(0x550A0B1520419110L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2424)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x5512DFA39DBE0C9AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // OFK ply 3 (after 22+b 36+e 22b-33)
        new OctiBotEngine.OpeningBookEntry(0x551A843CCE16CE1DL, 8,
            new OctiBotEngine.OpeningBookCandidate("46e+f", -2703),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3822),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3903)),
        // OFK ply 4 (after 32+b 36+e 32b+e 46+a)
        new OctiBotEngine.OpeningBookEntry(0x554641A8E279D156L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32be-43", 0),
            new OctiBotEngine.OpeningBookCandidate("52+h", 0)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5568E30E338FF8F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 3 (after 32+b 46+e 52+g)
        new OctiBotEngine.OpeningBookEntry(0x557E3DF6606D5235L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4103),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4103),
            new OctiBotEngine.OpeningBookCandidate("36e+d", -4103)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5588B29739739810L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+c 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x55A57E5F75D3E2E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // OFK ply 4 (after 22+b 46+f 42+h 26+d)
        new OctiBotEngine.OpeningBookEntry(0x55A61F5DCDE7C76EL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 640),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 640),
            new OctiBotEngine.OpeningBookCandidate("52h+b", -1020)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x55B33B04DF0C0DFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // 2P 3-Base ply 4 (after 33+h 37+e 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x55C0794B57077140L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x55DDB0D7A29FACFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 2P 1-Base ply 4 (after 53+a 37+d 53a+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x55E1BB230CCD2939L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", -470)),
        // OFK ply 4 (after 22+b 36+e 42+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x5623995E4DAD0BD0L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("32g+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("32g+h", -2108)),
        // 4P ply 3 (after 33+h 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x562B218CD3A40E36L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x562B6AADFD449265L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33+b", 338)),
        // 2P 3-Base ply 2 (after 53+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x5663B6390C3BDF4BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 2P 1-Base ply 2 (after 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x566A729A1F78E6D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x5675BF2EF9CBFEA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -810)),
        // 2P 1-Base ply 3 (after 53+c 57+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0x567F4A7C3CE13D41L, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2080)),
        // 2P 3-Base ply 3 (after 53+a 37+e 33+h)
        new OctiBotEngine.OpeningBookEntry(0x56A492B5207D48D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 4P ply 3 (after 33+g 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x56C53980B611F078L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 3-Base ply 3 (after 53+b 57+g 33+h)
        new OctiBotEngine.OpeningBookEntry(0x56FCD16F7F9AAF8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424)),
        // 2P 1-Base ply 3 (after 53+c 57+c 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x575089FA47092ADDL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -3350)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5776C0A60F3A8BBCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 3-Base ply 4 (after 33+a 77+e 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x5777B4A278E60CDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5796B48F653318CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x57B466A2E8D073C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // OFK ply 3 (after 32+b 46+f 52+g)
        new OctiBotEngine.OpeningBookEntry(0x57D83375DF28752AL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("36d+a", -3463)),
        // 2P 3-Base ply 3 (after 33+h 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x57E7FF4552E8EB19L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+d", -2024)),
        // 2P 1-Base ply 4 (after 33+b 37+d 33b-44 57+d)
        new OctiBotEngine.OpeningBookEntry(0x57E84C8E7E5B5F66L, 8,
            new OctiBotEngine.OpeningBookCandidate("73", 270),
            new OctiBotEngine.OpeningBookCandidate("53+a", -3390),
            new OctiBotEngine.OpeningBookCandidate("64h+g", -3510)),
        // 4P ply 3 (after 33+b 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x58191586A8A027A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 2P 3-Base ply 3 (after 33+b 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x582154B13ED9E262L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2702),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2702),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2702)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5831DEE1DFBE2551L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x583BA5A2F7C86AB8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+c", -730)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x583D33FBACA0E128L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5841234E6C802F9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // OFK ply 4 (after 32+b 56+f 52+g 36+d)
        new OctiBotEngine.OpeningBookEntry(0x5845C054D9E71884L, 8,
            new OctiBotEngine.OpeningBookCandidate("52g+b", -3440),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3840),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -3979)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x584DC42AEC63AF28L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260)),
        // 2P 3-Base ply 3 (after 53+b 57+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x58524938C0936B40L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2144),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2144),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2264)),
        // OFK ply 3 (after 32+a 26+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0x585A7F9294D4AAC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4796),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4823)),
        // 4P ply 4 (after 33+g 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5880F677D1513C70L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 4P ply 4 (after 33+c 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x58B292F90C173AF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x58BC0978F4C3EA24L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 3-Base ply 4 (after 33+b 57+e 33b+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x58BE126A2ABD4235L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 2P 3-Base ply 2 (after 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x58C1240C123DA616L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+e 33h+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x58C202DFEA9D8C82L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -378),
            new OctiBotEngine.OpeningBookCandidate("53+h", -378),
            new OctiBotEngine.OpeningBookCandidate("73+h", -378)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x58CC72DE493DD1DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+b", 280)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x58D1BB42BCA50C64L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -430),
            new OctiBotEngine.OpeningBookCandidate("53+h", -430),
            new OctiBotEngine.OpeningBookCandidate("73b+g", -462)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x58F6EF73BC07DF8DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+h", -62)),
        // 2P 1-Base ply 4 (after 53+b 37+d 53b-64 57+e)
        new OctiBotEngine.OpeningBookEntry(0x596E518934C94AB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5997B37B6D7BB885L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x59A480851E373158L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // OFK ply 4 (after 32+b 36+e 32b+e 46+f)
        new OctiBotEngine.OpeningBookEntry(0x59ACEBE305D44B2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("42eh-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("42eh+b", -1887)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x59B538010BF44D94L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -30),
            new OctiBotEngine.OpeningBookCandidate("73+a", -30),
            new OctiBotEngine.OpeningBookCandidate("53bc-64", -740)),
        // 4P ply 4 (after 33+b 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x59B56156E098D38FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x59D96232F88EBD3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // OFK ply 3 (after 22+b 46+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0x5A14F813248BBE62L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3690),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3690),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -4022)),
        // 4P ply 3 (after 33+g 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x5A18BBD86506DB04L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5A57608842864485L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 2P 1-Base ply 3 (after 53+b 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0x5A5B8C4F2CB6DF52L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2690),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090)),
        // 2P 3-Base ply 3 (after 53+b 57+g 33+b)
        new OctiBotEngine.OpeningBookEntry(0x5A97EA335E201A4FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424)),
        // 4P ply 3 (after 33+b 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x5ACE83793B775F56L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // OFK ply 4 (after 22+b 26+d 22b+a 26d-35)
        new OctiBotEngine.OpeningBookEntry(0x5B142BBB1512243AL, 8,
            new OctiBotEngine.OpeningBookCandidate("22ab+h", 343),
            new OctiBotEngine.OpeningBookCandidate("22ab-23", -23),
            new OctiBotEngine.OpeningBookCandidate("22ab-33", -820)),
        // 2P 3-Base ply 3 (after 33+b 77+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x5B568AFF93F16AB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2402)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5B73167A8DE049ADL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 3-Base ply 3 (after 53+a 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x5BB550BCCBDF35F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // OFK ply 4 (after 22+b 46+e 22b+c 56+f)
        new OctiBotEngine.OpeningBookEntry(0x5BB80248EA6E2E6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3107),
            new OctiBotEngine.OpeningBookCandidate("32+h", -3747)),
        // 4P ply 4 (after 33+g 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5BC7419AC43BCE72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+g 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x5BD52BC8C025DB3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73g+h", -462)),
        // 4P ply 4 (after 33+c 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5BEAA8E2E86E4719L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5BF2737299242BE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120)),
        // 4P ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x5C3D5244B85BC3A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+e 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5C44ECFD2843FB01L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5C4AB576455F41B6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 3-Base ply 4 (after 53+a 37+e 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5C5755607E7B7BE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // OFK ply 4 (after 32+b 56+f 32b+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x5C753B2BD92D4D52L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+c", -3367)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5C7CF9FE71275695L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x5C8F307538E4C5A4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 3-Base ply 3 (after 53+a 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x5C93573FAA9DC6EEL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("57d+f", -2734)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5CAAC15F2F56D2C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+b 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x5CABD9986245DD99L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+a", -1200)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5CC7CD6CAE5E581CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -310)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5CC9BD6D0DFE0545L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280),
            new OctiBotEngine.OpeningBookCandidate("73+a", 90)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+h 57+g)
        new OctiBotEngine.OpeningBookEntry(0x5CDA0859183D27E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 462),
            new OctiBotEngine.OpeningBookCandidate("73+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 462)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x5CDDB052EB25E161L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -340),
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", -470)),
        // 4P ply 3 (after 33+a 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x5CF184AAB14D316FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x5CF320C0F8C40B13L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62)),
        // 2P 1-Base ply 4 (after 53+a 37+d 53a+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x5D01175401BED35EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x5D1B6895D5EC571DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 62)),
        // 4P ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0x5D44FD4A11031CC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
        // 2P 3-Base ply 4 (after 33+b 77+f 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x5D539773D27FF79BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+b", -120)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5D5DF07AF975C543L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5D6EC3848A394C9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5D80DB88EF8CB2D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 3 (after 33+a 77+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x5DA327462642EC03L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5DAD32F0C3D93BBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+d 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5DD05AFB3EFBCD0EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x5DE80EEAD5B64F80L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338)),
        // OFK ply 4 (after 32+b 36+e 42+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x5DEC9DF5A7687B4EL, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1020),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1079)),
        // OFK ply 3 (after 22+b 36+e 22b+c)
        new OctiBotEngine.OpeningBookEntry(0x5E24036C2441EBB4L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3283),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3283),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+b 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x5E24493D7F577F4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+e 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5E28F9DD1B3EF30DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5E5496DE79EA32F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5EAB6F01E2F02E60L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 1-Base ply 3 (after 53+c 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x5EDF9190232FE926L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2830),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2840),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2950)),
        // 4P ply 3 (after 33+c 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5EE09C572C4EA041L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 2 (after 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x5EFA1EC30EA548D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5F6724CD0DC2DC83L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // OFK ply 3 (after 32+b 36+d 52+g)
        new OctiBotEngine.OpeningBookEntry(0x5F7A38DFBABDFABEL, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3980),
            new OctiBotEngine.OpeningBookCandidate("46f+d", -4542),
            new OctiBotEngine.OpeningBookCandidate("46f+e", -4843)),
        // 2P 1-Base ply 1 (after 53+b)
        new OctiBotEngine.OpeningBookEntry(0x5F84F587E76D3A0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -3350),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3350)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5F8A66856AA2BDB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 3 (after 53+a 57+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x5FB09F0F8F1CE16CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5FBC2A0D5EDAAA95L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 4 (after 22+b 36+d 32+a 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x5FCA047EE4ACD819L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1079),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 2P 3-Base ply 2 (after 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5FE7238F737F550AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -306),
            new OctiBotEngine.OpeningBookCandidate("53+h", -342)),
        // 4P ply 3 (after 33+a 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x602A77D796F4454DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 3 (after 53+b 77+e 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x6069F8C2344D639AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2480),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480)),
        // 2P 1-Base ply 3 (after 53+c 37+d 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x607A9F2786E5E183L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2290)),
        // 4P ply 4 (after 33+g 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x607F7BB767A63848L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x6081478ADA9D77E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x609163BB0213C606L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 2 (after 33+d 37+b)
        new OctiBotEngine.OpeningBookEntry(0x60A3A0F44C7D56B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // OFK ply 4 (after 22+b 56+f 42+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x60D228AD3AC7B019L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1615),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2348),
            new OctiBotEngine.OpeningBookCandidate("32+c", -2350)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x61410BEB2E15773BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6176C18DE4604F99L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6189975E0414DA42L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 3 (after 53+a 57+e 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x61902670F7A3C144L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3350)),
        // 2P 3-Base ply 3 (after 33+h 37+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x6196B029ACCB4AD4L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2024)),
        // 2P 1-Base ply 3 (after 53+c 57+c 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x61BBFFEB4B349179L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -3350)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x61BFDBD6306CCD61L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // OFK ply 4 (after 32+a 56+f 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x622FB5F8FB4D145BL, 8,
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1287),
            new OctiBotEngine.OpeningBookCandidate("42c+a", -2107),
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108)),
        // 2P 3-Base ply 4 (after 33+h 37+f 33h+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x62597E72840C8C83L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+h 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x626F343A54AF2638L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6278BEE54EEFE9C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -690)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+b 57c+f)
        new OctiBotEngine.OpeningBookEntry(0x628BDA65DF9CCC67L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 4P ply 3 (after 33+d 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x62A83A9DF3952F0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x62DA5FE302112F9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+b", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+d 53a+b 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x62F25BD7DBB17342L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 3-Base ply 1 (after 33+a)
        new OctiBotEngine.OpeningBookEntry(0x62F499414419B482L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 2P 3-Base ply 4 (after 53+b 57+g 53b+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x63333B1B166003B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 4P ply 4 (after 33+d 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x63684D29A3BBB594L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 3-Base ply 3 (after 33+b 77+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x6369DF1F63D15F69L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402)),
        // 4P ply 1 (after 33+h)
        new OctiBotEngine.OpeningBookEntry(0x6381082F6AFAE839L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6389E4CC7C23D36BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x639207B23FA9C903L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x63A57F749E8926CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x63AB36E1F1C0B861L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+h 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x640B761C408EABA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // OFK ply 4 (after 32+b 46+e 32b+c 36+e)
        new OctiBotEngine.OpeningBookEntry(0x6477F153A5376160L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("42gh-33", -2108)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x64C3781C368E795AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x64EB7C28EF2E2586L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 3 (after 53+a 37+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x65385A3FB7B5618BL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37f+e", -2086)),
        // 2P 1-Base ply 4 (after 33+b 77+f 53+h 77)
        new OctiBotEngine.OpeningBookEntry(0x6567591C117125C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73h-64", 260),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("73", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x658015EC441BDA95L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // 4P ply 4 (after 33+b 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x65B6E31FF619BD52L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 3 (after 53+b 57+f 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x65FF60ADC2D458AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2480)),
        // 2P 1-Base ply 4 (after 53+b 57+d 53b-64 77+e)
        new OctiBotEngine.OpeningBookEntry(0x660B135F658B42C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -340),
            new OctiBotEngine.OpeningBookCandidate("44h+a", -340),
            new OctiBotEngine.OpeningBookCandidate("73+a", -390)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6638E1A54C1144EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x665CB1C1C0CF581DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 2 (after 53+b 37+c)
        new OctiBotEngine.OpeningBookEntry(0x666405406EA10F99L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x666860D69D663B34L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x667F82ABC76B7CADL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53bg+h", -1330)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x6684D62B1779DB60L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // OFK ply 3 (after 22+b 46+f 22b+a)
        new OctiBotEngine.OpeningBookEntry(0x668811A8525F85C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("26+d", -3323),
            new OctiBotEngine.OpeningBookCandidate("46f+e", -3903)),
        // 2P 1-Base ply 4 (after 33+b 37+d 33b-44 57+f)
        new OctiBotEngine.OpeningBookEntry(0x66D363FB47DCE5BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33", -870),
            new OctiBotEngine.OpeningBookCandidate("44b+a", -3160),
            new OctiBotEngine.OpeningBookCandidate("53+a", -3310)),
        // 4P ply 3 (after 33+h 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x66DCE0E3D359D35CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x66DF905046D2FB00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x66F8C69ED75915D7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // OFK ply 4 (after 22+b 36+e 32+a 46+c)
        new OctiBotEngine.OpeningBookEntry(0x6702E9E7050B01A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("52h-43", 1527),
            new OctiBotEngine.OpeningBookCandidate("32+b", 1370),
            new OctiBotEngine.OpeningBookCandidate("32+c", 1370)),
        // 2P 1-Base ply 4 (after 53+b 57+f 33+a 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x67161A6D3030FF77L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+h 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x671C9757837749C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 4 (after 32+b 36+d 52+g 36d+f)
        new OctiBotEngine.OpeningBookEntry(0x6722EBA231C876C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42h+f", -2628),
            new OctiBotEngine.OpeningBookCandidate("32+g", -2628),
            new OctiBotEngine.OpeningBookCandidate("42h+a", -2628)),
        // OFK ply 4 (after 32+b 26+d 32b+e 36+e)
        new OctiBotEngine.OpeningBookEntry(0x673B9A4AAB950FF5L, 8,
            new OctiBotEngine.OpeningBookCandidate("32be-43", -1068),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1068),
            new OctiBotEngine.OpeningBookCandidate("32be+h", -2105)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x676175E065CEC5A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x677094339B612770L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6793ADA5ED45D7FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -378)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x67A20E2837B5C852L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+h 37e+d)
        new OctiBotEngine.OpeningBookEntry(0x67BBAB6EA5CCB618L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x67C7BBDB65EC78AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 3-Base ply 4 (after 33+a 57+d 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6812F67429A404ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -138),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -138),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -138)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+a 77+d)
        new OctiBotEngine.OpeningBookEntry(0x68136583CC5BC0D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 62)),
        // 2P 3-Base ply 3 (after 53+a 37+e 33+b)
        new OctiBotEngine.OpeningBookEntry(0x686167D05B84BC29L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086)),
        // OFK ply 4 (after 32+b 36+d 32b+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x6879C34B306F04BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x6891D7E5AFB9A7B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // OFK ply 4 (after 22+b 46+f 52+h 36+e)
        new OctiBotEngine.OpeningBookEntry(0x6898AA0550480C39L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1721),
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("42+g", -2961)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x68B04A4B82320A56L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120)),
        // 4P ply 3 (after 33+g 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x68E09D7CC7146464L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 3 (after 33+b 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x68EED4E9A85DFAC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 3 (after 53+c 77+f 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x69469456610D29DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2290)),
        // 4P ply 4 (after 33+h 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x695341EA1ECAEC32L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+c 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x697B6EB2DEFF8776L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -462)),
        // 2P 1-Base ply 2 (after 53+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x6990FABAD6347F57L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // OFK ply 4 (after 22+b 46+e 22b+c 36+a)
        new OctiBotEngine.OpeningBookEntry(0x69C5D9AAA382F0CFL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32+e", -497)),
        // OFK ply 4 (after 32+b 36+e 42+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x69E065954E2A32A0L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1020)),
        // 4P ply 4 (after 33+d 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x69EDD895AA086DA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6A49B3241640947BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+b", 0)),
        // 4P ply 4 (after 33+c 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6A4AB45DAE058593L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 3 (after 33+b 37+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x6A55D46E84399731L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2382),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2396),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2402)),
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+h 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x6A6B7612D2EF3600L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // 2P 3-Base ply 4 (after 33+h 37+d 53+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6A826FAC06E7AADEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -310)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6A85D55864D65166L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400)),
        // OFK ply 4 (after 32+a 56+f 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x6A8DBE529ED89BCFL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -179),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -179),
            new OctiBotEngine.OpeningBookCandidate("22+a", -1339)),
        // 2P 3-Base ply 4 (after 33+h 37+d 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x6AAA69674E6ECB3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -710),
            new OctiBotEngine.OpeningBookCandidate("53h+b", -710),
            new OctiBotEngine.OpeningBookCandidate("53h+g", -744)),
        // 4P ply 4 (after 33+d 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6AB5E28E4E711048L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+e 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6AEE7B58CAFC2F5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6B430F60CB36DA24L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6B4D47F2428A6CEFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // OFK ply 4 (after 22+b 46+e 32+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x6B4F8B5A9E522E8EL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1079),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 4P ply 4 (after 33+a 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6B6F95DFCF6907E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6B76CE61A4F5F903L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -258),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -710)),
        // 2P 3-Base ply 3 (after 53+b 57+c 33+b)
        new OctiBotEngine.OpeningBookEntry(0x6B94B4A249E8E7CBL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+d", -2424)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+h 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x6B951422FCDBA644L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk2() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // 4P ply 4 (after 33+h 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6BAD166B48760C0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 3 (after 33+b 37+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x6BD237ECAE20B850L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2282)),
        // OFK ply 2 (after 32+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x6C04DE9EF3210C59L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("32+b", -620),
            new OctiBotEngine.OpeningBookCandidate("32+a", -820)),
        // 4P ply 4 (after 33+g 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6C5349A540E29100L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // OFK ply 3 (after 22+b 26+d 32+a)
        new OctiBotEngine.OpeningBookEntry(0x6C7584FDEF38CC49L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435)),
        // OFK ply 4 (after 32+a 46+e 42+e 36+d)
        new OctiBotEngine.OpeningBookEntry(0x6C8CDAFEF293853DL, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // OFK ply 4 (after 32+b 36+d 52+a 56+g)
        new OctiBotEngine.OpeningBookEntry(0x6C9CF09187E7BB2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("22a+b", -657),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1200),
            new OctiBotEngine.OpeningBookCandidate("32+a", -1652)),
        // 2P 3-Base ply 4 (after 53+b 37+c 53b+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x6CD3C7889AFC0828L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 4P ply 3 (after 33+d 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6CE7EC206E288AFAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6CFC544C69123358L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53+b", 280),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 280)),
        // 2P 1-Base ply 2 (after 53+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x6D22F93442705BDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // OFK ply 4 (after 32+b 46+f 32b+e 46f-35)
        new OctiBotEngine.OpeningBookEntry(0x6D2964C77F2ABDBAL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("42eh-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("42eh+b", -1660)),
        // 4P ply 2 (after 33+h 37+f)
        new OctiBotEngine.OpeningBookEntry(0x6D66D649996D618CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6D724021C8B002D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 3 (after 33+g 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x6D7D03AD158C0642L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6D85AA473C6F01BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+h 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6D9902EB8AC491F4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6DFF736DD166B102L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // OFK ply 4 (after 32+a 56+f 22+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x6DFF8A85A14373D4L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1343),
            new OctiBotEngine.OpeningBookCandidate("42+c", -2667),
            new OctiBotEngine.OpeningBookCandidate("22c+h", -2667)),
        // 4P ply 4 (after 33+g 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6E84DF5AD335E9F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6E9F2EDCCA1886CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53gh-44", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6EAFA6D40AAD1FA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -258)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6EDF5947D8A5A8CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+b", -1320),
            new OctiBotEngine.OpeningBookCandidate("53bc+h", -1330)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6F46C0D38FF50EBAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338)),
        // 4P ply 4 (after 33+h 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6F4E94141913E901L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6F51C4E424998FE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 280),
            new OctiBotEngine.OpeningBookCandidate("53+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 270)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x6F5B05E639967141L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -258),
            new OctiBotEngine.OpeningBookCandidate("53+h", -258),
            new OctiBotEngine.OpeningBookCandidate("53+b", -310)),
        // 4P ply 4 (after 33+a 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6FA5D6DE5B677A23L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6FAA428781BF1100L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+c 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6FD72A8C7C9DE7B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 3 (after 53+a 37+f 33+b)
        new OctiBotEngine.OpeningBookEntry(0x707A59943E9E32B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2206),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2206),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2206)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+h 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x70E2A744D9E25460L, 8,
            new OctiBotEngine.OpeningBookCandidate("73", 550),
            new OctiBotEngine.OpeningBookCandidate("53", 460),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 4P ply 4 (after 33+a 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x70F4E0B47F32EE68L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x70F6A42DEDEB2F3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 3 (after 33+g 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x70FBA338A20EEAFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 2 (after 33+d 37+g)
        new OctiBotEngine.OpeningBookEntry(0x71093273DCBB1342L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 20),
            new OctiBotEngine.OpeningBookCandidate("77+a", 20),
            new OctiBotEngine.OpeningBookCandidate("77+g", -100)),
        // 2P 1-Base ply 3 (after 33+b 57+c 53+h)
        new OctiBotEngine.OpeningBookEntry(0x714FE5D39124A16EL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3630),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -3630),
            new OctiBotEngine.OpeningBookCandidate("37+d", -4340)),
        // OFK ply 4 (after 22+b 46+e 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x71A147F6A2C4AD0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("52h+g", -497)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x71D5E930F7607DBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 1-Base ply 3 (after 53+b 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x71DC7A81DE647897L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2360),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2360),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480)),
        // 4P ply 4 (after 33+d 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x71F6E6D1CF12E33AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 1-Base ply 4 (after 53+b 37+d 33+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x72000FB831B20318L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x721045B59FA91F40L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7223764BECE5969DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x72518A19CB1F0B0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+b 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x725419C6B91BAC4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 3-Base ply 3 (after 33+b 37+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x7269E32206E902E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086)),
        // 2P 1-Base ply 3 (after 53+c 57+f 33+b)
        new OctiBotEngine.OpeningBookEntry(0x727E60DDB0BFBDFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2950),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2950),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2950)),
        // OFK ply 4 (after 32+a 36+e 42+a 36e+d)
        new OctiBotEngine.OpeningBookEntry(0x727EB15E4C5AAA31L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -88),
            new OctiBotEngine.OpeningBookCandidate("22+b", -640),
            new OctiBotEngine.OpeningBookCandidate("22+c", -1243)),
        // 4P ply 4 (after 33+d 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x72AEDCCA2B6B9ED0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+d 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x72C2DFAE337DF062L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // OFK ply 4 (after 32+b 46+e 32b+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0x72C49F000D10778CL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("42+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("32bg+h", -2108)),
        // 2P 3-Base ply 4 (after 53+b 57+c 53b+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x72E91606E5BA9FC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 4P ply 4 (after 33+g 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x73027FCF64B7054BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+d 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7308B15099AE0304L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 3 (after 32+b 36+e 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x731FF27C74F7B6EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("26+a", -6783)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x73564D8C4E77ABA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120)),
        // OFK ply 4 (after 32+a 46+e 42+c 56+g)
        new OctiBotEngine.OpeningBookEntry(0x735B9DB65551AAE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1040),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1040),
            new OctiBotEngine.OpeningBookCandidate("22+c", -1040)),
        // 4P ply 4 (after 33+h 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x73B6282F2D6C8294L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x73C6D5809E52885AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 2 (after 33+d 37+c)
        new OctiBotEngine.OpeningBookEntry(0x73DEA48C4F6C6BB7L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -120),
            new OctiBotEngine.OpeningBookCandidate("77+a", -120),
            new OctiBotEngine.OpeningBookCandidate("77+g", -240)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x740860D80F7BA9F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520)),
        // 4P ply 4 (after 33+h 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x740D904B98701A73L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+h 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x743BDCC3AC080D50L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7463F392790DD85BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x7466BA9BD67B6214L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 2P 3-Base ply 3 (after 33+h 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x74B44850FBD3E5B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024)),
        // 2P 3-Base ply 2 (after 53+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x74E3F4D007B051D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", -306)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x751564AE9E770637L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // OFK ply 4 (after 22+b 26+d 22b-33 46+e)
        new OctiBotEngine.OpeningBookEntry(0x753A4FD9CC48666EL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3045),
            new OctiBotEngine.OpeningBookCandidate("43h+b", -3045),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3662)),
        // OFK ply 4 (after 32+b 26+d 52+g 46+f)
        new OctiBotEngine.OpeningBookEntry(0x755B013F7003FABBL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1343),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1343),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1839)),
        // OFK ply 4 (after 32+b 26+d 52+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0x755EF6136233FA46L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1839)),
        // 4P ply 3 (after 33+g 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x75663DE9709688DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 3 (after 33+b 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x7568747C1FDF1677L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x757DE80DFC77EF14L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x75E2D98F7E3D8934L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33+h", -900)),
        // OFK ply 4 (after 22+b 36+e 42+g 56+f)
        new OctiBotEngine.OpeningBookEntry(0x7681297943A5C9B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1560),
            new OctiBotEngine.OpeningBookCandidate("52h+a", -1560),
            new OctiBotEngine.OpeningBookCandidate("32c+b", -2108)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x76BD5ECCEDDD959FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 0)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x76CBE767F5088051L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x76D00419B6829A39L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -520),
            new OctiBotEngine.OpeningBookCandidate("73+a", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -520)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x76DA06B40BA76286L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 1-Base ply 3 (after 33+b 57+c 53+b)
        new OctiBotEngine.OpeningBookEntry(0x76E7BAC8986CC94EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3630),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -3630),
            new OctiBotEngine.OpeningBookCandidate("77+f", -4330)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x76E9354A78EBEB5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 57+g 53b+a 57g+f)
        new OctiBotEngine.OpeningBookEntry(0x77187878036B1885L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 2P 1-Base ply 3 (after 33+b 37+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x7726DA0455BDB9B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3600)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x773ABD4EC7C4BAFEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 120),
            new OctiBotEngine.OpeningBookCandidate("73+a", 120),
            new OctiBotEngine.OpeningBookCandidate("73+b", 0)),
        // 2P 1-Base ply 3 (after 53+c 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x7753EB5635F98AD0L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("57c+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2480)),
        // 4P ply 3 (after 33+h 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x77767264439F96AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x77C2F2510DA07EC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x77CC14C81987692DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 22+b 46+e 22b+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x77F118DCB24BE668L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107),
            new OctiBotEngine.OpeningBookCandidate("42+b", -3747)),
        // OFK ply 4 (after 22+b 46+e 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x782127A1E84581E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("22b+e", -3500)),
        // 4P ply 4 (after 33+b 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7830438A419B51ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 3 (after 33+d 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x784D5358C03341ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 32+a 36+e 42+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x787AB477968A02BAL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -761),
            new OctiBotEngine.OpeningBookCandidate("52+h", -761),
            new OctiBotEngine.OpeningBookCandidate("32a+b", -940)),
        // OFK ply 4 (after 32+b 26+d 52+h 46+f)
        new OctiBotEngine.OpeningBookEntry(0x78B8DE3664032551L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1020),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1068)),
        // 4P ply 3 (after 33+h 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x7955A7BDC6345DE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 2 (after 33+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x7962AED53C370F0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0)),
        // 2P 3-Base ply 3 (after 53+b 57+g 73+a)
        new OctiBotEngine.OpeningBookEntry(0x797EE812C98BDCD1L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2486)),
        // OFK ply 3 (after 22+b 36+d 42+e)
        new OctiBotEngine.OpeningBookEntry(0x7985C04A68B7D86FL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -3463)),
        // 2P 3-Base ply 3 (after 33+h 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x79A58A5910719890L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 2P 1-Base ply 4 (after 53+b 57+e 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x79B07676E70A57DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // OFK ply 4 (after 32+a 36+d 42+e 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x7A2E86FE43E77808L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // 2P 3-Base ply 4 (after 33+h 37+e 33h+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7A424036E116021BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+b", -62)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x7A6171A3779B1B95L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1970)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+b 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x7A651407E1B4D1F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 4P ply 4 (after 33+h 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7A740A7E31B5A8A0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 4 (after 32+b 46+e 32b-43 36+d)
        new OctiBotEngine.OpeningBookEntry(0x7A8C78EE7E3E992DL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 640),
            new OctiBotEngine.OpeningBookCandidate("32+a", 640),
            new OctiBotEngine.OpeningBookCandidate("22+h", -1020)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x7A99227D38EC465AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 4P ply 3 (after 33+d 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x7AB304D9968FA192L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+h 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x7AB4B5B1B468562AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33", 150),
            new OctiBotEngine.OpeningBookCandidate("53", 60),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+e 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7AC161A7670BA106L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -62)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+c 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x7AD0EDA660A8FB40L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -400)),
        // 2P 3-Base ply 3 (after 33+a 37+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x7AD20BD1CB18E5D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2382),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2402)),
        // OFK ply 4 (after 32+a 36+e 42+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x7ADCBAF429CF25A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1287),
            new OctiBotEngine.OpeningBookCandidate("22+b", -2348),
            new OctiBotEngine.OpeningBookCandidate("32a+c", -2371)),
        // 2P 1-Base ply 3 (after 53+c 57+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x7ADEBB31AF716998L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080)),
        // 4P ply 4 (after 33+b 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7AE7D575D24C2919L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x7B8BC8F406DD3B52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120)),
        // 4P ply 4 (after 33+g 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7BBE4130FB93A854L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+h 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7C33906C1A02D402L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // OFK ply 3 (after 32+b 36+e 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x7C55F0CEDB73E521L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4706),
            new OctiBotEngine.OpeningBookCandidate("36e+d", -4706)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x7CA8BB3410B57D94L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -520)),
        // 4P ply 4 (after 33+d 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7CD746FF82AE2C03L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // OFK ply 3 (after 32+a 36+d 42+c)
        new OctiBotEngine.OpeningBookEntry(0x7CEFDE909A39389CL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("26+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("46f+a", -3463)),
        // 4P ply 4 (after 33+c 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7CF0611853E2A327L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 2 (after 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7D67616678F4DB93L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0)),
        // 4P ply 3 (after 33+a 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x7DACD7422176A9F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x7DF249E00B19596CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120)),
        // 4P ply 4 (after 33+g 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7DF9DB22D024D4F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+d 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7E00D000117954F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7E0F4459CBA13FD5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 1-Base ply 3 (after 33+b 77+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x7E1AD175B25571ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7E23DFE1290BCA72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7E2D9674464254DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 4 (after 22+b 46+f 52+h 46f-35)
        new OctiBotEngine.OpeningBookEntry(0x7E3AF605E13CF10CL, 8,
            new OctiBotEngine.OpeningBookCandidate("22b+a", -1615),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1721),
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7E478F85A5D5D685L, 8,
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -62)),
        // 2P 1-Base ply 4 (after 53+b 77+f 53b-64 57+e)
        new OctiBotEngine.OpeningBookEntry(0x7E788B0FC9DBFF19L, 8,
            new OctiBotEngine.OpeningBookCandidate("64b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 3-Base ply 3 (after 33+a 77+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x7E903447270BEB6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x7E9CEDCE7C2F92C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 338)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+h 77e+f)
        new OctiBotEngine.OpeningBookEntry(0x7ECD6AB7308B4C03L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -460),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -460),
            new OctiBotEngine.OpeningBookCandidate("33+h", -1140)),
        // OFK ply 4 (after 32+b 36+d 32b+a 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x7EDB9F4B811BF989L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // 4P ply 4 (after 33+h 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7F07A913E66DC75AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+b 37+f)
        new OctiBotEngine.OpeningBookEntry(0x7F330A788F96E179L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x7F8902E9FC89A023L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 62)),
        // 2P 1-Base ply 4 (after 53+c 77+f 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x7FAC78F343D77C61L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x7FC7D6F3EDFD025CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8045F92A677E9D64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 2P 1-Base ply 4 (after 53+b 77+f 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x805F841422052419L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -610),
            new OctiBotEngine.OpeningBookCandidate("33", -920)),
        // 4P ply 4 (after 33+h 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8086BCFD03A799A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x808E25A277D9A3C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // 2P 3-Base ply 3 (after 33+b 57+d 53+a)
        new OctiBotEngine.OpeningBookEntry(0x809242E8E5A0A08BL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2086)),
        // OFK ply 4 (after 32+b 46+f 42+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x80B37780948F1F17L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1020)),
        // 2P 1-Base ply 3 (after 33+b 57+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x80B3DF46C82B0D6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -3230)),
        // 4P ply 3 (after 33+h 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x80C967AD24270626L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x812AF7B5CB4D952EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1010)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x815282A49D4291FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x81832312C89E1626L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -440),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -610),
            new OctiBotEngine.OpeningBookCandidate("73+h", -610)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x818E51B72E6B6128L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x81A2CA0FCCC1948FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x81AC839AA3880A22L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+d 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x81D1934822201A62L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 3 (after 33+b 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x820F1081495F9C04L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 2P 3-Base ply 3 (after 53+b 57+c 73+b)
        new OctiBotEngine.OpeningBookEntry(0x821D68CF3DD43388L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424)),
        // 4P ply 3 (after 33+h 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x821EF152B7F07ED3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 4 (after 33+a 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x82545574D7447FACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 2 (after 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x828BCB3A5D367AD2L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+h 57+g)
        new OctiBotEngine.OpeningBookEntry(0x82CAD30DBF0A0E67L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // OFK ply 4 (after 22+b 26+d 22b+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x82E509332340E23CL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+b", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+g", -3579)),
        // 4P ply 3 (after 33+d 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x830605B7B1F76297L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 3 (after 33+c 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x8308E32EA5D07578L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // OFK ply 3 (after 32+b 46+f 32b+e)
        new OctiBotEngine.OpeningBookEntry(0x835682D1DA8A680EL, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -3463)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x839721AF03F1E0F1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 4P ply 4 (after 33+h 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x83B28582FFC88AFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+d 73+h 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x83D1A2E1E439BF6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 3 (after 33+a 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x84301752C365F23DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+g 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x843F54DE1E59F6A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8440369923BD49FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x84A3ECDA7B9316BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // OFK ply 4 (after 32+a 56+f 22+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x84AC98907BE65E63L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1343),
            new OctiBotEngine.OpeningBookCandidate("42+c", -2667),
            new OctiBotEngine.OpeningBookCandidate("22c+h", -2667)),
        // 2P 1-Base ply 3 (after 33+b 37+d 33b-44)
        new OctiBotEngine.OpeningBookEntry(0x850CE8A27D9B5E4AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2270),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2750),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2750)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x85474956D5BA0E52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -650),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1150)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x854B86EF60BD77CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // OFK ply 2 (after 32+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x8557CC8B298421EEL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("32+b", -620),
            new OctiBotEngine.OpeningBookCandidate("32+a", -820)),
        // 4P ply 4 (after 33+b 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8566C09B378677E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+c 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x856CA108B1F1F8E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // OFK ply 4 (after 32+b 46+e 32b+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x8586E8EC1A07C343L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -3398),
            new OctiBotEngine.OpeningBookCandidate("32+b", -3398),
            new OctiBotEngine.OpeningBookCandidate("22+b", -3799)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x85E0812A0906B572L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // 4P ply 3 (after 33+d 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x860628488F2AEC37L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 2 (after 53+c 57+d)
        new OctiBotEngine.OpeningBookEntry(0x860FD6E552AF4CA6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1180)),
        // 2P 3-Base ply 3 (after 53+b 57+c 33+h)
        new OctiBotEngine.OpeningBookEntry(0x8618A77C7917E716L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2424)),
        // OFK ply 2 (after 32+b 26+d)
        new OctiBotEngine.OpeningBookEntry(0x86428795151724B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 0)),
        // 4P ply 3 (after 33+d 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x866A2B2C973C8285L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x86957DFF7748175EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x869E9BC7B76D043EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x86AD258B30068BB7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 3 (after 53+c 57+e 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x86AFD1C36CF626D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2950)),
        // 2P 1-Base ply 3 (after 53+a 57+c 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x86C47FC3C2DC58EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -3750)),
        // 4P ply 3 (after 33+a 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x86E781AD50B28AC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 1-Base ply 3 (after 53+a 37+d 53a+h)
        new OctiBotEngine.OpeningBookEntry(0x87051F0F0F0D2815L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2690)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x871978459D742B5FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 4P ply 3 (after 33+h 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x8772DFC9913B9EC1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8793B28CD17C10F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73+b", 338)),
        // 2P 3-Base ply 4 (after 33+a 77+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x87A72F1D28F11748L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 4 (after 33+b 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x87B15664A4510F11L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+d 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x87B5D16E360197F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x87BB37F72226801CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140)),
        // 4P ply 4 (after 33+a 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x87C9CBA505DC1D8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x87EEAF392381903CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33+h", 280)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x88129960BA8A9711L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62)),
        // OFK ply 4 (after 22+b 46+f 32+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x88189C669E27ABB2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 640),
            new OctiBotEngine.OpeningBookCandidate("22b+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1092)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8843E7BFE86A203FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x884D0126FC4D37D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x886E89D57AAA59A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 3-Base ply 3 (after 33+a 57+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x8882D287908470D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 2P 1-Base ply 4 (after 53+b 57+f 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x88BF28632F76DE7EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // OFK ply 3 (after 32+a 56+f 42+e)
        new OctiBotEngine.OpeningBookEntry(0x88E1C265838CE2C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3960)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x894AF28910C2DEACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x895B135AEE6D3C7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 1-Base ply 4 (after 53+b 77+f 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x89638F65C5EDEC41L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -470),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -470),
            new OctiBotEngine.OpeningBookCandidate("73+h", -620)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x896820A49D21B5A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 4 (after 22+b 56+f 42+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x89813AB8E0629DAEL, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1615),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2348),
            new OctiBotEngine.OpeningBookCandidate("42a+b", -2527)),
        // OFK ply 4 (after 32+b 36+e 32b+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x8988DAF48F5FA69FL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -812),
            new OctiBotEngine.OpeningBookCandidate("32+a", -940),
            new OctiBotEngine.OpeningBookCandidate("32+b", -940)),
        // 2P 3-Base ply 3 (after 53+b 37+c 33+h)
        new OctiBotEngine.OpeningBookEntry(0x8A28D6F24AFDD0F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2424)),
        // 4P ply 2 (after 33+d 37+d)
        new OctiBotEngine.OpeningBookEntry(0x8A42649CAD7F3079L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // 4P ply 4 (after 33+h 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8A70A7D3E311A0CBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8A90F64AE9EDC028L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8A9471407BBD58CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8A9EBFDF86A45E85L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8ABA5DF4A73AE62DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -520),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -860)),
        // 4P ply 3 (after 33+a 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x8ACBB3BF77F62380L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+d 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8B045052FD00D23DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 4 (after 33+h 37+e 33h+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8B6518E861E3EA58L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+b", -62),
            new OctiBotEngine.OpeningBookCandidate("33+h", -62)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8B685336E516BC8FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x8B6A4A1229C85853L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", -138)),
        // OFK ply 4 (after 32+a 56+f 42+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x8B7CA7ED21E839ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1287),
            new OctiBotEngine.OpeningBookCandidate("42c+a", -2107),
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108)),
        // 4P ply 4 (after 33+h 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8B8C85A57DBA448EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8B9705E505622954L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8BA80DFF5C6DAAF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120)),
        // 4P ply 3 (after 33+h 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8BC35EF55A3ADB0FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 3 (after 32+b 56+f 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x8C14DBD041706347L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4243),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4341),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4776)),
        // 2P 3-Base ply 4 (after 33+h 77+e 53+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8C1756D3FE49438FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62)),
        // 4P ply 4 (after 33+h 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8C373DC1C8A6DC69L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+d 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8C89A4BE7C645DF9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 3 (after 33+b 57+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x8CC0ED117C977E60L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2702),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2702),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2702)),
        // 4P ply 3 (after 33+b 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x8CF4B46CFDCC4DB9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+d 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8D2FC924CEA1C02DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+d 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8D43CA40D6B7AE9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+h 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x8D5481A149BAD196L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 2P 3-Base ply 3 (after 53+a 37+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x8D8380E10E02DDAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2086)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8D91505B7A6341BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8DA263A5092FC860L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8DA71CD34E1B569EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8DD09FF72ED555F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x8DD18CEB51A9C8CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // OFK ply 2 (after 32+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x8DF5C7214C11AE7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -28),
            new OctiBotEngine.OpeningBookCandidate("32+b", -28),
            new OctiBotEngine.OpeningBookCandidate("32+e", -180)),
        // 4P ply 3 (after 33+b 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x8E2322936E1B354CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 3 (after 53+b 37+c 73+b)
        new OctiBotEngine.OpeningBookEntry(0x8E2D19410E3E0469L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2424)),
        // OFK ply 4 (after 22+b 46+f 32+a 46f+c)
        new OctiBotEngine.OpeningBookEntry(0x8E51FBF2DFAE2C1EL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 2418),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 2418),
            new OctiBotEngine.OpeningBookCandidate("32+c", 1378)),
        // 4P ply 4 (after 33+d 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8E77F33F2AD8BDC7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 3 (after 53+a 37+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x8EB3F14C7DBB7ACBL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8F0D689B3B75A20EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x8F287584C5DB56E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8F47836A2582ED2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8F60D75B25203EC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8F708A2CDDCC2E6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 4 (after 33+a 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8F75F55A9AF8B095L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 3 (after 33+a 37+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x8FB6A0A8344032C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2486)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x8FE413F8B74D93FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 280),
            new OctiBotEngine.OpeningBookCandidate("53+b", 280),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 280)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x90054AFD5C915D68L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610),
            new OctiBotEngine.OpeningBookCandidate("73+b", -730)),
        // 2P 1-Base ply 4 (after 53+b 57+d 53b-64 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x900D1E073C06EC2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", -100),
            new OctiBotEngine.OpeningBookCandidate("33+a", -470),
            new OctiBotEngine.OpeningBookCandidate("64b+a", -470)),
        // 4P ply 4 (after 33+a 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9024C330BEAD24DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+c 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x90563F629957B948L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 3 (after 53+a 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x906C1CE5AEAC6D46L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3000),
            new OctiBotEngine.OpeningBookCandidate("57c+f", -3020)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x90A28281802DE648L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
        // 4P ply 4 (after 33+h 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x90CF81FAFCD9B7FCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // OFK ply 3 (after 22+b 46+e 42+e)
        new OctiBotEngine.OpeningBookEntry(0x90D6D25FB212F5D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // OFK ply 3 (after 32+b 36+e 52+g)
        new OctiBotEngine.OpeningBookEntry(0x913619A4011C9D40L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // 2P 3-Base ply 3 (after 33+a 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x9171A496D8B6A245L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024)),
        // 4P ply 3 (after 33+b 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x917214F94A4EA107L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 3 (after 33+b 57+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x91B3E37BAD1350E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3230),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3230),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -3650)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x91CD8C1E2C1EEE02L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -890)),
        // 2P 1-Base ply 4 (after 53+b 77+f 53b-64 37+d)
        new OctiBotEngine.OpeningBookEntry(0x91D1B901D69DDE10L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 270),
            new OctiBotEngine.OpeningBookCandidate("73+h", 120),
            new OctiBotEngine.OpeningBookCandidate("64b+a", 120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+c 57+d)
        new OctiBotEngine.OpeningBookEntry(0x91FB235AFFB73A9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1010),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -1010),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -2250)),
        // 4P ply 4 (after 33+h 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x921817056F0ECF09L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x925B7BEE145F7865L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -610)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x928BC80E8CF74EB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+d 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x92AC973021312761L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x92D1B127585743C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x92DD2E3CE5686EA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -610),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -1850)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x92F355CF2D7A5C2BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+d 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x931F6E16981A5CA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+a 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x935C396B3A043058L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x9370E55E38042F8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 3 (after 33+b 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x93A58206D999D9F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 3 (after 33+h 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x93B463D527363B25L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+h 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x93D860B13F205597L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x9410810F1469C2C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", -138)),
        // 4P ply 4 (after 33+h 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x942C0385ADBC52F1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 32+b 46+e 52+g 26+d)
        new OctiBotEngine.OpeningBookEntry(0x94AA1880CF335898L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -1020),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -1020)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x94C120B941B5451CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120),
            new OctiBotEngine.OpeningBookCandidate("73+a", 120)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x94CC521CA7403212L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+a 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x94EE80312AA35918L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+c 37+e)
        new OctiBotEngine.OpeningBookEntry(0x94F9223305076FCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x951D87BFAB2E7723L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 4P ply 4 (after 33+d 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9534F760ABBB4EB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 3 (after 33+d 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x957B2C308C3BD134L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -996),
            new OctiBotEngine.OpeningBookCandidate("73+e", -996),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1116)),
        // 2P 3-Base ply 4 (after 33+a 57+d 33a+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x9597628D3E70EDA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+c 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x95CBA1B34BCFDB6EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+b 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x95CC24F66FDBCDC5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x95CC2609FEF2F0FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -138)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9611A570B2E0C5EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 1-Base ply 4 (after 53+b 77+f 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x96194478F84C76D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -390)),
        // OFK ply 2 (after 22+b 36+e)
        new OctiBotEngine.OpeningBookEntry(0x9662F3A6215C0DDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -620),
            new OctiBotEngine.OpeningBookCandidate("42+h", -620),
            new OctiBotEngine.OpeningBookCandidate("22b-33", -620)),
        // 4P ply 4 (after 33+d 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x966CCD7B4FC2335FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 3 (after 33+b 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x9670A8E2CF17772EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024)),
        // 2P 3-Base ply 4 (after 33+b 37+e 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x96A713204DC94AC1L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62)),
        // 2P 3-Base ply 2 (after 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x96C4CE216B5A9C0FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+h 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x96D25404FB00B2CFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+b 77+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x96DB03958DE98476L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62)),
        // 2P 3-Base ply 2 (after 33+h 37+f)
        new OctiBotEngine.OpeningBookEntry(0x96ECCA15B2FAC0D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0)),
        // 2P 3-Base ply 2 (after 53+b 57+g)
        new OctiBotEngine.OpeningBookEntry(0x9705AEEDA68BECF5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // 4P ply 4 (after 33+b 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x971656DF5E6F2C96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x97181F4A3126B23BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // OFK ply 1 (after 32+a)
        new OctiBotEngine.OpeningBookEntry(0x97220033605ECA2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4143),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4143),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4143)),
        // 4P ply 3 (after 33+a 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x974D132AC074CF3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // OFK ply 4 (after 32+b 46+f 32b-43 26+d)
        new OctiBotEngine.OpeningBookEntry(0x9772EAE705E8D111L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("43b+h", -1020)),
        // 4P ply 3 (after 33+d 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x97ACBACF1FECA9C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1256)),
        // 4P ply 4 (after 33+d 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9815574EE607818CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x98231BC6D27F96AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // OFK ply 4 (after 32+b 46+f 32b+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x9838E855AA6C82DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1287),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -1287),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1615)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x98938F40D8829BF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+h 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x989D82B966BD173FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+a 57+d 33a+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x98B212EEF5093610L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -138),
            new OctiBotEngine.OpeningBookCandidate("33+a", -138),
            new OctiBotEngine.OpeningBookCandidate("53+h", -138)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+b 37e+d)
        new OctiBotEngine.OpeningBookEntry(0x98DDE40015508DDFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x98E0600E80049B5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x98EA019D06731457L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x98F181DD7EAB798DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x990DA3ABE0009DC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 2 (after 33+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x99B33FB18C84EEF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 3 (after 33+a 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x99B6B7C774E71E83L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x99B9F44BA9DB1A17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x99E9753878AC65C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+b 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9A37F6F113D3E3AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9A3D976295A46CA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9A4F6B30B25EF134L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x9A7DF5D0FAA6BAC5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -610),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -1850)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x9AA610533E66C0C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 390)),
        // OFK ply 4 (after 32+b 46+f 52+h 36+e)
        new OctiBotEngine.OpeningBookEntry(0x9ABDE5D987878AABL, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+e", -1068)),
        // 4P ply 4 (after 33+d 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9AC2C1B175D0F979L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x9AD42177F97539CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -120)),
        // 2P 3-Base ply 3 (after 33+a 37+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x9AF4A303BD6B61F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086)),
        // 4P ply 3 (after 33+c 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x9B13DD6AC0CAFBE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x9B278514E5F025FFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // OFK ply 4 (after 32+a 46+f 42+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x9B2DA34B96FF8786L, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -458),
            new OctiBotEngine.OpeningBookCandidate("42a+h", -458),
            new OctiBotEngine.OpeningBookCandidate("22+b", -640)),
        // 4P ply 3 (after 33+a 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x9B612138E7306676L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x9B7D0F5E27E5347EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x9B9049293577D5EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x9BA152D6103D88FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -1150),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -1150),
            new OctiBotEngine.OpeningBookCandidate("33+b", -1850)),
        // 2P 3-Base ply 3 (after 53+b 57+c 53b+a)
        new OctiBotEngine.OpeningBookEntry(0x9BB771A91358766AL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2606)),
        // 4P ply 4 (after 33+h 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9BDA355473D7E53DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9C08F12299E98D96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x9C218CCE4CC6BF7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -460),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -590)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9C246A9A7B437831L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9C2A230F140AE69CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 3 (after 32+b 56+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0x9C3D4FAF1A06711EL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("36+g", -6435)),
        // OFK ply 4 (after 32+b 26+d 32b+e 46+a)
        new OctiBotEngine.OpeningBookEntry(0x9C8EAF00BB1EC1F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42eh-33", 0),
            new OctiBotEngine.OpeningBookCandidate("22+b", 0)),
        // OFK ply 3 (after 32+b 36+e 52+h)
        new OctiBotEngine.OpeningBookEntry(0x9CD5C6AD151C42AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("46+a", -6570)),
        // 4P ply 4 (after 33+d 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9CE958C74671EB69L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 3-Base ply 4 (after 53+b 37+d 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x9CF02D78191A38A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -138)),
        // OFK ply 3 (after 32+a 36+e 42+c)
        new OctiBotEngine.OpeningBookEntry(0x9D1EC72F25099ABFL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3423),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3423),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // OFK ply 4 (after 22+b 46+f 22b+a 46f+e)
        new OctiBotEngine.OpeningBookEntry(0x9D9752474980498BL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -633),
            new OctiBotEngine.OpeningBookCandidate("42+a", -633),
            new OctiBotEngine.OpeningBookCandidate("32+e", -1363)),
        // 4P ply 3 (after 33+h 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x9D97B60CA29DF067L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 3 (after 33+a 57+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x9DC0D12C19AF23E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086)),
        // 2P 1-Base ply 3 (after 53+a 57+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0x9DE14C8234248E0EL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2480)),
        // 2P 1-Base ply 4 (after 33+b 57+f 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x9DFD2BC8A65D8D44L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9E3ECE38D5A6939CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 4P ply 3 (after 33+d 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x9E711568F2260C1DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x9E8E43BB125299C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9EB61BCF551C052FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9EDA18AB4D0A6B9DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // OFK ply 3 (after 32+b 56+f 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x9EE160750F21FED6L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435)),
        // 4P ply 3 (after 33+h 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x9F69E18DF4211059L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+a 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x9F809C671D66ADBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 3 (after 33+b 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x9F89B014FEDD70BAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9FAEEF2A531B196BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9FD2F5E160C69312L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA00B736B8F68E34BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA01FEDCE36F39F62L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 3 (after 33+a 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA044A83BA8E87CCAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+h 57f-46)
        new OctiBotEngine.OpeningBookEntry(0xA04D8C7D3CAC8FA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 4P ply 4 (after 33+b 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA073EEAA2EE5F1D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+c 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0xA0ABE74763570DDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xA0D6038AC5AB1E92L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA1181E61DA7C761EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 3 (after 33+h 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0xA14C457185248445L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+b 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0xA1AC14E88FD8E4A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+h 37+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0xA1B2E9A04DB7AA8DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+b 77e+f)
        new OctiBotEngine.OpeningBookEntry(0xA1EFB7505A370DE4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+b", 90),
            new OctiBotEngine.OpeningBookCandidate("33+b", -460),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -460)),
        // 4P ply 4 (after 33+a 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA1F7511D11C3070EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA201CE660A46EC2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // OFK ply 4 (after 22+b 26+d 22b-33 26d-35)
        new OctiBotEngine.OpeningBookEntry(0xA249F143931939FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("43h+b", 1530),
            new OctiBotEngine.OpeningBookCandidate("32+c", 1530),
            new OctiBotEngine.OpeningBookCandidate("32+b", 1136)),
        // 4P ply 4 (after 33+d 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA2514F15DB3193F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xA27D27FB65706CDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA2DCE5941CBF9BBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA2EFD66A6FF31263L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+b 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA3524369D9640498L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA3CBA098A0951F4BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // OFK ply 2 (after 32+b 56+f)
        new OctiBotEngine.OpeningBookEntry(0xA3FE4D54D9664919L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -620)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0xA4031EE49BACC862L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+g", -120)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA416D507F086EF51L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xA464000451EF3A1EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // 4P ply 4 (after 33+c 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA46BBD0C0DA419E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // 4P ply 2 (after 33+g 37+b)
        new OctiBotEngine.OpeningBookEntry(0xA4CA0E44DB5D81ADL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -20),
            new OctiBotEngine.OpeningBookCandidate("77+a", -20),
            new OctiBotEngine.OpeningBookCandidate("77+g", -140)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA4F20394682A1750L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA4F77CE22F1E89AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+b 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA51FC031082E11C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 3 (after 53+b 37+c 53b+a)
        new OctiBotEngine.OpeningBookEntry(0xA529CE1E7AF100B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2588),
            new OctiBotEngine.OpeningBookCandidate("77g+f", -2796),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2886)),
        // 2P 3-Base ply 2 (after 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA570029F2532E98EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 3 (after 33+c 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA5ABCAB85D8A837AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 3 (after 33+h 77+e 33h+a)
        new OctiBotEngine.OpeningBookEntry(0xA5B7261309747E13L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+a 57c+f)
        new OctiBotEngine.OpeningBookEntry(0xA5B74F7A36398BFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53 57f+e)
        new OctiBotEngine.OpeningBookEntry(0xA5CED9332079EAFBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 60),
            new OctiBotEngine.OpeningBookCandidate("33+b", 60),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -100)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA5D67566A74C1A78L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 4 (after 33+g 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA5E039EE93340D5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA616A69588B1E678L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA620EA1DBCC9F15BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA625956BFBFD6FA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+b 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA63474B805528D72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 2P 3-Base ply 2 (after 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA6407332568B4EEAL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA65877DC1D44E3C0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 3 (after 33+b 77+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0xA68757BE7ACDD977L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2024)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA68B8E3721E9A0D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 3 (after 33+g 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA6C1942D64B5F813L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 2P 1-Base ply 3 (after 53+c 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0xA6ECC4F40CBBA8DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 4P ply 4 (after 33+g 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA701E399349B628DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 1-Base ply 3 (after 53+a 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0xA7067DA37DFD8818L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600)),
        // 4P ply 4 (after 33+c 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA7338717E9DD640EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xA76F2BBB8FC0B836L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120)),
        // 4P ply 3 (after 33+c 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA77C5C47CE5DFB8FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA7B0CB0F3A747BACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+b 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA7C856CE9BF96937L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA7CCD1C409A9F1D5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA817A16180F94630L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA835734C0D1A2D3AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA836AFFDC8D8E4F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+d 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA83AE715D7C24619L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // OFK ply 4 (after 22+b 46+f 22b+c 46f+d)
        new OctiBotEngine.OpeningBookEntry(0xA86D43C997A9419AL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -3020),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3020),
            new OctiBotEngine.OpeningBookCandidate("42+h", -3020)),
        // 2P 3-Base ply 3 (after 33+a 57+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0xA881B02BE274D0DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2564)),
        // 4P ply 3 (after 33+g 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA88E4290F9085DE3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xA89FCC73AE737B85L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+c 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA8C5B1C637B6D3C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // OFK ply 4 (after 32+a 46+e 22+c 36+a)
        new OctiBotEngine.OpeningBookEntry(0xA8DDCE661F89F90FL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 0),
            new OctiBotEngine.OpeningBookCandidate("42+e", 0),
            new OctiBotEngine.OpeningBookCandidate("22c+b", 0)),
        // OFK ply 4 (after 22+b 26+d 42+h 46+c)
        new OctiBotEngine.OpeningBookEntry(0xA8F87A2D2F4C44E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 2440),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 2320),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 1931)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xA9138F23C830FA5CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -120)),
        // 4P ply 3 (after 33+d 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0xA914AD1D82ACD15BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 22+b 26+d 22b+c 26d-35)
        new OctiBotEngine.OpeningBookEntry(0xA9777613794E1C57L, 8,
            new OctiBotEngine.OpeningBookCandidate("52gh+b", -1374),
            new OctiBotEngine.OpeningBookCandidate("52gh-43", -1494),
            new OctiBotEngine.OpeningBookCandidate("32+a", -1853)),
        // 4P ply 3 (after 33+a 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0xA999079C4522D916L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // OFK ply 3 (after 32+b 26+d 32b+a)
        new OctiBotEngine.OpeningBookEntry(0xA9A811118D010EECL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4796),
            new OctiBotEngine.OpeningBookCandidate("46+g", -5033)),
        // 4P ply 4 (after 33+h 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA9D3A3BA2596D869L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAA09A779DCB9C6EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xAA122739A461AB37L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+c 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAA246BB19019BC14L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAAE9F6E0D645A60EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+d 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xAAED71EA44153EECL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+a 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAB014A33F1753E62L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 3 (after 33+a 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0xAB4E9163D6F5A1E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0xAB4F5E74B011C4E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xABC3EC37169FC619L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+a 77e+f)
        new OctiBotEngine.OpeningBookEntry(0xABE3CD8CCE6662D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+h", -900)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xABEE054F3ACA4F72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+c 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAC39BE4F97C0B927L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xAC4E3D6BF70EBA4FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xAC74906B24CBEA46L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 3 (after 33+c 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0xAC76651FB04026A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0xAC8933CC5034B37DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xACDDE26014F73BF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 4P ply 3 (after 33+a 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0xACF5290763E93904L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xAD1640908CF32EC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -1020)),
        // 4P ply 3 (after 33+b 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0xAD1D95D444D9A168L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+a 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xAD2AD345C2D42C72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+h 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0xAD607763A2602D0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 3 (after 33+b 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0xAD7196B05CCFCFDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAD8476253D28BABBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xADA99F5D117D33D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 32+b 36+d 42+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xADADB6EB3D6BFD28L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 640),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -3328),
            new OctiBotEngine.OpeningBookCandidate("52+e", -3463)),
        // 4P ply 4 (after 33+b 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAE0A749F87204303L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // OFK ply 4 (after 32+b 46+f 52+h 46f-35)
        new OctiBotEngine.OpeningBookEntry(0xAE386AFDFD797C3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+e", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1068)),
        // 4P ply 4 (after 33+g 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAEDC4C3ED951C751L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+c 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAEEE28B00417C1D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xAFA9A54ED4C9918FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 4P ply 3 (after 33+h 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0xAFB7E19C31B755F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+a 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB05DC39A810542F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 3 (after 33+h 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0xB0E6D7F615E2C1B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 3 (after 33+c 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0xB10175C0F391482CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 3 (after 33+d 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0xB10F9359E7B65FC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB17CCA1E0957D12EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB1B57F49A635DA94L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 4P ply 4 (after 33+h 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB1C89DFE408C56F1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 22+b 56+f 32+a 56f+a)
        new OctiBotEngine.OpeningBookEntry(0xB229E36500A74686L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 1160),
            new OctiBotEngine.OpeningBookCandidate("32+a", 1127),
            new OctiBotEngine.OpeningBookCandidate("32+e", 801)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0xB22D281006EBEB0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 3 (after 33+h 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0xB23141098635B946L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+a 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB28A556512D23A0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b-64 57+f)
        new OctiBotEngine.OpeningBookEntry(0xB28D5CEE378D62B6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("64b+a", 0)),
        // OFK ply 4 (after 22+b 56+f 22b+c 36+a)
        new OctiBotEngine.OpeningBookEntry(0xB2D35C3F0C816CB3L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42+e", -497)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xB2DE4C909798CEAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53bc+h", -1330)),
        // 4P ply 4 (after 33+b 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB2F2C8A4B35F2896L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB31A7477946FB0FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB354A53E019AB542L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB362E9B635E2A261L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+b 57d+e)
        new OctiBotEngine.OpeningBookEntry(0xB374825CF463C068L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB37677138C79DE48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+g 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB3AB5CE19A80A9DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB3C8793FFAE04816L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400)),
        // 4P ply 3 (after 33+c 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0xB3D6E33F604630D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+c 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB401583FCD4CC68CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 33+a 57+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0xB413E3A3669962C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -138),
            new OctiBotEngine.OpeningBookCandidate("53+h", -138),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -138)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB455032F921434D7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+c 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0xB46D5B5BD55AA83EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0xB4920D88352E3DE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+b 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB4B552B698E85434L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // OFK ply 4 (after 32+b 46+e 32b+g 56+f)
        new OctiBotEngine.OpeningBookEntry(0xB4C71F3991ACFACAL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // 4P ply 3 (after 33+a 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0xB4EE174306F3B79CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+b 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB506AB9021C32FF0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 32+b 36+d 32b+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xB526293E038860E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+c", -3367)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB531ED01A7CEA2EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB54970C00643B071L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB55DEE65BFD8CC58L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 2 (after 33+g 37+g)
        new OctiBotEngine.OpeningBookEntry(0xB5609CC34B9BC45BL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 3 (after 33+h 37+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0xB5770989EE85D6D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB59F486158323423L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB5B2A1197467BD48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB5BC47806040AAA7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 2P 1-Base ply 3 (after 53+b 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0xB5F2BE4133F0FE5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3260)),
        // 4P ply 3 (after 33+a 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0xB61040C2504F57A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0xB61271807C002B3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 62)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB6AB711EA45D277BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB6C7727ABC4B49C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB74CF69822DB5D76L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xB751A32AD2333CA7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 4 (after 33+g 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB7611FE00E8ED41DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB76BD17FF397D252L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // OFK ply 3 (after 22+b 46+f 22b+c)
        new OctiBotEngine.OpeningBookEntry(0xB7771179FEE4C603L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3283),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3283),
            new OctiBotEngine.OpeningBookCandidate("36d+f", -4523)),
        // 4P ply 4 (after 33+h 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB78F07EC6B3B2A53L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB79EE63F9594C884L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 2 (after 33+g 37+c)
        new OctiBotEngine.OpeningBookEntry(0xB7B70A3CD84CBCAEL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 4P ply 3 (after 33+a 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0xB833951BD5E49CE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 2 (after 33+c 37+d)
        new OctiBotEngine.OpeningBookEntry(0xB84514EBEF191AC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 4P ply 3 (after 33+a 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB85F967FCDF2F252L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+b 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB868D0EE4BFF7F48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB893013739DB7271L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk3() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // 4P ply 4 (after 33+g 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB8BEE84F158EFB1AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 4 (after 33+c 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB9032025BF66F886L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xB9170E6E806B149BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+c 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB96F2341A7709634L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB974A301DFA8FBEEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // OFK ply 3 (after 32+b 26+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0xB981856ED6771CB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6570),
            new OctiBotEngine.OpeningBookCandidate("26d+a", -6783)),
        // 4P ply 4 (after 33+d 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB9907592470403EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB99FE1CB9DDC68CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 3 (after 33+b 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0xB9B72AACEAC26A3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xBA3EE0E79AEB34C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBA4497C8AA0C0A84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBA4A7151BE2B1D6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+g 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xBA697EB0865983EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // OFK ply 4 (after 32+b 36+d 32b-43 56+f)
        new OctiBotEngine.OpeningBookEntry(0xBA6C2B8CAC0C332EL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 640),
            new OctiBotEngine.OpeningBookCandidate("52+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("22+c", -4944)),
        // 2P 3-Base ply 3 (after 53+a 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0xBA7573C9AC79A69DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xBA7E977C88561AC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+b", -1320),
            new OctiBotEngine.OpeningBookCandidate("53bg+h", -1330)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBB4877340E0B1039L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // OFK ply 3 (after 32+b 26+d 32b-43)
        new OctiBotEngine.OpeningBookEntry(0xBB5DAAB4C350937DL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+a", -6570)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBBA335FE4C7F831BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 3-Base ply 4 (after 33+b 77+f 33b+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xBBEEF70114198AB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0xBC002B54FA99BD0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // OFK ply 3 (after 32+b 46+e 32b+c)
        new OctiBotEngine.OpeningBookEntry(0xBC0F3295A8D77A9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3423),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3423),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0xBC22EE623E361F76L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -120)),
        // 4P ply 3 (after 33+g 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0xBC24FDE8571396B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+c 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBC70834868BE977CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 3 (after 33+h 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0xBCCAE5E432A668FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+b 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0xBCDB0437CC098A2CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // OFK ply 4 (after 22+b 36+e 42+h 26+d)
        new OctiBotEngine.OpeningBookEntry(0xBCF50D481742EAD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -179),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -227),
            new OctiBotEngine.OpeningBookCandidate("52h-43", -620)),
        // 4P ply 4 (after 33+a 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBD55A2CA09D2150AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBD7770E784317E00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // OFK ply 4 (after 32+b 56+f 32b+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0xBD842294661DEF71L, 8,
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -267),
            new OctiBotEngine.OpeningBookCandidate("42+a", -458),
            new OctiBotEngine.OpeningBookCandidate("42+h", -799)),
        // 2P 3-Base ply 2 (after 53+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0xBDE81D4AC635C40BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+b 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0xBE0C92C85FDEF2D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBE434998785E6D58L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBE8ED4C93E027742L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+g 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0xBEDAAA6901AF768BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // OFK ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xBF20205531084330L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("22+b", 0),
            new OctiBotEngine.OpeningBookCandidate("32+b", 0)),
        // 4P ply 4 (after 33+c 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBF28B9538CC7EA96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+c 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBF44BA3794D18424L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBF6976FFD871FEDDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+b 57f-46)
        new OctiBotEngine.OpeningBookEntry(0xBF83FECB6F960DB2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53bc-64", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xBFA39A6DB021360EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -62)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBFABF54B5F6EF534L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xBFACC897F80A8405L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xBFC389507D2B23B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+a", -1180),
            new OctiBotEngine.OpeningBookCandidate("53gh+b", -1180)),
        // OFK ply 3 (after 22+b 36+d 22b+c)
        new OctiBotEngine.OpeningBookEntry(0xBFD51AD39B714997L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -3463)),
        // 4P ply 4 (after 33+d 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBFD7EF806CB37F4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 3 (after 33+c 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC009B4B1AE163F63L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 2 (after 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC01336258CFDD7F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // OFK ply 4 (after 22+b 46+e 32+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xC05F5A08C6261CA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 4P ply 3 (after 33+c 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0xC065B7D5B60051D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 4P ply 3 (after 33+d 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0xC09AE1065674C40AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+b 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC0BDBE38FBB2ADDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC0C523F95A3FBF40L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC12D9F2A7D0F272CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC139018FC4945B05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+a 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC15502EBDC8235B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 2 (after 53+c 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC156F2316BA61CA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 3 (after 33+b 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0xC162447A5A8FB8ADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0xC1B47CDB04FE3CFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 1-Base ply 3 (after 53+a 57+d 53a+h)
        new OctiBotEngine.OpeningBookEntry(0xC1C0EB46CE22642EL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2570),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2690)),
        // OFK ply 4 (after 22+b 46+f 42+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0xC1C6FCD6B8632ABDL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3498)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC1C9C305FE38A5FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 2P 3-Base ply 3 (after 33+a 37+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0xC1DA94871A70E06DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2502)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC1E069328A3C3C11L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62)),
        // 4P ply 4 (after 33+a 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC212B506C9E8C7B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC21A08267A9C9431L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC2661893BABC5A86L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC2A39D90C707DE94L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC2AD7E996F7D55F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+b", -120)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC2ADC41BAA1B6423L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120)),
        // 4P ply 4 (after 33+g 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC2CF9EF4DF11B026L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC31E55FA6DEFDD08L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC339721DBCA3522CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 4 (after 32+a 36+e 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0xC33B618379E98EFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // 4P ply 3 (after 33+g 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0xC363EA249729440AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+h 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC387EB620861D3BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC3FA09D5EED85FD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 4P ply 4 (after 33+a 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC426A1860B5A5A4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 4 (after 22+b 36+d 42+e 36d-45)
        new OctiBotEngine.OpeningBookEntry(0xC4965B5AF72693BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("42e+h", -1068),
            new OctiBotEngine.OpeningBookCandidate("42e+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC50380046A36D83BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC524A7E3BB7A571FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+g 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC52E697C46635150L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+c 37+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC543A2CC81FD624EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -610)),
        // 2P 1-Base ply 2 (after 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC5929875768204D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 2P 1-Base ply 3 (after 53+b 57+e 53b-64)
        new OctiBotEngine.OpeningBookEntry(0xC5AFAD9BFB55FDC5L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2480)),
        // 4P ply 4 (after 33+h 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC5C0717023D6AF1EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC5D190A3DD794DC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // OFK ply 4 (after 22+b 36+d 42+e 46+e)
        new OctiBotEngine.OpeningBookEntry(0xC5D28C2D1AB8A3B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("32e+h", -1887),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3107)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC61A75B3DAF9B19AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 1 (after 33+g)
        new OctiBotEngine.OpeningBookEntry(0xC63E02E82E8A7F6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1096),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1096)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xC656162191BCAD3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xC6D41B1D2EF87E0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC6D8F6075DE6BA73L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+b 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC6FA242AD005D179L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC706065C4EAE353CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC71298F9F7354915L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC7304AD47AD6221FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC77E9B9DEF2327A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC7D03EFD10DFB16EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC7F3311C28AD2FEAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC7FDD7853C8A3805L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC80507CDF6C69826L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC80BE154E2E18FC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+g 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC826082CCEB406A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 1-Base ply 2 (after 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC8835A7C9D2079F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 3 (after 33+b 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC8BFEBDDB7451D71L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 3 (after 33+h 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0xC8C2096A51FC9114L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+g 37+f)
        new OctiBotEngine.OpeningBookEntry(0xC8D9DC8EDD1DF6DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC8E0877DBBB3AF3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120)),
        // 2P 3-Base ply 1 (after 33+h)
        new OctiBotEngine.OpeningBookEntry(0xC9142DB9AB20452FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 4P ply 3 (after 33+a 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0xC957570E9075851DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC96764FF91AA805DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -340),
            new OctiBotEngine.OpeningBookCandidate("33+a", -340)),
        // 4P ply 4 (after 33+b 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC97F9C69E76B87EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // OFK ply 4 (after 22+b 46+f 52+h 26+d)
        new OctiBotEngine.OpeningBookEntry(0xC98C7E7ED2EC969DL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1721),
            new OctiBotEngine.OpeningBookCandidate("42+h", -1721),
            new OctiBotEngine.OpeningBookCandidate("32+c", -2961)),
        // 2P 1-Base ply 2 (after 53+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xC9C0378CEFB5DA3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC9EC436204920656L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+b 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCA27A6720312FA05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+b 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xCA687D2224926584L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xCA86B4E4B7CFA0E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCADC77AB7136F73CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+g 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCAF19ED35D637E57L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 4 (after 33+c 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCB2055DDEF9D1379L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 3-Base ply 2 (after 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xCB39CEE37BC74410L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCB3BD59D97457EA3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCB4C56B9F78B7DCBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260)),
        // 4P ply 3 (after 33+a 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0xCB80C1F103A2FDE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xCB9175E888959DADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -30),
            new OctiBotEngine.OpeningBookCandidate("73+a", -30),
            new OctiBotEngine.OpeningBookCandidate("33+a", -740)),
        // 4P ply 4 (after 33+d 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCBDF030E0FE986A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCC0C3F0430B3E815L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 3 (after 33+a 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0xCC3B7995B6BE650FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+a 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xCC577AF1AEA80BBDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 3 (after 33+d 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0xCCB6D31471306D42L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 3 (after 33+c 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0xCCB8358D65177AADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCCEC4B2D5ABA7B64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCD0BCCABDC3C0169L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 3 (after 33+h 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0xCD5F97BB8364F332L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCD67CFCFC42A6FDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 22+b 46+e 32+a 36+a)
        new OctiBotEngine.OpeningBookEntry(0xCD7850705FA909A1L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 544),
            new OctiBotEngine.OpeningBookCandidate("32+a", 187),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 0)),
        // 4P ply 4 (after 33+d 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCD98991C245EFA00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 3 (after 33+b 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0xCDBFC622899893D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // OFK ply 4 (after 22+b 36+e 42+g 46+e)
        new OctiBotEngine.OpeningBookEntry(0xCDC8CECE2D3B4F61L, 8,
            new OctiBotEngine.OpeningBookCandidate("42g+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2110),
            new OctiBotEngine.OpeningBookCandidate("42g+b", -2168)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xCDD5677105EEA4D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCDE483D717837079L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCE121CAC0C069B5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+c 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0xCE46620C33AB9A93L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xCE60A114AB222B38L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62)),
        // 4P ply 3 (after 33+d 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0xCE6145EBE2E715B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 33+b 37+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xCEAB7D9CBB85159DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 2P 3-Base ply 3 (after 53+a 37+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0xCEB71AD629FC16D7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2206),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2206),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2206)),
        // 4P ply 4 (after 33+a 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCF1AD456413F9047L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 3 (after 32+b 36+d 52+a)
        new OctiBotEngine.OpeningBookEntry(0xCF372E35DA53ABC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3323),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("26+c", -3683)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+c 57+e)
        new OctiBotEngine.OpeningBookEntry(0xCF65EB470CBEB630L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -800),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -800),
            new OctiBotEngine.OpeningBookCandidate("73+a", -800)),
        // 4P ply 4 (after 33+d 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD01E398993DC16BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD06351826EFEE00BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD093930854521EF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 2 (after 33+g 37+e)
        new OctiBotEngine.OpeningBookEntry(0xD0C2E2CAB8077842L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
        // 4P ply 3 (after 33+h 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0xD0D9372E34E61F8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+d 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0xD1307381C6B281FCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 3 (after 33+h 37+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0xD132BF2924211474L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2176)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD164A22D82710977L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+h 57c+f)
        new OctiBotEngine.OpeningBookEntry(0xD1752665B3BC3BB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD184D604E8789A06L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD1B29A8CDC008D25L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 3 (after 33+c 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0xD1CF255226C61427L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 4 (after 33+b 57+f 53+b 57f+d)
        new OctiBotEngine.OpeningBookEntry(0xD204D4F0809A0927L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120)),
        // 2P 1-Base ply 3 (after 53+c 57+e 33+b)
        new OctiBotEngine.OpeningBookEntry(0xD22EADEB893E1895L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 4P ply 4 (after 33+b 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD23C98366608749DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD24405F7C7856606L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD2509B527E1E1A2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 4 (after 53+b 57+e 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xD250C28E0833A672L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD272497FF3FD7125L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 3 (after 33+g 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0xD2C978A307EF01FCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+c 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0xD318B3ADB5116CD2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD35768FD9291F353L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD3650C734FD7F5D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD39AF5ACD4CDE949L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD3B82781592E8243L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 1-Base ply 3 (after 53+a 37+d 33+b)
        new OctiBotEngine.OpeningBookEntry(0xD3C414BCF8783852L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600)),
        // 2P 1-Base ply 4 (after 53+b 37+d 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xD3D873F66A013B18L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 3 (after 33+d 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0xD3E7E57E5565F909L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD417014055A9668DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 3 (after 33+a 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0xD42047D1D3A4EB97L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xD44F2E1180ACB386L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -62),
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD46AE3F7B310EAE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD46F9C81F4247416L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 4 (after 33+b 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD47B02244DBF083FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 3 (after 33+c 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0xD4A30BC9000DF435L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 2 (after 53+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xD4B339E63E31F4B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 2P 3-Base ply 3 (after 33+h 77+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0xD537709A60E2C0EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 4P ply 3 (after 33+h 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0xD544A9FFE67E7DAAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xD551358E05D684C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+c 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD57CF18BA130E143L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD583A75841447498L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD5872052D314EC7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // OFK ply 3 (after 22+b 36+d 42+c)
        new OctiBotEngine.OpeningBookEntry(0xD5B1884372121E18L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -2962)),
        // 2P 3-Base ply 2 (after 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD5F054164CA45777L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD5FFBD937299FEE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD60922E8691C15C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 2 (after 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xD64AC089AA436A95L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD659A39BB86B6A1CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xD680C92F8F033F53L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+h 57d+e)
        new OctiBotEngine.OpeningBookEntry(0xD6B6D77E14DE2DABL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD6B80A7E67F30CE3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD6BD750820C7921DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // OFK ply 4 (after 32+b 36+d 52+g 36d+e)
        new OctiBotEngine.OpeningBookEntry(0xD6EC2CEB5FD247FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", 162),
            new OctiBotEngine.OpeningBookCandidate("32+a", 162),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -780)),
        // 4P ply 4 (after 33+b 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD750B6AD40C3948FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // OFK ply 4 (after 22+b 46+f 42+a 46f-35)
        new OctiBotEngine.OpeningBookEntry(0xD764A0D60917D788L, 8,
            new OctiBotEngine.OpeningBookCandidate("22b+a", -1584),
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD791E299B28B230EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -400)),
        // 4P ply 4 (after 33+g 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD7AF4F72DBD98816L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+c 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD8128AF5CB0CB1FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD846D1E5945443A0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD85D51A5EC8C2E7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+d 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD8A207760CF8BBA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD8A6807C9EA82343L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+a 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0xD8FDC58900B3C0EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+b 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD915795A27835887L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 2 (after 33+b 57+c)
        new OctiBotEngine.OpeningBookEntry(0xD91FF42F8932BA17L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // 4P ply 4 (after 33+a 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD94E3CAFB998BB2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 3 (after 33+b 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0xD9797A3E3F953635L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+b 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0xD98957DAE9D0F34DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD98C9AAB5E724354L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD9A173D37227CA3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDA58D7FDC814C37DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+d 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDA7591899F2FC354L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDA8AC75A7F5B568FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDAB8A3D4A21D500CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+h 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDB9CD5266D7B5D24L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xDBDB05C23F5C9E38L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -930)),
        // 4P ply 3 (after 33+b 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0xDC1554A5195ED627L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 4 (after 33+a 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDC3D9FC26E40D4D5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+c 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDCA15E2C4CFA449FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 3 (after 33+g 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0xDCEAAD7A8244CABEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 3 (after 33+d 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0xDD1C4193E1F628B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+a 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xDDFDE8763E6E4E4BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDE014BF7BFE33F02L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xDE2012C390CDEB34L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -1180)),
        // 2P 1-Base ply 4 (after 53+b 57+d 53b-64 77+f)
        new OctiBotEngine.OpeningBookEntry(0xDE284639F05A5A73L, 8,
            new OctiBotEngine.OpeningBookCandidate("64b+a", -470),
            new OctiBotEngine.OpeningBookCandidate("33+a", -470),
            new OctiBotEngine.OpeningBookCandidate("73+h", -590)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDE40845BEB552B49L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 32+b 46+f 52+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0xDE4E27413A47C869L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1839)),
        // 4P ply 4 (after 33+c 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDE76C8D3DF2D3C6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xDE7B5647504FF66AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -860),
            new OctiBotEngine.OpeningBookCandidate("33+b", -860),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1010)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDEBFD2880B21BE92L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 3 (after 33+b 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0xDEC2C25A8A89AED2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 3 (after 33+a 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xDF2A7E89ADB936BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 4 (after 33+a 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDF65A5D98A39A93FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDFCB00B975C53FF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDFE6E9C15990B69DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+b 37+d 53b-64 57+f)
        new OctiBotEngine.OpeningBookEntry(0xDFF4E13F1AC1684CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 1-Base ply 4 (after 53+c 77+f 33+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xDFFCB5C57A56D90BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610),
            new OctiBotEngine.OpeningBookCandidate("73+b", -730)),
        // 4P ply 3 (after 33+h 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0xE007373FF1705A2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 53+a 37+d 53a+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0xE022D408E9BB58B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // OFK ply 3 (after 22+b 36+e 32+a)
        new OctiBotEngine.OpeningBookEntry(0xE07F91EDC478B4D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435)),
        // OFK ply 4 (after 32+a 36+d 42+e 56+f)
        new OctiBotEngine.OpeningBookEntry(0xE086CFEED9D3FDA6L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3107)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE0BC23536597D966L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE0C4BE92C41ACBFDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+d 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0xE11FC3DAF7774669L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+d 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE150188AD0F7D9E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 4 (after 33+g 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE19D2AD7EDC54AB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 3 (after 33+c 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0xE1E095091703D3B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE21949FED1BA3C05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE26421F52C98CAB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+a 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE26BB5ACF640A193L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 3 (after 33+h 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0xE2D0A1C062A722D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 1-Base ply 3 (after 53+a 57+c 73+h)
        new OctiBotEngine.OpeningBookEntry(0xE3062AE12261B52CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -2900),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2900)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xE31A4DABB018B666L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 550),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 400)),
        // 4P ply 3 (after 33+c 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0xE33703F684D4AB47L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 3 (after 33+a 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0xE345FFA4A32E36D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+g 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE34ABC287E123245L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 3 (after 33+d 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0xE3C8552564A03E9CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE45ED3ECFA0D40A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+h 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE49D2298B3ED3782L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xE4B212B0B950DE46L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 280),
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 280)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xE517340457E6535CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE55769D679CB3776L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE55920431682A9DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 1 (after 53+a)
        new OctiBotEngine.OpeningBookEntry(0xE61535559B2C9569L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0xE63BF3F7DD240E75L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+h 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE6637519E551D7BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE6832480EFADB75FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0xE6A08DB1B807ACE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1960),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080)),
        // 4P ply 3 (after 33+d 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0xE6A47BBE426BDE8EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+a 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0xE6D8617571B654F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+d 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE77B81FCE356CBF8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE784D72F03225E23L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0xE79E5074BF818012L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 3 (after 33+h 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0xE7BC8F5B446CC2CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+h 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xE7D08C3F5C7AAC78L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE8001DACFAB37479L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE872E1FEDD49E9EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xE875B442344F52C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -378),
            new OctiBotEngine.OpeningBookCandidate("53+h", -378),
            new OctiBotEngine.OpeningBookCandidate("73b+g", -558)),
        // 4P ply 4 (after 33+b 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE878806D5B3E66E2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+d 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE88DB72D3D3D7C34L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+h 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE99543C83B3A6070L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xE9D503F11EF4AE4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 3 (after 33+d 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0xEA15FA82896A9B40L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 4P ply 4 (after 33+d 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEA5A21D2AEEA04C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEAA577014E9E911AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEAAF1692C8E91E17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEABEF7413646FCC0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+h 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEAD2F4252E509272L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEB42D537A8ED1885L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 3 (after 33+b 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0xEB815C9A9D878955L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xEB94C0EB7E2F7036L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEBA603A43041E084L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEC47F42CA93301F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEC71B8A49D4B16D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEC956E3705E7EED0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xECB8A2FF49479429L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xECF96D531DF18062L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xED5F00C9AF341DB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xED9D837D282B165FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEDB118C5CA81E3F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEDE199B61BF69C26L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xEDF798735A95A93BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 3 (after 33+g 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0xEE128BDE205675DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0xEE1648687CF08987L, 8,
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 3 (after 33+b 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0xEE1CC24B4F1FEB73L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEE4787BED10408DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+d 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEEA62E5B0E9C6E24L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 4P ply 3 (after 33+c 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0xEF1B31E4A390020FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 4P ply 4 (after 33+h 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEFA15748F988FD88L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+a 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF01B23E89FA9FAE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+g 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF037B8507D030F46L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+d 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF119258D2F89F7B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 4P ply 4 (after 33+c 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF13E026AFEC57897L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+d 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0xF20EC4C6EC7015D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 4P ply 3 (after 33+d 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0xF262C7A2F4667B6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+c 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0xF29D91711412EEB1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 4P ply 4 (after 33+h 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF2C9CA614B4A1CEAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 3 (after 32+a 46+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0xF34AAEC0CCA098E6L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4263),
            new OctiBotEngine.OpeningBookCandidate("36d+e", -4450),
            new OctiBotEngine.OpeningBookCandidate("56+f", -4483)),
        // 4P ply 3 (after 33+h 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0xF37A3347F261672EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+g 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0xF3942B4B97D49960L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 3 (after 33+b 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0xF39A62DEF89D07CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 53+b 57+f 33+h)
        new OctiBotEngine.OpeningBookEntry(0xF3B2FDC02FAA9AEDL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2024)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF3C1272B6686E465L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+d 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF3CEB372BC5E8F46L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 1-Base ply 4 (after 53+a 37+d 53a+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0xF46DD113DFD7BE6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 2P 1-Base ply 3 (after 33+b 57+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0xF471B6594DAEBD26L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3230),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3230),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -3230)),
        // 4P ply 3 (after 33+h 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xF4AD88475F6B917BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF4E2531778EB0EFAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF586BD394D3198C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 77+f 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xF5876844AE919EABL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33+a", -610),
            new OctiBotEngine.OpeningBookCandidate("33+b", -730)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF5F4416B6ACB0551L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+d 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF5FAA7F27EEC12BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF5FE20F8ECBC8A5CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 3 (after 33+b 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0xF607FC0F2A0565EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+h 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xF67A1EB8CCBCE98EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xF69424F712060375L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF704F0732850F280L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 4P ply 4 (after 33+c 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF723D794F91C7DA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+b 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF729B6077F6BF2A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xF75548B0867450BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // 2P 3-Base ply 3 (after 33+h 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0xF7B732736B694E73L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2276),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2276),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2282)),
        // 4P ply 4 (after 33+h 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF7BA690C9C927310L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+h 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF83FD14FABFC2586L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF8D1C943CE49DBC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF8DB07DC3350DD87L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF8DF80D6A1004565L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF92725AAADFB39C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+d 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF94B26CEB5ED5770L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 4 (after 33+h 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF9C3F3393557C1C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF9D873794D8FAC19L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+h 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF9F5BFB1012FD6E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+c 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFA0277BAB4A0B29DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFA0C9123A087A572L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFB058415582F5BE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFB1465C6A680B936L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 3 (after 33+a 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0xFB5EC1E0C634B849L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 1 (after 53+a)
        new OctiBotEngine.OpeningBookEntry(0xFB663B3F4AA8BBECL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -3100),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3100),
            new OctiBotEngine.OpeningBookCandidate("57+e", -3350)),
        // 2P 3-Base ply 2 (after 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0xFBB2210A0E3D24FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFC1B8A425A47A60EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 3 (after 33+d 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0xFC41127B71CDB028L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 4P ply 3 (after 33+c 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0xFC66359CA0813F0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+a 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFD3A83C6D21535D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xFD3FFCB09521AB26L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xFD421E0773982743L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+h 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFE784B5D804B5924L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+c 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0xFEB1A363335647F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 3 (after 33+d 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0xFEBF45FA27715016L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+a 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0xFEC35F3114ACDA6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+g 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFECC1CBDC990DEFBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+d 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFF0CBCDC9E5A2BD2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+d 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFF60BFB8864C4560L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFF9FE96B6638D0BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+h 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0xFFA7B11F21764C52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xFFE86A4F06F6D3D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 4P ply 4 (after 33+a 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFFED153941C24D2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140))
        };
    }
}
