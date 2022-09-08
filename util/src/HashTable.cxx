/*************************************************************************
 * $Id: HashTable.cxx,v 1.9 2001/08/09 03:11:11 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:11 $
 * $Locker:  $
 * $Revision: 1.9 $
 *
 * Created:  1/30/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include "HashTable.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

unsigned long hash_int16(const int16 &data) {
    return (unsigned long) data;
}

unsigned long hash_word16(const word16 &data) {
    return (unsigned long) data;
}

unsigned long hash_int32(const int32 &data) {
    return (unsigned long) data;
}

unsigned long hash_word32(const word32 &data) {
    return (unsigned long) data;
}

#ifdef __HAS_WORD64__

unsigned long hash_int64(const int64 &data) {
    return (unsigned long) data;
}

unsigned long hash_word64(const word64 &data) {
    return (unsigned long) data;
}

#endif

unsigned long hash_string(const char *string) {
    if(string == NULL) {
        return 0;
    }

    unsigned long h = 0;
    unsigned long len = strlen(string);

    for(unsigned long i = 0; i < len; i++) {
        h += (string[i] +  h);
    }

    return h;
}

unsigned long hashString(const String &string) {
    return string.hashCode();
}

const unsigned long G_HashTablePrimeList[] = {
    53,         97,           193,         389,       769,
    1543,       3079,         6151,        12289,     24593,
    49157,      98317,        196613,      393241,    786433,
    1572869,    3145739,      6291469,     12582917,  25165843,
    50331653,   100663319,    201326611,   402653189, 805306457,
    1610612741, 3221225473ul, 4294967291ul
};

const unsigned long G_HashTablePrimeListLength = sizeof(G_HashTablePrimeList) / sizeof(unsigned long);

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
