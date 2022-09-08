/*************************************************************************
 * $Id: basecfg.h,v 1.11 2001/08/15 00:57:21 samurai Exp $
 * $Author: samurai $
 * $Date: 2001/08/15 00:57:21 $
 * $Locker:  $
 * $Revision: 1.11 $
 *
 * Created:  1/24/99 by Charles Chiou
 * Comments: Place all OS/platform dependent definitions here.
 * Status:
 * Bugs:
 *
 * Copyright (c) 1999 Selfsoft Corp. All rights reserved.
 *************************************************************************/

#ifndef _BASECFG_H_
#define _BASECFG_H_

#include <stddef.h>

/***************************/
/* Default version string. */
/***************************/

#define _GENERAL_COPY_TM_MESSAGE_ "(c) Charles Chiou"
#define _DEFAULT_VERSION_ "v1.0"

/*********************/
/* Type definitions. */
/*********************/
typedef unsigned char boolean;
typedef unsigned char byte;
typedef unsigned short word16;
typedef short int16;
#ifdef __alpha
#ifdef _MSC_VER
typedef unsigned long word32;
typedef long int32;
#else  /* __alpha && _MSC_VER */
typedef unsigned int word32;
typedef int int32;
#endif /* __alpha && _MSC_VER */
#else  /* !__alpha */
typedef unsigned long word32;
typedef long int32;
#endif /* __alpha */

#if defined(__GNUC__) || defined(__MWERKS__)
#define __HAS_WORD64__
typedef unsigned long long word64;
typedef long long int64;
#define WORD64STR(s) s##LL
#elif defined(_MSC_VER) || defined(__BCPLUSPLUS__)
#define __HAS_WORD64__
typedef unsigned __int64 word64;
typedef __int64 int64;
#define WORD64STR(s) s##ui64
#endif  /* __GNUC__ ... */

// word should have the same size as the CPU registers
// dword should be twice as big as word
#if (defined(__GNUC__) && !defined(__alpha)) || defined(__MWERKS__)
typedef unsigned long word;
typedef unsigned long long dword;
#elif defined(_MSC_VER) || defined(__BCPLUSPLUS__)
typedef unsigned __int32 word;
typedef unsigned __int64 dword;
#else
typedef unsigned int word;
typedef unsigned long dword;
#endif

#define TRUE 1
#define true TRUE
#define FALSE 0
#define false FALSE

#if defined(__i386__) || defined(__i486__) || defined(__i586__) ||  \
    defined(__i686__) || defined(__alpha__) || defined(__arm__)
#define IS_LITTLE_ENDIAN
#else  /* Most other CPUs */
#define IS_BIG_ENDIAN
#endif /* 386, 486, ... */

/**********************************************/
/* Language-dependent prototype declarations. */
/**********************************************/
#ifdef __cplusplus
#define __BEGIN_CPLUSPLUS_DECLS extern "C++" {
#define __END_CPLUSPLUS_DECLS }
#define __BEGIN_C_DECLS extern "C" {
#define __END_C_DECLS }
#else
#define __BEGIN_C_DECLS
#define __END_C_DECLS
#endif /* __cplusplus */

/************************************/
/* Filesystem specific definitions. */
/************************************/
#ifdef _WIN32
#define LINE_SEPARATOR '\r'
#define FILE_SEPARATOR '\\'
#define PATH_SEPARATOR ';'
#define TEMP_DIR       "C:\\TEMP"
#else  /* !_WIN32 */
#define LINE_SEPARATOR '\n'
#define FILE_SEPARATOR '/'
#define PATH_SEPARATOR ':'
#define TEMP_DIR       "/tmp"
#endif /* _WIN32 */

/************************/
/* Literal definitions. */
/************************/
/* process.server@host:port */
#define SERVER_SEP_CHAR '.'
#define HOST_SEP_CHAR   '@'
#define PORT_SEP_CHAR   ':'


/******************************************/
/* Thread library dependent declarations. */
/******************************************/
#ifdef _WIN32
#define __USE_WINTHREAD__
#else  /* !_WIN32 */
#define __USE_POSIXTHREAD__
#endif /* _WIN32 */

/***************************/
/* Win32 DLL declarations. */
/***************************/
#ifdef _WIN32
#define WEXPORT __declspec (dllexport)
#define WIMPORT __declspec (dllimport)
#define __USE_WIN_DLL__
#else  /* !_WIN32 */
#define WEXPORT
#define WIMPORT
#define __USE_UNIX_SO__
#endif /* _WIN32 */

/*****************************************************/
/* Disable some warning messages under VC++ compiler */
/*****************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4251)  // Disable warnings on dll-interface used in templates,
#pragma warning(disable:4275)  // Disable warnings on dll-interface used in inheritence.
#pragma warning(disable:4290)  // Disable warnings on exception specification on prototypes.
#pragma warning(disable:4786)  // Diasble warnings on debug info trunctation in generated object codes
#endif

/***********************/
/* Namespaces for C++. */
/***********************/
#define SELFSOFT selfsoft
#ifdef __cplusplus
#ifdef __GNUC__  /* GNU compilers don't support namespace */
#define __USING_NAMESPACE(x)
#define __BEGIN_NAMESPACE(x)
#define __END_NAMESPACE(x)
#else
#define __USING_NAMESPACE(x) using namespace x;
#define __BEGIN_NAMESPACE(x) namespace x {
#define __END_NAMESPACE(x) }
#endif /* !__GUNC__ */
#endif /* __cplusplus */

#endif /* !_BASECFG_H_ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
