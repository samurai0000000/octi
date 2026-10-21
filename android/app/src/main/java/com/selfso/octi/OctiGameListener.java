/*
 * OctiGameListener.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public interface OctiGameListener {

    void gameUpdated(OctiGame game);

    void undoRedoStatusChanged(OctiGame game, boolean canUndo, boolean canRedo);
}
