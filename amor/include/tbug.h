/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 $Date: 08-01-03 20:13 $
 $Revision: 12 $
*/

/* $NoKeywords: $ */

#ifndef TEXTUS_TBUG_H
#define TEXTUS_TBUG_H
#ifndef NDEBUG
	#if defined( _MSC_VER )
	#include "textus_string.h"
	#include <windows.h>
	#if (_MSC_VER >= 1400 )
	#ifndef WBUG
	#define WBUG(...) { \
		char errMsg[2314]; \
		char msg[2048]; \
		TEXTUS_SNPRINTF(msg, sizeof(msg)-1, __VA_ARGS__); \
		TEXTUS_SNPRINTF(errMsg, sizeof(errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, msg); \
		OutputDebugString(errMsg); \
		}
	#endif
	#else
	void inline WBUG(char *format,...)
	{
		va_list va;
		char errMsg[2314]; 
		char msg[2048]; 
		va_start(va, format);
		TEXTUS_VSNPRINTF(msg, sizeof(msg)-1, format, va); 
		va_end(va);
		TEXTUS_SNPRINTF(errMsg, sizeof(errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, msg); 
		OutputDebugString(errMsg); 
	}
	#endif /* for _MSC_VER >= 1400 */
	#else
	#include <stdio.h>
	#if defined(__GNUC__) && (__GNUC__ >= 3 ) || defined(__SUNPRO_CC)  && (__SUNPRO_CC >= 0x560)
	#ifndef WBUG
	#define WBUG(...) {printf("%s(%d) ", __FILE__, __LINE__); printf (__VA_ARGS__); printf("\n");}
	#endif
	#else
	#include <stdarg.h>
	void inline WBUG(char *format,...) {
		va_list va;
		printf("%s(%d) ", __FILE__, __LINE__); 
		va_start(va, format);
		vprintf (format, va); 
		va_end(va);
		printf("\n");
	}
	#endif	/* for __GNUC__ >= 3*/
	#endif
#else
#if defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x560) || defined( _MSC_VER ) && (_MSC_VER >= 1400 ) || defined(__GNUC__) && (__GNUC__ >= 3 )
	#ifndef WBUG
	#define WBUG(...)
	#endif
#else
	void inline WBUG(char *format,...) { }
#endif
#endif
#if !defined (WLOG_OSERR)
#if defined (_WIN32 )
#define WLOG_OSERR(X) { \
	char amor_errMsg[1024]; \
	memset(amor_errMsg,0,sizeof(amor_errMsg)); \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	TEXTUS_SNPRINTF(amor_errMsg, 1024, "%s(%d) %s errno %d, %s", __FILE__, __LINE__, X, dw, error_string);\
	OutputDebugString(amor_errMsg); \
	}
#else
#define WLOG_OSERR(X)  \
	printf("%s(%d) %s errno %d, %s.", __FILE__, __LINE__, X, errno, strerror(errno));

#endif	/* for _WIN32 */
#endif

#endif	/* for H */
