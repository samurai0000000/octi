/*
 * OctiBoardListener.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public interface OctiBoardListener {

    void boardUpdated(OctiBoard board, OctiMove move);

    void gameChanged(OctiBoard board, OctiGame oldGame, OctiGame newGame);
}
