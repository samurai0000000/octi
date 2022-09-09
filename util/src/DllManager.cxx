/*************************************************************************
 * $Id: DllManager.cxx,v 1.8 2000/03/16 05:01:59 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:59 $
 * $Locker:  $
 * $Revision: 1.8 $
 *
 * Created:  1/12/99 by Charles Chiou
 * Comments:
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#define _UTILAPI_IMPL_

#include <string.h>
#include <stdlib.h>
#include "Void.hxx"
#include "StringTokenizer.hxx"
#include "DllManager.hxx"

__BEGIN_NAMESPACE(SELFSOFT);

IMPLEMENT_RUNTIME_SERIALIZABLE(DllException, Exception, 1);
IMPLEMENT_RUNTIME_DISCOVERABLE(Dll, BaseObject);
IMPLEMENT_RUNTIME_DISCOVERABLE(DllManager, BaseObject);

#ifdef __USE_WIN_DLL__
static const char *dlerror() {
    static String msg;
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM     |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf, 0, NULL);

    msg = (char *) lpMsgBuf;
    LocalFree(lpMsgBuf);
    return msg;
}
#endif

Dll *Dll::createDllObject(const char *libraryName, const char *libraryPath)
    throw(DllException) {
    Dll *newDll = NULL;

    String libraryFile = libraryPath;
    if(!libraryFile.endsWith(FILE_SEPARATOR)) {
        libraryFile += FILE_SEPARATOR;
    }
    libraryFile += libraryName;

#ifdef __USE_WIN_DLL__
    LPCTSTR lpLibFileName = libraryFile;
    HINSTANCE dlhandle = LoadLibrary(lpLibFileName);
#else
    void *dlhandle = dlopen(libraryFile, RTLD_LAZY);
#endif

    if(dlhandle != NULL) {
        newDll = new Dll();
        newDll->_dlhandle = dlhandle;
        newDll->_libraryName = libraryName;
        newDll->_libraryPath = libraryPath;
    } else {
        throw DllException(dlerror());
    }

    return newDll;
}

Dll::~Dll() {
#ifdef __USE_WIN_DLL__
    FreeLibrary(_dlhandle);
#else
    dlclose(_dlhandle);
#endif
}

void *Dll::getSymbol(const char *symbol) throw(DllException) {
    void *fptr = NULL;

    if((fptr = (Void *) _symbolTable.get(String (symbol))) != NULL) {
        return fptr;
    }

    ASSERT(_dlhandle != NULL);

#ifdef __USE_WIN_DLL__
    LPCSTR lpProcName = symbol;
    fptr = (void *) GetProcAddress(_dlhandle, lpProcName);
#else
    fptr = dlsym(_dlhandle, (char *) (const char *) symbol);
#endif

    if(fptr == NULL) {
        throw DllException(dlerror());
    }

    _symbolTable.put(String(symbol), Void(fptr));

    return fptr;
}

boolean DllManager::unloadLibrary(const char *library) {
    return _libraryTable.remove(String(library));
}

void *DllManager::getSymbol(const char *library, const char *symbol) {
    Dll *dll = NULL;
    if((dll = getDll(library)) == NULL) {
        return NULL;
    }

    void *fptr = NULL;
    try {
        fptr = dll->getSymbol(symbol);
    } catch(DllException e) {
        _lastErrMsg = e.getMessage();
    }

    return fptr;
}

void DllManager::loadSystemSearchPaths() {
#ifdef __USE_WIN_DLL__
    StringTokenizer tok(getenv("PATH"), " \t\n;");
#else
    StringTokenizer tok(getenv("LD_LIBRARY_PATH"), " \t\n:");
#endif
    const char *path;
    while((path = tok.nextToken()) != NULL) {
        if(strcmp(path, ".") != 0) {
            _systemSearchPaths.append(path);
        }
    }
}

Dll *DllManager::getDll(const char *library) {
    Dll *dll = NULL;

    if((dll = (Dll *) _libraryTable.get(library)) != NULL) {
        return dll;
    }

    String librarySpec = library;
    String libraryName;
    String libraryPath;

    if(librarySpec.contains(FILE_SEPARATOR)) {
        int idx = librarySpec.lastIndexOf(FILE_SEPARATOR);
        libraryName = librarySpec.after(idx);
        libraryPath = librarySpec.before(idx);

        try {
            dll = Dll::createDllObject(libraryName, libraryPath);
            _libraryTable.put(librarySpec, dll);
        } catch(DllException e) {
            _lastErrMsg = e.getMessage();
        }
    } else {
#ifdef __USE_WIN_DLL__
        if(librarySpec.endsWith(".dll")) {
            libraryName = librarySpec;
        } else {
            libraryName = librarySpec;
            libraryName += ".dll";
        }
#else
        if(librarySpec.endsWith(".so") || librarySpec.contains(".so.")) {
            libraryName = librarySpec;
        } else {
            libraryName = "lib";
            libraryName += librarySpec;
            libraryName += ".so";
        }
#endif

        // Try the system paths
        Ptr<SlistIterator<String> > it;
        it = (SlistIterator<String> *) _systemSearchPaths.getIterator();
        while(it->isNextNotNull()) {
            libraryPath = *it->next();
            if(!libraryPath.endsWith(FILE_SEPARATOR)) {
                libraryPath.append(FILE_SEPARATOR);
            }

            try {
                dll = Dll::createDllObject(libraryName, libraryPath);
                if(!dll) {
                    _libraryTable.put(librarySpec, dll);
                    break;
                }
            } catch(DllException e) {
                _lastErrMsg = e.getMessage();
            }
        }

        // Try the user paths
        it = (SlistIterator<String> *) _userSearchPaths.getIterator();
        while(it->isNextNotNull()) {
            libraryPath = *it->next();
            if(!libraryPath.endsWith(FILE_SEPARATOR)) {
                libraryPath.append(FILE_SEPARATOR);
            }

            try {
                dll = Dll::createDllObject(libraryName, libraryPath);
                if(!dll) {
                    _libraryTable.put(librarySpec, dll);
                    break;
                }
            } catch(DllException e) {
                _lastErrMsg = e.getMessage();
            }
        }

        // Try the current path
        libraryPath = '.';
        libraryPath += FILE_SEPARATOR;
        try {
            dll = Dll::createDllObject(libraryName, libraryPath);
            _libraryTable.put(librarySpec, dll);
        } catch(DllException e) {
            _lastErrMsg = e.getMessage();
        }
    }

    return dll;
}

__END_NAMESPACE(SELFSOFT);

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
