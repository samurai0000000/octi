/*************************************************************************
 * $Id: MPIAlphaBetaSearch.hxx,v 1.1 2000/11/04 18:08:03 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/04 18:08:03 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  11/3/00 by Charles Chiou
 * Comments: Parallel Alpha-Beta search algorithm using MPI.
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _MPIALPHABETASEARCH_HXX_
#define _MPIALPHABETASEARCH_HXX_

#include "AISearchAlgorithm.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class AIAPI MPIAlphaBetaSearch : public MinMaxAlgorithm {

};

__END_NAMESPACE(SELFSOFT);

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
