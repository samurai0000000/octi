/*************************************************************************
 * $Id: argproc.c,v 1.6 2000/11/18 10:24:19 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/18 10:24:19 $
 * $Locker:  $
 * $Revision: 1.6 $
 *
 * Created:  5/20/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define _ARGPROC_C_
#include "argproc.h"

#define ARG_SWITCH '-' /* The switch character before an argument. */

int is_arg(char *arg_ptr) {
  return arg_ptr[0] == ARG_SWITCH && arg_ptr[1] != '\0';
}

int check_all_digit_str(const char *str) {
  if(*str == '\0') {
    return 0;
  }
  
  for(; *str != '\0'; str++) {
    if(!isdigit(*str)) {
      return 0;
    }
  }
  return 1;
}

long time_unit_to_mult_factor(char *units) {
  if(strncmp(units, "sec", 3) == 0) {
    return 1L;
  } else if(strncmp(units, "min", 3) == 0) {
    return 60L;
  } else if(strncmp(units, "hour", 4) == 0) {
    return 60L * 60L;
  } else if(strncmp(units, "day", 3) == 0) {
    return 60L * 60L * 24L;
  } else {
    return 0;
  }
}

int parse_args(int argc, char *argv[], arg *arg_list[],
	       int arg_list_size, int warn_unused) {
 
  int i, j, k, off;
  int type, num_args, num_args_matched, this_arg_matched;
  char *arg_ptr = NULL;
  char program_name[256];

  /* Get the name of the program and strip its directory and extension. */
  arg_ptr = strrchr(argv[0], FILE_SEPARATOR);
  if(arg_ptr != 0) {
    arg_ptr++;
  } else {
    arg_ptr = argv[0];
  }

  strncpy(program_name, arg_ptr, sizeof(program_name) - 1);
  program_name[sizeof(program_name) - 1] = '\0';
  arg_ptr = strrchr(program_name, '.');
  if(arg_ptr && strcmp(arg_ptr, ".exe") == 0) {
    *arg_ptr = '\0';
  }
  
  for(i = 1; i < argc;) {
    arg_ptr = argv[i];
    num_args_matched = 0;
    if (!is_arg(arg_ptr)) {
      /* no flags on this arg */
      for(j = 0; j < arg_list_size; j++) {
	if(arg_list[j]->flag == NULL) {
	  break;
	}
      }
      
      /* found a match, gather args */
      if(j < arg_list_size) {
	
	/* if the list of no flag arguments have already been matched
	   and warned_unused is enabled, then print the error message
	   and cause the program to exit; otherwise, just ignore and discard
	   the old entries. */
	if((arg_list[j]->type & USED_ARG) && warn_unused) {
	  fprintf(stderr, "%s: unknown flag '%s'. See -help.\n",
		  argv[0], arg_list[j]->value[0]);
	  return -1;
	}
	
	k = i;
	if(arg_list[j]->type & IS_MULTI) {
	  while(argv[k] != NULL && !is_arg(argv[k])) {
	    k++;
	  }
	} else {
	  if(argv[k] != NULL && !is_arg(argv[k])) {
	    k++;
	  }
	}
	
	arg_list[j]->value = &argv[i];
	arg_list[j]->num_args = k - i;
	i = k;
	num_args_matched = 1;
	arg_list[j]->type |= USED_ARG;   /* Flag arg as already used. */
      }
    } else {
      /* match a flag */
      num_args_matched = 0;
      for(j = 0; j < arg_list_size; j++) {
	if(arg_list[j]->flag == NULL) {
	  continue;
	}
	
	/* scan to the first differing char */
	off = 0;
	while(arg_ptr[off] && arg_list[j]->flag[off] == arg_ptr[off]) {
	  off++;
	}
	
	/* if not a complete match, allow plurals (-library, -libraries) */
	this_arg_matched = 0;
	if(arg_list[j]->flag[off] != arg_ptr[off]) {
#ifdef _WIN32
	  if ((strcmpi(&arg_ptr[off], "s") == 0
	       && arg_list[j]->flag[off] == '\0') ||
	      (strcmpi(&arg_ptr[off], "ies") == 0 &&
	       strcmpi(&arg_list[j]->flag[off], "y") == 0)) {
#else
		if ((strcasecmp(&arg_ptr[off], "s") == 0
	       && arg_list[j]->flag[off] == '\0') ||
	      (strcasecmp(&arg_ptr[off], "ies") == 0 &&
	       strcasecmp(&arg_list[j]->flag[off], "y") == 0)) {
#endif
	    num_args_matched++;
	    this_arg_matched = 1;
	  }
	} else {
	  num_args_matched++;
	  this_arg_matched = 1;
	}
	
	/* this -flag matched multiple times */
	if(num_args_matched > 1) {
	  break;
	}
	
	/* gather args */
	if(this_arg_matched) {
	  if(arg_list[j]->type & USED_ARG) {
	    fprintf(stderr, "%s: duplicate flag '%s'. See -help.\n",
		    argv[0], arg_ptr);
	    return -1;
	  }
	  arg_list[j]->type |= USED_ARG;   /* Flag arg as already used. */
	  
	  k = i + 1;
	  
	  if(arg_list[j]->type & NO_ARG) {
	    /* skip if no arguments are expected. */
	  } else if(arg_list[j]->type & IS_MULTI) {
	    /* scan to next -flag, multiple args (-multi one two three) */
	    while(argv[k] != NULL && !is_arg(argv[k])) {
	      k++;
	    }
	  } else {
	    /* if a time arg, there are two args (-time 9 secs) */
	    if(arg_list[j]->type & TIME_ARG) {
	      if (argv[k] != NULL && !is_arg(argv[k])) {
		k++;
	      }
	    }
	    
	    /* skip just one arg */
	    if(argv[k] != NULL && !is_arg(argv[k])) {
	      k++;
	    }
	  }
	  
	  arg_list[j]->value = &argv[i + 1];
	  arg_list[j]->num_args = k - i - 1;
	  i = k;
	}
      }
    }
      
    /* not matched, warn only when warn_unused is true */
    if(num_args_matched != 1) {
      if(warn_unused && num_args_matched == 0) {
	fprintf(stderr, "%s: unknown flag '%s'. See -help.\n",
		argv[0], arg_ptr);
	return -1;
      }
      
      if(num_args_matched > 1) {
	fprintf(stderr, "%s: flag specified multiple times: '%s'. "
		"See -help.\n", argv[0], arg_ptr);
	return -1;
      }
      
      i++;
      while(argv[i] && !is_arg(argv[i])) {
	i++;
      }
    }
  }
    
  /* see if there are special args: -help, -version */
  for(i = 0; i < arg_list_size; i++) {
    if((arg_list[i]->type & HELP_ARG) && arg_list[i]->num_args != -1) {
      /* print help and cause main() to exit */
      print_args_help(argv[0], arg_list, arg_list_size);
      return -1;
    } else if((arg_list[i]->type & VERS_ARG) &&
	      arg_list[i]->num_args != -1) {
      /* print version and cause main() to exit */
      fprint_version(stderr, program_name);
      return -1;
    }
  }
  
  /* see if the args violate any conditions placed on the flag */
  for(i = 0; i < arg_list_size; i++) {
    arg_list[i]->type &= ~USED_ARG; /* In case parse_args is called twice. */
    type = arg_list[i]->type;
    num_args = arg_list[i]->num_args;
    switch (type & ~(IS_OPT | IS_MULTI | NO_ARG)) {
    case NAME_ARG:
    case FILE_ARG:
    case NUMB_ARG:
      if((type & IS_OPT) == 0 && num_args == -1) {
	goto required_param;
      } else if((type & IS_MULTI) == 0 && num_args > 1) {
	goto extra_arg;
      } else if ((type & NO_ARG) == 0 && num_args == 0) {
	goto missing_arg;
      }
      
      if(type & NUMB_ARG) {
	for(j = 0; j < num_args; j++) {
	  if(!check_all_digit_str(arg_list[i]->value[j])) {
	    goto bad_arg;
	  }
	}
      }
      break;
      
    case TIME_ARG:
      if(num_args > 0) {
	if(!check_all_digit_str(arg_list[i]->value[0])) {
	  goto bad_arg;
	}
	/* If argument is not zero, then units are required. */
	if(num_args == 1) {
	  if(atol(arg_list[i]->value[0]) != 0) {
	    goto bad_time;
	  }
	} else if(time_unit_to_mult_factor(arg_list[i]->value[1]) <= 0) {
	  goto bad_time;
	}
	if((type & IS_MULTI) == 0 && num_args > 2) {
	  goto extra_arg;
	}
      } else if((type & IS_OPT) == 0 && num_args == -1) {
	goto required_param;
      } else if((type & NO_ARG) == 0 && num_args == 0) {
	goto missing_arg;
      }
      break;
      
    default:
      if(type & NO_ARG) {
	if((type & IS_OPT) == 0 && num_args == -1) {
	  goto required_param;
	} else if(num_args > 0) {
	  goto extra_arg;
	}
      }
      break;
    }
  }

  return 0;
    
	       required_param:
  fprintf(stderr, "%s: the parameter '%s' is required. See -help.\n",
	  argv[0], arg_list[i]->flag ? arg_list[i]->flag : "");
  return -1;
  
	       missing_arg:
  fprintf(stderr, "%s: missing argument(s) after '%s'. See -help.\n",
	  argv[0], arg_list[i]->flag ? arg_list[i]->flag : "");
  return -1;
  
	       extra_arg:
  fprintf(stderr, "%s: extra argument after '%s'. See -help.\n",
	  argv[0], arg_list[i]->flag ? arg_list[i]->flag : "");
  return -1;
  
	       bad_arg:
  fprintf(stderr, "%s: invalid argument after '%s'. See -help.\n",
	  argv[0], arg_list[i]->flag ? arg_list[i]->flag : "");
  return -1;
  
	       bad_time:
  fprintf(stderr, "%s: expecting sec|min|hour|day for '%s' See -help..\n",
	  argv[0], arg_list[i]->flag ? arg_list[i]->flag : "");
  return -1;
}


void print_args_help(char *argv0, arg *arg_list[], int arg_list_size) {
  int i, len, max_len, col;
  char *ptr, *word;
  int *lens;
#define ZLEN(x) (((x) > 0) ? (x) : 0)
  
  fprintf(stderr, "usage: %s\n", argv0);
  max_len = 0;
  lens = (int *)malloc(sizeof(int) * arg_list_size);
  for(i = 0; i < arg_list_size; i++) {
    if(arg_list[i]->flag != NULL) {
      if(arg_list[i]->ex_arg != NULL) {
        len = strlen(arg_list[i]->flag) + strlen(arg_list[i]->ex_arg) + 1;
      } else {
        len = strlen(arg_list[i]->flag);
      }
    } else if(arg_list[i]->ex_arg != NULL) {
      len = strlen(arg_list[i]->ex_arg);
    } else {
      len = 0;
    }

    if(len > max_len) {
      max_len = len;
    }

    lens[i] = len;
  }

  if(max_len > 30) {
    max_len = 30;
  }

  for(i = 0; i < arg_list_size; i++) {
    len = lens[i];
    if(arg_list[i]->ex_arg != NULL) {
      if(arg_list[i]->flag != NULL) {
        fprintf(stderr, "  %s %s: %*s", arg_list[i]->flag,
                arg_list[i]->ex_arg, ZLEN(max_len - len), "");
      } else {
        fprintf(stderr, "  %s: %*s", arg_list[i]->ex_arg,
                ZLEN(max_len - len), "");
      }
    } else {
      if(arg_list[i]->flag != NULL) {
        fprintf(stderr, "  %s: %*s", arg_list[i]->flag,
                ZLEN(max_len - len), "");
      } else {
        fprintf(stderr, "  : %*s", max_len, "");
      }
    }

    if(lens[i] > max_len) {
      col = 75 - lens[i];
    } else {
      col = 75 - max_len;
    }

    for(ptr = arg_list[i]->descr; *ptr;) {
      word = ptr;
      while(*word && *word != ' ') {
        word++;
      }

      if(*word == '\0') {
        col--;   /* Allow room for period added to end. */
      }
      if(word - ptr > col) {
        fprintf(stderr, "\n%*s", max_len + 4, "");
        col = 75 - max_len;
      }

      while(ptr < word) {
        fprintf(stderr, "%c", *ptr);
        ptr++;
        col--;
      }

      while(*ptr == ' ') {
        if(col > 0) {
          fprintf(stderr, " ");
	}
        ptr++;
        col--;
      }
    }

    fprintf(stderr, ".\n");
  }

  free(lens);
}

void fprint_version(FILE *outfile, char *program_name) {
  fprintf(outfile, "%s version %s, %s\n",
	  program_name, __BUILD_VERSION__, __BUILD_DATE__);
}

long arg_to_seconds(arg *arg_el) {
  long secs, mult_factor;
  
  if(arg_el->num_args <= 0) {
    return 0;
  }

  secs = atol(arg_el->value[0]);
  if(arg_el->num_args > 1) {
    mult_factor = time_unit_to_mult_factor(arg_el->value[1]);
    if(mult_factor > 1) {
      secs *= mult_factor;
    }
  }

  return secs;
}
