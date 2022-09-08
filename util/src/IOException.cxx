/*************************************************************************
 * $Id: File.cxx,v 1.5 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  8/29/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "File.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(IOException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(FileNotFoundException, IOException, 1);

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
