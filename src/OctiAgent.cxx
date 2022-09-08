/*************************************************************************
 * $Id: OctiAgent.cxx,v 1.8 2001/08/25 09:17:29 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:17:29 $
 * $Locker:  $
 * $Revision: 1.8 $
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

#include <iostream.h>
#include "Error.hxx"
#include "OctiAgent.hxx"
#include "OctiGameFileHandler.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_DISCOVERABLE(OctiAgent, BaseObject);

int OctiAgent::execute() {
    String prompt;
    int movesize;
    boolean sim = FALSE;
    _game->start();

    while(!_game->isGameOver()) {
        movesize = _game->getMoveSize();

        if(!sim) {
            Octi2PlayerGameSearchState spec(&_board);
            if(movesize > 12 && _speculate) {
                if(_game->getPlayerToMove() == PLAYER_A) {
                    _alg->speculateMax(&spec, &_params);
                } else {
                    _alg->speculateMin(&spec, &_params);
                }
            }

            cout << "Enter Move: " << flush;
            cin >> prompt;

            if(movesize > 12 && _speculate) {
                _alg->stopSpeculation();
                Thread::sleep(1000);
            }
        }

        try {
            if(prompt == "load") {
                LoadOcti("game0.octi", &_board);
                continue;
            }
            if(prompt == "save") {
                SaveOcti("game0.octi", &_board);
                continue;
            }
        } catch(IOException e) {
            cerr << e.getMessage() << endl;
            continue;
        }

        if(prompt == "simulate") {
            prompt = "think";
            sim = TRUE;
        }
    
        if(prompt == "think") {
            cout << "Thinking..." << endl;
            String mstr;

            if(movesize < 12) {
                switch(movesize) {
                case 0:  mstr = "53+c";           break;
                case 1:  mstr = "57+g";           break;
                case 2:  mstr = "53";             break;
                case 3:  mstr = "57";             break;
                case 4:  mstr = "53+b";           break;
                case 5:  mstr = "57+f";           break;
                case 6:  mstr = "73+h";           break;
                case 7:  mstr = "37+d";           break;
                case 8:  mstr = "53";             break;
                case 9:  mstr = "57";             break;
                case 10: mstr = "53c-63,53b-64";  break;
                case 11: mstr = "57g-47,57f-46";  break;
                }
            } else {
                Octi2PlayerGameSearchState search(&_board), *result;
                if(_game->getPlayerToMove() == PLAYER_A) {
                    result = (Octi2PlayerGameSearchState *)
                        _alg->searchMax(&search, &_params);
                } else {
                    result = (Octi2PlayerGameSearchState *)
                        _alg->searchMin(&search, &_params);
                }
	
                if(result == NULL) {
                    Error::fatal("Search algorithm returned: result == NULL");
                }
	
                _alg->printLastSearchStatistics(cout);
                _alg->printTotalStatistics(cout);
                mstr = result->getDescription();
            }

            try {
                cout << "Choosing move: " << mstr << endl;
                OctiMove move(mstr);
                _game->move(move);
            } catch(Exception e) {
                cout << "Error occurred: "<< e.getMessage() << endl;
                cout << "exiting..." << endl;
                return -1;
            }
        } else if(prompt == "undo") {
            try {
                _game->undo();
            } catch(Exception e) {
                cout << e.getMessage() << endl;
            }
        } else if(prompt == "redo") {
            try {
                _game->redo();
            } catch(Exception e) {
                cout << e.getMessage() << endl;
            }
        } else {
            try {
                OctiMove move(prompt);
                _game->move(move);
            } catch(Exception e) {
                cout << "Your move is bogus!" << endl;
            }
        }
    }
    return 0;
}

void OctiAgent::gameChanged(OctiBoard *src, OctiGame *oldGame, OctiGame *newGame) {
    if(oldGame) {
        oldGame->removeOctiGameListener(this);
    }
    newGame->addOctiGameListener(this);
    _game = newGame;
}

void OctiAgent::boardUpdated(OctiBoard *src, const OctiMove *move, boolean undo) {
    cout << _board;
}

void OctiAgent::gameResetted(OctiGame *src) {

}

void OctiAgent::gameStarted(OctiGame *src) {

}

void OctiAgent::gameResigned(OctiGame *src) {

}

void OctiAgent::gameUpdated(OctiGame *src) {

}

void OctiAgent::gameOver(OctiGame *src) {

}

void OctiAgent::canUndo(OctiGame *src, int steps) {

}

void OctiAgent::canRedo(OctiGame *src, int steps) {

}

void OctiAgent::canMakeMove(OctiGame *src, boolean b) {

}

void OctiAgent::canCancelMove(OctiGame *src, boolean b) {

}

void OctiAgent::canPauseGame(OctiGame *src, boolean b) {

}

void OctiAgent::canResign(OctiGame *src, boolean b) {

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
