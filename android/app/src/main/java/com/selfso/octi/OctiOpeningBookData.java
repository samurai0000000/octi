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
        // 4P ply 4 (after 33+h 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x002E103759425879L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x00421353415436CBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // OFK ply 2 (after 32+b 46+f)
        new OctiBotEngine.OpeningBookEntry(0x004C97ACE487F4A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -28),
            new OctiBotEngine.OpeningBookCandidate("42+a", -28),
            new OctiBotEngine.OpeningBookCandidate("32b+e", -180)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0053F280BFFBD41CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x00599313398C5B11L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
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
        // 4P ply 4 (after 33+d 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x00A6C5C0D9F8CECAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
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
        // 4P ply 3 (after 33+d 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x00E91E90FE78514BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
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
        // 2P 1-Base ply 3 (after 53+a 57+c 33+b)
        new OctiBotEngine.OpeningBookEntry(0x014C92CB7EAFBC8AL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -2900),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x015AE7B647532A8FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 2 (after 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x016824F9093DBA3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 3 (after 33+b 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x017DB888EA95435EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 32+b 56+f 42+a 36+g)
        new OctiBotEngine.OpeningBookEntry(0x01A62CFEC767626DL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 2440),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 2320),
            new OctiBotEngine.OpeningBookCandidate("52+g", 1931)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x01BE3125DFFFD28EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 4 (after 33+d 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0271533F4A2FB63FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+a 77+d)
        new OctiBotEngine.OpeningBookEntry(0x02774F7C47B2791BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 462),
            new OctiBotEngine.OpeningBookCandidate("73+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 462)),
        // 4P ply 4 (after 33+b 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0284647F2C2CACE9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x028E05ECAA5B23E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
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
        // OFK ply 3 (after 32+a 56+f 42+c)
        new OctiBotEngine.OpeningBookEntry(0x02EAB3D9C48784AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("46+f", -4663)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x02FCF9BE8DA1BE72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 4 (after 33+h 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0369A7DA4C28AA7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
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
        // 2P 1-Base ply 3 (after 53+b 57+f 53)
        new OctiBotEngine.OpeningBookEntry(0x039B09895876D2BFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2630),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2630),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2630)),
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
        // 4P ply 4 (after 33+d 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x045ACA49798EA42FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
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
        // 2P 3-Base ply 4 (after 33+a 77+e 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x04B7ECA3EE733C69L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x04BB63ACA616C2D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 3 (after 33+b 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x04E02659380D2178L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 3 (after 33+g 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x04EE6FCC5744BFD5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
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
        // 2P_3B_ED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x052A15A7EB3794E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // OFK ply 3 (after 32+b 26+d 32b+e)
        new OctiBotEngine.OpeningBookEntry(0x055892E82B1AB81EL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3463)),
        // 4P ply 4 (after 33+h 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x055DB35A8E9A3783L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // 2P 1-Base ply 4 (after 53+b 57+d 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x05E1AE9791B694D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120)),
        // 4P ply 3 (after 33+c 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x05E7D5F6D482C804L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x060589416AE34A69L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x064446ED3E555E22L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 3 (after 33+b 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0667EC41F1FA77B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3490),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3490),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -3750)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x06698A2572F524DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x068D5CB6EA59DCDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 2P 1-Base ply 3 (after 53+c 57+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x068F63D0D9CDAA97L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 2P 3-Base ply 3 (after 33+b 37+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x06AB8A3D836CB2AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x06BB103EDE21CBF9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 3 (after 53+c 57+f 33+a)
        new OctiBotEngine.OpeningBookEntry(0x06BC09C2353A0DB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2830),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2840),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2950)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+h 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x06E56D50C2D49525L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x071D7DA46CE4562DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260)),
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
        // 4P ply 4 (after 33+g 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x074DFCD7BD9329F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0761676F5F39DC54L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 53+b 57+c 53b+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x07768DBBDCC4C11AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x07A3E4DBD826D7BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 77+e 33b+a 77e+f)
        new OctiBotEngine.OpeningBookEntry(0x07B32F64BD113D9CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
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
        // 4P ply 3 (after 33+h 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x082C45D215B5E8D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x08322B730FB11B54L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62)),
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
        // 4P ply 4 (after 33+a 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x089751BE81526B98L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0898C5E75B8A00BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
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
        // 2P 3-Base ply 3 (after 33+h 57+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x08E590B18D6F26BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024)),
        // 4P ply 4 (after 33+c 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x08E5ADECA6A8F60EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 2 (after 32+b 36+d)
        new OctiBotEngine.OpeningBookEntry(0x08EE9C0681127B36L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -620)),
        // 4P ply 3 (after 33+d 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x0934B13713B2F497L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 4 (after 33+g 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x09B6583A0900F84EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 3 (after 33+a 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x09B91BB6D43CFCDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+c 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x09CBE7E4F3C6614CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x09DA63262299475AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // OFK ply 3 (after 32+b 36+d 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x0A1ACBE9B0E0B357L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3801),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -3801),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4243)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0A385A80B30801F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0A40C7411285136DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
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
        // 4P ply 3 (after 33+h 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x0AFBD32D86629026L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 33+h 57+e 33h+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0B025ADE7C08BC30L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+b", -62)),
        // 4P ply 3 (after 33+c 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x0B1C711B601119B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
            new OctiBotEngine.OpeningBookCandidate("77+e", -3100),
            new OctiBotEngine.OpeningBookCandidate("57+f", -3100),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3100)),
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
        // 4P ply 4 (after 33+g 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x0B61CEC59AD780BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
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
        // 4P ply 4 (after 33+d 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0BACFC98A7E513E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 3 (after 33+d 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x0BE327C880658C62L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x0BF4AF592976E801L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // OFK ply 2 (after 32+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0x0BFBD718BD817E6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32+b", 0),
            new OctiBotEngine.OpeningBookCandidate("32+a", -59)),
        // 4P ply 3 (after 33+a 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x0C24856706A49EFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+d 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x0C589FAC35791485L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0x0C593C7C035B3913L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", 462),
            new OctiBotEngine.OpeningBookCandidate("33+a", 462),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 462)),
        // 2P 3-Base ply 3 (after 53+a 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0x0C637E934FB15138L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x0C7D02CB03B6FA63L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0C7FC09298BF7D54L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+h 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0C9F910B92431DB7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+c 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x0CADE0F76130DA36L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
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
        // 4P ply 3 (after 33+h 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x0D2C682D2B686673L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // OFK ply 3 (after 32+a 56+f 22+c)
        new OctiBotEngine.OpeningBookEntry(0x0D3A8CA49E89E320L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // 4P ply 3 (after 33+h 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x0D406B49337E08C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0D78333D74309428L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0D8765EE944401F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
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
        // 4P ply 4 (after 33+h 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x0E61C68AC4FFFD89L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 3 (after 33+b 77+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x0E9A87FCDBC3CF15L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2690),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2690),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2970)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0EA237FE8D1F8AACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
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
        // 4P ply 4 (after 33+b 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0FA5C451619063D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+c 33+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x0FA923303252DBC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x0FAB8DC40ED9FD7DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
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
        // OFK ply 3 (after 32+b 26+d 52+h)
        new OctiBotEngine.OpeningBookEntry(0x10DFD3BD3E5C3A31L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("26d+a", -6783)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x10F07531D7956F79L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x10FE93A8C3B27896L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 3 (after 33+a 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x11A225F2B1267242L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x11AEAB8CA7DA720DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -138)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x11E881D4D192733DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x11F5EF0867586F53L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x11F960072F3D91EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x122364C4D6128F6EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1227E3CE4442178CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x122D2D51B95B11C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x124076ABBFD51524L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -460),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -510)),
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
        // 4P ply 4 (after 33+h 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x12C3355DDCEEEF8DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
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
        // 4P ply 4 (after 33+h 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x13095BA3763D1CEBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+e 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x1312513B563F77FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1180),
            new OctiBotEngine.OpeningBookCandidate("33+a", -1180),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1200)),
        // 4P ply 4 (after 33+c 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1324976B3A9D6612L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
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
        // 4P ply 4 (after 33+h 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x133F172B42450BC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 2P 1-Base ply 4 (after 53+b 57+d 73+a 57d+f)
        new OctiBotEngine.OpeningBookEntry(0x13A76EFB4BFFC61FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -210)),
        // 4P ply 4 (after 33+d 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x13B7C2DCC2FF9D7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // OFK ply 3 (after 22+b 46+f 32+a)
        new OctiBotEngine.OpeningBookEntry(0x13CA98A302CF3F6DL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("36d+g", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+e", -6435)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x13DBC1B8DAE9F3C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
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
        // 4P ply 4 (after 33+g 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1430F8AFBE8214F0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // OFK ply 4 (after 32+b 46+f 32b+a 36+e)
        new OctiBotEngine.OpeningBookEntry(0x1432FD45812CFA41L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1287),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -1287),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1747)),
        // 4P ply 3 (after 33+a 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x143FBB2363BE1064L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 33+a 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x14432E955F18C7A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 3 (after 33+d 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x1443A1E850639A1DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x144D477144448DF2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+h 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1484AF4FF759932FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x14C00F04D90564BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x14E192AAF48EC959L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120)),
        // 4P ply 4 (after 33+a 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1511F12B36D08726L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x15148E5D71E419D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 2P 1-Base ply 3 (after 53+a 57+d 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x152152E68C6CF82CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -3350),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -3350)),
        // 2P 3-Base ply 3 (after 33+a 37+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x153B2D27583E7C6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2502)),
        // 4P ply 3 (after 33+h 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x155B550D56648659L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x15630D79112A1AB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 4 (after 33+d 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x159C5BAAF15E8F6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260)),
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
        // 4P ply 4 (after 33+d 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x15F058CEE948E1D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 1-Base ply 2 (after 53+c 77+f)
        new OctiBotEngine.OpeningBookEntry(0x15F73C0A3B55AEA0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
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
        // 4P ply 3 (after 33+c 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x169AD18ED793F507L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 22+b 26+d 42+a 46+a)
        new OctiBotEngine.OpeningBookEntry(0x16A0A8C47F2E89FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -580),
            new OctiBotEngine.OpeningBookCandidate("42+a", -580),
            new OctiBotEngine.OpeningBookCandidate("52h-43", -580)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x16B33CA2017FC13FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 400)),
        // 4P ply 3 (after 33+d 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x16BDF66906DF7A23L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 2P 3-Base ply 2 (after 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x16E47C32DE61104AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x16E76E502D556C05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x16FBB18638D20833L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -710),
            new OctiBotEngine.OpeningBookCandidate("53h+b", -710),
            new OctiBotEngine.OpeningBookCandidate("73+a", -744)),
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
        // 2P 3-Base ply 4 (after 53+a 37+d 33+h 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x17B8DC764A47ABFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -62)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x17BEFA15048AED48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x17C25A9AD10FEEDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+c", -730)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x17C318A2E233612DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 3-Base ply 4 (after 33+b 57+d 53+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x17C4CCC38A67654BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+a 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x17C667D4A507FFD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // OFK ply 4 (after 22+b 36+e 42+h 46+f)
        new OctiBotEngine.OpeningBookEntry(0x17E5DC1A4F36D8F6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 640),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 640),
            new OctiBotEngine.OpeningBookCandidate("52h+b", -1020)),
        // 4P ply 4 (after 33+h 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x18352E733C58D6E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // 2P_1B_ED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x18591BCD3AB3BA6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+c 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x18617563630024BAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
        // 2P 3-Base ply 3 (after 33+b 37+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x186210C8B40FA1CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396)),
        // 4P ply 3 (after 33+d 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x189E23B08374B161L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 3 (after 33+d 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x18F220D49B62DFD3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
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
        // 4P ply 4 (after 33+d 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x19325760CB4C454DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x193D1A7254E30148L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x193DC33911942E6EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 3 (after 33+b 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x196686CC8F8FCDC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x1968CF59E0C6536BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 1-Base ply 4 (after 33+b 57+c 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x196CC2CD6E062A11L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 400),
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0x1986B5441A26B922L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200)),
        // 4P ply 3 (after 33+h 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x1986D7558573AD25L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
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
            new OctiBotEngine.OpeningBookCandidate("46+e", -3343),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3343),
            new OctiBotEngine.OpeningBookCandidate("56+f", -3343)),
        // OFK ply 4 (after 32+b 36+e 42+a 26+a)
        new OctiBotEngine.OpeningBookEntry(0x1AC4F02B6B5A4241L, 8,
            new OctiBotEngine.OpeningBookCandidate("32b-43", 1101),
            new OctiBotEngine.OpeningBookCandidate("32b+a", 1101),
            new OctiBotEngine.OpeningBookCandidate("52+h", 260)),
        // 4P ply 4 (after 33+g 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1ACB5C420A11C54DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+a 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1AE7C7FAE8BB30EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 4 (after 33+c 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1BC2E67889D7B29CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 4 (after 33+d 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1BE5C19F589B3DB8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
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
        // 4P ply 3 (after 33+h 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x1C86FAAABBAE2385L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
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
        // 2P 3-Base ply 4 (after 33+b 37+d 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x1CD09F6BAC21B4E6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 3 (after 33+b 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x1CFB181D5D17AFE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 4 (after 33+h 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x1D468D1EEB80B91BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // 4P ply 4 (after 33+b 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1DD55215087938A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // OFK ply 4 (after 32+b 36+e 52+h 46+a)
        new OctiBotEngine.OpeningBookEntry(0x1DDC627DBB9C1FE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 544),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 544),
            new OctiBotEngine.OpeningBookCandidate("42h+a", 187)),
        // 4P ply 4 (after 33+c 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1DDF33868E0EB7AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+h 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x1DF2C5789339DA92L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", 0)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1DF814615F42388BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 3 (after 53+a 37+f 73+a)
        new OctiBotEngine.OpeningBookEntry(0x1E150F17625E4019L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77d+e", -2086)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1E1EB7050FF9C4F1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
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
        // 4P ply 3 (after 33+h 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x1E516C5528795B70L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 4 (after 33+b 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x1F02C4EA9BAE4057L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+d 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1F0643E009FED8B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1F08A5791DD9CF5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
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
        // 4P ply 4 (after 33+a 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x1F7A592B3A2352CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 4 (after 33+g 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x200D7AC12A71B45CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 2P 1-Base ply 4 (after 53+b 77+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x200F49221B848173L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -690),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -690),
            new OctiBotEngine.OpeningBookCandidate("73+h", -740)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x202093B906243D37L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
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
        // 2P 1-Base ply 4 (after 53+a 57+e 53a+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x208C68B39D99226EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0)),
        // 4P ply 3 (after 33+b 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x209499305380AF8FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 4 (after 33+b 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x20DB42607400300EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
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
        // 4P ply 4 (after 33+d 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2123E71C78FB4CA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+c 37+d 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x2126A67FFE622CA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -650),
            new OctiBotEngine.OpeningBookCandidate("33+a", -650),
            new OctiBotEngine.OpeningBookCandidate("33+b", -730)),
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
        // 4P ply 3 (after 33+a 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x217C25E374B037E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // OFK ply 4 (after 32+b 36+d 42+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x21A7A3FB162B85B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 640),
            new OctiBotEngine.OpeningBookCandidate("42h-33", 640),
            new OctiBotEngine.OpeningBookCandidate("22+h", -1020)),
        // 4P ply 4 (after 33+c 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x21B0B2AB8099B7C0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260)),
        // OFK ply 3 (after 22+b 56+f 42+e)
        new OctiBotEngine.OpeningBookEntry(0x21BF94B66BA7C442L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("46+a", -3960)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x21C7318FE057B4A8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 4 (after 33+c 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x21DCB1CF988FD972L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+h 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x22068C55F7A13561L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 2 (after 33+g 37+f)
        new OctiBotEngine.OpeningBookEntry(0x2225389CAA0F3CD1L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
        // 4P ply 3 (after 33+h 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x223EED7826EE5B1FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+b 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x22430FCFC057D77AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
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
        // 4P ply 4 (after 33+g 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x22DAEC3EB9A6CCA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0x22F0597755F9ABFCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53ab-64", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", -780)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x22F7054695F345C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x22F9E3DF81D4522DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 4 (after 32+b 26+d 32b-43 46+a)
        new OctiBotEngine.OpeningBookEntry(0x2301213CBD94DFA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 544),
            new OctiBotEngine.OpeningBookCandidate("52+h", 544),
            new OctiBotEngine.OpeningBookCandidate("43b+a", 187)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2310A7707380CC5DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
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
        // 4P ply 4 (after 33+b 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2383787B90794DE4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 3 (after 33+a 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x23ABB31CE7674F16L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x23B5DDBDFD63BC91L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
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
        // 4P ply 3 (after 33+d 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x249DA1F995F5DFBCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 4P ply 3 (after 33+c 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x24BA861E44B95098L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+a 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x24CB236606A09907L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -520)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x24EEF8BE7B145151L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
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
        // 2P 1-Base ply 4 (after 53+a 57+c 53a+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x2592892AF428B263L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 400),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
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
        // 4P ply 4 (after 33+a 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x25E63044362D5A4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2610AF3F2DA8B16FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 3 (after 33+c 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x2644D19F1205B0A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x264A37060622A749L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 3 (after 33+a 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x26AB9EE3D9BAC1B6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 3 (after 33+a 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x26C79D87C1ACAF04L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x26F0DB1647A1221EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 3 (after 53+c 57+d 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x26FF1CF5557783B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2170),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2290),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2290)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x271867C56091BA72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 4 (after 32+a 46+f 22+c 46f+e)
        new OctiBotEngine.OpeningBookEntry(0x271E42684607E3D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", 162),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -878),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1243)),
        // 4P ply 3 (after 33+b 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x27432230FE8A59DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+d 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x27647D0E534C300BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
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
        // 4P ply 4 (after 33+c 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x279B2BDDB338A5D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+h 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x27A373A9F4763939L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x27F728B9AB2ECB62L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
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
        // 2P 3-Base ply 4 (after 33+b 77+f 73+b 77f+e)
        new OctiBotEngine.OpeningBookEntry(0x283116190B004EC7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62)),
        // 4P ply 4 (after 33+g 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x28337AE6A8037A2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x283E44E3432BFCCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x285F7982B015149FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
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
        // 2P_3B_ED ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0x28D16B85C04AC61FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2752),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2752),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2752)),
        // 2P 3-Base ply 3 (after 33+a 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x28D21FB1A905D5B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2086)),
        // 4P ply 4 (after 33+a 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x28EE5114BEFA0DBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 2P 1-Base ply 4 (after 33+b 37+d 33b-44 77+f)
        new OctiBotEngine.OpeningBookEntry(0x28F63BC58B8053E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33", 270),
            new OctiBotEngine.OpeningBookCandidate("53+a", -3390),
            new OctiBotEngine.OpeningBookCandidate("44b+c", -3390)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2906EDC799CA95D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
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
        // 4P ply 4 (after 33+h 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x297B0F707F7319B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+g 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x299F0E36E03B8E01L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x29C5960FCBB19827L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 33+a 37+f 73+h 77+d)
        new OctiBotEngine.OpeningBookEntry(0x29D8927DB5962328L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 62),
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 2P 1-Base ply 4 (after 53+c 57+e 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x29DAE16FED02E423L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x29E2B1E81AFD1703L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 3-Base ply 3 (after 53+b 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x29FDDC37D2EDC225L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+d", -2486)),
        // 2P 1-Base ply 3 (after 53+b 77+f 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x2A069F95E413DCCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600)),
        // 4P ply 4 (after 33+a 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2A39C7EB2D2D754BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2A415A2A8CA067D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 3 (after 33+a 77+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x2A513FF4D07EFB2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2396)),
        // 4P ply 3 (after 33+d 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x2A66051421660E01L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+c 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x2A9953C7C1129BDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
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
        // 4P ply 2 (after 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x2AEFD237FBEF1DF8L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 3 (after 33+c 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x2AF550A3D904F568L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 22+b 46+e 42+c 56+g)
        new OctiBotEngine.OpeningBookEntry(0x2B16CCA8C48820C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1040),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1040),
            new OctiBotEngine.OpeningBookCandidate("22b+c", -1040)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2B352717892A6FF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 3 (after 33+g 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x2B4898C973ECF6F4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
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
        // 4P ply 3 (after 33+b 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x2B9EA0682D9D72A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+a 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2BA9E6F9AB90FFBCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2BC5E59DB386910EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2BD17B380A1DED27L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 2P 3-Base ply 3 (after 33+b 77+f 33b+a)
        new OctiBotEngine.OpeningBookEntry(0x2BD6DC76FA67D44CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("77f+e", -2086)),
        // 2P 3-Base ply 2 (after 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x2C034D1397B8165CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -252)),
        // 4P ply 4 (after 33+b 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2C06C038A7171B72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // OFK ply 2 (after 22+b 46+f)
        new OctiBotEngine.OpeningBookEntry(0x2C099A58C1AF076AL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32+b", -620),
            new OctiBotEngine.OpeningBookCandidate("52h+g", -1220)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2C2412152AF47078L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 1 (after 33+g)
        new OctiBotEngine.OpeningBookEntry(0x2CC2E6FA5998B564L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1096),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1096)),
        // 2P 3-Base ply 3 (after 33+a 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x2CE33DAEBB5C5228L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("57e+d", -2564)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2CE691A1ADEB7B91L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2D0133974B98F20EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2D0FD50E5FBFE5E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2D2CDAEF67CD7B65L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2D827F8F9831EDACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+b 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2DCCAEC60DC4E814L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 3 (after 33+b 57+c 73+b)
        new OctiBotEngine.OpeningBookEntry(0x2DE01A8F0328D072L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2760),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2760),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2DEE7CEB8027831EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2DFAE24E39BCFF37L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 3 (after 53+c 57+f 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x2E1FB082580479DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2170),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2290),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2290)),
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
        // 4P ply 4 (after 33+a 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x2EDA45947C489046L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 2 (after 53+c 57+e)
        new OctiBotEngine.OpeningBookEntry(0x2EFFC03F74E03DABL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2F2D74B1AA6B87C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x2F2DD3F1E9394613L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk1() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // 4P ply 4 (after 33+h 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2F3C956254C46515L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
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
        // 4P ply 4 (after 33+g 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x2FD28D6E31719B5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2FD843F1CC689D14L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+a 57g+d)
        new OctiBotEngine.OpeningBookEntry(0x2FF8C1BB93635F0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x2FFF64161D241230L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x304447C6D50F9A07L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
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
        // 4P ply 4 (after 33+c 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3076234808499C84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+b 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x3077F87F64D96BAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -1050)),
        // 4P ply 4 (after 33+d 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3089759BE83D095FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x30A433EFBF060976L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
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
        // 4P ply 4 (after 33+h 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x316031341A69972FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+f 33+b 57f+e)
        new OctiBotEngine.OpeningBookEntry(0x318920CD8F8BCA3DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -520),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -520),
            new OctiBotEngine.OpeningBookCandidate("73+a", -520)),
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
        // 4P ply 3 (after 33+a 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x3201219B77A10AE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 2P 1-Base ply 3 (after 53+c 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x3201438AE8F41EE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2080)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x325A646EE9BAE948L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+d 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x325EE3647BEA71AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x3270FC052528A9C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x32A1B5B79B9EE471L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x32BA35F7E34689ABL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
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
        // 4P ply 3 (after 33+c 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x32EE6EE7BC1E7BF0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
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
        // 2P 1-Base ply 4 (after 53+c 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3351440C3672F9F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x335D97C105350034L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x33707EB92960895FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 3 (after 33+b 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x33859E2C4887FC3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+a 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x33B2D8BDCE8A7124L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+a 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x33D711DEC745EF30L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 3 (after 33+b 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x33E99D485091928CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 3 (after 33+b 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x343E2648FD9B64D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 33+a 57+e 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x34404C392DE867AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // OFK ply 3 (after 22+b 36+e 42+c)
        new OctiBotEngine.OpeningBookEntry(0x344091FCCD22BC3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3496),
            new OctiBotEngine.OpeningBookCandidate("36+e", -4103)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3443369A7C337499L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x3444299DBBC7ADC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -1970)),
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
        // 4P ply 4 (after 33+c 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x348A2CC1A83FF661L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x34BC60499C47E142L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x34C2FE5F4E0A00CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 62)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+h)
        new OctiBotEngine.OpeningBookEntry(0x34FBE30F632DDFC4L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x34FDAFE5C8F1F509L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x351A0DD32E827C96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 22+b 56+f 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x3531909D3312EFF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42e+h", 0),
            new OctiBotEngine.OpeningBookCandidate("22b+c", -497)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3537E4AB02D7F5FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
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
        // 4P ply 4 (after 33+a 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x359941CBFD2B6334L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 3 (after 33+a 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x35D69A9BDAABFCB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // OFK ply 4 (after 22+b 36+e 22b-33 46+f)
        new OctiBotEngine.OpeningBookEntry(0x35DF821DF1DC5EE9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+h", -3603),
            new OctiBotEngine.OpeningBookCandidate("32+c", -3603),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3603)),
        // 4P ply 3 (after 33+g 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x36164968F55600B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
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
        // 4P ply 4 (after 33+c 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x365DBA3E3BE88E94L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x368BCEA3B468211CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 4 (after 33+a 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x36C17BD019521EDEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 3 (after 33+b 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x36E9B0B76E4C1C2CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 4 (after 33+h 37+f 73+b 37f+e)
        new OctiBotEngine.OpeningBookEntry(0x36F61227E7F6DE4AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0)),
        // 4P ply 3 (after 33+a 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x37010C64497C8440L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
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
        // 4P ply 3 (after 33+d 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x37E0A58196E4E2BFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x37FBF40BF2562A64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+g", -120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+c 57+f)
        new OctiBotEngine.OpeningBookEntry(0x3823B10B64EC018BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -1010),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -1010),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -2250)),
        // 4P ply 3 (after 33+g 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x38359CB170FDCBF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x3862A6D1A3B13FFBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // OFK ply 3 (after 32+b 56+f 52+g)
        new OctiBotEngine.OpeningBookEntry(0x3880C675E62D8870L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -3980),
            new OctiBotEngine.OpeningBookCandidate("36+f", -4542),
            new OctiBotEngine.OpeningBookCandidate("46+f", -5281)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x388EAD6D84EFBB2EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 3-Base ply 4 (after 33+a 37+f 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x389051A6D73DDE27L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x38AC7F40090CD024L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x38B8E1E5B097AC0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x38B9C665EB624684L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+b 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x38C07C24111ABE96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 37+d 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x38C34276E1033E9DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 2P 1-Base ply 3 (after 33+b 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x3903823A99E10FE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2980),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090)),
        // 4P ply 3 (after 33+d 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x391B016C22773302L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 4P ply 4 (after 33+a 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3944C3932E3C4848L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x396611BEA3DF2342L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 3 (after 32+b 36+e 32b+e)
        new OctiBotEngine.OpeningBookEntry(0x3969EDC23A1EDBD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+d", -3463),
            new OctiBotEngine.OpeningBookCandidate("36+a", -3463)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3999E86138C53FDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x39AB8CEFE5833958L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
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
        // 4P ply 3 (after 33+c 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x39E457BFC203A6D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+b 37+c 53b+a 37c+d)
        new OctiBotEngine.OpeningBookEntry(0x39E8320CB56DB7C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33+b", 400)),
        // 4P ply 3 (after 33+h 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x3A25D33C43F4D587L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 2 (after 33+g 37+e)
        new OctiBotEngine.OpeningBookEntry(0x3A3E06D8CF15B249L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
        // OFK ply 4 (after 22+b 56+f 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x3A62E4E66A949E7FL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("22b+e", -3500)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3A6F771A2340D4F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 2P 3-Base ply 3 (after 33+h 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x3A91106E378F53E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3A9FB59019EC2A00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 3-Base ply 3 (after 53+b 37+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x3AD6CFB0460928EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2486)),
        // 4P ply 4 (after 33+d 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3AE2DD9BE4CEDCB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 2 (after 53+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3B1A0BE82177D5B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+c 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x3B33C14051D4DE2CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3B4E7E9EAB12472EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3B7832169F6A500DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3B98463FF563C37CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 2 (after 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3B992A79A76A76ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 3-Base ply 4 (after 33+h 57+e 33h+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3BC117E8E909B4F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62)),
        // 4P ply 3 (after 33+d 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x3BCC9793B1A04BF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x3BCD8E7CEAE1563EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3C41911A57D55816L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3C44EE6C10E1C6E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
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
        // 2P 1-Base ply 4 (after 53+c 57+e 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0x3C98E2C46429B719L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3CA54789CF79A017L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+b 57+f 53 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x3CAAAE6F8C907B08L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 150),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 150),
            new OctiBotEngine.OpeningBookCandidate("33+b", -250)),
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
        // 4P ply 4 (after 33+g 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x3CF5C6FA1E0EDFC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+g 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3D53AB60ACCB421DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
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
        // 4P ply 4 (after 33+b 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3DAC52BF37D15E84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
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
        // 4P ply 3 (after 33+c 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x3E5FEFDB771F3E3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // OFK ply 1 (after 22+b)
        new OctiBotEngine.OpeningBookEntry(0x3E7C56E08875ECA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4143),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4143),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4143)),
        // 4P ply 4 (after 33+b 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3E87E6363AADC234L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3E9378938336BE1DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3E9607E5C40220E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 3-Base ply 3 (after 33+h 37+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x3EBCDBE9AAECDBAAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2402),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2402)),
        // 4P ply 3 (after 33+a 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x3EDCA3C3A4B6219CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 22+b 36+e 22b+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x3EE1054D1B8B7B40L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3979),
            new OctiBotEngine.OpeningBookCandidate("32+c", -3979),
            new OctiBotEngine.OpeningBookCandidate("32+a", -4000)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3EEBE55222BBAC86L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3F035981058B34EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 4 (after 33+b 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x3F7BC440A4062671L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3F7F434A3656BE93L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x3F801599D6222B48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 2 (after 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x3FADD19C72C44EC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 3 (after 33+h 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x3FB84DED916CB7A1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 4 (after 33+d 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x401195F83307F4E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x401512F2A1576C05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 57+f 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x4020991903FBD91EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -210)),
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
        // 4P ply 4 (after 33+c 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x40D88FA3E70B761FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
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
        // 2P 3-Base ply 4 (after 33+h 57+e 53+h 77+d)
        new OctiBotEngine.OpeningBookEntry(0x40EDD137D91D5115L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x40EEC32BD373613CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x40F5436BABAB0CE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4112E15D4DD88579L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // OFK ply 4 (after 32+b 36+d 32b-43 36d+g)
        new OctiBotEngine.OpeningBookEntry(0x411F69311F305B29L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 2418),
            new OctiBotEngine.OpeningBookCandidate("22+b", 2418),
            new OctiBotEngine.OpeningBookCandidate("22+c", 2418)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x413F0825618D0C12L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
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
        // 4P ply 3 (after 33+a 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x41B27571A1E76BE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 3 (after 53+c 57+c 33+a)
        new OctiBotEngine.OpeningBookEntry(0x41B89D4739C43174L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2940)),
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
        // 4P ply 4 (after 33+a 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x41FDAE218667F469L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
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
        // 4P ply 4 (after 33+c 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x423955D440A419C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
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
        // 4P ply 3 (after 33+g 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4272A6828E1A97E8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
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
        // 4P ply 4 (after 33+d 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x42C60307A0D08C12L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x42C9975E7A08E731L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 1-Base ply 3 (after 53+b 57+d 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x42E9BA2B3FC6ED03L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("57d+f", -3350),
            new OctiBotEngine.OpeningBookCandidate("77+e", -3350)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x42EB4573F7EB8C3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 1 (after 53+b)
        new OctiBotEngine.OpeningBookEntry(0x42F7FBED36E9148FL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+c", -2486),
            new OctiBotEngine.OpeningBookCandidate("57+g", -2486)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+c)
        new OctiBotEngine.OpeningBookEntry(0x431F8776FC773BB3L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1770),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1770),
            new OctiBotEngine.OpeningBookCandidate("57+d", -1800)),
        // 4P ply 4 (after 33+h 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x432F47A852841262L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
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
        // 4P ply 3 (after 33+a 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x4365E38E3230131DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
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
        // 4P ply 3 (after 33+d 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x43E8490FF5BE1B50L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 32+b 56+f 52+a 56f+g)
        new OctiBotEngine.OpeningBookEntry(0x43F160C1C2C181C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -180),
            new OctiBotEngine.OpeningBookCandidate("52a+h", -180),
            new OctiBotEngine.OpeningBookCandidate("32b+h", -1200)),
        // 4P ply 4 (after 33+c 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4412CCA273050BD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 4 (after 33+g 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4420A82CAE430D5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
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
        // 2P 3-Base ply 4 (after 33+a 37+f 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x44DCF328CB44D684L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 4P ply 4 (after 33+b 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x44F6908DF0328908L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
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
        // 4P ply 3 (after 33+h 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x454B058E46A59FF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 3 (after 33+a 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x4609CD1514FBF30FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4621067263E5F1FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 1-Base ply 3 (after 33+b 77+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x4625849542754430L, 8,
            new OctiBotEngine.OpeningBookCandidate("77", -3030),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090)),
        // 4P ply 3 (after 33+d 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x4675D7DE27267976L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x468A810DC752ECADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x46B2D979801C7044L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x46C55A5DE0D2732CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 1-Base ply 3 (after 53+a 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x46C65ED61A5B1B77L, 8,
            new OctiBotEngine.OpeningBookCandidate("57f-46", -2360),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2360),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2360)),
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
        // 4P ply 4 (after 33+c 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x47557B4F666FF9DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 3 (after 33+b 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x47607F3C6B886C6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x477892374A3A70B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 3 (after 33+b 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x478D72A22BDD05D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 2P 1-Base ply 4 (after 53+c 37+d 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x47932D13B3F749BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -400)),
        // 4P ply 3 (after 33+h 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x479C9371D572E706L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
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
        // 4P ply 4 (after 33+a 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x47D63757B5C6E679L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+b 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x47E171C633CB6B63L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 2P 3-Base ply 4 (after 33+a 37+f 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x4810072FBEAB60DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 62),
            new OctiBotEngine.OpeningBookCandidate("53+h", 62),
            new OctiBotEngine.OpeningBookCandidate("53+b", 62)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4813327818E1D868L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x482001866BAD51B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
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
        // 4P ply 4 (after 33+d 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x48ADAB07AC2359F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x48C0356E0E882379L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 60),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x48FD2A747D542626L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 1-Base ply 4 (after 53+b 57+d 73+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0x491C9268E4131146L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", -340),
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -590)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4937448AD787D540L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // OFK ply 3 (after 32+b 56+f 52+a)
        new OctiBotEngine.OpeningBookEntry(0x49393E0C2BC37BD6L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -3323),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("26d+c", -3683)),
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
        // 4P ply 3 (after 33+b 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x49AEA77BAE76CE93L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 2P 3-Base ply 4 (after 33+h 57+e 73+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x49F9DE72E755C591L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -830),
            new OctiBotEngine.OpeningBookCandidate("53+b", -830),
            new OctiBotEngine.OpeningBookCandidate("73b+h", -830)),
        // 2P 3-Base ply 3 (after 33+a 37+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0x4A3942D387B9B1CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37f+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 2P 3-Base ply 3 (after 33+a 37+d 33a+b)
        new OctiBotEngine.OpeningBookEntry(0x4A56B6C2F6C9373CL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086)),
        // 4P ply 3 (after 33+c 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x4A5703551445C7D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1356),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1476)),
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
        // 4P ply 4 (after 33+b 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4A8F0AB859F73BDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4AA606F702F42CA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 3 (after 33+a 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4AB84C29DFFAB6C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4AE309DC41E15569L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4AF79779F87A2940L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4B06B144284FD02CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120)),
        // 4P ply 4 (after 33+a 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4B0BB50F66D1CD05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+c)
        new OctiBotEngine.OpeningBookEntry(0x4B0C4416ED6513C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("77+e", -1686),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -1686)),
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
        // 4P ply 3 (after 33+b 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x4B50F0FAF8CA2EADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 3 (after 53+c 57+d 73+h)
        new OctiBotEngine.OpeningBookEntry(0x4B9D923BAA04BA1CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2950),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2950),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2950)),
        // 4P ply 3 (after 33+h 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x4BB0A163F2364E4EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4BCD320A4EA91B17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4BE4FA73AD6EBC15L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+g 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4C3223F7266CD3FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338)),
        // 4P ply 3 (after 33+g 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x4C3D703F13A73218L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // OFK ply 4 (after 32+a 56+f 22+c 36+a)
        new OctiBotEngine.OpeningBookEntry(0x4C919A6960E9AED0L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 0),
            new OctiBotEngine.OpeningBookCandidate("52g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("32+e", 0)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4CA493CE6A5629CBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
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
        // 4P ply 4 (after 33+b 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4CC890AA72404779L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4CD971798CEFA5AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4CDC0E0FCBDB3B50L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4CEA4287FFA32C73L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 4 (after 32+b 36+e 52+g 46+a)
        new OctiBotEngine.OpeningBookEntry(0x4D22DFF4E33F8C93L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 0),
            new OctiBotEngine.OpeningBookCandidate("52g+h", 0)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4D3035D67EBB3BDEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4D34B2DCECEBA33CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4D4C2F1D4D66B1A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+c 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4D80B855B94F3184L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 22+b 26+d 42+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x4DCCE9C693235226L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+b", -3498)),
        // 4P ply 4 (after 33+c 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4DCF63059ECFAE05L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 4 (after 33+g 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4DFD078B4389A886L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4E0B98F0580C43A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x4E0EE7861F38DD5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 3-Base ply 3 (after 53+b 37+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x4E1489541ADA398FL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2264)),
        // 4P ply 2 (after 33+g 37+b)
        new OctiBotEngine.OpeningBookEntry(0x4E36EA56AC4F4BA6L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -20),
            new OctiBotEngine.OpeningBookCandidate("77+a", -20),
            new OctiBotEngine.OpeningBookCandidate("77+g", -140)),
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
        // 4P ply 4 (after 33+c 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4E97591E7AB6D3EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4EEA31158794255AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x4EEF51F946A2AFE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73h+a", -520),
            new OctiBotEngine.OpeningBookCandidate("33+a", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+h", -520)),
        // 4P_ED ply 1 (after 33+f)
        new OctiBotEngine.OpeningBookEntry(0x4F03223E9DC27F4EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+c", -1177),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1177),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1177)),
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
        // 4P ply 4 (after 33+g 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4F1CDDFCE426C750L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x4F2A9174D05ED073L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 2P 3-Base ply 3 (after 53+b 57+g 53b+a)
        new OctiBotEngine.OpeningBookEntry(0x4F56C8095116EA68L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2486),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2486),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -2486)),
        // 4P ply 3 (after 33+c 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x4F572EAA2A984971L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 2P 3-Base ply 3 (after 53+b 57+g 33+a)
        new OctiBotEngine.OpeningBookEntry(0x4FD5E998D70B4975L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2486),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -2486)),
        // 4P ply 4 (after 33+b 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x4FE324237F3CDBC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
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
        // 4P ply 4 (after 33+g 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x50959AA2F14B49E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 3-Base ply 3 (after 33+a 57+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0x50B2E54FCDE09127L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2396),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2402)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x50B69543C939D760L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x50B873DADD1EC08FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // OFK ply 4 (after 32+b 56+f 52+g 46+f)
        new OctiBotEngine.OpeningBookEntry(0x50E7CBFEBC729710L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -227),
            new OctiBotEngine.OpeningBookCandidate("52g+h", -227),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -657)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5146976995D906C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x515FD1EC3B6D4910L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x51B493267919DA32L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
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
        // 4P ply 4 (after 33+g 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x52420C5D629C3111L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 2P 3-Base ply 3 (after 33+b 37+d 53+b)
        new OctiBotEngine.OpeningBookEntry(0x526A818E7419A2EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2382),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2402)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x526FE5254EC9B87AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // OFK ply 4 (after 32+b 36+d 52+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x5285488F6CDD3F7FL, 8,
            new OctiBotEngine.OpeningBookCandidate("52a+h", -3579),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3602),
            new OctiBotEngine.OpeningBookCandidate("32b+g", -3782)),
        // 4P ply 4 (after 33+b 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x529434FC3CEDB543L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 3 (after 53+a 37+e 73+b)
        new OctiBotEngine.OpeningBookEntry(0x52A15D0664BE9C48L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 4P ply 3 (after 33+a 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x52A3726DBAE03859L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 4P ply 2 (after 33+c 37+d)
        new OctiBotEngine.OpeningBookEntry(0x52B9F0F9980BD0C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 4P ply 3 (after 33+a 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x52CF7109A2F656EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
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
        // 4P ply 3 (after 33+b 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x534BCEBE9DD0A035L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x536305D9EACEA2C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+d 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x536C91803016C9E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x53884713A8BA31E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+c 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5393C753D0625C3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // OFK ply 3 (after 22+b 56+f 42+c)
        new OctiBotEngine.OpeningBookEntry(0x53BF987A8382CE08L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("46+f", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+d", -4663)),
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
        // 4P ply 4 (after 33+c 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x53FFC437C874328DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
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
        // 4P ply 3 (after 33+g 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x54264E7B76BDBC80L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 2P 3-Base ply 4 (after 33+b 57+e 33b+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x5447C50F0B5FABBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 2P 3-Base ply 4 (after 33+b 37+d 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x546F9C0235973FC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x547230DB4910BD49L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 57g+d)
        new OctiBotEngine.OpeningBookEntry(0x54725BC812DD7D06L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280)),
        // OFK ply 4 (after 22+b 36+e 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0x548597DDF2E82CCFL, 8,
            new OctiBotEngine.OpeningBookCandidate("32g+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("22+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3553)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x54BFAD8A0F4CA753L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // OFK ply 3 (after 22+b 26+d 22b+a)
        new OctiBotEngine.OpeningBookEntry(0x54E48BB7917BCC4FL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -3323),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("56f-45", -3395)),
        // 4P ply 3 (after 33+b 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x54F076DA28CC38D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 4 (after 33+d 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x552B0B921BA1B546L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 4 (after 32+b 36+e 32b+e 46+a)
        new OctiBotEngine.OpeningBookEntry(0x554641A8E279D156L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32be-43", 0),
            new OctiBotEngine.OpeningBookCandidate("52+h", 0)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x55571159287C3F3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 3 (after 32+b 46+e 52+g)
        new OctiBotEngine.OpeningBookEntry(0x557E3DF6606D5235L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4103),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4103),
            new OctiBotEngine.OpeningBookCandidate("36e+d", -4103)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x559592EDAF6334D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
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
        // 4P ply 4 (after 33+c 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x55B85E25E3C34E2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 2P 3-Base ply 4 (after 33+h 37+e 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x55C0794B57077140L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62)),
        // 4P ply 4 (after 33+c 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x55D45D41FBD5209DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 3 (after 33+b 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x5627E025BB1B4027L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x562B6AADFD449265L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338),
            new OctiBotEngine.OpeningBookCandidate("33+b", 338)),
        // 4P ply 3 (after 33+h 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x563601F645B4A2F0L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
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
        // 4P ply 4 (after 33+c 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x568C675A1FAC5D77L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 3 (after 53+a 37+e 33+h)
        new OctiBotEngine.OpeningBookEntry(0x56A492B5207D48D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024)),
        // 4P ply 3 (after 33+g 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x56D819FA20015CBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
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
        // 2P 3-Base ply 4 (after 33+a 77+e 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x5777B4A278E60CDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x578B94F5F323B40BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x57A946D87EC0DF01L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
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
        // 4P ply 4 (after 33+b 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x580E2CB6C44DE29DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 3 (after 33+b 57+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x582154B13ED9E262L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2702),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2702),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2702)),
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
        // 4P ply 4 (after 33+a 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5876B17765C0F006L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
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
        // 4P ply 3 (after 33+h 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x58CDA51BF127734DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 3 (after 33+c 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x592A072D1754FAD2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+g 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5957B8F3ED9263D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 1-Base ply 4 (after 53+b 37+d 53b-64 57+e)
        new OctiBotEngine.OpeningBookEntry(0x596E518934C94AB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x598A9301FB6B1443L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x599E0DA442F0686AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x59A8412C76887F49L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
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
        // 4P ply 4 (after 33+a 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x59E69065E37D7AF1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 3 (after 22+b 46+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0x5A14F813248BBE62L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3690),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3690),
            new OctiBotEngine.OpeningBookCandidate("36d-45", -4022)),
        // 4P ply 3 (after 33+h 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x5A1A33E462F00BB8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
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
        // 4P ply 4 (after 33+a 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5AA12788F61788F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // OFK ply 4 (after 22+b 26+d 22b+a 26d-35)
        new OctiBotEngine.OpeningBookEntry(0x5B142BBB1512243AL, 8,
            new OctiBotEngine.OpeningBookCandidate("22ab+h", 343),
            new OctiBotEngine.OpeningBookCandidate("22ab-23", -23),
            new OctiBotEngine.OpeningBookCandidate("22ab-33", -820)),
        // 4P ply 4 (after 33+h 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5B3479EC379E9CFAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5B499B5BD127109FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 3-Base ply 3 (after 33+b 77+f 53+h)
        new OctiBotEngine.OpeningBookEntry(0x5B568AFF93F16AB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2402)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5B802E0C7E451B25L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
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
        // 2P 3-Base ply 4 (after 53+a 57+d 73+g 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x5BD52BC8C025DB3BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73g+h", -462)),
        // 2P 3-Base ply 4 (after 33+h 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5BF2737299242BE6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -120)),
        // 4P ply 3 (after 33+d 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x5BF3774B90A495C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x5BFD91D284838227L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P_3B_ED ply 1 (after 33+a)
        new OctiBotEngine.OpeningBookEntry(0x5C13029A45CF7655L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2632),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2632),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2632)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5C3B9668CB5983C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 4 (after 53+a 37+e 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5C44ECFD2843FB01L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+e 73+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5C5755607E7B7BE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x5C57950CD34FED70L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // OFK ply 4 (after 32+b 56+f 32b+a 46+e)
        new OctiBotEngine.OpeningBookEntry(0x5C753B2BD92D4D52L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+c", -3367)),
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
        // 4P ply 3 (after 33+a 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x5CECA4D0275D9DA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 4P ply 2 (after 33+g 37+c)
        new OctiBotEngine.OpeningBookEntry(0x5D4BEE2EAF5E76A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 2P 3-Base ply 4 (after 33+b 77+f 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x5D539773D27FF79BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+b", -120)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5D62022DE286028FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+h 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5D73E3FE1C29E058L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5D97356D84851859L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+g 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5D9DFBF2799C1E16L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 3 (after 33+a 77+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0x5DA327462642EC03L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2144),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2396)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5DB0128A55C9977DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
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
        // 4P ply 3 (after 33+a 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x5E12F35171E17D97L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 4 (after 33+b 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5E49B6A4EFFA9E3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 3 (after 33+d 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x5E6EE99A423CF7EEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x5E91BF49A2486235L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5EA9E73DE506FEDCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 3 (after 53+c 57+d 73+a)
        new OctiBotEngine.OpeningBookEntry(0x5EDF9190232FE926L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2830),
            new OctiBotEngine.OpeningBookCandidate("57f+e", -2840),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2950)),
        // 4P ply 3 (after 33+c 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5EFDBC2DBA5E0C87L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5F40A392175260ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5F4E450B03757743L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5F63AC732F20FE28L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
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
        // 4P ply 2 (after 33+g 37+g)
        new OctiBotEngine.OpeningBookEntry(0x5F9C78D13C890E50L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5FA10A77C8CA0653L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 3 (after 53+a 57+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x5FB09F0F8F1CE16CL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x5FB594D271517A7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // OFK ply 4 (after 22+b 36+d 32+a 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x5FCA047EE4ACD819L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+a", -1127),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1127)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x5FCD0913D0DC68E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 2 (after 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0x5FE7238F737F550AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53+a", -306),
            new OctiBotEngine.OpeningBookCandidate("53+h", -342)),
        // 4P ply 3 (after 33+b 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x5FFA4F8256D1E5FBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 3 (after 33+a 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x603757AD00E4E98BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x60625BCDF1B6948EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x606895520CAF92C1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 1-Base ply 3 (after 53+b 77+e 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x6069F8C2344D639AL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2480),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2480)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x606C12589EFF0A23L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 3 (after 53+c 37+d 53c+b)
        new OctiBotEngine.OpeningBookEntry(0x607A9F2786E5E183L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2290)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x6081478ADA9D77E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+h 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x608C43C194036AC0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 2 (after 33+d 37+d)
        new OctiBotEngine.OpeningBookEntry(0x60BE808EDA6DFA72L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // OFK ply 4 (after 22+b 56+f 42+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x60D228AD3AC7B019L, 8,
            new OctiBotEngine.OpeningBookCandidate("42a+h", -1615),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2348),
            new OctiBotEngine.OpeningBookCandidate("32+c", -2350)),
        // 4P ply 3 (after 33+h 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0x613FBAE72D281104L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 33+b 77+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0x61410BEB2E15773BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk2() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // 4P ply 4 (after 33+c 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x616BE1F77270E35FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+h 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x617061B70AA88E85L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 3 (after 53+a 57+e 53a+b)
        new OctiBotEngine.OpeningBookEntry(0x61902670F7A3C144L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3350)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6194B72492047684L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
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
        // 4P ply 4 (after 33+d 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x61F8B4408A121836L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
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
        // 4P ply 4 (after 33+c 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x62B1E5348B5FFDDBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x62BF03AD9F78EA34L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
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
        // 2P 3-Base ply 3 (after 33+b 77+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x6369DF1F63D15F69L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2282),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2282),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2402)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x639207B23FA9C903L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6394C4B6EA337FADL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x63A7F748997FF670L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x63B6169B67D014A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // OFK ply 4 (after 32+b 46+e 32b+c 36+e)
        new OctiBotEngine.OpeningBookEntry(0x6477F153A5376160L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("42gh-33", -2108)),
        // 4P ply 4 (after 33+d 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x648B172D5DCA77CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x64C3781C368E795AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+b 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x64DFC6811909FF47L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
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
        // 4P ply 4 (after 33+a 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x65891148EDEA7A9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x658C6E3EAADEE460L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x65F18C894C676805L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 3 (after 53+b 57+f 53b-64)
        new OctiBotEngine.OpeningBookEntry(0x65FF60ADC2D458AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57d-66", -2480),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2480)),
        // 4P ply 3 (after 33+b 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x6608507E8ADE87B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 1-Base ply 4 (after 53+b 57+d 53b-64 77+e)
        new OctiBotEngine.OpeningBookEntry(0x660B135F658B42C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -340),
            new OctiBotEngine.OpeningBookCandidate("44h+a", -340),
            new OctiBotEngine.OpeningBookCandidate("73+a", -390)),
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
        // 4P ply 4 (after 33+d 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x667540AC0B7697F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
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
        // 4P ply 4 (after 33+h 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x66CBD9D3BFB41662L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 37+d 33b-44 57+f)
        new OctiBotEngine.OpeningBookEntry(0x66D363FB47DCE5BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33", -870),
            new OctiBotEngine.OpeningBookCandidate("44b+a", -3160),
            new OctiBotEngine.OpeningBookCandidate("53+a", -3310)),
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
        // OFK ply 4 (after 32+b 36+d 52+g 36d+f)
        new OctiBotEngine.OpeningBookEntry(0x6722EBA231C876C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("42h+f", -2628),
            new OctiBotEngine.OpeningBookCandidate("32+g", -2628),
            new OctiBotEngine.OpeningBookCandidate("42h+a", -2628)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x672C7BE559C79FFDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 4 (after 32+b 26+d 32b+e 36+e)
        new OctiBotEngine.OpeningBookEntry(0x673B9A4AAB950FF5L, 8,
            new OctiBotEngine.OpeningBookCandidate("32be-43", -1068),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1068),
            new OctiBotEngine.OpeningBookCandidate("32be+h", -2105)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x675BF8C139099C95L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 4P ply 4 (after 33+a 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x675E87B77E3D026BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x676DB4490D718BB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 3-Base ply 4 (after 33+h 57+d 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x6793ADA5ED45D7FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -378)),
        // 2P 3-Base ply 4 (after 33+h 37+e 53+h 37e+d)
        new OctiBotEngine.OpeningBookEntry(0x67BBAB6EA5CCB618L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 4P ply 4 (after 33+d 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x67BF2E52A1A56494L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P 3-Base ply 4 (after 33+b 57+e 53+h 57e+d)
        new OctiBotEngine.OpeningBookEntry(0x67C7BBDB65EC78AFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+d 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x67D32D36B9B30A26L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
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
        // 4P ply 4 (after 33+a 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x68A8B166A056B5A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x68B04A4B82320A56L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120)),
        // 4P ply 3 (after 33+h 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0x68E21540C0E2B4D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 3 (after 33+b 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x68F3F4933E4D560FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 2P 1-Base ply 3 (after 53+c 77+f 53c+h)
        new OctiBotEngine.OpeningBookEntry(0x69469456610D29DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2200),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2290)),
        // 4P ply 4 (after 33+h 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x694E619088DA40F4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 3 (after 33+c 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x69F4073CD2C2BF73L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0x69FAE1A5C6E5A89CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 3 (after 33+h 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0x6A3583BF5335CC2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6A49B3241640947BL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 62),
            new OctiBotEngine.OpeningBookCandidate("73+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73+b", 0)),
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
        // 4P ply 4 (after 33+h 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6A7A58EF74B553ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // 2P 1-Base ply 4 (after 53+c 57+e 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x6AEE7B58CAFC2F5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400)),
        // 4P ply 3 (after 33+d 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0x6B2D775A5532D069L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6B430F60CB36DA24L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 338)),
        // OFK ply 4 (after 22+b 46+e 32+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x6B4F8B5A9E522E8EL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1079),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 4P ply 4 (after 33+b 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6B506788D49AC029L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6B5E2E1DBBD35E84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6B72B5A55979AB23L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
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
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
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
        // 4P ply 3 (after 33+a 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x6C1B65BF27A040C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6C51C199471441BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+c 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x6C6999ED005ADD55L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
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
        // 4P ply 3 (after 33+d 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0x6C96CF3EE02E488EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
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
        // 4P ply 3 (after 33+d 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6CFACC5AF838263CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
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
        // 4P ply 4 (after 33+a 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6D352FB772CED781L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6D47D3E555344A17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140)),
        // 4P ply 4 (after 33+d 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6D49357C41135DF8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6D4DB276D343C51AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x6D85AA473C6F01BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 3 (after 33+h 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0x6D8E3BDBE62954CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 4 (after 33+g 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x6EC3B0CC694B3CA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+a 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x6ECCF340B4773836L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 4P ply 4 (after 33+c 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6F90451AC6E332E2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+b 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x6F9A24894094BDEFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x6FB762FD17AFBDC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 4P ply 4 (after 33+d 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x703E25A302C23372L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 2P 3-Base ply 3 (after 53+a 37+f 33+b)
        new OctiBotEngine.OpeningBookEntry(0x707A59943E9E32B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2206),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2206),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2206)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x70B38F22C54C3B3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x70C17370E2B6A6A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 4 (after 33+b 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x70CB12E364C129A4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+h 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x70E2A744D9E25460L, 8,
            new OctiBotEngine.OpeningBookCandidate("73", 550),
            new OctiBotEngine.OpeningBookCandidate("53", 460),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x70F6A42DEDEB2F3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+h 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7126D14604C52F36L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 2P 1-Base ply 3 (after 33+b 57+c 53+h)
        new OctiBotEngine.OpeningBookEntry(0x714FE5D39124A16EL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -3630),
            new OctiBotEngine.OpeningBookCandidate("57c+d", -3630),
            new OctiBotEngine.OpeningBookCandidate("37+d", -4340)),
        // 4P ply 3 (after 33+a 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0x719DC52A9022AC7DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
        // OFK ply 4 (after 22+b 46+e 42+e 36+a)
        new OctiBotEngine.OpeningBookEntry(0x71A147F6A2C4AD0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("52h+g", -497)),
        // 2P 1-Base ply 3 (after 53+b 37+d 33+h)
        new OctiBotEngine.OpeningBookEntry(0x71DC7A81DE647897L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2360),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2360),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2480)),
        // 4P ply 3 (after 33+c 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x71EF3978B7D831EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 1-Base ply 4 (after 53+b 37+d 33+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0x72000FB831B20318L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x720D65CF09B9B386L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7216E58F7161DE5CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x721C841CF7165151L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 1-Base ply 4 (after 33+b 57+e 53+b 57e+f)
        new OctiBotEngine.OpeningBookEntry(0x725419C6B91BAC4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
        // 4P ply 4 (after 33+h 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x726166AB11AFDD34L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // OFK ply 4 (after 32+b 46+e 32b+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0x72C49F000D10778CL, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("42+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("32bg+h", -2108)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x72DFFFD4A56D5CA4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 4 (after 53+b 57+c 53b+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0x72E91606E5BA9FC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+h", 400)),
        // 4P ply 4 (after 33+d 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x72E9B35C91154B87L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7315912A0FBEAFC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // OFK ply 3 (after 32+b 36+e 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x731FF27C74F7B6EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4725),
            new OctiBotEngine.OpeningBookCandidate("26+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("26+a", -6783)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x73451059DEC9D01CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+a 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0x734A53D503F5D488L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
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
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x73C6D5809E52885AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x73F147B9971257C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+d 33+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x740860D80F7BA9F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7426FCB93A18A196L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x744AFFDD220ECF24L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
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
        // 4P ply 3 (after 33+c 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x7472A7A9654053CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0x748DF17A8534C616L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 2P 3-Base ply 3 (after 33+h 57+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x74B44850FBD3E5B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x74C22A2AA2B45997L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 2P 3-Base ply 2 (after 53+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x74E3F4D007B051D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", -306)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x752E11F317D45919L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // OFK ply 4 (after 22+b 26+d 22b-33 46+e)
        new OctiBotEngine.OpeningBookEntry(0x753A4FD9CC48666EL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -3045),
            new OctiBotEngine.OpeningBookCandidate("43h+b", -3045),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3662)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x75520B382409D360L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
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
        // 4P ply 3 (after 33+b 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0x7575540689CFBAB1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+h 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0x7595059F8333DA52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x75E2D98F7E3D8934L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33+h", -900)),
        // 4P ply 4 (after 33+d 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7615BCD531632162L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
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
        // 2P 1-Base ply 4 (after 53+c 77+f 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x76D00419B6829A39L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -520),
            new OctiBotEngine.OpeningBookCandidate("73+a", -520),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -520)),
        // 4P ply 4 (after 33+b 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x76D6C71D63182C97L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x76D88E880C51B23AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 3 (after 33+b 57+c 53+b)
        new OctiBotEngine.OpeningBookEntry(0x76E7BAC8986CC94EL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3630),
            new OctiBotEngine.OpeningBookCandidate("57g+f", -3630),
            new OctiBotEngine.OpeningBookCandidate("77+f", -4330)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x76F41530EEFB479DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
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
        // 4P ply 3 (after 33+h 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0x776B521ED58F3A6CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 22+b 46+e 22b+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x77F118DCB24BE668L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107),
            new OctiBotEngine.OpeningBookCandidate("42+b", -3747)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x780FB1DD5A689620L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // OFK ply 4 (after 22+b 46+e 42+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x782127A1E84581E7L, 8,
            new OctiBotEngine.OpeningBookCandidate("42c+h", -2108),
            new OctiBotEngine.OpeningBookCandidate("32+a", -2108),
            new OctiBotEngine.OpeningBookCandidate("22b+e", -3500)),
        // 4P ply 3 (after 33+d 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0x785073225623ED6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x78772C1CFBE584BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
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
        // 4P ply 4 (after 33+h 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x78E4F317181C0502L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 3 (after 33+h 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0x792484A348329F9CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 3 (after 33+h 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0x794887C75024F12EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 3 (after 33+b 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0x79596614AE8B13F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1516),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1636)),
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
        // 4P ply 4 (after 33+d 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x79E38A04EF0896AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // OFK ply 4 (after 32+a 36+d 42+e 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x7A2E86FE43E77808L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+h", -3107),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // 4P ply 4 (after 33+h 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7A3365E88BCB7DF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
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
        // 4P ply 4 (after 33+c 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7AAADB70EE457343L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 4 (after 33+a 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7AD82722C9BFEED5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
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
        // 2P 1-Base ply 4 (after 53+a 37+d 33+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x7B8BC8F406DD3B52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120)),
        // 4P ply 3 (after 33+b 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0x7B8EF0EB3D5C6B0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P_3B_ED ply 1 (after 53+b)
        new OctiBotEngine.OpeningBookEntry(0x7C106036373FD658L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2752),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2752),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2808)),
        // 4P ply 4 (after 33+h 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7C2EB0168C1278C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 3 (after 32+b 36+e 32b+a)
        new OctiBotEngine.OpeningBookEntry(0x7C55F0CEDB73E521L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4706),
            new OctiBotEngine.OpeningBookCandidate("36e+d", -4706)),
        // 4P ply 4 (after 33+d 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7C90296938D0F954L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+a 57d+e)
        new OctiBotEngine.OpeningBookEntry(0x7CA8BB3410B57D94L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -520)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7CED4162C5F20FE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // OFK ply 3 (after 32+a 36+d 42+c)
        new OctiBotEngine.OpeningBookEntry(0x7CEFDE909A39389CL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("26+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("46f+a", -3463)),
        // 4P ply 3 (after 33+d 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7D505EDD68FE63CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1256)),
        // 2P 3-Base ply 2 (after 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7D67616678F4DB93L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0)),
        // 4P ply 3 (after 33+a 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0x7DB1F738B7660535L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1496),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1616)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7DE4FB5846347830L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x7DEAB2CD297DE69DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 4 (after 33+b 57+e 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x7DF249E00B19596CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120)),
        // 4P ply 4 (after 33+a 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x7E1264235DB19313L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 3 (after 33+b 77+f 53+b)
        new OctiBotEngine.OpeningBookEntry(0x7E1AD175B25571ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7E30B60ED052F819L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
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
        // 4P ply 4 (after 33+h 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7ED0E797DAAE98FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // OFK ply 4 (after 32+b 36+d 32b+a 36d-45)
        new OctiBotEngine.OpeningBookEntry(0x7EDB9F4B811BF989L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("52+g", -3367)),
        // 2P 3-Base ply 4 (after 53+b 57+c 33+b 37+f)
        new OctiBotEngine.OpeningBookEntry(0x7F330A788F96E179L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // 4P ply 4 (after 33+c 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7F3745A13CDD1165L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+d 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0x7F87C822FB291B3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -996),
            new OctiBotEngine.OpeningBookCandidate("73+e", -996),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1116)),
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
        // 4P ply 4 (after 33+d 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x7FC81372DCA984BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8045F92A677E9D64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 4P ply 4 (after 33+d 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8049069C3963DA43L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260)),
        // 2P 1-Base ply 4 (after 53+b 77+f 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0x805F841422052419L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -610),
            new OctiBotEngine.OpeningBookCandidate("33", -920)),
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
        // 4P ply 4 (after 33+c 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x80B6504FD9174F98L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+a 77+e)
        new OctiBotEngine.OpeningBookEntry(0x812AF7B5CB4D952EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1010)),
        // 4P ply 4 (after 33+h 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8151F2793F64C607L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
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
        // 4P ply 4 (after 33+a 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x819371CDB87BCDEEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+b 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x81B1A3E03598A6E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 3 (after 33+b 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x821230FBDF4F30C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+f 77+a)
        new OctiBotEngine.OpeningBookEntry(0x821C796EB006AE6FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 3-Base ply 3 (after 53+b 57+c 73+b)
        new OctiBotEngine.OpeningBookEntry(0x821D68CF3DD43388L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2424)),
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
        // 4P ply 4 (after 33+d 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x83113C87DD1AA7A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
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
        // 4P ply 4 (after 33+h 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0x83AFA5F869D82639L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 33+a 37+d 73+h 37d+e)
        new OctiBotEngine.OpeningBookEntry(0x83D1A2E1E439BF6AL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8440369923BD49FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // 4P ply 4 (after 33+g 37+c 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x84783B48A42723FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
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
        // 4P ply 4 (after 33+c 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x852BCE9E0B8F2DBEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x85474956D5BA0E52L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -650),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -1150)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8556A695F6ADDB0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // OFK ply 2 (after 32+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0x8557CC8B298421EEL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -461),
            new OctiBotEngine.OpeningBookCandidate("52+h", -461),
            new OctiBotEngine.OpeningBookCandidate("52+a", -620)),
        // 4P ply 4 (after 33+a 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x855932CC2C75B028L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // OFK ply 4 (after 32+b 46+e 32b+c 36+d)
        new OctiBotEngine.OpeningBookEntry(0x8586E8EC1A07C343L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -3398),
            new OctiBotEngine.OpeningBookCandidate("32+b", -3398),
            new OctiBotEngine.OpeningBookCandidate("22+b", -3799)),
        // 4P ply 4 (after 33+h 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x85B270066E01230AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+h 57+e)
        new OctiBotEngine.OpeningBookEntry(0x85E0812A0906B572L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
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
        // 4P ply 3 (after 33+d 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0x861B0832193A40F1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // OFK ply 2 (after 32+b 26+d)
        new OctiBotEngine.OpeningBookEntry(0x86428795151724B2L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("32b-43", 0)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x869E9BC7B76D043EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0)),
        // 4P ply 4 (after 33+g 37+g 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x86AFADB737F05B0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
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
        // 4P ply 4 (after 33+h 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8765E6F9FDD65BFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 4P ply 3 (after 33+g 37+e 77+g)
        new OctiBotEngine.OpeningBookEntry(0x8781E7BF629ECC49L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+a 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x878EA433BFA2C8DDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 4 (after 53+b 37+c 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8793B28CD17C10F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 338),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 338),
            new OctiBotEngine.OpeningBookCandidate("73+b", 338)),
        // 4P ply 2 (after 33+h 37+f)
        new OctiBotEngine.OpeningBookEntry(0x879A325BEE7FAB87L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 33+a 77+e 53+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x87A72F1D28F11748L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
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
        // 4P_ED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0x88262D8DCE9FCB7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+c", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+d 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0x8854DE8F8487E501L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
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
        // 4P ply 4 (after 33+h 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8893D02823BDEC33L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
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
        // 4P ply 4 (after 33+b 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8957D2F386D2726AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x89599B66E99BECC7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P_ED ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0x895F828367C7141AL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+c", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1577)),
        // 2P 1-Base ply 4 (after 53+b 77+f 33+a 57+d)
        new OctiBotEngine.OpeningBookEntry(0x89638F65C5EDEC41L, 8,
            new OctiBotEngine.OpeningBookCandidate("53b+a", -470),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -470),
            new OctiBotEngine.OpeningBookCandidate("73+h", -620)),
        // 4P ply 4 (after 33+a 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x897500DE0B311960L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 1 (after 33+h)
        new OctiBotEngine.OpeningBookEntry(0x897DEC3D1DE82232L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
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
        // 4P ply 4 (after 33+d 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8994A93BD4A97F9FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 3-Base ply 3 (after 53+b 37+c 33+h)
        new OctiBotEngine.OpeningBookEntry(0x8A28D6F24AFDD0F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2424),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2424)),
        // 4P ply 2 (after 33+d 37+b)
        new OctiBotEngine.OpeningBookEntry(0x8A5F44E63B6F9CBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8A6D87A975010C0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+g 37+b 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8A839FA510B4F243L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 1-Base ply 4 (after 53+c 57+d 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0x8ABA5DF4A73AE62DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", -520),
            new OctiBotEngine.OpeningBookCandidate("33+b", -520),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -860)),
        // 4P ply 3 (after 33+a 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x8AD693C5E1E68F46L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+d 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8B433FC4477E076AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 2P 3-Base ply 4 (after 33+h 37+e 33h+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8B6518E861E3EA58L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+b", -62),
            new OctiBotEngine.OpeningBookCandidate("33+h", -62)),
        // 2P 3-Base ply 4 (after 33+a 37+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x8B6A4A1229C85853L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", -138)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8B75734C73061049L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // OFK ply 4 (after 32+a 56+f 42+c 46+e)
        new OctiBotEngine.OpeningBookEntry(0x8B7CA7ED21E839ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("42c+h", -1287),
            new OctiBotEngine.OpeningBookCandidate("42c+a", -2107),
            new OctiBotEngine.OpeningBookCandidate("22+b", -2108)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x8B8A259F93728592L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 33+b 77+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8BA80DFF5C6DAAF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53+b", -120),
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120)),
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
        // 4P ply 3 (after 33+h 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0x8C2004F1A44B1957L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 4 (after 33+d 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8C9484C4EA74F13FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 2P 3-Base ply 3 (after 33+b 57+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0x8CC0ED117C977E60L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2702),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2702),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2702)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8CC405B73B038EE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+h 57c+d)
        new OctiBotEngine.OpeningBookEntry(0x8D5481A149BAD196L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0x8D5EEA3A40A70259L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 2P 3-Base ply 3 (after 53+a 37+e 73+h)
        new OctiBotEngine.OpeningBookEntry(0x8D8380E10E02DDAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086),
            new OctiBotEngine.OpeningBookCandidate("37e+d", -2086)),
        // 4P ply 4 (after 33+h 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8D8C7021EC73ED7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 4 (after 33+b 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8D9D91F212DC0FACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0x8DD18CEB51A9C8CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0)),
        // 4P ply 4 (after 33+h 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8DE07345F46583C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 2 (after 32+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x8DF5C7214C11AE7AL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -640),
            new OctiBotEngine.OpeningBookCandidate("22+b", -640),
            new OctiBotEngine.OpeningBookCandidate("42a+h", -640)),
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
        // 2P 3-Base ply 3 (after 53+a 37+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0x8EB3F14C7DBB7ACBL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2086),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2086)),
        // 4P ply 3 (after 33+h 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0x8EF7920E379C61A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 4 (after 33+b 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x8F4A070D810B7759L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 3-Base ply 4 (after 33+a 37+e 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x8F60D75B25203EC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33h+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62)),
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
        // 4P ply 4 (after 33+b 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x901B3167A55EE312L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 3 (after 33+d 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0x904FE0CBE19D6B99L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -856),
            new OctiBotEngine.OpeningBookCandidate("73+e", -856),
            new OctiBotEngine.OpeningBookCandidate("73+g", -976)),
        // 2P 1-Base ply 3 (after 53+a 57+c 73+a)
        new OctiBotEngine.OpeningBookEntry(0x906C1CE5AEAC6D46L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3000),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3000),
            new OctiBotEngine.OpeningBookCandidate("57c+f", -3020)),
        // 4P ply 4 (after 33+h 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9088EE6C46A762ABL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 3-Base ply 4 (after 53+a 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0x90A28281802DE648L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0)),
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
        // 4P ply 4 (after 33+g 37+e 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9142A5228C81625FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 3-Base ply 3 (after 33+a 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0x9171A496D8B6A245L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024)),
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
        // 2P 1-Base ply 4 (after 33+b 77+f 73+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x925B7BEE145F7865L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53+h", -610)),
        // 4P ply 3 (after 33+d 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0x92B1B74AB7218BA7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+b 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x92CCA79836899BE7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
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
        // 4P ply 3 (after 33+h 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0x93A943AFB12697E3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x9410810F1469C2C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", -138)),
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
        // 4P ply 4 (after 33+b 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0x94D1726631509ED4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x94DF3BF35E190079L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x94F3A04BBCB3F5DEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+c 37+e)
        new OctiBotEngine.OpeningBookEntry(0x94F9223305076FCCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400)),
        // 4P ply 4 (after 33+d 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x94FC3412666B9EFDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0x951D87BFAB2E7723L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -120),
            new OctiBotEngine.OpeningBookCandidate("73+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120)),
        // 2P 3-Base ply 4 (after 33+a 57+d 33a+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0x9597628D3E70EDA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
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
        // 4P ply 4 (after 33+h 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x95FB4D01917F0D51L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+c 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x960C850A24F0692CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 1-Base ply 4 (after 53+b 77+f 33+a 37+d)
        new OctiBotEngine.OpeningBookEntry(0x96194478F84C76D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 120),
            new OctiBotEngine.OpeningBookCandidate("53h-44", -390)),
        // 4P ply 4 (after 33+d 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0x962BA2EDF5BCE608L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // OFK ply 2 (after 22+b 36+e)
        new OctiBotEngine.OpeningBookEntry(0x9662F3A6215C0DDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -620),
            new OctiBotEngine.OpeningBookCandidate("42+h", -620),
            new OctiBotEngine.OpeningBookCandidate("22b-33", -620)),
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
        // 4P ply 4 (after 33+h 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x96CF747E6D101E09L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 4 (after 33+g 37+b 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0x97053F30A7361EFDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 2P 3-Base ply 2 (after 53+b 57+g)
        new OctiBotEngine.OpeningBookEntry(0x9705AEEDA68BECF5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 400)),
        // OFK ply 1 (after 32+a)
        new OctiBotEngine.OpeningBookEntry(0x97220033605ECA2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -4143),
            new OctiBotEngine.OpeningBookCandidate("26+d", -4143),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4143)),
        // 4P ply 3 (after 33+a 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0x97503350566463F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 32+b 46+f 32b-43 26+d)
        new OctiBotEngine.OpeningBookEntry(0x9772EAE705E8D111L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("43b+h", -1020)),
        // OFK ply 4 (after 32+b 46+f 32b+a 26+d)
        new OctiBotEngine.OpeningBookEntry(0x9838E855AA6C82DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -1287),
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -1287),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1615)),
        // 4P ply 4 (after 33+d 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0x983E3BBC446F3A69L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+d 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x985238D85C7954DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x98938F40D8829BF7L, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+c 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0x98AD6E0BBC0DC100L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
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
        // 4P ply 4 (after 33+a 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x98DF92599BF75C96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0x98ECA1A7E8BBD54BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 2 (after 33+d 37+c)
        new OctiBotEngine.OpeningBookEntry(0x9922409E387EA1BCL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -120),
            new OctiBotEngine.OpeningBookCandidate("77+a", -120),
            new OctiBotEngine.OpeningBookCandidate("77+g", -240)),
        // 4P ply 4 (after 33+h 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x994ACC3D5A7E489FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 2 (after 33+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0x99B33FB18C84EEF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 4 (after 33+d 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0x99F45542EEBCC90FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+g 37+c 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x99FE9BDD13A5CF40L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 3 (after 33+g 37+e 77+a)
        new OctiBotEngine.OpeningBookEntry(0x9A07472AD51C20F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+a 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9A0804A608202463L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
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
        // 4P ply 4 (after 33+d 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9B0A02C3B8002931L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 2P 3-Base ply 4 (after 53+a 37+f 73+b 57+e)
        new OctiBotEngine.OpeningBookEntry(0x9B278514E5F025FFL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9B290D228072B7B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // OFK ply 4 (after 32+a 46+f 42+a 36+d)
        new OctiBotEngine.OpeningBookEntry(0x9B2DA34B96FF8786L, 8,
            new OctiBotEngine.OpeningBookCandidate("32a+b", -458),
            new OctiBotEngine.OpeningBookCandidate("42a+h", -458),
            new OctiBotEngine.OpeningBookCandidate("22+b", -640)),
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
        // 4P ply 2 (after 33+d 37+g)
        new OctiBotEngine.OpeningBookEntry(0x9BF5D661ABA9D949L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 20),
            new OctiBotEngine.OpeningBookCandidate("77+a", 20),
            new OctiBotEngine.OpeningBookCandidate("77+g", -100)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0x9C15D1580FF92150L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P_ED ply 1 (after 33+a)
        new OctiBotEngine.OpeningBookEntry(0x9C1D8128EEEC4720L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+c", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1577)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0x9C218CCE4CC6BF7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -460),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -460),
            new OctiBotEngine.OpeningBookCandidate("53b-64", -590)),
        // 4P ply 4 (after 33+h 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9C26E2A67CB5A88DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+b 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9C370375821A4A5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
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
        // 4P ply 4 (after 33+c 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9D30F0DA6E95A326L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9D3E16437AB2B4C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 3 (after 33+h 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0x9D8A9676348D5CA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // OFK ply 4 (after 22+b 46+f 22b+a 46f+e)
        new OctiBotEngine.OpeningBookEntry(0x9D9752474980498BL, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", -633),
            new OctiBotEngine.OpeningBookCandidate("42+a", -633),
            new OctiBotEngine.OpeningBookCandidate("32+e", -1363)),
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
        // 4P ply 4 (after 33+h 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0x9EC738D1DB1AC75BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // OFK ply 3 (after 32+b 56+f 32b-43)
        new OctiBotEngine.OpeningBookEntry(0x9EE160750F21FED6L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6435),
            new OctiBotEngine.OpeningBookCandidate("46+c", -6435)),
        // 4P ply 4 (after 33+h 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9EF17459EF62D078L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+a 57f-46)
        new OctiBotEngine.OpeningBookEntry(0x9F809C671D66ADBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 2 (after 33+h 37+e)
        new OctiBotEngine.OpeningBookEntry(0x9F810C1F8B65251FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 3 (after 33+b 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0x9F94906E68CDDC7CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+f 77+g)
        new OctiBotEngine.OpeningBookEntry(0x9F9AD9FB078442D1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+d 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0x9FE980BCE965CC3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+b 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA002CDB4A0E333A4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+b 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA034813C949B2487L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 2P 1-Base ply 4 (after 53+c 57+f 73+h 57f-46)
        new OctiBotEngine.OpeningBookEntry(0xA04D8C7D3CAC8FA9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("73+a", -400),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -400)),
        // 4P ply 3 (after 33+c 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0xA0B6C73DF547A11CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 4 (after 33+a 37+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xA0D6038AC5AB1E92L, 8,
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
        // 4P ply 4 (after 33+g 37+f 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA0E2B99DCAEAA0D5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 2 (after 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA144F97AF09CD1E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 3 (after 33+h 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0xA151650B13342883L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 3 (after 33+h 37+e 73+a)
        new OctiBotEngine.OpeningBookEntry(0xA1B2E9A04DB7AA8DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA1EA716787D3ABC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+b 77e+f)
        new OctiBotEngine.OpeningBookEntry(0xA1EFB7505A370DE4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h+b", 90),
            new OctiBotEngine.OpeningBookCandidate("33+b", -460),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -460)),
        // 4P ply 4 (after 33+g 37+d 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA21CEE1C9C5640EBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+c 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA24890BCA3FB4122L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // OFK ply 4 (after 22+b 26+d 22b-33 26d-35)
        new OctiBotEngine.OpeningBookEntry(0xA249F143931939FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("43h+b", 1530),
            new OctiBotEngine.OpeningBookCandidate("32+c", 1530),
            new OctiBotEngine.OpeningBookCandidate("32+b", 1136)),
        // 2P 3-Base ply 4 (after 33+h 37+f 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xA27D27FB65706CDAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
        // 4P ply 4 (after 33+b 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA2E317C3074C5C72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 4P ply 4 (after 33+h 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA2F2F610F9E3BEA5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 4P ply 1 (after 33+a)
        new OctiBotEngine.OpeningBookEntry(0xA2FA1AF3EF3A85F7L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
        // 4P ply 4 (after 33+a 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA31426E6D16F4BF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 3 (after 33+b 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA34F63134F74A85EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA3BA83862E93DD3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA3D680E23685B38DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // OFK ply 2 (after 32+b 56+f)
        new OctiBotEngine.OpeningBookEntry(0xA3FE4D54D9664919L, 8,
            new OctiBotEngine.OpeningBookCandidate("52+h", -620),
            new OctiBotEngine.OpeningBookCandidate("42+a", -620),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -620)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA4013BE29B8F45D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0xA4031EE49BACC862L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33+g", -120)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA40BF57D66964397L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA42CD29AB7DACCB3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 3-Base ply 4 (after 53+a 37+e 33+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xA464000451EF3A1EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 0)),
        // 4P ply 2 (after 33+g 37+d)
        new OctiBotEngine.OpeningBookEntry(0xA4D72E3E4D4D2D6BL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -280),
            new OctiBotEngine.OpeningBookCandidate("77+a", -280),
            new OctiBotEngine.OpeningBookCandidate("77+g", -400)),
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
        // 4P ply 3 (after 33+a 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA57064C0C54EC667L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA5911AF01D32CF2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
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
        // 4P ply 4 (after 33+g 37+b 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA5FD19940524A19DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+a 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA60B86EF1EA14ABEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA62954C2934221B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 2 (after 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA6407332568B4EEAL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a", 0)),
        // 4P ply 4 (after 33+a 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA667858B06B7240CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
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
        // 4P ply 4 (after 33+g 37+g 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA6D6AD1D08583D2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 1-Base ply 3 (after 53+c 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0xA6ECC4F40CBBA8DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -2080),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -2080),
            new OctiBotEngine.OpeningBookCandidate("77+e", -2080)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA6FB4465240DB446L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 2P 1-Base ply 3 (after 53+a 37+d 33+a)
        new OctiBotEngine.OpeningBookEntry(0xA7067DA37DFD8818L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2600),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2600),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2600)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xA76F2BBB8FC0B836L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120)),
        // 4P ply 4 (after 33+b 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA78F39582187BC60L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+a 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0xA7A7F23F5699BE92L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA80A811B16E9EAF6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xA82853369B0A81FCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xA836AFFDC8D8E4F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
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
        // 4P ply 3 (after 33+g 37+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xA89362EA6F18F125L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 1-Base ply 4 (after 33+b 57+d 53+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xA89FCC73AE737B85L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 0),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 0)),
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
        // 4P ply 3 (after 33+g 37+e 77+b)
        new OctiBotEngine.OpeningBookEntry(0xA8FF618E770E9F97L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+c 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xA90D72B4FA66038AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+b 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xA9138F23C830FA5CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -120),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33+a", -120)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xA9209BCCD6338AE1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // OFK ply 4 (after 22+b 26+d 22b+c 26d-35)
        new OctiBotEngine.OpeningBookEntry(0xA9777613794E1C57L, 8,
            new OctiBotEngine.OpeningBookCandidate("52gh+b", -1374),
            new OctiBotEngine.OpeningBookCandidate("52gh-43", -1494),
            new OctiBotEngine.OpeningBookCandidate("32+a", -1853)),
        // OFK ply 3 (after 32+b 26+d 32b+a)
        new OctiBotEngine.OpeningBookEntry(0xA9A811118D010EECL, 8,
            new OctiBotEngine.OpeningBookCandidate("36+d", -4483),
            new OctiBotEngine.OpeningBookCandidate("46+e", -4796),
            new OctiBotEngine.OpeningBookCandidate("46+g", -5033)),
        // 4P ply 2 (after 33+c 37+g)
        new OctiBotEngine.OpeningBookEntry(0xA9F2A616E9CFF3F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAA1487034AA96A2BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAA394BCB060910D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+c 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAA5548AF1E1F7E60L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAAAA1E7CFE6BEBBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+a 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAAD604B7CDB661C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 2 (after 33+c 37+c)
        new OctiBotEngine.OpeningBookEntry(0xAB2530E97A188B07L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+b 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAB3EB864EA86F9AEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 3-Base ply 4 (after 53+b 37+c 73+a 57+f)
        new OctiBotEngine.OpeningBookEntry(0xAB4F5E74B011C4E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xABDECC4D808F6ADFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400))
        };
    }

    private static OctiBotEngine.OpeningBookEntry[] chunk3() {
        return new OctiBotEngine.OpeningBookEntry[] {
        // 2P 1-Base ply 4 (after 53+b 77+e 33+a 77e+f)
        new OctiBotEngine.OpeningBookEntry(0xABE3CD8CCE6662D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+h", -900)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xABF32535ACDAE3B4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140)),
        // 4P ply 4 (after 33+h 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAC0952FD4D706F18L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 2 (after 33+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0xAC1DC4951CAD0C42L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 33+b 57+d 73+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xAC74906B24CBEA46L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xACE210370F04FC3AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 2P 1-Base ply 4 (after 53+a 57+d 33+h 57d-66)
        new OctiBotEngine.OpeningBookEntry(0xAD1640908CF32EC2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -1020),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -1020),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -1020)),
        // 4P ply 3 (after 33+b 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0xAD6CB6CACADF631CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+g 77+h)
        new OctiBotEngine.OpeningBookEntry(0xAD934F1551C57F85L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 32+b 36+d 42+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xADADB6EB3D6BFD28L, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", 640),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -3328),
            new OctiBotEngine.OpeningBookCandidate("52+e", -3463)),
        // 4P ply 4 (after 33+c 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xADEEF0CBAB03E687L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xAE3586C89CD384CFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // OFK ply 4 (after 32+b 46+f 52+h 46f-35)
        new OctiBotEngine.OpeningBookEntry(0xAE386AFDFD797C3CL, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+e", -1068),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1068)),
        // 4P ply 4 (after 33+b 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xAE4D1B093D5E9654L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+h 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAEDEC402DEA717EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+d 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAF3780AD2CF3899DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xAF39663438D49E72L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 3 (after 33+g 37+c 77+h)
        new OctiBotEngine.OpeningBookEntry(0xAF44D9EAC2120770L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xAFA9A54ED4C9918FL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+b", 62),
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 62)),
        // 4P ply 3 (after 33+b 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0xB032676B4C65955DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB0AB849A35948E8EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 3 (after 33+g 37+f 77+b)
        new OctiBotEngine.OpeningBookEntry(0xB0E45FCA1214110FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+c 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB1164CF09F7C8D12L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // 4P ply 4 (after 33+g 37+e 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB13BA588B3290479L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+h 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB18FF268FAF283A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 4 (after 33+c 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB24E76EB7B05F0F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+g 37+g 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB27C1265A643F67BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // 2P 1-Base ply 4 (after 53+b 77+e 53b-64 57+f)
        new OctiBotEngine.OpeningBookEntry(0xB28D5CEE378D62B6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 150),
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("64b+a", 0)),
        // 4P ply 4 (after 33+d 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB2B120389B716523L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260)),
        // 4P ply 4 (after 33+a 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB2CD3AF3A8ACEF5AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 3 (after 33+b 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0xB2E5F194DFB2EDA8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1396),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1516)),
        // 4P ply 4 (after 33+b 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB32586208F9C7736L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+b 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB3498544978A1984L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+h 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB35864976925FB53L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB36B57691A69728EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 4 (after 53+a 57+d 53a+b 57d+e)
        new OctiBotEngine.OpeningBookEntry(0xB374825CF463C068L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53ah-44", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0)),
        // 2P 3-Base ply 4 (after 33+a 57+d 53+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB3C8793FFAE04816L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -400),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -400)),
        // 4P ply 2 (after 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xB406FAD179B782DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 2P 3-Base ply 4 (after 33+a 57+d 73+a 77+f)
        new OctiBotEngine.OpeningBookEntry(0xB413E3A3669962C9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -138),
            new OctiBotEngine.OpeningBookCandidate("53+h", -138),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -138)),
        // 4P ply 3 (after 33+c 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB41C78455B5C6A4AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB4578B1395E2E46BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB4A872CC0EF8F8F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // OFK ply 4 (after 32+b 46+e 32b+g 56+f)
        new OctiBotEngine.OpeningBookEntry(0xB4C71F3991ACFACAL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // OFK ply 4 (after 32+b 36+d 32b+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xB526293E038860E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+b", -1900),
            new OctiBotEngine.OpeningBookCandidate("22+c", -3367)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB540CE1F29C8609EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+a 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB57682971DB077BDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 3 (after 33+h 37+d 53+h)
        new OctiBotEngine.OpeningBookEntry(0xB5770989EE85D6D6L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024)),
        // 2P 1-Base ply 3 (after 53+b 57+e 33+a)
        new OctiBotEngine.OpeningBookEntry(0xB5F2BE4133F0FE5BL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+d", -3090),
            new OctiBotEngine.OpeningBookCandidate("77+f", -3090),
            new OctiBotEngine.OpeningBookCandidate("57e+f", -3260)),
        // 4P ply 3 (after 33+a 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0xB60D60B8C65FFB64L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1496)),
        // 2P 3-Base ply 4 (after 53+a 37+f 33+a 37f+e)
        new OctiBotEngine.OpeningBookEntry(0xB61271807C002B3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 62),
            new OctiBotEngine.OpeningBookCandidate("33+b", 62),
            new OctiBotEngine.OpeningBookCandidate("73a+h", 62)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB656254D584418CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB6801DEC06359C9EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+g 37+d 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB6B65164324D8BBDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xB6C1B656CF4909ADL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+h", 0)),
        // 4P ply 4 (after 33+d 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB72CBEE949E90705L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 3-Base ply 4 (after 53+b 57+c 73+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xB751A32AD2333CA7L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 400),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 400)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB751D6E2B4CBF1B0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // OFK ply 3 (after 22+b 46+f 22b+c)
        new OctiBotEngine.OpeningBookEntry(0xB7771179FEE4C603L, 8,
            new OctiBotEngine.OpeningBookCandidate("56+f", -3283),
            new OctiBotEngine.OpeningBookCandidate("46+e", -3283),
            new OctiBotEngine.OpeningBookCandidate("36d+f", -4523)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB77C3F9A989E78DBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB7922796FD2B8695L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P_ED ply 1 (after 33+h)
        new OctiBotEngine.OpeningBookEntry(0xB79A77E61C3EE0E5L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1437),
            new OctiBotEngine.OpeningBookCandidate("37+c", -1437),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1437)),
        // 4P ply 4 (after 33+a 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB7A114688E670F48L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 4P ply 1 (after 33+b)
        new OctiBotEngine.OpeningBookEntry(0xB7B819586611D6CDL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1376),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1376)),
        // 4P ply 3 (after 33+a 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0xB82EB56143F43026L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 2 (after 33+c 37+b)
        new OctiBotEngine.OpeningBookEntry(0xB85834917909B604L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 120),
            new OctiBotEngine.OpeningBookCandidate("77+a", 120),
            new OctiBotEngine.OpeningBookCandidate("77+g", 0)),
        // 4P ply 4 (after 33+g 37+e 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB8F987D9AFF02E4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 1-Base ply 4 (after 53+a 57+e 33+a 37+e)
        new OctiBotEngine.OpeningBookEntry(0xB9170E6E806B149BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0)),
        // 4P ply 3 (after 33+g 37+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xB939F06DFFDEB4D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+c 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xB9444FB305182DD1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 4P ply 2 (after 33+b 37+f)
        new OctiBotEngine.OpeningBookEntry(0xB95FC73E95865F78L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 0),
            new OctiBotEngine.OpeningBookCandidate("77+a", 0),
            new OctiBotEngine.OpeningBookCandidate("77+g", -120)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB972033B31603AF2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // OFK ply 3 (after 32+b 26+d 42+a)
        new OctiBotEngine.OpeningBookEntry(0xB981856ED6771CB5L, 8,
            new OctiBotEngine.OpeningBookCandidate("36+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("46+f", -6570),
            new OctiBotEngine.OpeningBookCandidate("26d+a", -6783)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xB98D55E8D114AF29L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xB9A0139C862FAF00L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xBA3EE0E79AEB34C8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -610),
            new OctiBotEngine.OpeningBookCandidate("53ch-44", -610),
            new OctiBotEngine.OpeningBookCandidate("73+a", -610)),
        // 4P ply 3 (after 33+c 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0xBA53AEF8C6E1CFBAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
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
        // 4P ply 4 (after 33+a 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xBB0F18A2B475C56EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // OFK ply 3 (after 32+b 26+d 32b-43)
        new OctiBotEngine.OpeningBookEntry(0xBB5DAAB4C350937DL, 8,
            new OctiBotEngine.OpeningBookCandidate("46+e", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+d", -6570),
            new OctiBotEngine.OpeningBookCandidate("36+a", -6570)),
        // 4P ply 4 (after 33+b 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBB77856315F8D7F5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 4 (after 33+c 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBB93D94C96CF5524L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 4P ply 4 (after 33+g 37+f 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBBA1BDC24B8953A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+g 37+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xBBEE66926C09CC26L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
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
        // 4P ply 3 (after 33+g 37+b 77+h)
        new OctiBotEngine.OpeningBookEntry(0xBC39DD92C1033A73L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 4P ply 3 (after 33+h 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0xBCD7C59EA4B6C43DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 22+b 36+e 42+h 26+d)
        new OctiBotEngine.OpeningBookEntry(0xBCF50D481742EAD9L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+a", -179),
            new OctiBotEngine.OpeningBookCandidate("32b-43", -227),
            new OctiBotEngine.OpeningBookCandidate("52h-43", -620)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBD4882B09FC2B9CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+b 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBD6A509D1221D2C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // OFK ply 4 (after 32+b 56+f 32b+a 46+f)
        new OctiBotEngine.OpeningBookEntry(0xBD842294661DEF71L, 8,
            new OctiBotEngine.OpeningBookCandidate("32ab-43", -267),
            new OctiBotEngine.OpeningBookCandidate("42+a", -458),
            new OctiBotEngine.OpeningBookCandidate("42+h", -799)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xBD8A24B4782841B7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 2P 3-Base ply 2 (after 53+a 37+f)
        new OctiBotEngine.OpeningBookEntry(0xBDE81D4AC635C40BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 4 (after 33+a 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBE7CBBCF63ADAA94L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+c 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBE93F4B3A812DB84L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // OFK ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xBF20205531084330L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", 0),
            new OctiBotEngine.OpeningBookCandidate("42+a", 0),
            new OctiBotEngine.OpeningBookCandidate("22+b", 0)),
        // 4P ply 4 (after 33+c 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xBF599A4D02C128E2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260)),
        // 4P ply 4 (after 33+h 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBF7456854E61521BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 2P 1-Base ply 4 (after 53+c 57+f 53c+b 57f-46)
        new OctiBotEngine.OpeningBookEntry(0xBF83FECB6F960DB2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53bc-64", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
        // 4P ply 4 (after 33+b 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xBF94071C449D32F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xBFA39A6DB021360EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -62),
            new OctiBotEngine.OpeningBookCandidate("33a+b", -62)),
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
        // 4P ply 2 (after 33+a 37+b)
        new OctiBotEngine.OpeningBookEntry(0xC00E165F1AED7B35L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // 4P ply 4 (after 33+g 37+g 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC03364F9EEAE7336L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
        // OFK ply 4 (after 22+b 46+e 32+a 56+f)
        new OctiBotEngine.OpeningBookEntry(0xC05F5A08C6261CA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("42+h", -1020),
            new OctiBotEngine.OpeningBookCandidate("42+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 4P ply 3 (after 33+c 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0xC07897AF2010FD17L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0xC087C17CC06468CCL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 4 (after 33+b 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC0FAD1AE41CC788CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+a 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC1126D7D66FCE0E0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 4 (after 33+a 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC12421F55284F7C3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 1-Base ply 2 (after 53+c 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC156F2316BA61CA2L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33+a", -400)),
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
        // 4P ply 3 (after 33+a 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0xC2058C36A505028BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 33+h 57+e 53+b 37+d)
        new OctiBotEngine.OpeningBookEntry(0xC21A08267A9C9431L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -62)),
        // 4P ply 4 (after 33+b 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC22D4751D21B0079L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 140)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC2661893BABC5A86L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73+h", -120)),
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
        // 4P ply 4 (after 33+g 37+d 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC2D2BE8E49011CE0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280)),
        // 4P ply 4 (after 33+g 37+c 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC2E4F2067D790BC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 4 (after 33+d 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC32452672AB3FEEAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // OFK ply 4 (after 32+a 36+e 42+c 46+f)
        new OctiBotEngine.OpeningBookEntry(0xC33B618379E98EFFL, 8,
            new OctiBotEngine.OpeningBookCandidate("22+b", -3553),
            new OctiBotEngine.OpeningBookCandidate("42c+h", -3553),
            new OctiBotEngine.OpeningBookCandidate("52+a", -3553)),
        // 4P ply 4 (after 33+c 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC3593A6CD791085FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+g 37+e 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC374D314FBC48134L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P_SPED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xC3BB37EE45A91717L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a!", 640),
            new OctiBotEngine.OpeningBookCandidate("33+g!", 327),
            new OctiBotEngine.OpeningBookCandidate("33+e!", 103)),
        // 4P ply 4 (after 33+a 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC3C5FB82F52B9815L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+b 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC41953D110A99D81L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 3 (after 33+g 37+f 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC480B02069588652L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // OFK ply 4 (after 22+b 36+d 42+e 36d-45)
        new OctiBotEngine.OpeningBookEntry(0xC4965B5AF72693BBL, 8,
            new OctiBotEngine.OpeningBookCandidate("42e+h", -1068),
            new OctiBotEngine.OpeningBookCandidate("42e+b", -1660),
            new OctiBotEngine.OpeningBookCandidate("32+a", -3107)),
        // 4P ply 4 (after 33+g 37+c 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC4CF6B704ED819D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 2 (after 33+b 37+g)
        new OctiBotEngine.OpeningBookEntry(0xC4E6877303006DF9L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // 4P ply 4 (after 33+c 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC51EA07EFC2674FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+g 37+b 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC5334906D073FD96L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
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
        // OFK ply 4 (after 22+b 36+d 42+e 46+e)
        new OctiBotEngine.OpeningBookEntry(0xC5D28C2D1AB8A3B9L, 8,
            new OctiBotEngine.OpeningBookCandidate("32e+b", -1068),
            new OctiBotEngine.OpeningBookCandidate("32e+h", -1887),
            new OctiBotEngine.OpeningBookCandidate("42+a", -3107)),
        // 4P ply 4 (after 33+h 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC5DD510AB5C603D8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+g 37+g 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC618FD8FDD0F6126L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+c 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC62A9901004967A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 2 (after 33+b 37+c)
        new OctiBotEngine.OpeningBookEntry(0xC631118C90D7150CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 2P 3-Base ply 4 (after 53+b 57+g 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xC656162191BCAD3FL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 280),
            new OctiBotEngine.OpeningBookCandidate("53b+a", 280),
            new OctiBotEngine.OpeningBookCandidate("73+h", 280)),
        // 4P ply 4 (after 33+a 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC6C5D67DCBF616B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 120)),
        // 2P 3-Base ply 4 (after 53+a 57+e 33+b 37+e)
        new OctiBotEngine.OpeningBookEntry(0xC6D41B1D2EF87E0AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+b 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC6E7045046157DBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+a 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC70FB8836125E5D3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 4P ply 4 (after 33+b 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC72D6AAEECC68ED9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 4 (after 33+b 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xC74169CAF4D0E06BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 3 (after 33+g 37+e 77+h)
        new OctiBotEngine.OpeningBookEntry(0xC831311CA259C39CL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 2P 1-Base ply 2 (after 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xC8835A7C9D2079F2L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0)),
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
        // 4P ply 4 (after 33+b 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC938F3FF5D1552B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 4 (after 33+a 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xC9406E3EFC984023L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 4 (after 53+b 77+e 73+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xC96764FF91AA805DL, 8,
            new OctiBotEngine.OpeningBookCandidate("53b-64", -340),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -340),
            new OctiBotEngine.OpeningBookCandidate("33+a", -340)),
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
        // 4P ply 4 (after 33+g 37+f 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xC9EECB5E0364D6EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCA18542518E13DC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P 3-Base ply 4 (after 33+b 37+e 53+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xCA86B4E4B7CFA0E1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0)),
        // 4P ply 4 (after 33+c 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCA9B183DCB48226BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 2P 3-Base ply 2 (after 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xCB39CEE37BC74410L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+c 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCB3D75A7798DBFBFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+h 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCB7CBA0B2D3BABF4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 2P 1-Base ply 4 (after 53+c 57+c 53c+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xCB9175E888959DADL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -30),
            new OctiBotEngine.OpeningBookCandidate("73+a", -30),
            new OctiBotEngine.OpeningBookCandidate("33+a", -740)),
        // 4P ply 4 (after 33+d 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCBC2237499F92A64L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+b 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCBEF6500CEC22A4DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 4P ply 3 (after 33+a 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0xCC4A5A8B38B8A77BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 2 (after 33+c 37+e)
        new OctiBotEngine.OpeningBookEntry(0xCC50D81F1A534FEBL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 4P ply 4 (after 33+g 37+b 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCCF16B57CCAAD7A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+c 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCD16ECD14A2CADAFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+c 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCD20A0597E54BA8CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 3 (after 33+g 37+g 77+b)
        new OctiBotEngine.OpeningBookEntry(0xCD5D1F878492238EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // OFK ply 4 (after 22+b 46+e 32+a 36+a)
        new OctiBotEngine.OpeningBookEntry(0xCD7850705FA909A1L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+b", 544),
            new OctiBotEngine.OpeningBookCandidate("32+a", 187),
            new OctiBotEngine.OpeningBookCandidate("52h+g", 0)),
        // 4P ply 3 (after 33+b 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0xCDA2E6581F883F17L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
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
        // 4P ply 4 (after 33+d 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCDDFF68A9E202F57L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 2P_3B_SP ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xCE0042FEA3C2985CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a!", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a!", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 3 (after 33+c 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0xCE5B4276A5BB3655L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P 3-Base ply 4 (after 53+a 37+d 33+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xCE60A114AB222B38L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33+b", -62)),
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
        // 4P ply 4 (after 33+h 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCEE124DAFFA3C9D2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+a 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xCF07F42CD72F3C81L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+d 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCF0860750DF757A2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // OFK ply 3 (after 32+b 36+d 52+a)
        new OctiBotEngine.OpeningBookEntry(0xCF372E35DA53ABC6L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -3323),
            new OctiBotEngine.OpeningBookCandidate("36+e", -3323),
            new OctiBotEngine.OpeningBookCandidate("26+c", -3683)),
        // 4P ply 3 (after 33+b 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0xCF5CB1D94934DF29L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 4 (after 53+c 37+d 33+c 57+e)
        new OctiBotEngine.OpeningBookEntry(0xCF65EB470CBEB630L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -800),
            new OctiBotEngine.OpeningBookCandidate("53c+b", -800),
            new OctiBotEngine.OpeningBookCandidate("73+a", -800)),
        // 4P ply 3 (after 33+g 37+c 77+b)
        new OctiBotEngine.OpeningBookEntry(0xCF8A897817455B7BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+g 37+f 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xCFC5522830C5C4FAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xCFF736A6ED83C279L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140)),
        // 4P ply 3 (after 33+g 37+f 77+h)
        new OctiBotEngine.OpeningBookEntry(0xD02A0F58C7434D04L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -956),
            new OctiBotEngine.OpeningBookCandidate("73+e", -956),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1076)),
        // 4P ply 4 (after 33+d 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD059561F29A2C3E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 4 (after 33+c 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD07E71F8F8EE4CCDL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 2P 3-Base ply 3 (after 33+h 37+e 53+b)
        new OctiBotEngine.OpeningBookEntry(0xD132BF2924211474L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("77e+f", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2176)),
        // 4P ply 4 (after 33+a 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD15B507A9982CEBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -260)),
        // 2P 1-Base ply 4 (after 53+c 57+c 73+h 57c+f)
        new OctiBotEngine.OpeningBookEntry(0xD1752665B3BC3BB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("53c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 4 (after 33+g 37+b 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD199F67E7E6836C0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 2 (after 33+a 37+g)
        new OctiBotEngine.OpeningBookEntry(0xD1A484D88A2B3EC3L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // 4P ply 4 (after 33+g 37+e 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD1F5F51A667E5872L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+a 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD2036A617DFBB351L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 4 (after 33+b 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD24DBB28E80EB6E9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 2P 1-Base ply 4 (after 53+b 57+e 33+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xD250C28E0833A672L, 8,
            new OctiBotEngine.OpeningBookCandidate("53h-44", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+a 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD26F690565EDDDE3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 4P ply 4 (after 33+c 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD2802679AE52ACF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+d 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD28EC0E0BA75BB1CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 3 (after 33+g 37+d 77+d)
        new OctiBotEngine.OpeningBookEntry(0xD2D458D991FFAD3AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 4P ply 4 (after 33+h 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD367844F4821256CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 2 (after 33+a 37+c)
        new OctiBotEngine.OpeningBookEntry(0xD373122719FC4636L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 2P_1B_SP ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xD3734C947246B6D9L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a!", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+b 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD387D5D642DD458FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 260),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 120)),
        // 4P ply 4 (after 33+a 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD3A507FBCF3E2E85L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 2 (after 33+c 37+f)
        new OctiBotEngine.OpeningBookEntry(0xD44BE65B7F49C173L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -140),
            new OctiBotEngine.OpeningBookCandidate("77+a", -140),
            new OctiBotEngine.OpeningBookCandidate("77+g", -260)),
        // 2P 3-Base ply 4 (after 33+b 37+d 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xD44F2E1180ACB386L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+b", -62),
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53+h", -120)),
        // 4P ply 4 (after 33+b 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD4506ED6EFD7B3DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -120)),
        // 4P ply 4 (after 33+b 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD466225EDBAFA4F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+g 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD499DB8140B5B860L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 2P 3-Base ply 2 (after 53+a 57+e)
        new OctiBotEngine.OpeningBookEntry(0xD4B339E63E31F4B8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+b", 0)),
        // 4P ply 4 (after 33+c 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD4B432F96CE0310BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 20)),
        // 2P 3-Base ply 3 (after 33+h 77+e 53+h)
        new OctiBotEngine.OpeningBookEntry(0xD537709A60E2C0EAL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+f", -2024)),
        // 4P ply 2 (after 33+b 37+b)
        new OctiBotEngine.OpeningBookEntry(0xD54C15F493C6280FL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // OFK ply 3 (after 22+b 36+d 42+c)
        new OctiBotEngine.OpeningBookEntry(0xD5B1884372121E18L, 8,
            new OctiBotEngine.OpeningBookCandidate("26+d", -2962),
            new OctiBotEngine.OpeningBookCandidate("36+e", -2962),
            new OctiBotEngine.OpeningBookCandidate("46f-35", -2962)),
        // 4P ply 4 (after 33+a 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD5B8D205C8E72BB6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 0)),
        // 2P 3-Base ply 2 (after 53+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD5F054164CA45777L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P ply 4 (after 33+d 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD64483E12E7BC6DAL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 2P 1-Base ply 2 (after 33+b 77+f)
        new OctiBotEngine.OpeningBookEntry(0xD64AC089AA436A95L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0)),
        // 4P_ED ply 1 (after 33+d)
        new OctiBotEngine.OpeningBookEntry(0xD64CD391CF6EA0AAL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+c", -1437),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1437),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1437)),
        // 4P ply 4 (after 33+g 37+c 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD64E4D7ED362C095L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+c 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD663A406FF3749FEL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -120)),
        // 2P 1-Base ply 4 (after 53+c 77+f 73+h 57+f)
        new OctiBotEngine.OpeningBookEntry(0xD680C92F8F033F53L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+h", -730),
            new OctiBotEngine.OpeningBookCandidate("73+h", -730),
            new OctiBotEngine.OpeningBookCandidate("53g+b", -730)),
        // 4P ply 4 (after 33+b 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD687F8297C00CB2FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 20)),
        // 2P 1-Base ply 4 (after 53+a 57+d 73+h 57d+e)
        new OctiBotEngine.OpeningBookEntry(0xD6B6D77E14DE2DABL, 8,
            new OctiBotEngine.OpeningBookCandidate("53a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("73+a", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -120)),
        // OFK ply 4 (after 32+b 36+d 52+g 36d+e)
        new OctiBotEngine.OpeningBookEntry(0xD6EC2CEB5FD247FDL, 8,
            new OctiBotEngine.OpeningBookCandidate("42h-33", 162),
            new OctiBotEngine.OpeningBookCandidate("32+a", 162),
            new OctiBotEngine.OpeningBookCandidate("22c+b", -780)),
        // 4P ply 4 (after 33+h 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD6FA1A9E9AB9474AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+b 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0xD7478F9D2C2E51B1L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // OFK ply 4 (after 22+b 46+f 42+a 46f-35)
        new OctiBotEngine.OpeningBookEntry(0xD764A0D60917D788L, 8,
            new OctiBotEngine.OpeningBookCandidate("22b+a", -1584),
            new OctiBotEngine.OpeningBookCandidate("32+b", -1721),
            new OctiBotEngine.OpeningBookCandidate("52+h", -1721)),
        // 4P ply 4 (after 33+a 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xD76F44FA5B305343L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 280),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 140)),
        // 2P 3-Base ply 4 (after 53+a 57+d 73+g 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD791E299B28B230EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+b", -400)),
        // 4P ply 4 (after 33+g 37+b 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD7B26F084DC924D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+g 37+e 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD7DE6C6C55DF4A62L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+h 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD85BF19F0244EF66L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+a 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0xD8E0E5F396A36C2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 2P_3B_ED ply 1 (after 53+a)
        new OctiBotEngine.OpeningBookEntry(0xD8F2AE8E9AFA57BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2632),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2632),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2632)),
        // 4P ply 3 (after 33+b 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0xD9085920B193F441L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 1-Base ply 2 (after 33+b 57+c)
        new OctiBotEngine.OpeningBookEntry(0xD91FF42F8932BA17L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53+h", 400),
            new OctiBotEngine.OpeningBookCandidate("73+a", 400)),
        // 2P 3-Base ply 3 (after 33+b 37+d 73+b)
        new OctiBotEngine.OpeningBookEntry(0xD98957DAE9D0F34DL, 8,
            new OctiBotEngine.OpeningBookCandidate("57+d", -2024),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2024)),
        // 4P ply 4 (after 33+g 37+d 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xD991BAD1C862EF92L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -400)),
        // 4P ply 4 (after 33+c 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xD9BC53A9E43766F9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 3 (after 33+a 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0xDA1EB272C01F8C13L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+b 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDA45F7875E046FBBL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33b+c", 0)),
        // 4P ply 1 (after 33+c)
        new OctiBotEngine.OpeningBookEntry(0xDAAC383D8CDE48C6L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1236),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1236)),
        // 4P ply 4 (after 33+c 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDB420428B28B86C7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // 4P ply 4 (after 33+g 37+f 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDB6FED509EDE0FACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 53+a 37+d 33+h 57+d)
        new OctiBotEngine.OpeningBookEntry(0xDBDB05C23F5C9E38L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -930)),
        // 4P ply 4 (after 33+b 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xDC026D9575B31319L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -260)),
        // 4P ply 4 (after 33+a 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDC7AF054D43E0182L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 20),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -120)),
        // 4P ply 3 (after 33+g 37+e 77+e)
        new OctiBotEngine.OpeningBookEntry(0xDC9B8E640C4208CAL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -836),
            new OctiBotEngine.OpeningBookCandidate("73+e", -836),
            new OctiBotEngine.OpeningBookCandidate("73+g", -956)),
        // 4P ply 3 (after 33+g 37+b 77+b)
        new OctiBotEngine.OpeningBookEntry(0xDCF78D0014546678L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 4P ply 3 (after 33+d 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0xDD0161E977E68472L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0xDD26460EA6AA0B56L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+g 37+g 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDD5BF9D05C6C9254L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -120),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -120)),
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
        // 4P ply 4 (after 33+c 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDE31A7456553E93DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -260)),
        // OFK ply 4 (after 32+b 46+f 52+g 36+e)
        new OctiBotEngine.OpeningBookEntry(0xDE4E27413A47C869L, 8,
            new OctiBotEngine.OpeningBookCandidate("32+a", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h-33", -1343),
            new OctiBotEngine.OpeningBookCandidate("42h+b", -1839)),
        // 4P ply 4 (after 33+c 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDE5DA4217D45878FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 2P 1-Base ply 4 (after 53+c 57+d 53c+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xDE7B5647504FF66AL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -860),
            new OctiBotEngine.OpeningBookCandidate("33+b", -860),
            new OctiBotEngine.OpeningBookCandidate("73+a", -1010)),
        // 4P ply 4 (after 33+d 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDEA2F2F29D311254L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+a 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xDEAD66AB47E97977L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 160),
            new OctiBotEngine.OpeningBookCandidate("33a+c", 20)),
        // 4P ply 4 (after 33+b 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDF5A578E91CA6EF3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+c 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xDF8C6F2FCFBBEAA1L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -260),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -260)),
        // 4P ply 3 (after 33+c 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0xDFF1D0F1357D73A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1256),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1376)),
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
        // 4P ply 4 (after 33+d 37+f 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE087560EEC348648L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+c 37+g 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE089B097F81391A7L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P ply 3 (after 33+g 37+c 77+g)
        new OctiBotEngine.OpeningBookEntry(0xE0F40F4902D508A5L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+a 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE0FB4CC5DFE90C31L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+h 37+f 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE19FA2EBEA339A0CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 3 (after 33+g 37+g 77+g)
        new OctiBotEngine.OpeningBookEntry(0xE22399B691027050L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+c 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE25E26686BC4E952L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0)),
        // 4P ply 4 (after 33+d 37+d 77+d 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE279018FBA886676L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 4 (after 33+h 37+e 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE2C798F00E4AE7E6L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 2P 1-Base ply 3 (after 53+a 57+c 73+h)
        new OctiBotEngine.OpeningBookEntry(0xE3062AE12261B52CL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+f", -2880),
            new OctiBotEngine.OpeningBookCandidate("57g+d", -2900),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2900)),
        // 4P ply 4 (after 33+g 37+e 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE30DD3BEC46CE712L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P 1-Base ply 4 (after 33+b 57+c 53+h 77+f)
        new OctiBotEngine.OpeningBookEntry(0xE31A4DABB018B666L, 8,
            new OctiBotEngine.OpeningBookCandidate("53", 550),
            new OctiBotEngine.OpeningBookCandidate("33+a", 400),
            new OctiBotEngine.OpeningBookCandidate("53b-64", 400)),
        // 4P ply 4 (after 33+c 37+d 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE443F3966C1DEC61L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P_ED ply 1 (after 33+c)
        new OctiBotEngine.OpeningBookEntry(0xE44BA3E68D088A11L, 8,
            new OctiBotEngine.OpeningBookCandidate("37+c", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+e", -1577),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1577)),
        // 4P ply 4 (after 33+d 37+c 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE464D471BD516345L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -240),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -240)),
        // 4P ply 4 (after 33+h 37+b 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE48002E225FD9B44L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
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
        // 4P ply 4 (after 33+g 37+b 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE54A49ACEFDB9BB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+a 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE566D2140D716E17L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
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
        // 4P ply 2 (after 33+d 37+f)
        new OctiBotEngine.OpeningBookEntry(0xE64C962C3D2FEBC8L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // 4P ply 4 (after 33+g 37+f 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE6904D6F16F48534L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE69E04FA79BD1B99L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 2P 1-Base ply 3 (after 53+c 77+f 73+b)
        new OctiBotEngine.OpeningBookEntry(0xE6A08DB1B807ACE5L, 8,
            new OctiBotEngine.OpeningBookCandidate("57+e", -1960),
            new OctiBotEngine.OpeningBookCandidate("57+f", -1960),
            new OctiBotEngine.OpeningBookCandidate("77+f", -2080)),
        // 4P ply 4 (after 33+d 37+g 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE6B3428E2E861BB0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -100),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -100)),
        // 2P 1-Base ply 4 (after 53+c 57+c 33+b 57c+d)
        new OctiBotEngine.OpeningBookEntry(0xE79E5074BF818012L, 8,
            new OctiBotEngine.OpeningBookCandidate("53g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", 0),
            new OctiBotEngine.OpeningBookCandidate("73h+a", 0)),
        // 4P ply 3 (after 33+h 37+d 77+e)
        new OctiBotEngine.OpeningBookEntry(0xE7CDAC45CA6A00BEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+b 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE83FEFFBE140B3B5L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+h 37+g 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xE8420D4C07F93FD0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 3-Base ply 4 (after 33+h 37+d 73+b 57+d)
        new OctiBotEngine.OpeningBookEntry(0xE875B442344F52C4L, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -378),
            new OctiBotEngine.OpeningBookCandidate("53+h", -378),
            new OctiBotEngine.OpeningBookCandidate("73b+g", -558)),
        // 4P ply 1 (after 33+d)
        new OctiBotEngine.OpeningBookEntry(0xE8AB484ACEB8627DL, 8,
            new OctiBotEngine.OpeningBookCandidate("37+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("37+d", -1116),
            new OctiBotEngine.OpeningBookCandidate("37+f", -1116)),
        // 4P ply 4 (after 33+d 37+f 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xE945745FF0EDAC7CL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380)),
        // 4P ply 4 (after 33+g 37+g 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xE9667BBEC89F32F8L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 3 (after 33+h 37+e 77+d)
        new OctiBotEngine.OpeningBookEntry(0xE9827AF857D7A54EL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 2P 3-Base ply 4 (after 33+h 57+e 73+h 77+e)
        new OctiBotEngine.OpeningBookEntry(0xE9D503F11EF4AE4CL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a", -62),
            new OctiBotEngine.OpeningBookCandidate("73b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 4 (after 33+h 37+c 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEA959BB3942E4725L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 4P ply 4 (after 33+h 37+d 77+b 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEAA3D73BA0565006L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 2 (after 33+h 37+b)
        new OctiBotEngine.OpeningBookEntry(0xEB89E091E83FDCF0L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 260),
            new OctiBotEngine.OpeningBookCandidate("77+a", 260),
            new OctiBotEngine.OpeningBookCandidate("77+g", 140)),
        // 4P ply 4 (after 33+g 37+c 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEBB1ED415B484A0DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 4P ply 4 (after 33+d 37+d 77+h 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEBBB23DEA6514C42L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+d 37+e 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEC36D7322735C386L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 4P ply 4 (after 33+d 37+b 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEC5AD4563F23AD34L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 0)),
        // 4P ply 4 (after 33+h 37+d 77+e 73+f)
        new OctiBotEngine.OpeningBookEntry(0xEC884E4D93F74216L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+c 37+b 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xECA58285DF5738EFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+h 37+b 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xED4220B33924B170L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 260),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 120)),
        // 2P_1B_SPED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xED94D74F7390740EL, 8,
            new OctiBotEngine.OpeningBookCandidate("53+a!", 0),
            new OctiBotEngine.OpeningBookCandidate("53+b", 0),
            new OctiBotEngine.OpeningBookCandidate("53+h", 0)),
        // 4P ply 4 (after 33+b 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEDA2712A33D8D193L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+d 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xEDAC38BF5C914F3EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 3 (after 33+d 37+g 77+d)
        new OctiBotEngine.OpeningBookEntry(0xEDF6A086771B5918L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1136),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1256)),
        // 2P 3-Base ply 4 (after 53+b 57+f 33+b 77+e)
        new OctiBotEngine.OpeningBookEntry(0xEDF798735A95A93BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+a", -62),
            new OctiBotEngine.OpeningBookCandidate("53b+a", -62),
            new OctiBotEngine.OpeningBookCandidate("33b+a", -62)),
        // 4P ply 3 (after 33+g 37+b 77+a)
        new OctiBotEngine.OpeningBookEntry(0xEE0FABA4B646D918L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 1-Base ply 4 (after 53+b 77+e 33+b 57+f)
        new OctiBotEngine.OpeningBookEntry(0xEE1648687CF08987L, 8,
            new OctiBotEngine.OpeningBookCandidate("73h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("53h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33+b", -120)),
        // 4P ply 4 (after 33+a 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xEE5AA7C44714A41DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 3 (after 33+c 37+d 77+g)
        new OctiBotEngine.OpeningBookEntry(0xEF06119E3580AEC9L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 3 (after 33+d 37+c 77+d)
        new OctiBotEngine.OpeningBookEntry(0xEF213679E4CC21EDL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -996),
            new OctiBotEngine.OpeningBookCandidate("73+e", -996),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1116)),
        // 4P ply 4 (after 33+b 37+f 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF024D1BF845A3D2DL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+d 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF02A982AEB13A380L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 2P_3B_SPED ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xF0E7D925A2145A8BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h!", 0),
            new OctiBotEngine.OpeningBookCandidate("53+a!", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a!", 0)),
        // 4P ply 4 (after 33+c 37+b 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF123221068D5D451L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 120),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 120)),
        // 4P ply 4 (after 33+d 37+e 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF15E4A1B95F722E4L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380)),
        // 4P ply 3 (after 33+h 37+f 77+d)
        new OctiBotEngine.OpeningBookEntry(0xF19944BC32CD2BD6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 3 (after 33+d 37+d 77+b)
        new OctiBotEngine.OpeningBookEntry(0xF27FE7D86276D7ACL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -976),
            new OctiBotEngine.OpeningBookCandidate("73+e", -976),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1096)),
        // 4P ply 3 (after 33+c 37+d 77+a)
        new OctiBotEngine.OpeningBookEntry(0xF280B10B82024277L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // OFK ply 3 (after 32+a 46+f 42+a)
        new OctiBotEngine.OpeningBookEntry(0xF34AAEC0CCA098E6L, 8,
            new OctiBotEngine.OpeningBookCandidate("46+f", -4263),
            new OctiBotEngine.OpeningBookCandidate("36d+e", -4450),
            new OctiBotEngine.OpeningBookCandidate("56+f", -4483)),
        // 4P ply 3 (after 33+h 37+d 77+h)
        new OctiBotEngine.OpeningBookEntry(0xF367133D6471CBE8L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 3 (after 33+g 37+b 77+g)
        new OctiBotEngine.OpeningBookEntry(0xF3890B3101C435A6L, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1216),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1336)),
        // 2P 3-Base ply 3 (after 53+b 57+f 33+h)
        new OctiBotEngine.OpeningBookEntry(0xF3B2FDC02FAA9AEDL, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2024),
            new OctiBotEngine.OpeningBookCandidate("57d+e", -2024)),
        // 4P ply 4 (after 33+a 37+d 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF3DC0751F09648A3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 2P_3B_ED ply 1 (after 33+h)
        new OctiBotEngine.OpeningBookEntry(0xF3F679D1EE355366L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+e", -2512),
            new OctiBotEngine.OpeningBookCandidate("37+e", -2512),
            new OctiBotEngine.OpeningBookCandidate("57+e", -2512)),
        // 4P ply 4 (after 33+d 37+f 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF42DE976422F4D1EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
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
        // 4P ply 4 (after 33+g 37+g 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF4E0DB2B7F1DDE46L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33g+a", 0)),
        // 4P ply 4 (after 33+h 37+g 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF52A9065B53BDEB2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 160),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 20)),
        // 2P 1-Base ply 4 (after 53+c 77+f 53c+h 37+d)
        new OctiBotEngine.OpeningBookEntry(0xF5876844AE919EABL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+h", -610),
            new OctiBotEngine.OpeningBookCandidate("33+a", -610),
            new OctiBotEngine.OpeningBookCandidate("33+b", -730)),
        // 4P ply 4 (after 33+b 37+e 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF5B94F6E56C25F0BL, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
        // 4P ply 4 (after 33+g 37+c 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF6374DD4ECCAA6B3L, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -140)),
        // 2P 3-Base ply 4 (after 53+a 57+d 33+c 77+e)
        new OctiBotEngine.OpeningBookEntry(0xF69424F712060375L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+b", -400),
            new OctiBotEngine.OpeningBookCandidate("73g+h", -400),
            new OctiBotEngine.OpeningBookCandidate("53a+h", -400)),
        // 4P ply 4 (after 33+d 37+b 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF719D009BE405E46L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
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
        // 4P ply 4 (after 33+h 37+c 77+d 73+e)
        new OctiBotEngine.OpeningBookEntry(0xF7FD069A26ECA647L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -120)),
        // 4P ply 4 (after 33+h 37+d 77+h 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF822F1353DEC8940L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+d 37+e 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF89C684A892E08D0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+g 37+b 77+g 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF8CCE9395859770EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -20),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -20)),
        // 4P ply 4 (after 33+a 37+f 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF8E07281BAF382A9L, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 2 (after 33+h 37+c)
        new OctiBotEngine.OpeningBookEntry(0xF8F4E4E9EB2EE1F3L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 140),
            new OctiBotEngine.OpeningBookCandidate("77+a", 140),
            new OctiBotEngine.OpeningBookCandidate("77+g", 20)),
        // 4P ply 4 (after 33+d 37+d 77+b 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF93A05D03BEB9504L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -260),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -260)),
        // 4P ply 4 (after 33+h 37+e 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF9849CAF8F291494L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -140)),
        // 4P ply 4 (after 33+c 37+d 77+a 73+f)
        new OctiBotEngine.OpeningBookEntry(0xF9C55303DB9F00DFL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", -140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", -140)),
        // 4P ply 4 (after 33+h 37+b 77+e 73+g)
        new OctiBotEngine.OpeningBookEntry(0xF9E89FCB973F7A26L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 2 (after 33+h 37+g)
        new OctiBotEngine.OpeningBookEntry(0xFA23721678F99906L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", 280),
            new OctiBotEngine.OpeningBookCandidate("77+a", 280),
            new OctiBotEngine.OpeningBookCandidate("77+g", 160)),
        // 4P ply 4 (after 33+h 37+f 77+d 73+f)
        new OctiBotEngine.OpeningBookEntry(0xFADCA6B46B50697EL, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33h+c", -260)),
        // 4P ply 4 (after 33+g 37+f 77+a 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFB16EDFAA176698AL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+b 37+d 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFB18A46FCE3FF727L, 8,
            new OctiBotEngine.OpeningBookCandidate("33b+a", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33b+c", -140)),
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
        // 4P ply 3 (after 33+d 37+b 77+d)
        new OctiBotEngine.OpeningBookEntry(0xFC5C3201E7DD1CEEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1116),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1236)),
        // 4P ply 4 (after 33+h 37+g 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFCE8B234A9E2F486L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 280),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 140)),
        // 4P ply 4 (after 33+c 37+g 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFD0F10024F917D19L, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 140),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 140)),
        // 4P_SP ply 0 (initial)
        new OctiBotEngine.OpeningBookEntry(0xFD5CAC35447FD5C0L, 8,
            new OctiBotEngine.OpeningBookCandidate("33+a!", 640),
            new OctiBotEngine.OpeningBookCandidate("33+c!", 0),
            new OctiBotEngine.OpeningBookCandidate("33+a", -467)),
        // 4P ply 3 (after 33+g 37+c 77+a)
        new OctiBotEngine.OpeningBookEntry(0xFD72AFDCB557E41BL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1096),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1216)),
        // 4P ply 4 (after 33+a 37+e 77+g 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFD7DEC50686BE08FL, 8,
            new OctiBotEngine.OpeningBookCandidate("33a+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33a+c", -140)),
        // 4P ply 4 (after 33+d 37+g 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFDF046D1AFE5E8C2L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+b", 20),
            new OctiBotEngine.OpeningBookCandidate("33d+h", 20)),
        // 4P ply 4 (after 33+h 37+c 77+h 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFE3F24CB3A358C73L, 8,
            new OctiBotEngine.OpeningBookCandidate("33h+a", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+b", 140),
            new OctiBotEngine.OpeningBookCandidate("33h+c", 0)),
        // 4P ply 2 (after 33+d 37+e)
        new OctiBotEngine.OpeningBookEntry(0xFE57A86858356550L, 8,
            new OctiBotEngine.OpeningBookCandidate("77+h", -260),
            new OctiBotEngine.OpeningBookCandidate("77+a", -260),
            new OctiBotEngine.OpeningBookCandidate("77+g", -380)),
        // 4P ply 4 (after 33+g 37+e 77+g 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFE8B732B73EE0BACL, 8,
            new OctiBotEngine.OpeningBookCandidate("33g+b", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+h", -280),
            new OctiBotEngine.OpeningBookCandidate("33g+a", -280)),
        // 4P ply 4 (after 33+d 37+d 77+e 73+e)
        new OctiBotEngine.OpeningBookEntry(0xFF119CA6084A8714L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -380),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -380)),
        // 4P ply 4 (after 33+d 37+c 77+b 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFF27D02E3C329037L, 8,
            new OctiBotEngine.OpeningBookCandidate("33d+a", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+b", -120),
            new OctiBotEngine.OpeningBookCandidate("33d+h", -120)),
        // 4P ply 3 (after 33+g 37+g 77+a)
        new OctiBotEngine.OpeningBookEntry(0xFFA5392326809CEEL, 8,
            new OctiBotEngine.OpeningBookCandidate("73+f", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+e", -1236),
            new OctiBotEngine.OpeningBookCandidate("73+g", -1356)),
        // 4P ply 4 (after 33+c 37+c 77+a 73+g)
        new OctiBotEngine.OpeningBookEntry(0xFFD886FDDC4605ECL, 8,
            new OctiBotEngine.OpeningBookCandidate("33c+b", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+h", 0),
            new OctiBotEngine.OpeningBookCandidate("33c+a", 0))
        };
    }
}
