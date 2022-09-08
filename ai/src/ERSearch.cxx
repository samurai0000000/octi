/*************************************************************************
 * $Id: ERSearch.cxx,v 1.3 2001/08/25 01:34:22 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 01:34:22 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 12/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _AIAPI_IMPL_

#include <iostream>
#include "debug.h"
#include "Date.hxx"
#include "ERSearch.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(ERSearchParameters, SearchParameters, 1);
IMPLEMENT_RUNTIME_DISCOVERABLE(ERSearch, BaseObject);

const SearchState *ERSearch::searchMax(SearchState *state,
				       const SearchParameters *params) {
  return NULL;
}

const SearchState *ERSearch::searchMin(SearchState *state,
				       const SearchParameters *params) {
  return NULL;
}

void ERSearch::printLastSearchStatistics(ostream &out) const {

}

void ERSearch::printTotalStatistics(ostream &out) const {

}

__END_NAMESPACE(SELFSOFT);
