/*************************************************************************
 * $Id: Tree.hxx,v 1.4 2000/11/14 06:15:03 samurai Exp $
 * $Author: samurai $
 * $Date: 2000/11/14 06:15:03 $
 * $Locker:  $
 * $Revision: 1.4 $
 *
 * Created:  11/17/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _TREE_HXX_
#define _TREE_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI TreeException : public Exception {

  DECLARE_RUNTIME_SERIALIZABLE(TreeException);

public:

  TreeException();
  TreeException(const char *msg);

};

class Tree;

class UTILAPI TreeNode : public BaseObject {

};

class UTILAPI Tree : public BaseObject {

};

// Inline functions

inline TreeException::TreeException() : Exception() {

}

inline TreeException::TreeException(const char *msg) : Exception(msg) {

}

__END_NAMESPACE(SELFSOFT);

#endif
