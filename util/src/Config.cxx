/*************************************************************************
 * $Id: Config.cxx,v 1.4 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  11/17/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Config.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(ConfigEntry, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Config, BaseObject, 1);

__END_NAMESPACE(SELFSOFT);
