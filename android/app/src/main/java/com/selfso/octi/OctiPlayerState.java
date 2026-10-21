/*
 * OctiPlayerState.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.Objects;

public class OctiPlayerState {

    public int freePods = 0;
    public int capturedPods = 0;
    public int freeProngs = 0;
    public int freeSuperProngs = 0;
    public long timeRemainingMs = 0;
    public boolean eliminated = false;

    public OctiPlayerState() {
    }

    public OctiPlayerState(OctiPlayerState other) {
        if (other != null) {
            this.freePods = other.freePods;
            this.capturedPods = other.capturedPods;
            this.freeProngs = other.freeProngs;
            this.freeSuperProngs = other.freeSuperProngs;
            this.timeRemainingMs = other.timeRemainingMs;
            this.eliminated = other.eliminated;
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof OctiPlayerState)) {
            return false;
        }
        OctiPlayerState other = (OctiPlayerState) obj;
        return this.freePods == other.freePods
                && this.capturedPods == other.capturedPods
                && this.freeProngs == other.freeProngs
                && this.freeSuperProngs == other.freeSuperProngs
                && this.timeRemainingMs == other.timeRemainingMs
                && this.eliminated == other.eliminated;
    }

    @Override
    public int hashCode() {
        return Objects.hash(freePods, capturedPods, freeProngs,
                freeSuperProngs, timeRemainingMs, eliminated);
    }
}
