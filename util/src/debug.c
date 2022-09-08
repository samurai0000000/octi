/*************************************************************************
 * $Id: debug.c,v 1.1 2000/01/09 15:20:51 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/01/09 15:20:51 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  11/13/99 by Charles Chiou
 * Comments: Used for producing debugging messages in different levels.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdlib.h>
#include <stdio.h>
#include "basecfg.h"
#include "debug.h"
#include "utilapi.h"

void _Assert(char *file, unsigned int line) {
    fflush(NULL);
    fprintf(stderr, "\nAssertion failed: %s, line %u\n", file, line);
    fflush(stderr);
    abort();
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
