/*************************************************************************
 * $Id: BaseRuntimeClass.hxx,v 1.3 2000/03/16 05:01:57 cychiou Exp $
 * $Author: cychiou $
 * $Date: 2000/03/16 05:01:57 $
 * $Locker:  $
 * $Revision: 1.3 $
 *
 * Created:  9/10/99 by Charles Chiou
 * Comments: The BaseRuntimeClass class maintains run-time type information.
 *           This is inspired from CRuntimeClass from MFC.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BASERUNTIMECLASS_HXX_
#define _BASERUNTIMECLASS_HXX_

#include "utilapi.h"

__BEGIN_NAMESPACE(SELFSOFT);

// Macros for declaration and implementation of BaseRuntimeClass

#define DECLARE_RUNTIME_DISCOVERABLE(className)                 \
protected:                                                      \
    static const BaseRuntimeClass *_getParentClass();           \
    static const BaseRuntimeClass *CS_class##className;         \
public:                                                         \
    virtual const BaseRuntimeClass *getRuntimeClass() const;    \

#define DECLARE_RUNTIME_CREATABLE(className)    \
	DECLARE_RUNTIME_DISCOVERABLE(className)     \
    static BaseObject *createObject();          \


#define DECLARE_RUNTIME_SERIALIZABLE(className)                         \
	DECLARE_RUNTIME_CREATABLE(className)                                \
    /*friend CArchive& AFXAPI operator>>(CArchive& ar, className* &pOb);*/ \


#define BASERUNTIME_CLASS(className) className::CS_class##className

/*
// generate static object constructor for class registration
struct AFX_CLASSINIT
{ AFX_CLASSINIT(BaseRuntimeClass* pNewClass); };
*/

#define IMPLEMENT_BASERUNTIMECLASS(className, parentClassName,          \
                                   createObjectFn)                      \
                                                                        \
    const BaseRuntimeClass *className::_getParentClass() {              \
        return BASERUNTIME_CLASS(parentClassName);                      \
    }                                                                   \
                                                                        \
    const BaseRuntimeClass *className::CS_class##className =            \
        new BaseRuntimeClass(#className, sizeof(class className),       \
                             createObjectFn, className::_getParentClass); \
                                                                        \
    const BaseRuntimeClass *className::getRuntimeClass() const {        \
        return BASERUNTIME_CLASS(className);                            \
    }                                                                   \

#define IMPLEMENT_RUNTIME_DISCOVERABLE(className, parentClassName)  \
	IMPLEMENT_BASERUNTIMECLASS(className, parentClassName, NULL)

#define IMPLEMENT_RUNTIME_CREATABLE(className, parentClassName) \
	IMPLEMENT_BASERUNTIMECLASS(className, parentClassName,      \
                               className::createObject)         \
                                                                \
    BaseObject *className::createObject() {                     \
        return new className;                                   \
    }                                                           \


#define IMPLEMENT_RUNTIME_SERIALIZABLE(className, parentClassName, version) \
    IMPLEMENT_BASERUNTIMECLASS(className, parentClassName,              \
                               className::createObject)                 \
    BaseObject * className::createObject() {                            \
        return new className;                                           \
    }                                                                   \
                                                                        \
    /* static const AFX_CLASSINIT _init_##className(RUNTIME_CLASS(className)); */ \
    /* CArchive& AFXAPI operator>>(CArchive& ar, className* &pOb) */    \
    /*	{ pOb = (className*) ar.ReadObject(RUNTIME_CLASS(className)); */ \
    /*			return ar; } */                                         \

class BaseObject;

class UTILAPI BaseRuntimeClass {

    typedef BaseObject * (* createObjectFn)();
    typedef const BaseRuntimeClass * (* getParentClassFn)();

public:

    BaseRuntimeClass(const char *className,
                     const size_t objectSize,
                     createObjectFn createObject,
                     getParentClassFn getParentClass);
    ~BaseRuntimeClass();

    const char *getClassName() const;
    const BaseRuntimeClass *getParentClass() const;

private:

    friend class BaseObject;

    BaseObject *createObject();

    const char *_className;
    size_t _objectSize;
    createObjectFn _createObject;
    getParentClassFn _getParentClass;

};


// Inline functions below

inline BaseRuntimeClass::BaseRuntimeClass(const char *className,
                                          const size_t objectSize,
                                          createObjectFn createObject,
                                          getParentClassFn getParentClass) {
    _className = className;
    _objectSize = objectSize;
    _createObject = createObject;
    _getParentClass = getParentClass;
}

inline BaseRuntimeClass::~BaseRuntimeClass() {

}

inline const char *BaseRuntimeClass::getClassName() const {
    return _className;
}

inline const BaseRuntimeClass *BaseRuntimeClass::getParentClass() const{
    if(_getParentClass == NULL) {
        return NULL;
    }

    return (_getParentClass)();
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
