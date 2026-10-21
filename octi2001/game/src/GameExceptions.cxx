/*************************************************************************
 * $Id: GameExceptions.cxx,v 1.2 2000/11/18 10:25:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/18 10:25:08 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/13/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _GAMEAPI_IMPL_

#include "GameExceptions.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(IllegalMoveSyntaxException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(IllegalMoveException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(GameSessionException, Exception, 1);

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
