/*************************************************************************
 * $Id: gameapi.h,v 1.2 2001/07/31 05:58:15 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/07/31 05:58:15 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/3/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _GAMEAPI_H_
#define _GAMEAPI_H_

#include "basecfg.h"
#include "BaseObject.hxx"
#include "Thread.hxx"

#if defined(_WIN32) && defined(GAME_DYN_LIB_EXPORTS)
#ifdef _GAMEAPI_IMPL_
#define GAMEAPI WEXPORT
#else
#define GAMEAPI WIMPORT
#endif
#else
#define GAMEAPI
#endif

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
