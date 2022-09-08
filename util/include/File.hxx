/*************************************************************************
 * $Id: File.hxx,v 1.2 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.2 $
 *
 * Created:  11/17/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _FILE_HXX_
#define _FILE_HXX_

#include "utilapi.h"
#include "BaseObject.hxx"
#include "IOException.hxx"
#include "Slist.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI File : public BaseObject {

    DECLARE_RUNTIME_SERIALIZABLE(File);

public:

    File(const char *file = NULL);
    ~File();

    static File *createTempFile() throw(IOException);

    const char *getName() const;
    const char *getAbsoluteName() const;
    const char *getAbsolutePath() const;


    boolean exists() const;
    boolean isFile() const;
    boolean isDirectory() const;
    boolean isHidden() const;

    boolean isReadable() const;
    boolean isWritable() const;
    boolean isExecutable() const;
    boolean setReadable() throw(IOException);
    boolean setWritable() throw(IOException);
    boolean setExecutable() throw(IOException);

    long lastModified() const;
    long setLastModified() const;
    long size() const;

    boolean createFile() throw(IOException);
    boolean deleteFile() throw(IOException);
    boolean moveTo(const char *file) throw(IOException);

    Slist<File> list() throw(IOException);

};

// Inline functions

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
