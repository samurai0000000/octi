/*
 * IllegalMoveSyntaxException.java
 *
 * Copyright (C) 2026, Charles Chiou
 */

package com.selfso.octi;

public class IllegalMoveSyntaxException extends RuntimeException {

    public IllegalMoveSyntaxException() {
        super("Illegal move syntax");
    }

    public IllegalMoveSyntaxException(String message) {
        super(message);
    }

    public IllegalMoveSyntaxException(String message, Throwable cause) {
        super(message, cause);
    }
}
