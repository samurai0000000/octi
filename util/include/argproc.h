/*************************************************************************
 * $Id: argproc.h,v 1.6 2001/01/03 04:57:24 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/01/03 04:57:24 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  5/20/99 by Charles Chiou
 * Comments:
 *     Processor for (int argc, char **argv) type arguments.
 *     Define VERSION_STRING macro to override the default string defined in
 *     'config.h' when printing out the version info.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ARGPROC_H_
#define _ARGPROC_H_

#include <stdio.h>
#include "utilapi.h"

#ifndef _ARGPROC_C_
__BEGIN_C_DECLS
const char * __BUILD_DATE__ = __DATE__;
#ifndef _VERSION_STRING_
const char * __BUILD_VERSION__ = _DEFAULT_VERSION_ " " _GENERAL_COPY_TM_MESSAGE_;
#else
const char * __BUILD_VERSION__ = _VERSION_STRING_;
#endif
__END_C_DECLS
#else
extern const char * __BUILD_DATE__;
extern const char * __BUILD_VERSION__;
#endif

enum arg_type_enum {
    IS_OPT       = 0x0001,  /* Optional argument. */
    IS_MULTI     = 0x0002,
    NO_ARG       = 0x0004,
    FILE_ARG     = 0x0008,
    NAME_ARG     = 0x0010,
    TIME_ARG     = 0x0020,
    NUMB_ARG     = 0x0040,
    HELP_ARG     = 0x0080,
    VERS_ARG     = 0x0100,
    USED_ARG     = 0x0200   /* Used to identify when flag used twice. */
};

typedef struct _arg {
    char *flag;     /* The name of the flag. If NULL then all arguments 
                       with no preceeding '-' are saved here.           */
    char **value;   /* The storage of parsed arguments.                 */
    int num_args;   /* The number of arguments retrieved.
                       It should always initialize to -1.               */
    int type;       /* The type of argument from arg_type_enum.         */
    char *ex_arg;   /* Explanation of expected arguments                */
    char *descr;    /* Description for printing in -help.               */
} arg;

__BEGIN_C_DECLS

extern UTILAPI int parse_args(int argc, char *argv[], arg *arg_list[],
                              int arg_list_size, int warn_unused);

extern UTILAPI void print_args_help(char *argv0, arg *arg_list[],
                                    int arg_list_size);

extern UTILAPI void fprint_version(FILE *fp, char *argv0);

extern UTILAPI long arg_to_seconds(arg *arg_el);

__END_C_DECLS

#endif

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
