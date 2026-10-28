/*
 * OctiMove.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

public class OctiMove {

    private final List<OctiMoveOp> ops = new ArrayList<>();
    private String str = "";

    public OctiMove() {
    }

    public OctiMove(String str) {
        if (str == null) {
            throw new IllegalMoveSyntaxException("null move string");
        }
        parseMoveString(str, this.ops);
        this.str = formatOps(this.ops);
    }

    public OctiMove(List<OctiMoveOp> ops) {
        if (ops == null || ops.isEmpty()) {
            this.str = "";
            return;
        }

        for (OctiMoveOp op : ops) {
            this.ops.add(new OctiMoveOp(op));
        }

        boolean seenRegular = false;
        int bonusCount = 0;
        int firstRegX0 = -1;
        int firstRegY0 = -1;
        int regularCount = 0;

        for (OctiMoveOp op : this.ops) {
            if (op.x0 < 0 || op.x0 > 8 || op.y0 < 0 || op.y0 > 8) {
                throw new IllegalMoveSyntaxException("op coordinate out of bounds");
            }

            if (op.isSuperProngBonusMove) {
                if (seenRegular) {
                    throw new IllegalMoveSyntaxException(
                            "superprong bonus move cannot appear after regular move");
                }
                bonusCount++;
                if (bonusCount > 2) {
                    throw new IllegalMoveSyntaxException("too many superprong bonus moves");
                }
                if (op.hops.isEmpty() || op.addPod
                        || op.addProngDir != OctiDir.DIR_UNDEF
                        || op.rmProngDir != OctiDir.DIR_UNDEF) {
                    throw new IllegalMoveSyntaxException(
                            "superprong bonus move must be a hop chain");
                }
            } else {
                seenRegular = true;
                regularCount++;
                if (regularCount == 1) {
                    firstRegX0 = op.x0;
                    firstRegY0 = op.y0;
                } else {
                    if (op.x0 != firstRegX0 || op.y0 != firstRegY0) {
                        throw new IllegalMoveSyntaxException(
                                "composite regular moves must share the same starting coordinate");
                    }
                    if (op.hops.isEmpty() || op.addPod
                            || op.addProngDir != OctiDir.DIR_UNDEF
                            || op.rmProngDir != OctiDir.DIR_UNDEF) {
                        throw new IllegalMoveSyntaxException(
                                "composite moves must consist solely of hop chains");
                    }
                }
            }

            if (op.addPod) {
                if (!op.hops.isEmpty() || op.addProngDir != OctiDir.DIR_UNDEF
                        || op.rmProngDir != OctiDir.DIR_UNDEF) {
                    throw new IllegalMoveSyntaxException("invalid pod placement op structure");
                }
            } else if (op.addProngDir != OctiDir.DIR_UNDEF
                    && op.rmProngDir != OctiDir.DIR_UNDEF) {
                if (op.addProngDir == op.rmProngDir || op.superProng
                        || !op.hops.isEmpty()) {
                    throw new IllegalMoveSyntaxException("invalid reposition op structure");
                }
                if (op.pod.prongAt(op.rmProngDir) != OctiProngType.NORMAL_PRONG) {
                    throw new IllegalMoveSyntaxException(
                            "cannot reposition non-existent or superprong");
                }
                if (op.pod.prongAt(op.addProngDir) != OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException(
                            "cannot reposition to already occupied prong slot");
                }
            } else if (op.addProngDir != OctiDir.DIR_UNDEF) {
                if (!op.hops.isEmpty()) {
                    throw new IllegalMoveSyntaxException("invalid prong add op structure");
                }
                if (op.pod.prongAt(op.addProngDir) != OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException(
                            "cannot add prong to already occupied slot");
                }
            } else if (!op.hops.isEmpty()) {
                for (OctiHop h : op.hops) {
                    if (h.x < 0 || h.x > 8 || h.y < 0 || h.y > 8) {
                        throw new IllegalMoveSyntaxException("hop coordinate out of bounds");
                    }
                }
            } else {
                throw new IllegalMoveSyntaxException("unrecognized move op structure");
            }
        }

        if (regularCount == 0 && bonusCount > 0) {
            throw new IllegalMoveSyntaxException("superprong prelude without regular move");
        }

        this.str = formatOps(this.ops);
    }

    public List<OctiMoveOp> getOps() {
        return Collections.unmodifiableList(ops);
    }

    public int size() {
        return ops.size();
    }

    public boolean isEmpty() {
        return ops.isEmpty();
    }

    public String asString() {
        return str;
    }

    public String format() {
        return str;
    }

    private static String trimWhitespace(String s) {
        if (s == null) {
            return "";
        }
        int start = 0;
        while (start < s.length() && Character.isWhitespace(s.charAt(start))) {
            start++;
        }
        if (start == s.length()) {
            return "";
        }
        int end = s.length();
        while (end > start && Character.isWhitespace(s.charAt(end - 1))) {
            end--;
        }
        return s.substring(start, end);
    }

    private static OctiMoveOp parseSingleRegularOp(String s) {
        if (s.length() < 2) {
            throw new IllegalMoveSyntaxException("move operation too short");
        }

        char c0 = s.charAt(0);
        char c1 = s.charAt(1);
        if (c0 < '1' || c0 > '9' || c1 < '1' || c1 > '9') {
            throw new IllegalMoveSyntaxException("invalid start coordinate digits");
        }

        OctiMoveOp op = new OctiMoveOp();
        op.x0 = c0 - '1';
        op.y0 = c1 - '1';

        if (s.length() == 2) {
            op.addPod = true;
            op.liberate = false;
            return op;
        }

        if (s.length() == 3 && s.charAt(2) == '*') {
            op.addPod = true;
            op.liberate = true;
            return op;
        }

        int idx = 2;
        boolean[] seenDir = new boolean[OctiDir.DIR_MAX];

        while (idx < s.length()) {
            char c = s.charAt(idx);
            OctiDir dir = OctiDir.charToDir(c);
            if (dir == OctiDir.DIR_UNDEF) {
                break;
            }

            if (seenDir[dir.getIndex()]) {
                throw new IllegalMoveSyntaxException("duplicate direction in pod configuration");
            }
            seenDir[dir.getIndex()] = true;
            idx++;

            boolean isSuper = false;
            if (idx < s.length() && s.charAt(idx) == '!') {
                isSuper = true;
                idx++;
            }
            op.pod.addProng(dir, isSuper);
        }

        if (idx >= s.length()) {
            throw new IllegalMoveSyntaxException("incomplete move syntax after pod config");
        }

        char action = s.charAt(idx);
        if (action == '+') {
            idx++;
            if (idx >= s.length()) {
                throw new IllegalMoveSyntaxException("expected direction after '+'");
            }
            OctiDir addDir = OctiDir.charToDir(s.charAt(idx));
            if (addDir == OctiDir.DIR_UNDEF) {
                throw new IllegalMoveSyntaxException("invalid direction after '+'");
            }
            idx++;

            boolean addSuper = false;
            if (idx < s.length() && s.charAt(idx) == '!') {
                addSuper = true;
                idx++;
            }

            if (idx == s.length()) {
                if (op.pod.prongAt(addDir) != OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot add prong that already exists");
                }
                op.addProngDir = addDir;
                op.superProng = addSuper;
                return op;
            }

            if (s.charAt(idx) == '-') {
                idx++;
                if (idx >= s.length()) {
                    throw new IllegalMoveSyntaxException("expected direction after '-' in reposition");
                }
                OctiDir rmDir = OctiDir.charToDir(s.charAt(idx));
                if (rmDir == OctiDir.DIR_UNDEF) {
                    throw new IllegalMoveSyntaxException("invalid remove direction in reposition");
                }
                idx++;

                if (idx < s.length() && s.charAt(idx) == '!') {
                    throw new IllegalMoveSyntaxException("superprong cannot be removed in reposition");
                }
                if (addSuper) {
                    throw new IllegalMoveSyntaxException("superprong cannot be added in reposition");
                }
                if (idx != s.length()) {
                    throw new IllegalMoveSyntaxException("extra trailing characters after reposition");
                }

                if (addDir == rmDir) {
                    throw new IllegalMoveSyntaxException(
                            "reposition with identical add and remove directions");
                }
                if (op.pod.prongAt(rmDir) == OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot remove prong that does not exist");
                }
                if (op.pod.prongAt(rmDir) == OctiProngType.SUPER_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot reposition a superprong");
                }
                if (op.pod.prongAt(addDir) != OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot add prong that already exists");
                }

                op.addProngDir = addDir;
                op.rmProngDir = rmDir;
                op.superProng = false;
                return op;
            }

            throw new IllegalMoveSyntaxException("unexpected character after prong add direction");
        }

        if (action == '-') {
            idx++;
            if (idx >= s.length()) {
                throw new IllegalMoveSyntaxException("unexpected end of string after '-'");
            }

            char nextC = s.charAt(idx);
            if ((nextC >= 'A' && nextC <= 'H') || (nextC >= 'a' && nextC <= 'h')) {
                OctiDir rmDir = OctiDir.charToDir(nextC);
                if (rmDir == OctiDir.DIR_UNDEF) {
                    throw new IllegalMoveSyntaxException("invalid remove direction in reposition");
                }
                idx++;

                if (idx < s.length() && s.charAt(idx) == '!') {
                    throw new IllegalMoveSyntaxException("superprong cannot be removed in reposition");
                }
                if (idx >= s.length() || s.charAt(idx) != '+') {
                    throw new IllegalMoveSyntaxException("expected '+' after remove direction in reposition");
                }
                idx++;

                if (idx >= s.length()) {
                    throw new IllegalMoveSyntaxException("expected add direction after '+' in reposition");
                }
                OctiDir addDir = OctiDir.charToDir(s.charAt(idx));
                if (addDir == OctiDir.DIR_UNDEF) {
                    throw new IllegalMoveSyntaxException("invalid add direction in reposition");
                }
                idx++;

                if (idx < s.length() && s.charAt(idx) == '!') {
                    throw new IllegalMoveSyntaxException("superprong cannot be added in reposition");
                }
                if (idx != s.length()) {
                    throw new IllegalMoveSyntaxException("extra trailing characters after reposition");
                }

                if (addDir == rmDir) {
                    throw new IllegalMoveSyntaxException(
                            "reposition with identical add and remove directions");
                }
                if (op.pod.prongAt(rmDir) == OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot remove prong that does not exist");
                }
                if (op.pod.prongAt(rmDir) == OctiProngType.SUPER_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot reposition a superprong");
                }
                if (op.pod.prongAt(addDir) != OctiProngType.NO_PRONG) {
                    throw new IllegalMoveSyntaxException("cannot add prong that already exists");
                }

                op.addProngDir = addDir;
                op.rmProngDir = rmDir;
                op.superProng = false;
                return op;
            }

            if (nextC >= '1' && nextC <= '9') {
                while (true) {
                    if (idx + 1 >= s.length()) {
                        throw new IllegalMoveSyntaxException("truncated hop destination coordinate");
                    }
                    char hx = s.charAt(idx);
                    char hy = s.charAt(idx + 1);
                    if (hx < '1' || hx > '9' || hy < '1' || hy > '9') {
                        throw new IllegalMoveSyntaxException("invalid hop destination coordinate digits");
                    }
                    int hopX = hx - '1';
                    int hopY = hy - '1';
                    idx += 2;

                    boolean capture = false;
                    if (idx < s.length() && (s.charAt(idx) == 'x' || s.charAt(idx) == 'X')) {
                        capture = true;
                        idx++;
                        if (idx < s.length() && (s.charAt(idx) == 'x' || s.charAt(idx) == 'X')) {
                            throw new IllegalMoveSyntaxException("duplicate capture marker");
                        }
                    }

                    op.hops.add(new OctiHop(hopX, hopY, capture));

                    if (idx >= s.length()) {
                        break;
                    }
                    if (s.charAt(idx) != '-') {
                        throw new IllegalMoveSyntaxException("unexpected character in hop chain");
                    }
                    idx++;
                }

                if (op.hops.isEmpty()) {
                    throw new IllegalMoveSyntaxException("hop chain has no hops");
                }
                return op;
            }

            throw new IllegalMoveSyntaxException("invalid character following '-'");
        }

        throw new IllegalMoveSyntaxException("unrecognized move operation character");
    }

    private static OctiMoveOp parseSingleHopChain(String s) {
        OctiMoveOp op = parseSingleRegularOp(s);
        if (op.hops.isEmpty() || op.addPod || op.addProngDir != OctiDir.DIR_UNDEF
                || op.rmProngDir != OctiDir.DIR_UNDEF) {
            throw new IllegalMoveSyntaxException("expected hop chain operation");
        }
        return op;
    }

    public static void parseMoveString(String str, List<OctiMoveOp> outOps) {
        if (outOps == null) {
            return;
        }
        outOps.clear();

        if (str == null) {
            throw new IllegalMoveSyntaxException("null move string");
        }
        String s = trimWhitespace(str);
        if (s.isEmpty()) {
            throw new IllegalMoveSyntaxException("empty move string");
        }

        List<String> semiParts = new ArrayList<>();
        int start = 0;
        while (true) {
            int pos = s.indexOf(';', start);
            if (pos == -1) {
                semiParts.add(s.substring(start));
                break;
            }
            semiParts.add(s.substring(start, pos));
            start = pos + 1;
        }

        if (semiParts.size() > 3) {
            throw new IllegalMoveSyntaxException("more than 2 superprong prelude operations");
        }

        for (int i = 0; i < semiParts.size(); i++) {
            String trimmed = trimWhitespace(semiParts.get(i));
            if (trimmed.isEmpty()) {
                throw new IllegalMoveSyntaxException("empty segment around ';'");
            }
            semiParts.set(i, trimmed);
        }

        for (int i = 0; i + 1 < semiParts.size(); i++) {
            if (semiParts.get(i).indexOf(',') != -1) {
                throw new IllegalMoveSyntaxException("superprong prelude move cannot be composite");
            }
            OctiMoveOp op = parseSingleHopChain(semiParts.get(i));
            op.isSuperProngBonusMove = true;
            outOps.add(op);
        }

        String regStr = semiParts.get(semiParts.size() - 1);
        if (regStr.indexOf(',') != -1) {
            List<String> commaParts = new ArrayList<>();
            int cStart = 0;
            while (true) {
                int cPos = regStr.indexOf(',', cStart);
                if (cPos == -1) {
                    commaParts.add(regStr.substring(cStart));
                    break;
                }
                commaParts.add(regStr.substring(cStart, cPos));
                cStart = cPos + 1;
            }

            int commonX0 = -1;
            int commonY0 = -1;

            for (int i = 0; i < commaParts.size(); i++) {
                String cTrimmed = trimWhitespace(commaParts.get(i));
                if (cTrimmed.isEmpty()) {
                    throw new IllegalMoveSyntaxException("empty composite operation around ','");
                }
                OctiMoveOp op = parseSingleHopChain(cTrimmed);
                op.isSuperProngBonusMove = false;
                if (i == 0) {
                    commonX0 = op.x0;
                    commonY0 = op.y0;
                } else {
                    if (op.x0 != commonX0 || op.y0 != commonY0) {
                        throw new IllegalMoveSyntaxException(
                                "composite move operations must share the same starting coordinate");
                    }
                }
                outOps.add(op);
            }
        } else {
            OctiMoveOp op = parseSingleRegularOp(regStr);
            op.isSuperProngBonusMove = false;
            outOps.add(op);
        }
    }

    public static String formatOps(List<OctiMoveOp> ops) {
        if (ops == null || ops.isEmpty()) {
            return "";
        }

        StringBuilder result = new StringBuilder();
        for (int i = 0; i < ops.size(); i++) {
            OctiMoveOp op = ops.get(i);

            result.append((char) ('1' + op.x0));
            result.append((char) ('1' + op.y0));

            if (op.addPod) {
                if (op.liberate) {
                    result.append('*');
                }
            } else if (op.addProngDir != OctiDir.DIR_UNDEF
                    && op.rmProngDir != OctiDir.DIR_UNDEF) {
                result.append(op.pod.getConfigString());
                result.append('+');
                result.append(op.addProngDir.toChar());
                result.append('-');
                result.append(op.rmProngDir.toChar());
            } else if (op.addProngDir != OctiDir.DIR_UNDEF) {
                result.append(op.pod.getConfigString());
                result.append('+');
                result.append(op.addProngDir.toChar());
                if (op.superProng) {
                    result.append('!');
                }
            } else if (!op.hops.isEmpty()) {
                result.append(op.pod.getConfigString());
                for (OctiHop h : op.hops) {
                    result.append('-');
                    result.append((char) ('1' + h.x));
                    result.append((char) ('1' + h.y));
                    if (h.capture) {
                        result.append('x');
                    }
                }
            }

            if (i + 1 < ops.size()) {
                if (op.isSuperProngBonusMove) {
                    result.append(';');
                } else {
                    result.append(',');
                }
            }
        }

        return result.toString();
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!(obj instanceof OctiMove)) {
            return false;
        }
        OctiMove other = (OctiMove) obj;
        return Objects.equals(this.ops, other.ops);
    }

    @Override
    public int hashCode() {
        return Objects.hash(ops);
    }

    @Override
    public String toString() {
        return str;
    }
}
