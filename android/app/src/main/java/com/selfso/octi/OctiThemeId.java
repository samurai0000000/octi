/*
 * OctiThemeId.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public enum OctiThemeId {
    OBSIDIAN_BRASS(0),
    ROSEWOOD_IVORY(1),
    NEON_CIRCUIT(2),
    JADE_DYNASTY(3),
    ARCTIC_AURORA(4),
    VOLCANIC_FORGE(5),
    CANDY_POP(6),
    OCEAN_PEARL(7),
    STEAMPUNK_BRASS(8),
    COSMIC_NEBULA(9);

    private final int id;

    OctiThemeId(int id) {
        this.id = id;
    }

    public int id() {
        return id;
    }

    public String toConfigKey() {
        switch (this) {
            case OBSIDIAN_BRASS:
                return "obsidian_brass";
            case ROSEWOOD_IVORY:
                return "rosewood_ivory";
            case NEON_CIRCUIT:
                return "neon_circuit";
            case JADE_DYNASTY:
                return "jade_dynasty";
            case ARCTIC_AURORA:
                return "arctic_aurora";
            case VOLCANIC_FORGE:
                return "volcanic_forge";
            case CANDY_POP:
                return "candy_pop";
            case OCEAN_PEARL:
                return "ocean_pearl";
            case STEAMPUNK_BRASS:
                return "steampunk_brass";
            case COSMIC_NEBULA:
                return "cosmic_nebula";
            default:
                return "obsidian_brass";
        }
    }

    public String toWebKey() {
        switch (this) {
            case OBSIDIAN_BRASS:
                return "obsidian";
            case ROSEWOOD_IVORY:
                return "rosewood";
            case NEON_CIRCUIT:
                return "neon";
            case JADE_DYNASTY:
                return "jade";
            case ARCTIC_AURORA:
                return "aurora";
            case VOLCANIC_FORGE:
                return "forge";
            case CANDY_POP:
                return "candy";
            case OCEAN_PEARL:
                return "pearl";
            case STEAMPUNK_BRASS:
                return "steam";
            case COSMIC_NEBULA:
                return "nebula";
            default:
                return "obsidian";
        }
    }

    public String displayName() {
        switch (this) {
            case OBSIDIAN_BRASS:
                return "Obsidian Brass";
            case ROSEWOOD_IVORY:
                return "Rosewood Ivory";
            case NEON_CIRCUIT:
                return "Neon Circuit";
            case JADE_DYNASTY:
                return "Jade Dynasty";
            case ARCTIC_AURORA:
                return "Arctic Aurora";
            case VOLCANIC_FORGE:
                return "Volcanic Forge";
            case CANDY_POP:
                return "Candy Pop";
            case OCEAN_PEARL:
                return "Ocean Pearl";
            case STEAMPUNK_BRASS:
                return "Steampunk Brass";
            case COSMIC_NEBULA:
                return "Cosmic Nebula";
            default:
                return "Obsidian Brass";
        }
    }

    public static OctiThemeId fromInt(int id) {
        for (OctiThemeId t : values()) {
            if (t.id == id) {
                return t;
            }
        }
        return OBSIDIAN_BRASS;
    }

    public static OctiThemeId fromConfigValue(Object val) {
        if (val == null) {
            return OBSIDIAN_BRASS;
        }
        if (val instanceof OctiThemeId) {
            return (OctiThemeId) val;
        }
        if (val instanceof Number n) {
            return fromInt(n.intValue());
        }
        if (val instanceof String s) {
            String trimmed = s.trim();
            if (trimmed.isEmpty()) {
                return OBSIDIAN_BRASS;
            }
            try {
                int n = Integer.parseInt(trimmed);
                return fromInt(n);
            } catch (NumberFormatException ignored) {
            }
            for (OctiThemeId t : values()) {
                if (t.name().equalsIgnoreCase(trimmed)
                        || t.toConfigKey().equalsIgnoreCase(trimmed)
                        || t.toWebKey().equalsIgnoreCase(trimmed)
                        || t.displayName().equalsIgnoreCase(trimmed)) {
                    return t;
                }
            }
        }
        return OBSIDIAN_BRASS;
    }
}

/*
 * Local variables:
 * mode: Java
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
