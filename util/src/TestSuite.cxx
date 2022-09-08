/*************************************************************************
 * $Id: TestSuite.cxx,v 1.12 2000/11/14 06:16:51 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:16:51 $
 * $Locker:  $
 * $Revision: 1.12 $
 *
 * Created:  5/5/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include <iostream>
#include "argproc.h"
#include "String.hxx"
#include "Stack.hxx"
#include "Base64.hxx"

__USING_NAMESPACE(SELFSOFT);

extern void doStringTest();
extern void doStringTokenizerTest();
extern void doStackTest();
extern void doQueueTest();
extern void doArrayTest();
extern void doSlistTest();
extern void doDlistTest();
extern void doHashTableTest();
extern void doHexTest();
extern void doBase64Test();
extern void doDateTest();
extern void doCalendarTest();
extern void doTimeZoneTest();
extern void doThreadTest();

static arg all_arg = {
    "-all", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform all tests"
};

static arg runtime_arg = {
    "-runtime", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the runtime environment classes"
};

static arg string_arg = {
    "-string", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"String\" class"
};

static arg tokenizer_arg = {
    "-tokenizer", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"StringTokenizer\" class"
};

static arg stack_arg = {
    "-stack", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Stack\" class"
};

static arg queue_arg = {
    "-queue", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Queue\" class"
};

static arg array_arg = {
    "-array", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Array\" class"
};

static arg slist_arg = {
    "-slist", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Slist\" class"
};

static arg dlist_arg = {
    "-dlist", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Dlist\" class"
};

static arg hash_arg = {
    "-hash", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"HashTable\" class"
};

static arg hex_arg = {
    "-hex", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"HexEncoder\" and \"HexDecoder\" class"
};

static arg base64_arg = {
    "-base64", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Base64Encoder\" and \"Base64Decoder\" class"
};

static arg date_arg = {
    "-date", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Date\" class"
};

static arg calendar_arg = {
    "-calendar", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Calendar\" class"
};

static arg timezone_arg = {
    "-timezone", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"TimeZone\" class"
};

static arg thread_arg = {
    "-thread", NULL, -1, IS_OPT | NO_ARG, NULL,
    "Perform tests on the \"Thread\" class"
};

static arg version_arg = {
    "-version", NULL, -1, VERS_ARG, NULL,
    "Version info"
};

static arg help_arg = {
    "-help",  NULL, -1, HELP_ARG, NULL,
    "This program tests the correctness of util modules"
};

static arg *arg_list[] = {
    &all_arg,
    &runtime_arg,
    &string_arg,
    &tokenizer_arg,
    &stack_arg,
    &queue_arg,
    &array_arg,
    &slist_arg,
    &dlist_arg,
    &hash_arg,
    &hex_arg,
    &base64_arg,
    &date_arg,
    &calendar_arg,
    &timezone_arg,
    &thread_arg,
    &version_arg,
    &help_arg
};

static int arg_list_size = sizeof(arg_list) / sizeof(arg_list[0]);

/**************************************************************************/
/* Runtime Test                                                           */
/**************************************************************************/
void print_class_hierarchy(const BaseObject *object) {
    if(object == NULL) {
        return;
    }

    for(const BaseRuntimeClass *runtimeClass = object->getRuntimeClass();
        runtimeClass != NULL; runtimeClass = runtimeClass->getParentClass()) {
        cout << runtimeClass->getClassName() << " -> " << flush;
    }
    cout << endl;
}

void doRuntimeTest() {
    String string;
    Base64Encoder b64encoder;
    Stack<int> intStack;

    print_class_hierarchy(&string);
    print_class_hierarchy(&b64encoder);
    print_class_hierarchy(&intStack);

    intStack.push(3);
    intStack.push(4);
    int a, b;
    intStack.pop(a);
    intStack.pop(b);
    cout << a << " " << b << endl;
  
    Stack<Ptr<BaseObject> > objStack;
    Ptr<BaseObject> object1 = new String("hello");
    cout << object1->getRuntimeClass()->getClassName() << endl;
    objStack.push(object1);
    Ptr<BaseObject> object2 = new Exception("exception");
    cout << object2->getRuntimeClass()->getClassName() << endl;
    objStack.push(object2);

    Ptr<BaseObject> p;
    while(objStack.pop(p) == 0) {
        cout << "poped a class of type " << p->getRuntimeClass()->getClassName() << endl;
        if(p->isInstanceOf("String")) {
            cout << "Modifying String" << endl;
            *((String *) object1.ptr()) += " world!";
        } else if(p->isInstanceOf("Exception")) {
            cout << "Figured out Exception" << endl;
        } else {
            cout << "Can't figure out instance" << endl;
        }

    }

    cout << *((String *) object1.ptr()) << endl;
    cout << ((Exception *) object2.ptr())->getMessage() << endl;
    cout << "String is a kind of Exception? " <<
        (object1->isKindOf("Exception") ? "TRUE" : "FALSE") << endl;
    Ptr<BaseObject> object3 = new StringException("str exp");
    cout << "StringException is a kind of Exception? " <<
        (object3->isKindOf(object2->getRuntimeClass()) ? "TRUE" : "FALSE") << endl;
    cout << "Exception is a kind of StringException? " <<
        (object2->isKindOf(object3->getRuntimeClass()) ? "TRUE" : "FALSE") << endl;
}

int main(int argc, char **argv) {
    if(parse_args(argc, argv, arg_list, arg_list_size, 1) != 0) {
        exit(-1);
    }

    boolean doAllTests = FALSE;
    if(all_arg.num_args == 0 || argc == 1) {
        doAllTests = TRUE;
    }

    if(doAllTests || runtime_arg.num_args == 0) {
        doRuntimeTest();
    }

    if(doAllTests || string_arg.num_args == 0) {
        doStringTest();
    }

    if(doAllTests || tokenizer_arg.num_args == 0) {
        doStringTokenizerTest();
    }

    if(doAllTests || stack_arg.num_args == 0) {
        doStackTest();
    }

    if(doAllTests || queue_arg.num_args == 0) {
        doQueueTest();
    }

    if(doAllTests || array_arg.num_args == 0) {
        doArrayTest();
    }

    if(doAllTests || slist_arg.num_args == 0) {
        doSlistTest();
    }

    if(doAllTests || dlist_arg.num_args == 0) {
        doDlistTest();
    }

    if(doAllTests || hash_arg.num_args == 0) {
        doHashTableTest();
    }

    if(doAllTests || hex_arg.num_args == 0) {
        doHexTest();
    }

    if(doAllTests || base64_arg.num_args == 0) {
        doBase64Test();
    }

    if(doAllTests || date_arg.num_args == 0) {
        doDateTest();
    }

    if(doAllTests || calendar_arg.num_args == 0) {
        doCalendarTest();
    }

    if(doAllTests || timezone_arg.num_args == 0) {
        doTimeZoneTest();
    }

    if(thread_arg.num_args == 0) {
        doThreadTest();
    }

    return 0;
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
