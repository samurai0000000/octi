/*
 * OctiHop.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.Objects;

public class OctiHop {

    public int x = -1;
    public int y = -1;
    public boolean capture = false;

    public OctiHop() {
    }

    public OctiHop(int x, int y, boolean capture) {
        this.x = x;
        this.y = y;
        this.capture = capture;
    }

    public OctiHop(OctiHop other) {
        if (other != null) {
            this.x = other.x;
            this.y = other.y;
            this.capture = other.capture;
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof OctiHop)) {
            return false;
        }
        OctiHop other = (OctiHop) obj;
        return this.x == other.x && this.y == other.y && this.capture == other.capture;
    }

    @Override
    public int hashCode() {
        return Objects.hash(x, y, capture);
    }

    @Override
    public String toString() {
        return (x + 1) + "" + (y + 1) + (capture ? "x" : "");
    }
}
