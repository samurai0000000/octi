/*************************************************************************
 * $Id: BigInteger.cxx,v 1.1 2000/04/27 05:37:07 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/04/27 05:37:07 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  3/2/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "BigInteger.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(BigInteger, BaseObject, 1);

#if defined(__ASM__IX86__)

#else

///////////////////////////////////////////////////////////////////////////
// Generic implementation in C++

BigInteger::BigInteger()
{

}

BigInteger::~BigInteger()
{

}

#endif

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
