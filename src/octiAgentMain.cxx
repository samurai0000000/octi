/*************************************************************************
 * $Id: octiAgentMain.cxx,v 1.8 2001/08/25 09:17:30 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/25 09:17:30 $
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

#define _VERSION_STRING_ "v1.0a " _GENERAL_COPY_TM_MESSAGE_

#include <stdio.h>
#include "argproc.h"
#include "String.hxx"
#include "OctiAgent.hxx"

__USING_NAMESPACE(SELFSOFT);

static arg algorithm_arg = {
  "-alg", NULL, -1, NAME_ARG, "<algorithm>",
  "Specify the minimax search algorithm to use. "
  " Currently supported algorithms are as follows: "
  "\"seq_ab\" (Sequential alphabeta search) "
  "\"thr_ab\" (Thread-based shared memory alphabeta search) NO! "
  "\"mpi_ab\" (MPI-based distributed memory alphabeta search) NO! "
  "\"seq_er\" (Sequential evaluate-and-refute search) NO!"
};

static arg iterative_arg = {
  "-iterative", NULL, -1, IS_OPT | NO_ARG, NULL,
  "If this flag is specified, then an iterative deepending search is used"
};

static arg depth_arg = {
  "-depth", NULL, -1, IS_OPT | NUMB_ARG, "<depth>",
  "The depth cutoff limit of the search tree (default = 5)"
};

static arg timeout_arg = {
  "-timeout", NULL, -1, IS_OPT | NUMB_ARG, "<seconds>",
  "The timeout value of search (default = 0, no timeout)"
};

static arg hashsize_arg = {
  "-hashsize", NULL, -1, IS_OPT | NUMB_ARG, "<size>",
  "The size of the hash table"
};

static arg speculate_arg = {
  "-nospec", NULL, -1, IS_OPT | NO_ARG, NULL,
  "If this flag is specified, then speculative search is disabled"
};

static arg version_arg = {
  "-version", NULL, -1, VERS_ARG, NULL,
  "Version info"
};

static arg help_arg = {
  "-help",  NULL, -1, HELP_ARG, NULL,
  "This is a software agent capable of playing an OCTI(TM) game. "
  "See more information about OCTI on http://www.octi.net"
};


static arg *arg_list[] = {
  &algorithm_arg,
  &iterative_arg,
  &depth_arg,
  &timeout_arg,
  &version_arg,
  &hashsize_arg,
  &speculate_arg,
  &help_arg
};

static int arg_list_size = sizeof(arg_list) / sizeof(arg_list[0]);

int main(int argc, char **argv) {
  String algStr;
  OctiAgentSearchAlgorithm alg = OASA_SEQ_ALPHABETA;
  boolean iterative = DEFAULT_ITERATIVE;
  int depth = DEFAULT_DEPTH;
  unsigned long timeout = DEFAULT_TIMEOUT;
  unsigned long hashsize = 0;

  if(parse_args(argc, argv, arg_list, arg_list_size, 1) != 0) {
    exit(-1);
  }

  algStr = algorithm_arg.value[0];
  if(algStr == "seq_ab") {
    alg = OASA_SEQ_ALPHABETA;
  } else if(algStr == "thr_ab") {
    alg = OASA_THREAD_ALPHABETA;
  } else if(algStr == "mpi_ab") {
    alg = OASA_MPI_ALPHABETA;
    cerr << "Sorry, not yet functional" << endl;
    exit(0);
  } else if(algStr == "seq_er") {
    alg = OASA_SEQ_ER;
  } else {
    cerr << "Unrecognized algorithm: " << algStr << endl;
    exit(-1);
  }

  if(iterative_arg.num_args == 0) {
    iterative = TRUE;
  }

  if(depth_arg.num_args == 1) {
    depth = atoi(depth_arg.value[0]);
  }

  if(timeout_arg.num_args == 1) {
    timeout = (unsigned long) atol(timeout_arg.value[0]) * 1000;
  }

  if(hashsize_arg.num_args == 1) {
    hashsize = (unsigned long) atol(hashsize_arg.value[0]);
  }

  OctiAgent agent(alg, hashsize);
  agent.setParameters(iterative, depth, timeout);

  if(speculate_arg.num_args == 0) {
    agent.setSpeculateOn(FALSE);
  } else {
    agent.setSpeculateOn(TRUE);
  }

  return agent.execute();
}
