/*************************************************************************
 * $Id: octiTrainer.cxx,v 1.2 2001/06/19 00:45:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:50 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created: 3/9/01 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Work done while a student at Yale University.
 * Copyright (c) 2001 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#include <stdlib.h>
#include <qapplication.h>
#include "argproc.h"
#include "String.hxx"
#include "OctiGame.hxx"
#include "OctiBoard.hxx"
#include "Octi2PlayerGameSearchState.hxx"
#include "OctiWidgetOptions.hxx"
#include "OctiTrainerWindow.hxx"

__USING_NAMESPACE(SELFSOFT);

static arg version_arg = {
    "-version", NULL, -1, VERS_ARG, NULL,
    "Version info"
};

static arg help_arg = {
    "-help",  NULL, -1, HELP_ARG, NULL,
    "This is a training-by-example program for OCTI. "
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
    OctiTrainerWindow trainer;
    qApp.setMainWidget(&trainer);
    trainer.show();
  
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
