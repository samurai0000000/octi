/*
 * OctiSquare.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class OctiSquare {

    private OctiGameMode mode;
    private OctiSquareType type;
    private int x;
    private int y;
    private final List<OctiPod> pods = new ArrayList<>();

    public OctiSquare() {
        this.mode = OctiGameMode.GAME_MODE_UNDEF;
        this.type = OctiSquareType.NEUTRAL_SQUARE;
        this.x = -1;
        this.y = -1;
    }

    public OctiSquare(OctiGameMode mode, OctiSquareType type, int x, int y) {
        this.mode = mode;
        this.type = type;
        this.x = x;
        this.y = y;
    }

    public void configure(OctiGameMode mode, OctiSquareType type, int x, int y) {
        this.mode = mode;
        this.type = type;
        this.x = x;
        this.y = y;
    }

    public void clear() {
        pods.clear();
    }

    public OctiGameMode getGameMode() {
        return mode;
    }

    public OctiSquareType getType() {
        return type;
    }

    public int getLocId() {
        return (x + 1) * 10 + (y + 1);
    }

    public int getLocX() {
        return x;
    }

    public int getLocY() {
        return y;
    }

    public int getNumPods() {
        return pods.size();
    }

    public OctiPod getPodAt(int index) {
        if (index < 0 || index >= pods.size()) {
            throw new IndexOutOfBoundsException("OctiSquare.getPodAt: index out of range: " + index);
        }
        return pods.get(index);
    }

    public OctiPod podAt(int index) {
        return getPodAt(index);
    }

    public List<OctiPod> pods() {
        return Collections.unmodifiableList(pods);
    }

    public void addPod(OctiPod pod) {
        if (pod != null) {
            pods.add(pod.copy());
        }
    }

    public boolean removePod(OctiPod pod) {
        if (pod == null) {
            return false;
        }
        for (int i = pods.size() - 1; i >= 0; i--) {
            if (pods.get(i).equals(pod)) {
                pods.remove(i);
                return true;
            }
        }
        return false;
    }

    public boolean containsPod(OctiPod pod) {
        if (pod == null) {
            return false;
        }
        for (OctiPod p : pods) {
            if (p.equals(pod)) {
                return true;
            }
        }
        return false;
    }

    public boolean isEmpty() {
        return pods.isEmpty();
    }

    public boolean isEmptyWithout(List<OctiPod> ghostPods) {
        int count = pods.size();
        if (ghostPods == null || ghostPods.isEmpty()) {
            return count == 0;
        }

        boolean[] matched = new boolean[ghostPods.size()];
        for (OctiPod p : pods) {
            for (int j = 0; j < ghostPods.size(); j++) {
                if (!matched[j] && ghostPods.get(j).equals(p)) {
                    matched[j] = true;
                    count--;
                    break;
                }
            }
        }

        return count <= 0;
    }

    public boolean isCaptured() {
        if (pods.isEmpty()) {
            return false;
        }

        OctiPlayer baseOwner;
        switch (type) {
            case A_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_A;
                break;
            case B_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_B;
                break;
            case C_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_C;
                break;
            case D_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_D;
                break;
            default:
                return false;
        }

        for (OctiPod p : pods) {
            OctiPlayer podPlayer = p.getType().toPlayer();
            if (podPlayer != OctiPlayer.PLAYER_UNDEF
                    && !OctiPlayer.areAllied(mode, baseOwner, podPlayer)) {
                return true;
            }
        }

        return false;
    }

    public boolean isCapturedBy(OctiPlayer player) {
        if (!isCaptured()) {
            return false;
        }

        OctiPlayer baseOwner;
        switch (type) {
            case A_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_A;
                break;
            case B_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_B;
                break;
            case C_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_C;
                break;
            case D_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_D;
                break;
            default:
                return false;
        }

        if (OctiPlayer.areAllied(mode, baseOwner, player)) {
            return false;
        }

        return isOccupiedBy(player);
    }

    public boolean isCapturedByTeamOf(OctiPlayer player) {
        if (!isCaptured()) {
            return false;
        }

        OctiPlayer baseOwner;
        switch (type) {
            case A_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_A;
                break;
            case B_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_B;
                break;
            case C_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_C;
                break;
            case D_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_D;
                break;
            default:
                return false;
        }

        if (OctiPlayer.areAllied(mode, baseOwner, player)) {
            return false;
        }

        for (OctiPod p : pods) {
            OctiPlayer podPlayer = p.getType().toPlayer();
            if (podPlayer != OctiPlayer.PLAYER_UNDEF
                    && OctiPlayer.areAllied(mode, player, podPlayer)) {
                return true;
            }
        }

        return false;
    }

    public boolean isOccupiedBy(OctiPlayer player) {
        if (player == null || player == OctiPlayer.PLAYER_UNDEF) {
            return false;
        }

        for (OctiPod p : pods) {
            if (p.getType().toPlayer() == player) {
                return true;
            }
        }

        return false;
    }

    public boolean hasEnemyOf(OctiPlayer player) {
        if (player == null || player == OctiPlayer.PLAYER_UNDEF) {
            return false;
        }

        for (OctiPod p : pods) {
            OctiPlayer podPlayer = p.getType().toPlayer();
            if (podPlayer != OctiPlayer.PLAYER_UNDEF
                    && !OctiPlayer.areAllied(mode, player, podPlayer)) {
                return true;
            }
        }

        return false;
    }

    public boolean isEnemyOf(OctiPlayer player) {
        if (player == null || player == OctiPlayer.PLAYER_UNDEF) {
            return false;
        }

        OctiPlayer baseOwner;
        switch (type) {
            case A_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_A;
                break;
            case B_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_B;
                break;
            case C_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_C;
                break;
            case D_HOME_SQUARE:
                baseOwner = OctiPlayer.PLAYER_D;
                break;
            default:
                return false;
        }

        return !OctiPlayer.areAllied(mode, player, baseOwner);
    }

    public boolean hasSameMaterialsAs(OctiSquare other) {
        if (other == null || pods.size() != other.pods.size()) {
            return false;
        }

        List<OctiPod> s1 = new ArrayList<>(pods);
        List<OctiPod> s2 = new ArrayList<>(other.pods);
        Collections.sort(s1);
        Collections.sort(s2);

        return s1.equals(s2);
    }
}
