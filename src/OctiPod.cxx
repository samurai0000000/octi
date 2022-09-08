/*************************************************************************
 * $Id: OctiPod.cxx,v 1.4 2001/08/24 03:00:35 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:35 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <string.h>
#include "String.hxx"
#include "OctiDefs.hxx"
#include "OctiPod.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(OctiPod, BaseObject, 1);

const OctiPod OctiPod::EMPTY_POD;

void OctiPod::addProng(OctiDir dir, boolean superProng) {
    OctiProngType p;
    int shift;

    p = superProng ? SUPER_PRONG : NORMAL_PRONG;
    shift = (dir << 1);  // dir * 2
  
    // Mask the bit first
    _config &= (~((0x3) << shift));
  
    // OR to final
    _config |= (p << shift);
}

void OctiPod::removeProng(OctiDir dir) {
    int shift;
  
    shift = (dir << 1);  // dir * 2
    _config &= (~((0x3) << shift));
}

void OctiPod::getProngConfigString(char *buf) const {
    int i = 0;
    if((_config & 0x0001) != 0x0) {
        buf[i++] = 'a';
    }
    if((_config & 0x0002) != 0x0) {
        buf[i++] = 'a';
        buf[i++] = '!';
    }
    if((_config & 0x0004) != 0x0) {
        buf[i++] = 'b';
    }
    if((_config & 0x0008) != 0x0) {
        buf[i++] = 'b';
        buf[i++] = '!';
    }
    if((_config & 0x0010) != 0x0) {
        buf[i++] = 'c';
    }
    if((_config & 0x0020) != 0x0) {
        buf[i++] = 'c';
        buf[i++] = '!';
    }
    if((_config & 0x0040) != 0x0) {
        buf[i++] = 'd';
    }
    if((_config & 0x0080) != 0x0) {
        buf[i++] = 'd';
        buf[i++] = '!';
    }
    if((_config & 0x0100) != 0x0) {
        buf[i++] = 'e';
    }
    if((_config & 0x0200) != 0x0) {
        buf[i++] = 'e';
        buf[i++] = '!';
    }
    if((_config & 0x0400) != 0x0) {
        buf[i++] = 'f';
    }
    if((_config & 0x0800) != 0x0) {
        buf[i++] = 'f';
        buf[i++] = '!';
    }
    if((_config & 0x1000) != 0x0) {
        buf[i++] = 'g';
    }
    if((_config & 0x2000) != 0x0) {
        buf[i++] = 'g';
        buf[i++] = '!';
    }
    if((_config & 0x4000) != 0x0) {
        buf[i++] = 'h';
    }
    if((_config & 0x8000) != 0x0) {
        buf[i++] = 'h';
        buf[i++] = '!';
    }
    buf[i] = '\0';
}

String OctiPod::getProngConfigString() const {
    String ret;

    if((_config & 0x0001) != 0x0) {
        ret += 'a';
    }
    if((_config & 0x0002) != 0x0) {
        ret += "a!";
    }
    if((_config & 0x0004) != 0x0) {
        ret += 'b';
    }
    if((_config & 0x0008) != 0x0) {
        ret += "b!";
    }
    if((_config & 0x0010) != 0x0) {
        ret += 'c';
    }
    if((_config & 0x0020) != 0x0) {
        ret += "c!";
    }
    if((_config & 0x0040) != 0x0) {
        ret += 'd';
    }
    if((_config & 0x0080) != 0x0) {
        ret += "d!";
    }
    if((_config & 0x0100) != 0x0) {
        ret += 'e';
    }
    if((_config & 0x0200) != 0x0) {
        ret += "e!";
    }
    if((_config & 0x0400) != 0x0) {
        ret += 'f';
    }
    if((_config & 0x0800) != 0x0) {
        ret += "f!";
    }
    if((_config & 0x1000) != 0x0) {
        ret += 'g';
    }
    if((_config & 0x2000) != 0x0) {
        ret += "g!";
    }
    if((_config & 0x4000) != 0x0) {
        ret += 'h';
    }
    if((_config & 0x8000) != 0x0) {
        ret += "h!";
    }

    return ret;
}

inline int calc_pod_numprongs(word32 config) {
    int numprongs = 0;

    if((config & 0x0003) != 0x0) {
        numprongs++;
    }
    if((config & 0x000c) != 0x0) {
        numprongs++;
    }
    if((config & 0x0030) != 0x0) {
        numprongs++;
    }
    if((config & 0x00c0) != 0x0) {
        numprongs++;
    }
    if((config & 0x0300) != 0x0) {
        numprongs++;
    }
    if((config & 0x0c00) != 0x0) {
        numprongs++;
    }
    if((config & 0x3000) != 0x0) {
        numprongs++;
    }
    if((config & 0xc000) != 0x0) {
        numprongs++;
    }

    return numprongs;
}

boolean OctiPod::lighterThan(const OctiPod &pod) const {
    if(_config == pod._config) {
        return FALSE;
    }

    int my_numprongs = calc_pod_numprongs(_config);
    int other_numprongs = calc_pod_numprongs(pod._config);

    if(my_numprongs == other_numprongs) {
        return _config < pod._config;
    }

    return my_numprongs < other_numprongs;
}

boolean OctiPod::heavierThan(const OctiPod &pod) const {
    if(_config == pod._config) {
        return FALSE;
    }

    int my_numprongs = calc_pod_numprongs(_config);
    int other_numprongs = calc_pod_numprongs(pod._config);

    if(my_numprongs == other_numprongs) {
        return _config > pod._config;
    }

    return my_numprongs > other_numprongs;
}

// Hashing OctiPod is simply a cast of _config into unsigned long.
// It is possible to 'compact' _config since only 3 states are used for
// every 2 bits that store the directions. However, it will be slow that way.
unsigned long OctiPod::hashCode() const {
    return (unsigned long) _config;
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
