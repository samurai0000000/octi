/*
 * OctiBoard.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.List;

public class OctiBoard {

    private final OctiSquare[][] boardSquares = new OctiSquare[9][9];
    private OctiGameMode mode = OctiGameMode.GAME_MODE_UNDEF;
    private int width = 9;
    private int height = 9;
    private OctiGame game = null;
    private final List<OctiBoardListener> listeners = new ArrayList<>();
    private boolean suppressNotification = false;

    public OctiBoard() {
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                boardSquares[x][y] = new OctiSquare(OctiGameMode.GAME_MODE_UNDEF,
                        OctiSquareType.NEUTRAL_SQUARE, x, y);
            }
        }
        setGameMode(OctiGameMode.GAME_MODE_2, false, false, OctiGameBases.GAME_BASES_3);
    }

    public OctiBoard(OctiGameMode mode, boolean useSuperProngs, boolean useEdgelessBoard,
                     OctiGameBases bases) {
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                boardSquares[x][y] = new OctiSquare(OctiGameMode.GAME_MODE_UNDEF,
                        OctiSquareType.NEUTRAL_SQUARE, x, y);
            }
        }
        setGameMode(mode, useSuperProngs, useEdgelessBoard, bases);
    }

    public OctiBoard(OctiGameMode mode) {
        this(mode, false, false, OctiGameBases.GAME_BASES_3);
    }

    public void setGameMode(OctiGameMode mode, boolean useSuperProngs, boolean useEdgelessBoard,
                            OctiGameBases bases) {
        if (mode != OctiGameMode.GAME_MODE_2 && mode != OctiGameMode.GAME_MODE_4
                && mode != OctiGameMode.GAME_MODE_OFK) {
            throw new IllegalMoveException("invalid game mode");
        }

        this.mode = mode;
        this.suppressNotification = false;

        if (this.mode == OctiGameMode.GAME_MODE_OFK) {
            this.width = 6;
            this.height = 7;
            bases = OctiGameBases.GAME_BASES_1;
        } else {
            this.width = 9;
            this.height = 9;
        }

        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                boardSquares[x][y].configure(this.mode, OctiSquareType.NEUTRAL_SQUARE, x, y);
                boardSquares[x][y].clear();
            }
        }

        if (this.mode == OctiGameMode.GAME_MODE_2) {
            boardSquares[2][2].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 2, 2);
            boardSquares[4][2].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 4, 2);
            boardSquares[6][2].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 6, 2);

            boardSquares[2][6].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 2, 6);
            boardSquares[4][6].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 4, 6);
            boardSquares[6][6].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 6, 6);
        } else if (this.mode == OctiGameMode.GAME_MODE_4) {
            boardSquares[2][2].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 2, 2);
            boardSquares[2][6].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 2, 6);
            boardSquares[6][6].configure(this.mode, OctiSquareType.C_HOME_SQUARE, 6, 6);
            boardSquares[6][2].configure(this.mode, OctiSquareType.D_HOME_SQUARE, 6, 2);

            boardSquares[4][2].configure(this.mode, OctiSquareType.NEUTRAL_OCTI_SQUARE, 4, 2);
            boardSquares[4][6].configure(this.mode, OctiSquareType.NEUTRAL_OCTI_SQUARE, 4, 6);
        } else if (this.mode == OctiGameMode.GAME_MODE_OFK) {
            boardSquares[1][1].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 1, 1);
            boardSquares[2][1].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 2, 1);
            boardSquares[3][1].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 3, 1);
            boardSquares[4][1].configure(this.mode, OctiSquareType.A_HOME_SQUARE, 4, 1);

            boardSquares[1][5].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 1, 5);
            boardSquares[2][5].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 2, 5);
            boardSquares[3][5].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 3, 5);
            boardSquares[4][5].configure(this.mode, OctiSquareType.B_HOME_SQUARE, 4, 5);
        }

        OctiGame oldGame = this.game;
        this.game = new OctiGame(this, this.mode, useSuperProngs, useEdgelessBoard, bases);
        notifyGameChanged(oldGame, this.game);
        this.game.reset();
    }

    public void setGameMode(OctiGameMode mode) {
        setGameMode(mode, false, false, OctiGameBases.GAME_BASES_3);
    }

    public void clear() {
        for (int x = 0; x < 9; x++) {
            for (int y = 0; y < 9; y++) {
                boardSquares[x][y].clear();
            }
        }
    }

    public OctiGameMode getGameMode() {
        return mode;
    }

    public int getBoardWidth() {
        return width;
    }

    public int getBoardHeight() {
        return height;
    }

    public boolean isValidCoord(int x, int y) {
        return (x >= 0 && x < width && y >= 0 && y < height);
    }

    public OctiSquare squareAt(int x, int y) {
        if (!isValidCoord(x, y)) {
            throw new IllegalMoveException("square coordinate out of bounds: (" + x + "," + y + ")");
        }
        return boardSquares[x][y];
    }

    public OctiGame getGame() {
        return game;
    }

    public boolean canLiberate(OctiPlayer player) {
        if (mode == OctiGameMode.GAME_MODE_UNDEF || mode == OctiGameMode.GAME_MODE_OFK) {
            return false;
        }

        if (mode == OctiGameMode.GAME_MODE_2) {
            if (player == OctiPlayer.PLAYER_A) {
                return boardSquares[2][6].isOccupiedBy(OctiPlayer.PLAYER_A)
                        || boardSquares[4][6].isOccupiedBy(OctiPlayer.PLAYER_A)
                        || boardSquares[6][6].isOccupiedBy(OctiPlayer.PLAYER_A);
            }
            if (player == OctiPlayer.PLAYER_B) {
                return boardSquares[2][2].isOccupiedBy(OctiPlayer.PLAYER_B)
                        || boardSquares[4][2].isOccupiedBy(OctiPlayer.PLAYER_B)
                        || boardSquares[6][2].isOccupiedBy(OctiPlayer.PLAYER_B);
            }
            return false;
        }

        if (mode == OctiGameMode.GAME_MODE_4) {
            if (boardSquares[4][2].isOccupiedBy(player)
                    || boardSquares[4][2].isCapturedByTeamOf(player)
                    || boardSquares[4][6].isOccupiedBy(player)
                    || boardSquares[4][6].isCapturedByTeamOf(player)) {
                return true;
            }

            if (player == OctiPlayer.PLAYER_A || player == OctiPlayer.PLAYER_C) {
                if (boardSquares[2][6].isOccupiedBy(player)
                        || boardSquares[2][6].isCapturedByTeamOf(player)
                        || boardSquares[6][2].isOccupiedBy(player)
                        || boardSquares[6][2].isCapturedByTeamOf(player)) {
                    return true;
                }
            } else if (player == OctiPlayer.PLAYER_B || player == OctiPlayer.PLAYER_D) {
                if (boardSquares[2][2].isOccupiedBy(player)
                        || boardSquares[2][2].isCapturedByTeamOf(player)
                        || boardSquares[6][6].isOccupiedBy(player)
                        || boardSquares[6][6].isCapturedByTeamOf(player)) {
                    return true;
                }
            }

            if (game != null) {
                if (player == OctiPlayer.PLAYER_A && game.isPlayerEliminated(OctiPlayer.PLAYER_C)) {
                    if (boardSquares[6][6].isOccupiedBy(OctiPlayer.PLAYER_A)) {
                        return true;
                    }
                } else if (player == OctiPlayer.PLAYER_C && game.isPlayerEliminated(OctiPlayer.PLAYER_A)) {
                    if (boardSquares[2][2].isOccupiedBy(OctiPlayer.PLAYER_C)) {
                        return true;
                    }
                } else if (player == OctiPlayer.PLAYER_B && game.isPlayerEliminated(OctiPlayer.PLAYER_D)) {
                    if (boardSquares[6][2].isOccupiedBy(OctiPlayer.PLAYER_B)) {
                        return true;
                    }
                } else if (player == OctiPlayer.PLAYER_D && game.isPlayerEliminated(OctiPlayer.PLAYER_B)) {
                    if (boardSquares[2][6].isOccupiedBy(OctiPlayer.PLAYER_D)) {
                        return true;
                    }
                }
            }
        }

        return false;
    }

    public void addBoardListener(OctiBoardListener listener) {
        if (listener != null && !listeners.contains(listener)) {
            listeners.add(listener);
        }
    }

    public void removeBoardListener(OctiBoardListener listener) {
        listeners.remove(listener);
    }

    public void suppressBoardUpdateNotification(boolean suppress) {
        this.suppressNotification = suppress;
    }

    public boolean isBoardUpdateNotificationSuppressed() {
        return suppressNotification;
    }

    public void notifyBoardTurnUpdate(OctiMove move) {
        if (!suppressNotification) {
            for (OctiBoardListener l : listeners) {
                l.boardUpdated(this, move);
            }
        }
    }

    private void notifyGameChanged(OctiGame oldGame, OctiGame newGame) {
        for (OctiBoardListener l : listeners) {
            l.gameChanged(this, oldGame, newGame);
        }
    }

    public String toPositionString() {
        StringBuilder sb = new StringBuilder();

        // 1. mode
        if (mode == OctiGameMode.GAME_MODE_2) {
            sb.append("2");
        } else if (mode == OctiGameMode.GAME_MODE_4) {
            sb.append("4");
        } else if (mode == OctiGameMode.GAME_MODE_OFK) {
            sb.append("OFK");
        } else {
            sb.append("UNDEF");
        }
        sb.append(":");

        // 2. bases
        sb.append(game != null ? game.getBases().getValue() : 3).append(":");

        // 3. superprongs
        sb.append((game != null && game.useSuperProngs()) ? "1" : "0").append(":");

        // 4. edgeless
        sb.append((game != null && game.useEdgelessBoard()) ? "1" : "0").append(":");

        // 5. status
        if (game == null || !game.isGameStarted()) {
            sb.append("N");
        } else if (game.isGameResigned()) {
            sb.append("R");
        } else if (game.isGameOver()) {
            sb.append("O");
        } else {
            sb.append("P");
        }
        sb.append(":");

        // 6. turn
        if (game == null || !game.isGameStarted() || game.isGameOver()) {
            sb.append("-");
        } else {
            switch (game.getTurn()) {
                case PLAYER_A:
                    sb.append("A");
                    break;
                case PLAYER_B:
                    sb.append("B");
                    break;
                case PLAYER_C:
                    sb.append("C");
                    break;
                case PLAYER_D:
                    sb.append("D");
                    break;
                default:
                    sb.append("-");
                    break;
            }
        }
        sb.append(":");

        // 7. winner
        if (game == null || game.getWinner() == OctiPlayer.PLAYER_UNDEF) {
            sb.append("-");
        } else {
            switch (game.getWinner()) {
                case PLAYER_A:
                    sb.append("A");
                    break;
                case PLAYER_B:
                    sb.append("B");
                    break;
                case PLAYER_C:
                    sb.append("C");
                    break;
                case PLAYER_D:
                    sb.append("D");
                    break;
                default:
                    sb.append("-");
                    break;
            }
        }
        sb.append(":");

        // 8. elimMask
        int elimMask = 0;
        if (mode == OctiGameMode.GAME_MODE_4 && game != null) {
            if (game.isPlayerEliminated(OctiPlayer.PLAYER_A)) elimMask |= 1;
            if (game.isPlayerEliminated(OctiPlayer.PLAYER_B)) elimMask |= 2;
            if (game.isPlayerEliminated(OctiPlayer.PLAYER_C)) elimMask |= 4;
            if (game.isPlayerEliminated(OctiPlayer.PLAYER_D)) elimMask |= 8;
        }
        sb.append(elimMask).append(":");

        // 9. pNRes
        int numPlayers = (mode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int p = 1; p <= numPlayers; p++) {
            OctiPlayer player = OctiPlayer.fromValue(p);
            if (game != null) {
                sb.append(game.getFreePods(player)).append(",")
                  .append(game.getCapturedPods(player)).append(",")
                  .append(game.getFreeProngs(player)).append(",")
                  .append(game.getFreeSuperProngs(player));
            } else {
                sb.append("0,0,0,0");
            }
            sb.append(":");
        }

        // 10. squares: row-major order (y then x)
        boolean firstPod = true;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                OctiSquare sq = boardSquares[x][y];
                for (int i = 0; i < sq.getNumPods(); i++) {
                    if (!firstPod) {
                        sb.append(",");
                    }
                    firstPod = false;

                    OctiPod pod = sq.getPodAt(i);
                    char owner = '?';
                    switch (pod.getType()) {
                        case A_POD:
                            owner = 'A';
                            break;
                        case B_POD:
                            owner = 'B';
                            break;
                        case C_POD:
                            owner = 'C';
                            break;
                        case D_POD:
                            owner = 'D';
                            break;
                        default:
                            break;
                    }
                    sb.append(owner).append(x + 1).append(y + 1).append(pod.getConfigString());
                }
            }
        }

        if (firstPod) {
            sb.append("-");
        }

        return sb.toString();
    }

    public void fromPositionString(String pos) {
        if (pos == null) {
            throw new IllegalMoveSyntaxException("null position string");
        }

        String[] tokens = pos.split(":", -1);
        if (tokens.length < 11) {
            throw new IllegalMoveSyntaxException("malformed OCTI-FEN position string");
        }

        // 1. Mode
        OctiGameMode newMode;
        if ("2".equals(tokens[0])) {
            newMode = OctiGameMode.GAME_MODE_2;
        } else if ("4".equals(tokens[0])) {
            newMode = OctiGameMode.GAME_MODE_4;
        } else if ("OFK".equals(tokens[0])) {
            newMode = OctiGameMode.GAME_MODE_OFK;
        } else {
            throw new IllegalMoveSyntaxException("invalid mode in position string");
        }

        int expectedTokens = (newMode == OctiGameMode.GAME_MODE_4) ? 13 : 11;
        if (tokens.length != expectedTokens) {
            throw new IllegalMoveSyntaxException("token count mismatch in position string");
        }

        // 2. Bases
        OctiGameBases newBases = "1".equals(tokens[1]) ? OctiGameBases.GAME_BASES_1
                                                       : OctiGameBases.GAME_BASES_3;

        // 3. Superprongs
        boolean useSuper = "1".equals(tokens[2]);

        // 4. Edgeless
        boolean useEdgeless = "1".equals(tokens[3]);

        setGameMode(newMode, useSuper, useEdgeless, newBases);
        clear();

        // 5. Status
        char statusChar = tokens[4].isEmpty() ? 'P' : tokens[4].charAt(0);

        // 6. Turn
        OctiPlayer turn = OctiPlayer.PLAYER_UNDEF;
        if (!tokens[5].isEmpty() && !"-".equals(tokens[5])) {
            switch (tokens[5].charAt(0)) {
                case 'A':
                    turn = OctiPlayer.PLAYER_A;
                    break;
                case 'B':
                    turn = OctiPlayer.PLAYER_B;
                    break;
                case 'C':
                    turn = OctiPlayer.PLAYER_C;
                    break;
                case 'D':
                    turn = OctiPlayer.PLAYER_D;
                    break;
                default:
                    turn = OctiPlayer.PLAYER_UNDEF;
                    break;
            }
        }

        // 7. Winner
        OctiPlayer winner = OctiPlayer.PLAYER_UNDEF;
        if (!tokens[6].isEmpty() && !"-".equals(tokens[6])) {
            switch (tokens[6].charAt(0)) {
                case 'A':
                    winner = OctiPlayer.PLAYER_A;
                    break;
                case 'B':
                    winner = OctiPlayer.PLAYER_B;
                    break;
                case 'C':
                    winner = OctiPlayer.PLAYER_C;
                    break;
                case 'D':
                    winner = OctiPlayer.PLAYER_D;
                    break;
                default:
                    winner = OctiPlayer.PLAYER_UNDEF;
                    break;
            }
        }

        // 8. Eliminated mask
        int elimMask = Integer.parseInt(tokens[7]);

        OctiGameSnapshot snap = new OctiGameSnapshot();
        snap.gameStarted = (statusChar != 'N');
        snap.gameResigned = (statusChar == 'R');
        snap.gameOver = (statusChar == 'O' || statusChar == 'R');
        snap.turn = turn;
        snap.winner = winner;

        if (newMode == OctiGameMode.GAME_MODE_4) {
            snap.players[OctiPlayer.PLAYER_A.getValue()].eliminated = ((elimMask & 1) != 0);
            snap.players[OctiPlayer.PLAYER_B.getValue()].eliminated = ((elimMask & 2) != 0);
            snap.players[OctiPlayer.PLAYER_C.getValue()].eliminated = ((elimMask & 4) != 0);
            snap.players[OctiPlayer.PLAYER_D.getValue()].eliminated = ((elimMask & 8) != 0);
        }

        // 9. Player resources
        int numPlayers = (newMode == OctiGameMode.GAME_MODE_4) ? 4 : 2;
        for (int p = 1; p <= numPlayers; p++) {
            String resStr = tokens[7 + p];
            String[] parts = resStr.split(",", -1);
            int[] vals = new int[4];
            for (int vi = 0; vi < parts.length && vi < 4; vi++) {
                vals[vi] = Integer.parseInt(parts[vi]);
            }
            snap.players[p].freePods = vals[0];
            snap.players[p].capturedPods = vals[1];
            snap.players[p].freeProngs = vals[2];
            snap.players[p].freeSuperProngs = vals[3];
        }

        // 10. Squares
        String squaresStr = tokens[tokens.length - 1];
        if (!squaresStr.isEmpty() && !"-".equals(squaresStr)) {
            String[] podTokens = squaresStr.split(",", -1);
            for (String podToken : podTokens) {
                if (podToken.length() < 3) {
                    continue;
                }
                char ownerChar = podToken.charAt(0);
                char xChar = podToken.charAt(1);
                char yChar = podToken.charAt(2);

                OctiPlayer owner = OctiPlayer.PLAYER_UNDEF;
                switch (ownerChar) {
                    case 'A':
                        owner = OctiPlayer.PLAYER_A;
                        break;
                    case 'B':
                        owner = OctiPlayer.PLAYER_B;
                        break;
                    case 'C':
                        owner = OctiPlayer.PLAYER_C;
                        break;
                    case 'D':
                        owner = OctiPlayer.PLAYER_D;
                        break;
                    default:
                        break;
                }

                int x = xChar - '1';
                int y = yChar - '1';
                if (!isValidCoord(x, y)) {
                    throw new IllegalMoveSyntaxException(
                            "square coordinate out of bounds in position string");
                }

                OctiPod pod = new OctiPod(owner.toPodType());
                int idx = 3;
                while (idx < podToken.length()) {
                    char dChar = podToken.charAt(idx++);
                    OctiDir dir = OctiDir.charToDir(dChar);
                    if (dir == OctiDir.DIR_UNDEF) {
                        break;
                    }
                    boolean superProng = false;
                    if (idx < podToken.length() && podToken.charAt(idx) == '!') {
                        superProng = true;
                        idx++;
                    }
                    pod.addProng(dir, superProng);
                }

                squareAt(x, y).addPod(pod);
                snap.squares[x][y].add(pod);
            }
        }

        if (game != null) {
            game.restoreSnapshot(snap);
        }
    }

    public String serializeGame() {
        if (game != null) {
            return game.serialize();
        }
        return "";
    }

    public void deserializeGame(String transcript) {
        if (transcript == null) {
            return;
        }

        OctiGameMode parsedMode = OctiGameMode.GAME_MODE_2;
        OctiGameBases parsedBases = OctiGameBases.GAME_BASES_3;
        boolean useSuper = false;
        boolean useEdgeless = false;
        OctiPlayer startPlayer = OctiPlayer.PLAYER_A;
        String statusStr = "IN_PROGRESS";
        OctiPlayer winner = OctiPlayer.PLAYER_UNDEF;
        List<String> moveLines = new ArrayList<>();
        boolean inMoves = false;

        try (BufferedReader reader = new BufferedReader(new StringReader(transcript))) {
            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith(";") || line.startsWith("#")) {
                    continue;
                }

                if (inMoves) {
                    int bracePos = line.indexOf('{');
                    if (bracePos != -1) {
                        line = line.substring(0, bracePos).trim();
                    }
                    int dotPos = line.indexOf('.');
                    if (dotPos != -1 && dotPos + 1 < line.length()) {
                        line = line.substring(dotPos + 1).trim();
                        if (line.length() >= 2 && (line.charAt(0) >= 'A' && line.charAt(0) <= 'D')
                                && (line.charAt(1) == ' ' || line.charAt(1) == ':')) {
                            line = line.substring(2).trim();
                        }
                    }
                    if (!line.isEmpty()) {
                        moveLines.add(line);
                    }
                    continue;
                }

                int colonPos = line.indexOf(':');
                if (colonPos != -1) {
                    String key = line.substring(0, colonPos).trim();
                    String val = line.substring(colonPos + 1).trim();

                    if ("GAME_MODE".equals(key)) {
                        if ("2".equals(val) || "1".equals(val)) {
                            parsedMode = OctiGameMode.GAME_MODE_2;
                        } else if ("4".equals(val)) {
                            parsedMode = OctiGameMode.GAME_MODE_4;
                        } else if ("OFK".equalsIgnoreCase(val) || "3".equals(val)) {
                            parsedMode = OctiGameMode.GAME_MODE_OFK;
                        }
                    } else if ("GAME_BASES".equals(key)) {
                        parsedBases = "1".equals(val) ? OctiGameBases.GAME_BASES_1
                                                      : OctiGameBases.GAME_BASES_3;
                    } else if ("USE_SUPER_PRONGS".equals(key)) {
                        useSuper = ("Yes".equalsIgnoreCase(val) || "1".equals(val)
                                || "true".equalsIgnoreCase(val));
                    } else if ("USE_EDGELESS_BOARD".equals(key)) {
                        useEdgeless = ("Yes".equalsIgnoreCase(val) || "1".equals(val)
                                || "true".equalsIgnoreCase(val));
                    } else if ("START_PLAYER".equals(key)) {
                        if ("A".equals(val) || "1".equals(val)) startPlayer = OctiPlayer.PLAYER_A;
                        else if ("B".equals(val) || "2".equals(val)) startPlayer = OctiPlayer.PLAYER_B;
                        else if ("C".equals(val) || "3".equals(val)) startPlayer = OctiPlayer.PLAYER_C;
                        else if ("D".equals(val) || "4".equals(val)) startPlayer = OctiPlayer.PLAYER_D;
                    } else if ("STATUS".equals(key)) {
                        statusStr = val;
                    } else if ("WINNER".equals(key)) {
                        if ("A".equals(val) || "1".equals(val)) winner = OctiPlayer.PLAYER_A;
                        else if ("B".equals(val) || "2".equals(val)) winner = OctiPlayer.PLAYER_B;
                        else if ("C".equals(val) || "3".equals(val)) winner = OctiPlayer.PLAYER_C;
                        else if ("D".equals(val) || "4".equals(val)) winner = OctiPlayer.PLAYER_D;
                        else winner = OctiPlayer.PLAYER_UNDEF;
                    } else if ("MOVES".equals(key)) {
                        inMoves = true;
                    }
                }
            }
        } catch (IOException e) {
            throw new RuntimeException("failed to read transcript", e);
        }

        setGameMode(parsedMode, useSuper, useEdgeless, parsedBases);
        clear();
        game.reset();

        if (!"NOT_STARTED".equals(statusStr)) {
            game.start(startPlayer);
            for (String mStr : moveLines) {
                game.move(mStr);
            }
            if ("RESIGNED".equals(statusStr)) {
                game.resign(winner == OctiPlayer.PLAYER_A ? OctiPlayer.PLAYER_B
                                                         : OctiPlayer.PLAYER_A);
            }
        }
    }

    @Override
    public String toString() {
        int w = getBoardWidth();
        int h = getBoardHeight();
        StringBuilder sb = new StringBuilder();

        sb.append("  ");
        for (int x = 0; x < w; x++) {
            sb.append("   ").append(x + 1).append("  ");
        }
        sb.append("\n");

        for (int y = h - 1; y >= 0; y--) {
            sb.append("  +");
            for (int x = 0; x < w; x++) {
                sb.append("-----+");
            }
            sb.append("\n");

            sb.append(y + 1).append(" |");
            for (int x = 0; x < w; x++) {
                OctiSquare sq = squareAt(x, y);
                if (sq.isEmpty()) {
                    if (sq.getType() == OctiSquareType.A_HOME_SQUARE) {
                        sb.append(" (A) |");
                    } else if (sq.getType() == OctiSquareType.B_HOME_SQUARE) {
                        sb.append(" (B) |");
                    } else if (sq.getType() == OctiSquareType.C_HOME_SQUARE) {
                        sb.append(" (C) |");
                    } else if (sq.getType() == OctiSquareType.D_HOME_SQUARE) {
                        sb.append(" (D) |");
                    } else if (sq.getType() == OctiSquareType.NEUTRAL_OCTI_SQUARE) {
                        sb.append(" (*) |");
                    } else {
                        sb.append("     |");
                    }
                } else {
                    char owner = ' ';
                    switch (sq.getPodAt(sq.getNumPods() - 1).getType()) {
                        case A_POD:
                            owner = 'A';
                            break;
                        case B_POD:
                            owner = 'B';
                            break;
                        case C_POD:
                            owner = 'C';
                            break;
                        case D_POD:
                            owner = 'D';
                            break;
                        default:
                            break;
                    }
                    if (sq.getNumPods() == 1) {
                        String cfg = sq.getPodAt(0).getConfigString();
                        if (cfg.length() > 3) {
                            cfg = cfg.substring(0, 3);
                        }
                        while (cfg.length() < 3) {
                            cfg += " ";
                        }
                        sb.append(" ").append(owner).append(":").append(cfg).append("|");
                    } else {
                        sb.append(" ").append(owner).append("[").append(sq.getNumPods()).append("] |");
                    }
                }
            }
            sb.append(" ").append(y + 1).append("\n");
        }

        sb.append("  +");
        for (int x = 0; x < w; x++) {
            sb.append("-----+");
        }
        sb.append("\n");

        sb.append("  ");
        for (int x = 0; x < w; x++) {
            sb.append("   ").append(x + 1).append("  ");
        }
        sb.append("\n");

        return sb.toString();
    }
}
