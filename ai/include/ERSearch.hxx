/*************************************************************************
 * $Id: ERSearch.hxx,v 1.4 2001/08/25 01:34:22 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 01:34:22 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 12/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _ERSEARCH_HXX_
#define _ERSEARCH_HXX_

#include "BaseObject.hxx"
#include "HashTable.hxx"
#include "Timer.hxx"
#include "AISearchAlgorithm.hxx"

#define DEFAULT_ER_TRANSTAB_SIZE 16384ul

__BEGIN_NAMESPACE(SELFSOFT);

/**
 * Parameters to the ERSearch algorithm.
 */
class AIAPI ERSearchParameters : public SearchParameters {

    DECLARE_RUNTIME_SERIALIZABLE(ERSearchParameters);

public:

    ERSearchParameters();
    ~ERSearchParameters();

};

/**
 * A sequential implementation of the evaluate and refute search algorithm.
 */
class AIAPI ERSearch : public MinMaxAlgorithm {


    DECLARE_RUNTIME_DISCOVERABLE(ERSearch);

public:

    ERSearch(unsigned long tableSize = DEFAULT_ER_TRANSTAB_SIZE);
    ~ERSearch();

    const SearchState *searchMax(SearchState *state,
                                 const SearchParameters *params = NULL);
    const SearchState *searchMin(SearchState *state,
                                 const SearchParameters *params = NULL);

    void printLastSearchStatistics(ostream &out) const;
    void printTotalStatistics(ostream &out) const;
    void resetStatistics();


};

// Inline functions

inline ERSearchParameters::ERSearchParameters() {

};

inline ERSearchParameters::~ERSearchParameters() {

}

inline ERSearch::ERSearch(unsigned long tableSize) {

}

inline ERSearch::~ERSearch() {

}

inline void ERSearch::resetStatistics() {

}

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
