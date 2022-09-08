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

#ifndef _OCTIGAMEFILEHANDLER_HXX_
#define _OCTIGAMEFILEHANDLER_HXX_

#include "IOException.hxx"
#include "OctiDefs.hxx"
#include "OctiGame.hxx"
#include "OctiBoard.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

extern OCTIAPI void LoadOcti(const char *path, OctiBoard *board) throw(IOException);
extern OCTIAPI void SaveOcti(const char *path, OctiBoard *board) throw(IOException);

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
