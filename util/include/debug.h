/*************************************************************************
 * $Id: debug.h,v 1.4 2000/11/20 07:41:22 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/20 07:41:22 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  11/13/99 by Charles Chiou
 * Comments: Used for producing debugging messages in different levels.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "utilapi.h"

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef _DEBUG
#define ASSERT(X)                               \
    if(X) {                                     \
    } else {                                    \
        _Assert(__FILE__, __LINE__);            \
    }
#else
#define ASSERT(X)
#endif

__BEGIN_C_DECLS

extern UTILAPI void _Assert(char *file, unsigned int line);

__END_C_DECLS

#endif

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
