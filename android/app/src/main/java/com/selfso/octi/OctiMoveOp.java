/*
 * OctiMoveOp.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class OctiMoveOp {

    public int x0 = -1;
    public int y0 = -1;
    public OctiPod pod = new OctiPod();
    public boolean addPod = false;
    public boolean liberate = false;
    public OctiDir addProngDir = OctiDir.DIR_UNDEF;
    public OctiDir rmProngDir = OctiDir.DIR_UNDEF;
    public boolean superProng = false;
    public boolean isSuperProngBonusMove = false;
    public final List<OctiHop> hops = new ArrayList<>();

    public OctiMoveOp() {
    }

    public OctiMoveOp(OctiMoveOp other) {
        if (other != null) {
            this.x0 = other.x0;
            this.y0 = other.y0;
            this.pod = (other.pod != null) ? other.pod.copy() : new OctiPod();
            this.addPod = other.addPod;
            this.liberate = other.liberate;
            this.addProngDir = other.addProngDir;
            this.rmProngDir = other.rmProngDir;
            this.superProng = other.superProng;
            this.isSuperProngBonusMove = other.isSuperProngBonusMove;
            for (OctiHop h : other.hops) {
                this.hops.add(new OctiHop(h));
            }
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof OctiMoveOp)) {
            return false;
        }
        OctiMoveOp other = (OctiMoveOp) obj;
        return this.x0 == other.x0
                && this.y0 == other.y0
                && Objects.equals(this.pod, other.pod)
                && this.addPod == other.addPod
                && this.liberate == other.liberate
                && this.addProngDir == other.addProngDir
                && this.rmProngDir == other.rmProngDir
                && this.superProng == other.superProng
                && this.isSuperProngBonusMove == other.isSuperProngBonusMove
                && Objects.equals(this.hops, other.hops);
    }

    @Override
    public int hashCode() {
        return Objects.hash(x0, y0, pod, addPod, liberate, addProngDir,
                rmProngDir, superProng, isSuperProngBonusMove, hops);
    }

    @Override
    public String toString() {
        List<OctiMoveOp> singleList = new ArrayList<>();
        singleList.add(this);
        return OctiMove.formatOps(singleList);
    }
}
