/*************************************************************************
 * $Id: OctiGameFileHandler.java,v 1.4 2000/12/04 06:10:23 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/12/04 06:10:23 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  10/9/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _OCTIAPI_IMPL_

#include <fstream.h>
#include "debug.h"
#include "Boolean.hxx"
#include "OctiGameFileHandler.hxx"

#define OCTIFILE_HEADER              "; OCTI Game File"
#define OCTIFILE_GAME_MODE           "GAME_MODE"
#define OCTIFILE_USE_SUPER_PRONGS    "USE_SUPER_PRONGS"
#define OCTIFILE_USE_EDGELESS_BOARD  "USE_EDGELESS_BOARD"
#define OCTIFILE_START_PLAYER        "START_PLAYER"

__BEGIN_NAMESPACE(SELFSOFT);

void LoadOcti(const char *path, OctiBoard *board) throw(IOException) {
    ASSERT(board != NULL);

    String s;
    ifstream f;

    f.open(path);
    if(!f.is_open()) {
        String msg;
        msg.format("cannot open \"%s\"", path);
        throw FileNotFoundException(msg);
    }

    while(!(f >> s).eof()) {
        s = s.getTrimmed();
        if(s.length() > 0 && s[0] != ';') {
            if(s.startsWith(OCTIFILE_GAME_MODE)) {
            } else if(s.startsWith(OCTIFILE_USE_SUPER_PRONGS)) {
            } else if(s.startsWith(OCTIFILE_USE_EDGELESS_BOARD)) {
            } else if(s.startsWith(OCTIFILE_START_PLAYER)) {
            }
        }
    }

    f.close();
}

void SaveOcti(const char *path, OctiBoard *board) throw(IOException) {
    ASSERT(board != NULL);

    OctiGame *game;
    ofstream f;

    f.open(path);
    if(!f.is_open()) {
        String msg;
        msg.format("cannot open \"%s\"", path);
        throw FileNotFoundException(msg);
    }

    game = board->getGame();

    f << OCTIFILE_HEADER << endl;
    f << OCTIFILE_GAME_MODE << ": "
      << game->getMode() << endl;
    f << OCTIFILE_USE_SUPER_PRONGS << ": "
      << YesNoStr(game->useSuperProngs()) << endl;
    f << OCTIFILE_USE_EDGELESS_BOARD << ": "
      << YesNoStr(game->useEdgelessBoard()) << endl;
    f << OCTIFILE_START_PLAYER << ": " << game->getStartPlayer() << endl;
    Slist<OctiMove> moves = game->getAllMoves();
    Ptr<SlistIterator<OctiMove> > it = (SlistIterator<OctiMove> *)
        moves.getIterator();
    for(it->first(); it->isNotNull(); it->next()) {
        f << it->current()->getNotation() << endl;
    }
    f.close();
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
