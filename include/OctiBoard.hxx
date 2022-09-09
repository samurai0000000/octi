/*************************************************************************
 * $Id: OctiBoard.hxx,v 1.7 2001/06/19 00:45:46 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:46 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created: 10/1/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _OCTIBOARD_HXX_
#define _OCTIBOARD_HXX_

#include <iostream>
#include "BaseObject.hxx"
#include "Event.hxx"
#include "OctiDefs.hxx"
#include "OctiSquare.hxx"
#include "OctiMove.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class OCTIAPI OctiGame;
class OCTIAPI OctiBoard;

/**
 * OctiBoardListener is an interface for listening to OCTI board
 * change/update events.
 */
class OCTIAPI OctiBoardListener : public EventListener {

public:

    virtual void gameChanged(OctiBoard *src,
                             OctiGame *oldGame, OctiGame *newGame) = 0;
    virtual void boardUpdated(OctiBoard *src, const OctiMove *move,
                              boolean undo) = 0;

};

/**
 * The OctiBoard class provides data structure suitable for a user-interface
 * application to manipulate the board and apply different actions to the
 * board and flow of game. It is not optimized for AI search applications.
 *
 * The functional requirements are:
 *   - maintain a consitent board state
 *   - use game controllers internally for game-specific operators
 *   - expose essential operators for making 'moves'
 *   - query the status of the board and the game
 *   - provide an asynchronous callback mechanism for board state
 *     changes
 */
class OCTIAPI OctiBoard : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(OctiBoard);

public:

    OctiBoard();
    ~OctiBoard();

    void setGameMode(OctiGameMode mode, boolean useSuperProngs = FALSE,
                     boolean useEdgelessBoard = FALSE);
    OctiGameMode getGameMode() const;
    OctiGame *getGame();

    void addOctiBoardListener(OctiBoardListener *listener);
    void removeOctiBoardListener(OctiBoardListener *listener);

    OctiSquare &squareAt(int x, int y);

    int getNumFreePods(OctiPlayer player) const;
    int getNumCapturedPods(OctiPlayer player) const;
    int getNumFreeProngs(OctiPlayer player) const;
    int getNumFreeSuperProngs(OctiPlayer player) const;
    boolean canLiberate(OctiPlayer player) const;

    friend ostream &operator<<(ostream &out, const OctiBoard &board);

private:

    friend class OctiGameBaseImpl;
    friend class Octi2PlayerGame;
    friend class Octi4PlayerGame;

    void clear();
    void allocateSquares(OctiGameMode mode);
    void deallocateSquares();

    void boardUpdated(const OctiMove *move, boolean undo);
    void gameChanged(OctiGame *oldGame, OctiGame *newGame);

private:

    boolean _areSquaresAllocated;
    OctiSquare *_boardSquares[9][9];
    OctiGameMode _mode;
    OctiGame *_game;
    EventListenerList _listeners;

};

// Inline functions

inline OctiGameMode OctiBoard::getGameMode() const {
    return _mode;
}

inline OctiGame *OctiBoard::getGame() {
    return _game;
}

inline void OctiBoard::addOctiBoardListener(OctiBoardListener *listener) {
    _listeners.addEventListener(listener);
}

inline void OctiBoard::removeOctiBoardListener(OctiBoardListener *listener) {
    _listeners.removeEventListener(listener);
}

inline OctiSquare &OctiBoard::squareAt(int x, int y) {
    return *_boardSquares[x][y];
}

inline void OctiBoard::boardUpdated(const OctiMove *move, boolean undo) {
    for(int i = _listeners.getNumListeners() - 1; i >= 0; i--) {
        OctiBoardListener *listener =
            (OctiBoardListener *) _listeners.getListenerAt(i);
        listener->boardUpdated(this, move, undo);
    }
}

inline void OctiBoard::gameChanged(OctiGame *oldGame, OctiGame *newGame) {
    for(int i = _listeners.getNumListeners() - 1; i >= 0; i--) {
        OctiBoardListener *listener =
            (OctiBoardListener *) _listeners.getListenerAt(i);
        listener->gameChanged(this, oldGame, newGame);
    }
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
