/*************************************************************************
 * $Id: octiapi.h,v 1.1 2000/11/04 18:08:17 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/04 18:08:17 $
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

#ifndef _OCTIAPI_H_
#define _OCTIAPI_H_

#include "basecfg.h"
#include "BaseObject.hxx"

#if defined(_WIN32) && defined(OCTI_DYN_LIB_EXPORTS)
#ifdef _OCTIAPI_IMPL_
#define OCTIAPI WEXPORT
#else
#define OCTIAPI WIMPORT
#endif
#else
#define OCTIAPI
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
