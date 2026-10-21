/*
 * IllegalMoveException.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class IllegalMoveException extends RuntimeException {

    public IllegalMoveException() {
        super("Illegal move");
    }

    public IllegalMoveException(String message) {
        super(message);
    }

    public IllegalMoveException(String message, Throwable cause) {
        super(message, cause);
    }
}
