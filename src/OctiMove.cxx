/*************************************************************************
 * $Id: OctiMove.cxx,v 1.5 2001/06/19 00:45:50 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/06/19 00:45:50 $
 * $Locker:  $
 * $Revision: 1.5 $
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
#include "OctiMove.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(OctiMoveOp, BaseObject, 1);
IMPLEMENT_RUNTIME_SERIALIZABLE(OctiMove, BaseObject, 1);

static OctiDir get_dir_value_of(char c) {
  if(c >= 'A' && c <= 'H') {
    return (OctiDir) (c - 'A');
  } else if(c >= 'a' && c <= 'h') {
    return (OctiDir) (c - 'a');
  }
  
  return DIR_UNDEF;
}

OctiMove::OctiMove(const char *s) throw(IllegalMoveSyntaxException) {
  if(s == NULL) {
    throw IllegalMoveSyntaxException("s is NULL");
  }

  _notation = s;
  _proc_s = s;
  _proc_index = 0;
  _proc_s_length = _notation.length();
  _x = -1;
  _y = -1;

  for(_state = S_START; _state != S_FINISH; nextState()) {
    if(_state == S_ERROR) {
      String msg = "invalid move syntax: ";
      msg += s;
      throw IllegalMoveSyntaxException(msg);
    }
  }
}

void OctiMove::nextState() {

  // Classify the characters first
  CharType t = T_ERR;
  char c = _proc_index >= _proc_s_length ? '\0' : _proc_s[_proc_index];
  int peek;

  if(c >= '1' && c <= '9') {
    t = T_NUM;
  } else if((c >= 'A' && c <= 'H') || (c >= 'a' && c <= 'h')) {
    t =  T_PRO;
  } else {
    switch(c) {
    case '+':
      t = T_ARP;
      break;
    case '-':  // Need peeking to get what '-' means
      peek = peekMove();
      if(peek == 0) {
	t = T_MOV;
      } else if(peek == 1) {
	t = T_ARP;
      } else {
	t = T_ERR;
      }
      break;
    case '!':
      t = T_SUP;
      break;
    case 'x':
    case 'X':
      t = T_CAP;
      break;
    case ',':
      t = T_COM;
      break;
    case ' ':
    case '\t':
    case '\n':
      t = T_WHI;
      break;
    case '\0':
      t = T_TER;
      break;
    default:
      t = T_ERR;
      break;
    }
  }
	
  // Ignore white spaces
  if(t == T_WHI) {
    _proc_index++;
    return;
  }

  // Terminate on syntax error
  if(t == T_ERR) {
    _state = S_ERROR;
    return;
  }
  
  // State transitions
  switch(_state) {
  case S_START:
    if(t != T_NUM) {
      _state = S_ERROR;
    } else {
      // Create the start of a new move chain
      _current = new OctiMoveOp();
      _current->reset();
      _current->x0 = c - '1';
      _state = S_READX0;
    }
    break;
  case S_READX0:
    if(t != T_NUM) {
      _state = S_ERROR;
    } else {
      _current->y0 = c - '1';

      if(_x == -1 && _y == -1) {
	_x = _current->x0;
	_y = _current->y0;
	_state = S_READY0;
      } else {
	if(_current->x0 == _x && _current->y0 == _y) {
	  _state = S_READY0;
	} else {
	  _state = S_ERROR;  // Illegal move of different squares
	}
      }
    }
    break;
  case S_READY0:
    if(t == T_PRO) {
      // Peek ahead for super prong designation
      char sup = _proc_index + 1 >= _proc_s_length ? '\0' : _proc_s[_proc_index + 1];
      if(sup == '!') {
	_current->pod.addProng((OctiDir) get_dir_value_of(c), TRUE);
	_proc_index++;
      } else {
	_current->pod.addProng((OctiDir) get_dir_value_of(c), FALSE);
      }
      _state = S_READY0;
    } else if(t == T_ARP && _ops.size() == 0)  {
      _lastAddRemoveIsAdd = (c == '+');
      _state = S_ADDRMV1;
    } else if(t == T_MOV) {
      _state = S_MOVE;
    } else if(t == T_TER) {
      _current->x1 = _current->x0;
      _current->y1 = _current->y0;
      _ops.append(_current);
      _state = S_FINISH;
    } else {
      _state = S_ERROR;
    }
    break;
  case S_ADDRMV1:
    if(t != T_PRO) {
      _state = S_ERROR;
    } else {
      if(_lastAddRemoveIsAdd) {
	if(_current->prongAddDir != DIR_UNDEF ||
	   _current->prongRemoveDir == get_dir_value_of(c)) {
	  _state = S_ERROR;
	} else {
	  _current->prongAddDir = get_dir_value_of(c);
	  // Peek for super prong
	  char sup = _proc_index + 1 >= _proc_s_length ? '\0' : _proc_s[_proc_index + 1];
	  if(sup == '!') {
	    _current->addSuperProng = TRUE;
	    _proc_index++;
	  }
	  _state = S_RHALF;
	}
      } else {
	if(_current->prongRemoveDir != DIR_UNDEF ||
	   _current->prongAddDir == get_dir_value_of(c)) {
	  _state = S_ERROR;
	} else {
	  _current->prongRemoveDir = get_dir_value_of(c);
	  // Peek for super prong
	  char sup = _proc_index + 1 >= _proc_s_length ? '\0' : _proc_s[_proc_index + 1];
	  if(sup == '!') {
	    _current->removeSuperProng = TRUE;
	    _proc_index++;
	  }
	  _state = S_RHALF;
	}
      }
    }
    break;
  case S_RHALF:
    if(t == T_TER) {
      _ops.append(_current);
      _state = S_FINISH;
    } else if(t == T_ARP) {
      _lastAddRemoveIsAdd = (c == '+');
      _state = S_ADDRMV2;
    } else {
      _state = S_ERROR;
    }
    break;
  case S_ADDRMV2:
    if(t != T_PRO) {
      _state = S_ERROR;
    } else {
      if(_lastAddRemoveIsAdd) {
	if(_current->prongAddDir != DIR_UNDEF ||
	   _current->prongRemoveDir == get_dir_value_of(c)) {
	  _state = S_ERROR;
	} else {
	  _current->prongAddDir = get_dir_value_of(c);
	  // Peek for super prong
	  char sup = _proc_index + 1 >= _proc_s_length ? '\0' : _proc_s[_proc_index + 1];
	  if(sup == '!') {
	    _current->addSuperProng = TRUE;
	    _proc_index++;
	  }
	  _state = S_RCOMP;
	}
      } else {
	if(_current->prongRemoveDir != DIR_UNDEF ||
	   _current->prongAddDir == get_dir_value_of(c)) {
	  _state = S_ERROR;
	} else {
	  _current->prongRemoveDir = get_dir_value_of(c);
	  // Peek for super prong
	  char sup = _proc_index + 1 >= _proc_s_length ? '\0' : _proc_s[_proc_index + 1];
	  if(sup == '!') {
	    _current->removeSuperProng = TRUE;
	    _proc_index++;
	  }
	  _state = S_RCOMP;
	}
      }
    }
    break;
  case S_RCOMP:
    if(t == T_TER) {
      _ops.append(_current);
      _state = S_FINISH;
    } else {
      _state = S_ERROR;
    }
    break;
  case S_MOVE:
    if(t != T_NUM) {
      _state = S_ERROR;
    } else {
      _current->x1 = c - '1';
      _state = S_READX1;
    }
    break;
  case S_READX1:
    if(t != T_NUM) {
      _state = S_ERROR;
    } else {
      _current->y1 = c - '1';
      _state = S_READY1;
    }
    break;
  case S_READY1:
    if(t == T_TER) {
      _ops.append(_current);
      _state = S_FINISH;
    } else if(t == T_CAP) {
      if(!_current->capture) {
	_current->capture = TRUE;
	_state = S_READY1;
      } else {
	_state = S_ERROR;
      }
    } else if(t == T_COM) {
      _ops.append(_current);
      _state = S_START;
    } else if(t == T_MOV) {
      _ops.append(_current);
      Ptr<OctiMoveOp> old = _current;
      _current = new OctiMoveOp();
      _current->reset();
      _current->x0 = old->x1;
      _current->y0 = old->y1;
      _current->pod = old->pod;
      _state = S_MOVE;
    } else {
      _state = S_ERROR;
    }
    break;
  }
	
  _proc_index++;
}

int OctiMove::peekMove() {
  int i;

  // Skip white spaces
  for(i = _proc_index + 1; i < _proc_s_length &&
	(_proc_s[i] == ' ' || _proc_s[i] == '\t' || _proc_s[i] == '\n');
      i++);
  
  if(i <= _proc_s_length) {
    if(_proc_s[i] >= '0' && _proc_s[i] <= '9') {
      return 0;
    } else if((_proc_s[i] >= 'A' && _proc_s[i] <= 'H') ||
	      (_proc_s[i] >= 'a' && _proc_s[i] <= 'h')) {
      return 1;
    }
  }
  
  return -1;
}

OctiMove::OctiMove(const Dlist<Ptr<OctiMoveOp> > &ops)
  throw(IllegalMoveSyntaxException) {
  Ptr<OctiMoveOp> op, prev, firstop;
  Ptr<DlistIterator<Ptr<OctiMoveOp> > > iterator =
    (DlistIterator<Ptr<OctiMoveOp> > *) ops.getIterator();

  iterator->first();
  if(!iterator->isNotNull()) {
    throw IllegalMoveSyntaxException("first op is null");
  }

  prev = firstop = *iterator->current();
  if(firstop->x0 >= 0 && firstop->x0 < 9 &&
     firstop->y0 >= 0 && firstop->y0 < 9 &&
     firstop->pod == OctiPod::EMPTY_POD &&
     firstop->x1 == firstop->x0 &&
     firstop->y1 == firstop->y0 &&
     firstop->capture == FALSE &&
     firstop->prongAddDir == DIR_UNDEF &&
     firstop->prongRemoveDir == DIR_UNDEF &&
     firstop->addSuperProng == FALSE &&
     firstop->removeSuperProng == FALSE &&
     !iterator->isNextNotNull()) {
    // add a pod
    _notation += ('1' + firstop->x0);
    _notation += ('1' + firstop->y0);
    _ops.append(new OctiMoveOp(*firstop.ptr()));
    _x = firstop->x0;
    _y = firstop->y0;
  } else if(firstop->x0 >= 0 && firstop->x0 < 9 &&
	    firstop->y0 >= 0 && firstop->y0 < 9 &&
	    firstop->x1 == firstop->x0 &&
	    firstop->y1 == firstop->y0 &&
	    firstop->capture == FALSE &&
	    firstop->prongAddDir != DIR_UNDEF &&
	    !iterator->isNextNotNull()) {
    // add or reposition a prong
    char buf[18];
    firstop->pod.getProngConfigString(buf);
    _notation += ('1' + firstop->x0);
    _notation += ('1' + firstop->y0);
    _notation += buf;
    _notation += '+';
    _notation += ('a' + firstop->prongAddDir);
    if(firstop->addSuperProng) {
      _notation += '!';
    }
    if(firstop->prongRemoveDir != DIR_UNDEF) {
      _notation += ('a' + firstop->prongRemoveDir);
      if(firstop->removeSuperProng) {
	_notation += '!';
      }
    } else if(firstop->removeSuperProng) {
      throw IllegalMoveSyntaxException("op specified remove super prong without direction");
    }
    _ops.append(new OctiMoveOp(*firstop.ptr()));
    _x = firstop->x0;
    _y = firstop->y0;
  } else if(firstop->x0 >= 0 && firstop->x0 < 9 &&
	    firstop->y0 >= 0 && firstop->y0 < 9 &&
	    firstop->x1 >= 0 && firstop->x1 < 9 &&
	    firstop->y1 >= 0 && firstop->y1 < 9 &&
	    (firstop->x0 != firstop->x1 || firstop->y0 != firstop->y1) && 
	    firstop->prongAddDir == DIR_UNDEF &&
	    firstop->prongRemoveDir == DIR_UNDEF &&
	    firstop->addSuperProng == FALSE &&
	    firstop->removeSuperProng == FALSE) {
    // move, jump, and capture
    char buf[18];
    firstop->pod.getProngConfigString(buf);
    _notation += ('1' + firstop->x0);
    _notation += ('1' + firstop->y0);
    _notation += buf;
    _notation += '-';
    _notation += ('1' + firstop->x1);
    _notation += ('1' + firstop->y1);
    if(firstop->capture) {
      _notation += 'x';
    }
    _ops.append(new OctiMoveOp(*firstop.ptr()));
    _x = firstop->x0;
    _y = firstop->y0;

    for(iterator->next(); iterator->isNotNull(); iterator->next()) {
      op = *iterator->current();
      if(op->x0 == prev->x1 && op->y0 == prev->y1 && op->pod == prev->pod &&
	 op->x0 >= 0 && op->x0 < 9 &&
	 op->y0 >= 0 && op->y0 < 9 &&
	 op->x1 >= 0 && op->x1 < 9 &&
	 op->y1 >= 0 && op->y1 < 9 &&
	 (op->x0 != op->x1 || op->y0 != op->y1) && 
	 op->prongAddDir == DIR_UNDEF &&
	 op->prongRemoveDir == DIR_UNDEF &&
	 op->addSuperProng == FALSE &&
	 op->removeSuperProng == FALSE) {
	_notation += '-';
	_notation += ('1' + op->x1);
	_notation += ('1' + op->y1);
	if(op->capture) {
	  _notation += 'x';
	}
	_ops.append(new OctiMoveOp(*op.ptr()));
      } else if(op->x0 != prev->x1 && op->y0 != prev->y1 &&
		op->x0 == firstop->x0 && op->y0 == firstop->y0 &&
		op->x0 >= 0 && op->x0 < 9 &&
		op->y0 >= 0 && op->y0 < 9 &&
		op->x1 >= 0 && op->x1 < 9 &&
		op->y1 >= 0 && op->y1 < 9 &&
		(op->x0 != op->x1 || op->y0 != op->y1) && 
		op->prongAddDir == DIR_UNDEF &&
		op->prongRemoveDir == DIR_UNDEF &&
		op->addSuperProng == FALSE &&
		op->removeSuperProng == FALSE) {
	_notation += ',';
	_notation += ('1' + op->x0);
	_notation += ('1' + op->y0);
	_notation += buf;
	_notation += '-';
	_notation += ('1' + op->x1);
	_notation += ('1' + op->y1);
	if(op->capture) {
	  _notation += 'x';
	}
	_ops.append(new OctiMoveOp(*op.ptr()));
      } else {
	throw IllegalMoveSyntaxException("ops failed in move");
      }
      prev = op;
    }
  } else {
    throw IllegalMoveSyntaxException("verify ops failed");
  }
}

__END_NAMESPACE(SELFSOFT);
