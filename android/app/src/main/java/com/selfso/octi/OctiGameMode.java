/*
 * OctiGameMode.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

public enum OctiGameMode {
    GAME_MODE_UNDEF(0),
    GAME_MODE_2(1),
    GAME_MODE_4(2),
    GAME_MODE_OFK(3);

    public static final List<String> ALL_VARIANT_CODES = Collections.unmodifiableList(Arrays.asList(
            "2P_3B", "2P_3B_SP", "2P_3B_ED", "2P_3B_SPED",
            "2P_1B", "2P_1B_SP", "2P_1B_ED", "2P_1B_SPED",
            "4P", "4P_SP", "4P_ED", "4P_SPED",
            "OFK"
    ));

    public static final List<String> OFFICIAL_RATED_VARIANT_CODES = Collections.unmodifiableList(Arrays.asList(
            "2P_3B", "2P_1B", "OFK"
    ));

    public static final class ParsedVariant {
        public final OctiGameMode mode;
        public final OctiGameBases bases;
        public final boolean superProngs;
        public final boolean edgeless;
        public final String code;

        public ParsedVariant(
                OctiGameMode mode,
                OctiGameBases bases,
                boolean superProngs,
                boolean edgeless,
                String code) {
            this.mode = mode;
            this.bases = bases;
            this.superProngs = superProngs;
            this.edgeless = edgeless;
            this.code = code;
        }
    }

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

    public static List<String> allVariantCodes() {
        return ALL_VARIANT_CODES;
    }

    public static List<String> officialRatedVariantCodes() {
        return OFFICIAL_RATED_VARIANT_CODES;
    }

    public static String variantCode(
            OctiGameMode mode, int bases, boolean superProngs, boolean edgeless) {
        if (mode == GAME_MODE_OFK) {
            return "OFK";
        }
        String baseCode;
        if (mode == GAME_MODE_4) {
            baseCode = "4P";
        } else {
            baseCode = (bases == 1) ? "2P_1B" : "2P_3B";
        }
        if (superProngs && edgeless) {
            return baseCode + "_SPED";
        }
        if (superProngs) {
            return baseCode + "_SP";
        }
        if (edgeless) {
            return baseCode + "_ED";
        }
        return baseCode;
    }

    public static String variantCode(
            OctiGameMode mode, OctiGameBases bases, boolean superProngs, boolean edgeless) {
        int bVal = (bases == OctiGameBases.GAME_BASES_1) ? 1 : 3;
        return variantCode(mode, bVal, superProngs, edgeless);
    }

    public static ParsedVariant parseVariantCode(String code) {
        if (code == null) {
            return null;
        }
        String u = code.trim().toUpperCase(Locale.ROOT);
        if ("OFK".equals(u)) {
            return new ParsedVariant(
                    GAME_MODE_OFK, OctiGameBases.GAME_BASES_1, false, false, "OFK");
        }
        if ("4P".equals(u) || "4P_SP".equals(u) || "4P_ED".equals(u) || "4P_SPED".equals(u)) {
            boolean sp = "4P_SP".equals(u) || "4P_SPED".equals(u);
            boolean ed = "4P_ED".equals(u) || "4P_SPED".equals(u);
            return new ParsedVariant(
                    GAME_MODE_4, OctiGameBases.GAME_BASES_3, sp, ed, u);
        }
        if ("2P_3B".equals(u) || "2P_3B_SP".equals(u) || "2P_3B_ED".equals(u) || "2P_3B_SPED".equals(u)) {
            boolean sp = "2P_3B_SP".equals(u) || "2P_3B_SPED".equals(u);
            boolean ed = "2P_3B_ED".equals(u) || "2P_3B_SPED".equals(u);
            return new ParsedVariant(
                    GAME_MODE_2, OctiGameBases.GAME_BASES_3, sp, ed, u);
        }
        if ("2P_1B".equals(u) || "2P_1B_SP".equals(u) || "2P_1B_ED".equals(u) || "2P_1B_SPED".equals(u)) {
            boolean sp = "2P_1B_SP".equals(u) || "2P_1B_SPED".equals(u);
            boolean ed = "2P_1B_ED".equals(u) || "2P_1B_SPED".equals(u);
            return new ParsedVariant(
                    GAME_MODE_2, OctiGameBases.GAME_BASES_1, sp, ed, u);
        }
        return null;
    }

    public static boolean isValidVariantCode(String code) {
        return parseVariantCode(code) != null;
    }

    public static String variantCodeFromModeStr(
            String modeStr, int bases, boolean superProngs, boolean edgeless) {
        String u = (modeStr == null) ? "" : modeStr.trim().toUpperCase(Locale.ROOT);
        if ("OFK".equals(u)) {
            return "OFK";
        }
        if ("4P".equals(u) || "4".equals(u)) {
            return variantCode(GAME_MODE_4, 3, superProngs, edgeless);
        }
        if ("2P".equals(u) || "2".equals(u) || u.isEmpty()) {
            return variantCode(GAME_MODE_2, bases, superProngs, edgeless);
        }
        ParsedVariant pv = parseVariantCode(u);
        if (pv != null) {
            return pv.code;
        }
        return variantCode(GAME_MODE_2, bases, superProngs, edgeless);
    }

    public static boolean isOfficialRatedVariant(
            OctiGameMode mode, int bases, boolean superProngs, boolean edgeless) {
        String v = variantCode(mode, bases, superProngs, edgeless);
        return "2P_3B".equals(v) || "2P_1B".equals(v) || "OFK".equals(v);
    }

    public static boolean isOfficialRatedVariant(
            OctiGameMode mode, OctiGameBases bases, boolean superProngs, boolean edgeless) {
        int bVal = (bases == OctiGameBases.GAME_BASES_1) ? 1 : 3;
        return isOfficialRatedVariant(mode, bVal, superProngs, edgeless);
    }

    public static boolean isOfficialRatedVariantCode(String code) {
        ParsedVariant pv = parseVariantCode(code);
        if (pv == null) {
            return false;
        }
        return isOfficialRatedVariant(pv.mode, pv.bases, pv.superProngs, pv.edgeless);
    }
}
