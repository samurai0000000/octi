/*************************************************************************
 * $Id: octiAppMain.cxx,v 1.5 2001/06/19 00:45:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:50 $
 * $Locker:  $
 * $Revision: 1.5 $
 *
 * Created: 11/19/00 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2000 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _VERSION_STRING_ "v1.0a " _GENERAL_COPY_TM_MESSAGE_

#include <stdio.h>
#include <qapplication.h>
#include "argproc.h"
#include "OctiWidgetOptions.hxx"
#include "OctiMainWindow.hxx"

__USING_NAMESPACE(SELFSOFT);

static arg version_arg = {
    "-version", NULL, -1, VERS_ARG, NULL,
    "Version info"
};

static arg help_arg = {
    "-help",  NULL, -1, HELP_ARG, NULL,
    "This is a GUI client program for playing OCTI(TM). "
    "See more information about OCTI on http://www.octi.net"
};

static arg *arg_list[] = {
    &version_arg,
    &help_arg
};

static int arg_list_size = sizeof(arg_list) / sizeof(arg_list[0]);

int main(int argc, char **argv) {
    if(parse_args(argc, argv, arg_list, arg_list_size, 1) != 0) {
        exit(-1);
    }

    QApplication qApp(argc, argv);

    InitOctiOptionsModel();
    OctiMainWindow window;
    qApp.setMainWidget(&window);
    window.show();
  
    return qApp.exec();
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
