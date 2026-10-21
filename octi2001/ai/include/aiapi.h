/*************************************************************************
 * $Id: aiapi.h,v 1.1 2000/11/04 18:08:03 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/04 18:08:03 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  11/3/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _AIAPI_H_
#define _AIAPI_H_

#include "basecfg.h"
#include "BaseObject.hxx"

#if defined(_WIN32) && defined(AI_DYN_LIB_EXPORTS)
#ifdef _AIAPI_IMPL_
#define AIAPI WEXPORT
#else
#define AIAPI WIMPORT
#endif
#else
#define AIAPI
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
