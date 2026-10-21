/*************************************************************************
 * $Id: DllManager.hxx,v 1.7 2001/08/09 03:11:08 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/09 03:11:08 $
 * $Locker:  $
 * $Revision: 1.7 $
 *
 * Created:  1/12/99 by Charles Chiou
 * Comments:
 *     This classes is used to manage dynamic linked libraries (DLL) and
 *     shared objects (on UNIX) and function pointers.
 *     The usage of this DllManager is as follows:
 *
 *     typedef size_t (*STRLENF)(const char *s);
 *     DllManager dllManager;
 *     STRLENF strlenf= (STRLENF) dllManager.getSymbol("libc.so", "strlen");
 *     if(strlenf == NULL) {
 *       cout << dllManager.getErrorMessage() << endl;
 *     } else {
 *       size_t size = strlenf("a character string");
 *       cout << size << endl;
 *     }
 *
 *     The DllManager class uses the Lib class to retrieve symbols from that
 *     particular lib module. The Lib objects are stored as a list in the
 *     library for simplicity. The assumption is that there will not be a great
 *     number of library modules to load for a given apllication. Thus, using
 *     an array and performing an iterative search is probably more efficient
 *     than using a hash table.
 *
 *     On the other hand, there can be many symbols associated with a given
 *     library module, therefore, the set of symbols are stored in a hash
 *     table inside the Lib class for fast lookups.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _DLLMANAGER_HXX_
#define _DLLMANAGER_HXX_

#ifdef __USE_WIN_DLL__
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "utilapi.h"
#include "BaseObject.hxx"
#include "Exception.hxx"
#include "String.hxx"
#include "Slist.hxx"
#include "HashTable.hxx"
#include "Void.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

class UTILAPI DllException : public Exception {

    DECLARE_RUNTIME_SERIALIZABLE(DllException);

public:

    DllException();
    DllException(const char *msg);

};

class DllManager;

class UTILAPI Dll : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(Dll);

public:

    ~Dll();

    void *getSymbol(const char *symbol) throw(DllException);

private:

    friend class DllManager;

    static Dll *createDllObject(const char *libraryName, const char *libraryPath)
        throw(DllException);
    Dll();

#ifdef __USE_WIN_DLL__
    HINSTANCE _dlhandle;
#else
    void *_dlhandle;
#endif

    String _libraryName;
    String _libraryPath;
    HashTable<String, Void> _symbolTable;

};

class UTILAPI DllManager : public BaseObject {

    DECLARE_RUNTIME_DISCOVERABLE(DllManager);

public:

    DllManager();
    ~DllManager();

    boolean loadLibrary(const char *library);
    boolean unloadLibrary(const char *library);
    boolean isLibraryLoaded(const char *library) const;

    void *getSymbol(const char *library, const char *symbol);
    const char *getErrorMessage() const;

    boolean addSearchPath(const char *path);
    boolean removeSearchPath(const char *path);
    Slist<String> getSearchPaths() const;

protected:

    void loadSystemSearchPaths();
    Dll *getDll(const char *library);

    HashTable<String, Ptr<Dll> >  _libraryTable;
    Slist<String> _systemSearchPaths;
    Slist<String> _userSearchPaths;
    String _lastErrMsg;

};

// Inline functions

inline DllException::DllException() : Exception() {

}

inline DllException::DllException(const char *msg) : Exception(msg) {

}

inline Dll::Dll() : _symbolTable(hashString) {

}

inline DllManager::DllManager() : _libraryTable(hashString) {
    loadSystemSearchPaths();
}

inline DllManager::~DllManager() {

}

inline boolean DllManager::loadLibrary(const char *library) {
    return getDll(library) != NULL;
}

inline const char *DllManager::getErrorMessage() const {
    return _lastErrMsg;
}

inline boolean DllManager::addSearchPath(const char *path) {
    if(_userSearchPaths.firstIndexOf(path) >= 0) {
        return FALSE;
    }

    if(path != NULL && strcmp(path, ".") != 0) {
        _userSearchPaths.append(path);
    }
    return TRUE;
}

inline boolean DllManager::removeSearchPath(const char *path) {
    return _userSearchPaths.removeElement(path);
}

inline Slist<String> DllManager::getSearchPaths() const {
    Slist<String> searchPaths(_systemSearchPaths);
    searchPaths.append(_userSearchPaths);
    return searchPaths;
}

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
