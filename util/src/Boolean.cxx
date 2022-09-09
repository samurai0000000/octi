/*************************************************************************
 * $Id: Boolean.cxx,v 1.4 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <string.h>
#include "Boolean.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(Boolean, BaseObject, 1);

const char *__bool_true_str = "true";
const char *__bool_false_str = "false";
const char *__bool_yes_str = "yes";
const char *__bool_no_str = "no";
const char *__bool_one_str = "1";
const char *__bool_zero_str = "0";
const char *__bool_on_str = "on";
const char *__bool_off_str = "off";

const char *TrueFalseStr(boolean b) {
    return b ? __bool_true_str : __bool_false_str;
}

const char *TrueFalseStr(const Boolean &b) {
    return b ? __bool_true_str : __bool_false_str;
}

const char *YesNoStr(boolean b) {
    return b ? __bool_yes_str : __bool_no_str;
}

const char *YesNoStr(const Boolean &b) {
    return b ? __bool_yes_str : __bool_no_str;
}

const char *OneZeroStr(boolean b) {
    return b ? __bool_one_str : __bool_zero_str;
}

const char *OneZeroStr(const Boolean &b) {
    return b ? __bool_one_str : __bool_zero_str;
}

const char *OnOffStr(boolean b) {
    return b ? __bool_on_str : __bool_off_str;
}

const char *OnOffStr(const Boolean &b) {
    return b ? __bool_on_str : __bool_off_str;
}

boolean IsZeroOne(const char *s) {
    if(s == NULL) {
        return FALSE;
    }

    if(strcasecmp(s, __bool_true_str) == 0 ||
       strcasecmp(s, __bool_yes_str) == 0 ||
       strcasecmp(s, __bool_one_str) == 0 ||
       strcasecmp(s, __bool_on_str) == 0) {
        return TRUE;
    }

    return FALSE;
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
