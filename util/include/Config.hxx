/*************************************************************************
 * $Id: Config.hxx,v 1.4 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
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

#ifndef _CONFIG_HXX_
#define _CONFIG_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "String.hxx"
#include "Dlist.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class Config;

class UTILAPI ConfigEntry : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(ConfigEntry);

public:

  ConfigEntry();
  ~ConfigEntry();

private:

  friend class Config;

  String comments;
  String variableName;
  String rawValue;
  String realValue;

};

class UTILAPI Config : public BaseObject {

  DECLARE_RUNTIME_SERIALIZABLE(Config);

public:

  Config();
  ~Config();

  boolean readFromFile(const char *file);
  boolean saveToFile(const char *file);

  void reset();

private:

  Dlist<ConfigEntry> _entries;

};

__END_NAMESPACE(SELFSOFT);

#endif
