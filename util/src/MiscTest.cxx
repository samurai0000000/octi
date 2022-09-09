/*************************************************************************
 * $Id: MiscTest.cxx,v 1.6 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 11/27/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include "String.hxx"
#include "StringTokenizer.hxx"
#include "Stack.hxx"
#include "Queue.hxx"

#define VERIFY_STRING(X, Y)                             \
    if(X != Y) {                                        \
        cout << endl;                                   \
        cout << "Failed for \"" << X << "\"" << endl;   \
        cout << "expecting  \"" << Y << "\"" << endl;   \
        return;                                         \
    }

#define FAIL_STR_IDX(MSG)                       \
    cout << "Failed for " << MSG << endl;       \
    return;

#define COMP_TOKEN(X, Y)                                    \
    if((X == NULL && Y != NULL) ||                          \
       (X != NULL && Y == NULL) ||                          \
       (X != NULL && Y != NULL && strcmp(Y, X) != 0)) {     \
        cout << "failed for " << Y << " != " << X << endl;  \
        return;                                             \
    }

__USING_NAMESPACE(SELFSOFT);

/**************************************************************************/
/* String Test                                                            */
/**************************************************************************/

void doStringTest() {
    cout << "String test ... " << flush;
    String s = "abc";

    VERIFY_STRING(s, "abc");
    VERIFY_STRING(String('a'), "a");
    VERIFY_STRING(String("xyz"), "xyz");
    VERIFY_STRING(String('a'), "a");
    VERIFY_STRING(String(s), "abc");

    VERIFY_STRING(s + 'x', "abcx");
    VERIFY_STRING(s + "xy", "abcxy");
    VERIFY_STRING(s + s, "abcabc");

    s.append('d');
    VERIFY_STRING(s, "abcd");
    s.append("ef");
    VERIFY_STRING(s, "abcdef");
    s.append(s);
    VERIFY_STRING(s, "abcdefabcdef");
    s.prepend('i');
    VERIFY_STRING(s, "iabcdefabcdef");
    s.prepend("jk");
    VERIFY_STRING(s, "jkiabcdefabcdef");
    s.prepend(s);
    VERIFY_STRING(s, "jkiabcdefabcdefjkiabcdefabcdef");
    s.insert('u', 0);
    VERIFY_STRING(s, "ujkiabcdefabcdefjkiabcdefabcdef");
    s.insert('v', 5);
    VERIFY_STRING(s, "ujkiavbcdefabcdefjkiabcdefabcdef");
    s.insert("shit", 10);
    VERIFY_STRING(s, "ujkiavbcdeshitfabcdefjkiabcdefabcdef");
    s.insert(s, 4);
    VERIFY_STRING(s, "ujkiujkiavbcdeshitfabcdefjkiabcdefabcdefavbcdeshitfabcdefjkiabcdefabcdef");

    s.remove(0, 10);
    VERIFY_STRING(s, "cdeshitfabcdefjkiabcdefabcdefavbcdeshitfabcdefjkiabcdefabcdef");
    s.remove(10, 15);
    VERIFY_STRING(s, "cdeshitfabiabcdefabcdefavbcdeshitfabcdefjkiabcdefabcdef");

    s.delFirst('c');
    VERIFY_STRING(s, "deshitfabiabcdefabcdefavbcdeshitfabcdefjkiabcdefabcdef");
    s.delFirst("shit");
    VERIFY_STRING(s, "defabiabcdefabcdefavbcdeshitfabcdefjkiabcdefabcdef");
    s.delFirst('d', 10);
    VERIFY_STRING(s, "defabiabcdefabcefavbcdeshitfabcdefjkiabcdefabcdef");
    s.delFirst("def", 2);
    VERIFY_STRING(s, "defabiabcabcefavbcdeshitfabcdefjkiabcdefabcdef");
    s.delLast('a');
    VERIFY_STRING(s, "defabiabcabcefavbcdeshitfabcdefjkiabcdefbcdef");
    s.delLast("ab");
    VERIFY_STRING(s, "defabiabcabcefavbcdeshitfabcdefjkicdefbcdef");
    s.delLast('f', 20);
    VERIFY_STRING(s, "defabiabcabceavbcdeshitfabcdefjkicdefbcdef");
    s.delLast("bc", 24);
    VERIFY_STRING(s, "defabiabcabceavdeshitfabcdefjkicdefbcdef");

    if(!s.startsWith('d')) {
        FAIL_STR_IDX("startsWith('d')");
    } else if(!s.startsWith("defab")) {
        FAIL_STR_IDX("startsWith(\"defab\")");
    } else if(!s.endsWith('f')) {
        FAIL_STR_IDX("endsWith('f')");
    } else if(!s.endsWith("bcdef")) {
        FAIL_STR_IDX("endsWith(\"bcdef\")");
    } else if(!s.contains("shit")) {
        FAIL_STR_IDX("contains(\"shit\")");
    } else if(s.contains("shit", 25)) {
        FAIL_STR_IDX("contains(\"shit\", 25)");
    }

    VERIFY_STRING(s.before(5), "defab");
    VERIFY_STRING(s.after(5), "abcabceavdeshitfabcdefjkicdefbcdef");
    VERIFY_STRING(s.substring(10, 20), "bceavdeshit");

    s = "   whaT a bAbe!   ";
    s.trim();
    VERIFY_STRING(s, "whaT a bAbe!");
    VERIFY_STRING(s.getLowerCaseForm(), "what a babe!");
    VERIFY_STRING(s.getUpperCaseForm(), "WHAT A BABE!");
    VERIFY_STRING(s.getReversed(), "!ebAb a Tahw");
    VERIFY_STRING(s.getReplaced('b', 'z'), "whaT a zAze!");

    String f;
    f.format("Hello %d, %s dude", 69, "surfer");
    VERIFY_STRING(f, "Hello 69, surfer dude");

    cout << "passed" << endl;
}

/**************************************************************************/
/* StringTokenizer Test                                                   */
/**************************************************************************/

void doStringTokenizerTest() {
    cout << "StringTokenizer test ... " << flush;

    StringTokenizer tok("Speak, I charge thee!", " ,!");
    const char *t;

    for(int i = 0; i < tok.countTokens(); i++) {
        switch(i) {
        case 0:
            t = tok.nextToken(); COMP_TOKEN(t, "Speak");
            break;
        case 1:
            t = tok.nextToken(); COMP_TOKEN(t, "I");
            break;
        case 2:
            t = tok.nextToken(); COMP_TOKEN(t, "charge");
            break;
        case 3:
            t = tok.nextToken(); COMP_TOKEN(t, "thee");
            break;
        default:
            cout << "failed  here" << endl;
            return;
        }
    }

    tok.setString("la la la, I like \"singing\" in the showers!, why,,!!! not?");
    t = tok.nextToken(); COMP_TOKEN(t, "la");
    t = tok.nextToken(); COMP_TOKEN(t, "la");
    t = tok.nextToken(); COMP_TOKEN(t, "la");
    t = tok.nextToken(); COMP_TOKEN(t, "I");
    t = tok.nextToken(); COMP_TOKEN(t, "like");
    t = tok.nextToken(); COMP_TOKEN(t, "\"singing\"");
    t = tok.nextToken(); COMP_TOKEN(t, "in");
    t = tok.nextToken(); COMP_TOKEN(t, "the");
    t = tok.nextToken(); COMP_TOKEN(t, "showers");
    t = tok.nextToken(); COMP_TOKEN(t, "why");
    t = tok.nextToken(); COMP_TOKEN(t, "not?");
    t = tok.nextToken(); COMP_TOKEN(t, (char *) NULL);
    tok.reset();
    t = tok.nextToken(); COMP_TOKEN(t, "la");
    t = tok.nextToken(); COMP_TOKEN(t, "la");

    cout << "passed." << endl;
}

/**************************************************************************/
/* Stack Test                                                             */
/**************************************************************************/
void doStackTest() {
    int stack_test[5] = { 11, 22, 33, 44, 55 };

    cout << "Stack test ... " << flush;

    Stack<int> stack(5);
    int i, p;
    for(i = 0; i < 5; i++) {
        if(stack.push(stack_test[i]) != 0) {
            cout << "push failed for element " << i + 1 << " (failure)" << endl;
            return;
        }
    }

    if(stack.push(66) == 0) {
        cout << "push exceeded capacity! (failure)" << endl;
        return;
    }

    for(i = 4; i >= 0; i--) {
        if(stack.pop(p) != 0) {
            cout << "pop failed for element " << i + 1 << " (failure)" << endl;
            return;
        } else if(p != stack_test[i]) {
            cout << "poped value incosistent with element " << i + 1 <<
                " (failure)" << endl;
            return;
        }
    }

    if(stack.pop(p) == 0) {
        cout << "popped an empty stack! (failure)" << endl;
        return;
    }

    cout << "passed" << endl;
}

/**************************************************************************/
/* Queue Test                                                             */
/**************************************************************************/
void doQueueTest() {
    int queue_test[5] = { 11, 22, 33, 44, 55 };

    cout << "Queue test ... " << flush;

    Queue<int> queue(5);
    int i, p;
    for(i = 0; i < 5; i++) {
        if(queue.append(queue_test[i]) != 0) {
            cout << "append failed for element " << i + 1 << " (failure)" << endl;
            return;
        }
    }

    if(queue.append(66) == 0) {
        cout << "append exceeded capacity! (failure)" << endl;
        return;
    }

    for(i = 0; i < 5; i++) {
        if(queue.remove(p) != 0) {
            cout << "remove failed for element " << i + 1 << " (failure)" << endl;
            return;
        } else if(p != queue_test[i]) {
            cout << "removed value incosistent with element " << i + 1 <<
                " (failure)" << endl;
            return;
        }
    }

    if(queue.remove(p) == 0) {
        cout << "popped an empty queue! (failure)" << endl;
        return;
    }

    cout << "passed" << endl;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
