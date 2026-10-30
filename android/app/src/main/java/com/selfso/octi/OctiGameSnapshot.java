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
    public int searchPlyOffset = 0;
    public List<OctiGame.OctiPlyMetadata> plyMetadata = new ArrayList<>();
    public OctiGame.OctiPlyMetadata lastPlyMeta = new OctiGame.OctiPlyMetadata();

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
            this.searchPlyOffset = other.searchPlyOffset;
            this.plyMetadata = new ArrayList<>();
            if (other.plyMetadata != null) {
                for (OctiGame.OctiPlyMetadata pm : other.plyMetadata) {
                    this.plyMetadata.add(new OctiGame.OctiPlyMetadata(pm));
                }
            }
            this.lastPlyMeta = (other.lastPlyMeta != null)
                    ? new OctiGame.OctiPlyMetadata(other.lastPlyMeta)
                    : new OctiGame.OctiPlyMetadata();
        }
    }
}
