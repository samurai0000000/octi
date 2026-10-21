/*
 * OctiPod.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class OctiPod implements Comparable<OctiPod> {

    public static final OctiPod EMPTY_POD = new OctiPod();

    private static final int OWNER_MASK = 0x00070000;
    private static final int OWNER_SHIFT = 16;
    private static final int PRONGS_MASK = 0x0000FFFF;

    private int conf;

    public OctiPod() {
        this.conf = 0;
    }

    public OctiPod(OctiPodType type) {
        this.conf = 0;
        setType(type);
    }

    public OctiPod(int rawConf) {
        this.conf = rawConf;
    }

    public OctiPod(OctiPod other) {
        this.conf = (other != null) ? other.conf : 0;
    }

    public OctiPod copy() {
        return new OctiPod(this.conf);
    }

    public void clear() {
        this.conf = 0;
    }

    public OctiPodType getType() {
        int val = (conf & OWNER_MASK) >>> OWNER_SHIFT;
        return OctiPodType.fromValue(val);
    }

    public void setType(OctiPodType type) {
        int val = (type != null ? type.getValue() : 0) & 0x7;
        conf = (conf & ~OWNER_MASK) | (val << OWNER_SHIFT);
    }

    public OctiProngType prongAt(OctiDir dir) {
        if (dir == null || dir.getIndex() < 0 || dir.getIndex() >= OctiDir.DIR_MAX) {
            return OctiProngType.NO_PRONG;
        }

        int shift = dir.getIndex() * 2;
        int val = (conf >>> shift) & 0x3;
        return OctiProngType.fromValue(val);
    }

    public void addProng(OctiDir dir, boolean superProng) {
        if (dir == null || dir.getIndex() < 0 || dir.getIndex() >= OctiDir.DIR_MAX) {
            return;
        }

        int shift = dir.getIndex() * 2;
        int mask = 0x3 << shift;
        int pval = superProng ? OctiProngType.SUPER_PRONG.getValue()
                              : OctiProngType.NORMAL_PRONG.getValue();
        conf = (conf & ~mask) | ((pval & 0x3) << shift);
    }

    public void addProng(OctiDir dir) {
        addProng(dir, false);
    }

    public void removeProng(OctiDir dir) {
        if (dir == null || dir.getIndex() < 0 || dir.getIndex() >= OctiDir.DIR_MAX) {
            return;
        }

        int shift = dir.getIndex() * 2;
        int mask = 0x3 << shift;
        conf &= ~mask;
    }

    public boolean hasSuperProng() {
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            if (prongAt(OctiDir.fromIndex(d)) == OctiProngType.SUPER_PRONG) {
                return true;
            }
        }
        return false;
    }

    public int getNumProngs() {
        int count = 0;
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            if (prongAt(OctiDir.fromIndex(d)) != OctiProngType.NO_PRONG) {
                count++;
            }
        }
        return count;
    }

    public int getNumNormalProngs() {
        int count = 0;
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            if (prongAt(OctiDir.fromIndex(d)) == OctiProngType.NORMAL_PRONG) {
                count++;
            }
        }
        return count;
    }

    public int getNumSuperProngs() {
        int count = 0;
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            if (prongAt(OctiDir.fromIndex(d)) == OctiProngType.SUPER_PRONG) {
                count++;
            }
        }
        return count;
    }

    public String getConfigString() {
        StringBuilder sb = new StringBuilder();
        for (int d = 0; d < OctiDir.DIR_MAX; d++) {
            OctiDir dir = OctiDir.fromIndex(d);
            OctiProngType pt = prongAt(dir);
            if (pt == OctiProngType.NORMAL_PRONG) {
                sb.append(dir.toChar());
            } else if (pt == OctiProngType.SUPER_PRONG) {
                sb.append(dir.toChar());
                sb.append('!');
            }
        }
        return sb.toString();
    }

    public int rawConfig() {
        return conf;
    }

    @Override
    public int compareTo(OctiPod other) {
        if (other == null) {
            return 1;
        }

        int myProngs = getNumProngs();
        int otherProngs = other.getNumProngs();
        if (myProngs != otherProngs) {
            return Integer.compare(myProngs, otherProngs);
        }

        int mySuper = getNumSuperProngs();
        int otherSuper = other.getNumSuperProngs();
        if (mySuper != otherSuper) {
            return Integer.compare(mySuper, otherSuper);
        }

        int myP = conf & PRONGS_MASK;
        int otherP = other.conf & PRONGS_MASK;
        if (myP != otherP) {
            return Integer.compare(myP, otherP);
        }

        return Integer.compare(conf >>> OWNER_SHIFT, other.conf >>> OWNER_SHIFT);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof OctiPod)) {
            return false;
        }
        OctiPod other = (OctiPod) obj;
        return this.conf == other.conf;
    }

    @Override
    public int hashCode() {
        return Integer.hashCode(conf);
    }

    @Override
    public String toString() {
        char ownerChar;
        switch (getType()) {
            case A_POD:
                ownerChar = 'A';
                break;
            case B_POD:
                ownerChar = 'B';
                break;
            case C_POD:
                ownerChar = 'C';
                break;
            case D_POD:
                ownerChar = 'D';
                break;
            default:
                ownerChar = '?';
                break;
        }

        return ownerChar + getConfigString();
    }
}
