/*************************************************************************
 * $Id: OctiDefs.hxx,v 1.5 2001/08/24 03:00:33 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:33 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created:  11/3/99 by Charles Chiou
 * Comments: Octi Constants
 * Status:
 * Bugs:
 *
 * Work done as part of courses taken in Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIDEFS_H_
#define _OCTIDEFS_H_

#include "octiapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

/********************/
/* OCTI definitions */
/********************/

// OCTI games supported:
// 2 player OCTI
// 4 player OCTI
// OCTI for Kids
enum OctiGameMode {
    GAME_MODE_UNDEF = -1,
    GAME_MODE_2     =  1,
    GAME_MODE_4     =  2,
    GAME_MODE_OFK   =  3
};

// The number of bases to occupy to win
// This is only relevant to OctiGameMode == GAME_MODE_2
enum OctiGameBases {
    GAME_BASES_1,
    GAME_BASES_3
};

// Player definitions
enum OctiPlayer {
    PLAYER_UNDEF = -1,
    PLAYER_A     = 1,
    PLAYER_B     = 2,
    PLAYER_C     = 3,
    PLAYER_D     = 4
};

// The type of OCTI pods:
// This denotes the ownership of the pod to the players.
enum OctiPodType {
    A_POD = 1,
    B_POD = 2,
    C_POD = 3,
    D_POD = 4
};

// For each directions, there are 3 possible prong states:
//   - no prong present
//   - has normal prong
//   - has super prong
enum OctiProngType {
    NO_PRONG     = 0x0,
    NORMAL_PRONG = 0x1,
    SUPER_PRONG  = 0x3,
};


// A square type is used to associate each square on an OCTI board.
// For ordinary squares, it is denoted 'NEUTRAL_SQUARE', otherwise,
// for squares that are occupied with players' pods are tagged with
// player IDs. This is used for specialized query operations to the
// squares.
enum OctiSquareType {
    NEUTRAL_SQUARE      = 0,
    A_HOME_SQUARE       = 1,
    B_HOME_SQUARE       = 2,
    C_HOME_SQUARE       = 3,
    D_HOME_SQUARE       = 4,
    NEUTRAL_OCTI_SQUARE = 5
};

// The eight valid directions that a pod has are:
// North, NorthEast, East, SouthEast,  South, SouthWest, West, NorthWest
// The official OCTI notation uses the letters 'a' to 'h'. The OctiDir
// enumeration is the cannonical form used internally by this module.
enum OctiDir {
    DIR_UNDEF = -1,
    DIR_N     =  0,
    DIR_NE    =  1,
    DIR_E     =  2,
    DIR_SE    =  3,
    DIR_S     =  4,
    DIR_SW    =  5,
    DIR_W     =  6,
    DIR_NW    =  7
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
