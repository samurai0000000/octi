/*************************************************************************
 * $Id: utilapi.h,v 1.4 2000/01/08 11:10:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/01/08 11:10:59 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  9/15/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _UTILAPI_H_
#define _UTILAPI_H_

#include "basecfg.h"

#if defined(_WIN32) && defined(UTIL_DYN_LIB_EXPORTS)
#ifdef _UTILAPI_IMPL_
#define UTILAPI WEXPORT
#else
#define UTILAPI WIMPORT
#endif
#else
#define UTILAPI
#endif

#ifdef __cplusplus

class Root {

};

#endif

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
