/*************************************************************************
 * $Id: widgetapi.h,v 1.1 2001/08/09 04:06:55 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 04:06:55 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  8/6/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _WIDGETAPI_H_
#define _WIDGETAPI_H_

#include "basecfg.h"
#include "BaseObject.hxx"

#if defined(_WIN32) && defined(WIDGET_DYN_LIB_EXPORTS)
#ifdef _WIDGETAPI_IMPL_
#define WIDGETAPI WEXPORT
#else
#define WIDGETAPI WIMPORT
#endif
#else
#define WIDGETAPI
#endif

#endif
