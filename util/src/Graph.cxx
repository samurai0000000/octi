/*************************************************************************
 * $Id: Graph.cxx,v 1.6 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created: 11/14/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "Graph.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(GraphException, Exception, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Vertex, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(Edge, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(MatrixGraph, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(MatirxDiGraph, MatrixGraph, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(ListGraph, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(ListDiGraph, ListGraph, 1);

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
