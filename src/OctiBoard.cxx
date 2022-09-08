/*************************************************************************
 * $Id: OctiBoard.cxx,v 1.10 2001/08/24 03:00:34 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:00:34 $
 * $Locker:  $
 * $Revision: 1.10 $
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

#include "OctiDefs.hxx"
#include "OctiSquare.hxx"
#include "OctiSquareImpl.hxx"
#include "OctiBoard.hxx"
#include "OctiGame.hxx"
#include "OctiGameImpl.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(OctiBoard, BaseObject, 1);

OctiBoard::OctiBoard() {
    _areSquaresAllocated = FALSE;
    _game = NULL;
    setGameMode(GAME_MODE_2);
}

OctiBoard::~OctiBoard() {
    deallocateSquares();
    if(_game != NULL) {
        delete _game;
    }
}

void OctiBoard::clear() {
    for(int x = 0; x < 9; x++) {
        for(int y = 0; y < 9; y++) {
            ((OctiSquareBaseImpl *) _boardSquares[x][y])->clear();
        }
    }
}


void OctiBoard::allocateSquares(OctiGameMode mode) {
    int x, y;
    deallocateSquares();

    switch(mode) {
    case GAME_MODE_2:
        for(x = 0; x < 9; x++) {
            for(y = 0; y < 9; y++) {
                if((x == 2 || x == 4 || x == 6) && y == 2) {
                    _boardSquares[x][y] = new Octi2PlayerSquare(A_HOME_SQUARE, x, y);
                } else if((x == 2 || x == 4 || x == 6) && y == 6) {
                    _boardSquares[x][y] = new Octi2PlayerSquare(B_HOME_SQUARE, x, y);
                } else {
                    _boardSquares[x][y] = new Octi2PlayerSquare(NEUTRAL_SQUARE, x, y);
                }
            }
        }
        break;
    case GAME_MODE_4:
        for(x = 0; x < 9; x++) {
            for(y = 0; y < 9; y++) {
                if(x == 2 && y == 2) {
                    _boardSquares[x][y] = new Octi4PlayerSquare(A_HOME_SQUARE, x, y);
                } else if(x == 6 && y == 2) {
                    _boardSquares[x][y] = new Octi4PlayerSquare(B_HOME_SQUARE, x, y);
                } else if(x == 2 && y == 6) {
                    _boardSquares[x][y] = new Octi4PlayerSquare(C_HOME_SQUARE, x, y);
                } else if(x == 6 && y == 6) {
                    _boardSquares[x][y] = new Octi4PlayerSquare(D_HOME_SQUARE, x, y);
                } else if(x == 4 && (y == 2 || y == 6)) {
                    _boardSquares[x][y] = new Octi4PlayerSquare(NEUTRAL_OCTI_SQUARE, x, y);
                } else {
                    _boardSquares[x][y] = new Octi4PlayerSquare(NEUTRAL_SQUARE, x, y);
                }
            }
        }
        break;
    }

    _areSquaresAllocated = TRUE;
}

void OctiBoard::deallocateSquares() {
    if(_areSquaresAllocated) {
        for(int x = 0; x < 9; x++) {
            for(int y = 0; y < 9; y++) {
                delete _boardSquares[x][y];
                _boardSquares[x][y] = NULL;
            }
        }
    }

    _areSquaresAllocated = FALSE;
}


void OctiBoard::setGameMode(OctiGameMode mode, boolean useSuperProngs,
                            boolean useEdgelessBoard) {
    _mode = mode;
    allocateSquares(mode);

    OctiGame *oldGame = _game;

    switch(mode) {
    case GAME_MODE_2:
        _game = new Octi2PlayerGame(this, useSuperProngs, useEdgelessBoard);
        gameChanged(oldGame, _game);
        break;
    case GAME_MODE_4:
        _game = new Octi2PlayerGame(this, useSuperProngs, useEdgelessBoard);
        gameChanged(oldGame, _game);
        break;
    }

    if(oldGame) {
        delete oldGame;
    }
}

int OctiBoard::getNumFreePods(OctiPlayer player) const {
    return _game->getNumFreePods(player);
}

int OctiBoard::getNumCapturedPods(OctiPlayer player) const {
    return _game->getNumCapturedPods(player);
}

int OctiBoard::getNumFreeProngs(OctiPlayer player) const {
    return _game->getNumFreeProngs(player);
}

int OctiBoard::getNumFreeSuperProngs(OctiPlayer player) const {
    return _game->getNumFreeSuperProngs(player);
}

boolean OctiBoard::canLiberate(OctiPlayer player) const {

    if(_mode == GAME_MODE_2) {
        switch(player) {
        case PLAYER_A:
            return
                _boardSquares[2][6]->isCapturedBy(player) ||
                _boardSquares[4][6]->isCapturedBy(player) ||
                _boardSquares[6][6]->isCapturedBy(player);
        case PLAYER_B:
            return
                _boardSquares[2][2]->isCapturedBy(player) ||
                _boardSquares[4][2]->isCapturedBy(player) ||
                _boardSquares[6][2]->isCapturedBy(player);
        }
    } else {
        switch(player) {
        case PLAYER_A:
        case PLAYER_C:
            return
                _boardSquares[2][6]->isCapturedBy(player) ||
                _boardSquares[6][6]->isCapturedBy(player);
        case PLAYER_B:
        case PLAYER_D:
            return
                _boardSquares[2][2]->isCapturedBy(player) ||
                _boardSquares[6][2]->isCapturedBy(player);
        }
    }

    return FALSE;
}

struct pod_config_t {
    int x;
    int y;
    OctiPod pod;
};

ostream &operator<<(ostream &out, const OctiBoard &board) {
    int x, y, k, l;
    OctiPod pod;
    int numPods;
    int prongs[9][8];
    char podOwners[9];
    pod_config_t pc[4][7];
    int pc_count[4] = {0, 0, 0, 0};

    out << "A's [Pods, Captured, Prongs, Super] = ";
    out << board._game->getNumFreePods(PLAYER_A);
    out << ", " << board._game->getNumCapturedPods(PLAYER_A);
    out << ", " << board._game->getNumFreeProngs(PLAYER_A);
    out << ", " << board._game->getNumFreeSuperProngs(PLAYER_A) << endl;
    out << "B's [Pods, Captured, Prongs, Super] = ";
    out << board._game->getNumFreePods(PLAYER_B);
    out << ", " << board._game->getNumCapturedPods(PLAYER_B);
    out << ", " << board._game->getNumFreeProngs(PLAYER_B);
    out << ", " << board._game->getNumFreeSuperProngs(PLAYER_B) << endl << endl;
    if(board._game->getMode() == GAME_MODE_4) {
        out << "C's [Pods, Captured, Prongs, Super] = ";
        out << board._game->getNumFreePods(PLAYER_C);
        out << ", " << board._game->getNumCapturedPods(PLAYER_C);
        out << ", " << board._game->getNumFreeProngs(PLAYER_C);
        out << ", " << board._game->getNumFreeSuperProngs(PLAYER_C) << endl;
        out << "D's [Pods, Captured, Prongs, Super] = ";
        out << board._game->getNumFreePods(PLAYER_D);
        out << ", " << board._game->getNumCapturedPods(PLAYER_D);
        out << ", " << board._game->getNumFreeProngs(PLAYER_D);
        out << ", " << board._game->getNumFreeSuperProngs(PLAYER_D) << endl << endl;
    }

    out << "+-----+-----+-----+-----+-----+-----+-----+-----+-----+" << endl;
    for(y = 8; y >= 0; y--) {
        for(x = 0; x < 9; x++) {
            for(l = 0; l < 8; l++) {
                prongs[x][l] = 0;
            }
        }

        for(x = 0; x < 9; x++) {
            numPods = board._boardSquares[x][y]->getNumPods();
            if(board._boardSquares[x][y]->isEmpty()) {
                podOwners[x] = ' ';
            } else {
                podOwners[x] = board._boardSquares[x][y]->getPodAt(0)->getType() +
                    'A' - 1;
            }
            for(k = numPods - 1; k >= 0; k--) {
                pod = *board._boardSquares[x][y]->getPodAt(k);
                for(l = 0; l < 8; l++) {
                    prongs[x][l] |= (pod.prongAt((OctiDir) l) > prongs[x][l] ?
                                     pod.prongAt((OctiDir) l) : prongs[x][l]);
                }

                // Add pod to the pod config data structure
                int owner = pod.getType() - 1;
                pc[owner][pc_count[owner]].x = x + 1;
                pc[owner][pc_count[owner]].y = y + 1;
                pc[owner][pc_count[owner]].pod = pod;
                pc_count[owner]++;
            }
        }
		
        // Print grid level 1
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][7] == 0 ? " " : "\\");
            out << (prongs[x][0] == 0 ? " " : "|");
            out << (prongs[x][1] == 0 ? " " : "/");
            out << " ";
        }
        out << "|" << endl;
	    
        // Print grid level 2
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][6] == 0 ? " " : "-");
            out << (podOwners[x]);
            out << (prongs[x][2] == 0 ? " " : "-");
            out << " ";
        }
        out << "|" << endl;
	    
        // Print grid level 1
        for(x = 0; x < 9; x++) {
            out << "| ";
            out << (prongs[x][5] == 0 ? " " : "/");
            out << (prongs[x][4] == 0 ? " " : "|");
            out << (prongs[x][3] == 0 ? " " : "\\");
            out << " ";
        }
        out << "|" << endl;
        out << "+-----+-----+-----+-----+-----+-----+-----+-----+-----+" << endl;
    }
    out << endl;

    // Print pod condfigurations
    out << "Onboard Pod Configurations:" << endl;
    if(board._game->getMode() == GAME_MODE_4) {
        out << "[A]\t\t[B]\t\t[C]\t\t[D]" << endl;
    } else {
        out << "[A]\t\t[B]" << endl;
    }

    char buf[9];
    buf[0] = '\0';

    for(int z = 0; z < 7; z++) {
        if(z < pc_count[0]) {
            pc[0][z].pod.getProngConfigString(buf);
            out << pc[0][z].x << pc[0][z].y << buf;
        } else {
            out << "-";
        }
        out << "\t\t";
        if(z < pc_count[1]) {
            pc[1][z].pod.getProngConfigString(buf);
            out << pc[1][z].x << pc[1][z].y << buf;
        } else {
            out << "-";
        }
        if(board._game->getMode() == GAME_MODE_4) {
            out << "\t\t";
            if(z < pc_count[2]) {
                pc[2][z].pod.getProngConfigString(buf);
                out << pc[2][z].x << pc[2][z].y << buf;
            } else {
                out << "-";
            }
            out << "\t\t";
            if(z < pc_count[3]) {
                pc[3][z].pod.getProngConfigString(buf);
                out << pc[3][z].x << pc[3][z].y << buf;
            } else {
                out << "-";
            }
        }
        out << endl;
    }

    return out;
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
