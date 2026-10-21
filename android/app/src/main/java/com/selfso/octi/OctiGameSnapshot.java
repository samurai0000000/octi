/*
 * OctiGameSnapshot.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.List;

public class OctiGameSnapshot {

    @SuppressWarnings("unchecked")
    public final List<OctiPod>[][] squares = new List[9][9];
    public final OctiPlayerState[] players = new OctiPlayerState[5];
    public OctiPlayer turn = OctiPlayer.PLAYER_UNDEF;
    public OctiPlayer winner = OctiPlayer.PLAYER_UNDEF;
    public boolean gameStarted = false;
    public boolean gameOver = false;
    public boolean gameResigned = false;
    public OctiMove lastMove = new OctiMove();
    public OctiPlayer lastPlayer = OctiPlayer.PLAYER_UNDEF;

    public OctiGameSnapshot() {
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                squares[x][y] = new ArrayList<>();
            }
        }
        for (int p = 0; p < 5; p++) {
            players[p] = new OctiPlayerState();
        }
    }

    public OctiGameSnapshot(OctiGameSnapshot other) {
        this();
        if (other != null) {
            for (int x = 0; x < 9; x++) {
                for (int y = 0; y < 9; y++) {
                    if (other.squares[x][y] != null) {
                        for (OctiPod pod : other.squares[x][y]) {
                            this.squares[x][y].add(pod.copy());
                        }
                    }
                }
            }
            for (int p = 0; p < 5; p++) {
                if (other.players[p] != null) {
                    this.players[p] = new OctiPlayerState(other.players[p]);
                }
            }
            this.turn = other.turn;
            this.winner = other.winner;
            this.gameStarted = other.gameStarted;
            this.gameOver = other.gameOver;
            this.gameResigned = other.gameResigned;
            this.lastMove = (other.lastMove != null) ? new OctiMove(other.lastMove.asString()) : new OctiMove();
            this.lastPlayer = other.lastPlayer;
        }
    }
}
