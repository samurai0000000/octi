/*************************************************************************
 * $Id: TranspositionTable.hxx,v 1.3 2001/08/25 09:16:54 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:16:54 $
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

#ifndef _TRANSPOSITIONTABLE_HXX_
#define _TRANSPOSITIONTABLE_HXX_

#include "debug.h"
#include "aiapi.h"
#include "PtrReference.hxx"
#include "BaseObject.hxx"
#include "HashTable.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class SearchState;
class TranspositionTable;

/**
 * Transposition table entry used in mini-max type algorithms.
 *
 * Several common variables are that should handle most of the cases.
 * Their usage is algorithm dependent...
 */
typedef struct _TranspositionTableEntry {
  
  int depth;
  byte flag;
  float xxx;
  float yyy;
  unsigned long zzz;

  // Don't modify the below variables directly,
  // The TranspositionTable class manages them.
  word32 checksum;
  boolean stale;

} TranspositionTableEntry;

/**
 * A high performance (!) transposition table for hashing and storing
 * states (usually a game with perfect information) as generated by
 * algorithms.
 *
 * Requirement: simple, fast, and low memory foot-print.
 *              (Therefore decision was made to drop the class HashTable
 *               which is too sophisticated for this particiluar usage).
 *
 * This class derives from BaseObject, therefore, it may be used with
 * the synchronization helper routines to support multithreaded access.
 *
 */
class AIAPI TranspositionTable : public BaseObject {
  
  DECLARE_RUNTIME_DISCOVERABLE(TranspositionTable);
  
public:
  
  // If the size argument to the constructor is zero, then the constructor
  // does its best to allocate as much memory as possible without causing
  // trashing to virtual (swap) memory.
  TranspositionTable(int size = 0);
  ~TranspositionTable();
  
  // The fetch() and store() methods
  // store() automatically takes care of depth relevance, ie. if
  // an entry is requested to be stored but the original entry contains
  // greater depth, then the store request is ignored.
  const TranspositionTableEntry *fetch(const SearchState *state) const;
  void store(const SearchState *state, const TranspositionTableEntry *tte);
  
  void setAllStale();
  void clearAll();

protected:
  
  int _size;
  TranspositionTableEntry *_entries;
  
};

__END_NAMESPACE(SELFSOFT);

#endif