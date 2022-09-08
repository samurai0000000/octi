/*************************************************************************
 * $Id: TranspositionTable.cxx,v 1.3 2001/08/25 09:16:56 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:56 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created: 10/3/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _AIAPI_IMPL_

#include <string.h>
#include "AISearchAlgorithm.hxx"
#include "TranspositionTable.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(TranspositionTable, BaseObject);

TranspositionTable::TranspositionTable(int size) {
    if(size <= 0) {
        _size = 999997;
    } else {
        _size = size;
    }

    _entries = (TranspositionTableEntry *)
        malloc(sizeof(TranspositionTableEntry) * _size);
    memset(_entries, 0x0, sizeof(TranspositionTableEntry) * _size);
}

TranspositionTable::~TranspositionTable() {
    if(_entries) {
        free(_entries);
    }
}

const TranspositionTableEntry *
TranspositionTable::fetch(const SearchState *state) const {
    const TranspositionTableEntry *tte =
        &_entries[state->hashCode() % _size];
    if(tte->checksum == state->hashChecksum()) {
        ((TranspositionTableEntry *) tte)->stale = FALSE;
    } else {
        tte = NULL;
    }

    return tte;
}

void TranspositionTable::store(const SearchState *state,
                               const TranspositionTableEntry *tte) {
    ASSERT(state != NULL);
    ASSERT(tte != NULL);

    TranspositionTableEntry *stte =
        &_entries[state->hashCode() % _size];
    if(stte->stale || tte->depth >= stte->depth) {
        stte->depth = tte->depth;
        stte->flag = tte->flag;
        stte->xxx = tte->xxx;
        stte->yyy = tte->yyy;
        stte->zzz = tte->zzz;
        stte->checksum = state->hashChecksum();
        stte->stale = FALSE;
    }
}


void TranspositionTable::setAllStale() {
    for(int i = 0; i < _size; i++) {
        _entries[i].stale = TRUE;
    }
}

void TranspositionTable::clearAll() {
    memset(_entries, 0x0, sizeof(TranspositionTableEntry) * _size);
}

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
