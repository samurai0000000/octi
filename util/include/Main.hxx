/*************************************************************************
 * $Id: Main.hxx,v 1.1 2001/08/24 03:04:42 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/24 03:04:42 $
 * $Locker:  $
 * $Revision: 1.1 $
 *
 * Created:  8/6/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _MAIN_HXX_
#define _MAIN_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI Main : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Main);

public:

    static int start(int argc, char **argv);

    virtual ~Main() = 0;  // Must implement destructor to release resources

protected:

    Main();

    // main() is the entry point to the program
    // (or compiled executable 'binary image')
    virtual int main(int argc, char **argv) = 0;

private:

    static boolean G_started;
    static Main *G_impl;

};

__END_NAMESPACE(SELFSOFT);

#ifdef __MAIN__
int main(int argc, char **argv) {
    return Main::start(argc, argv);
}
#endif

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
