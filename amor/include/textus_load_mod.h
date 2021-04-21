/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 $Date$
 $Revision$
*/
/* $NoKeywords: $ */

#if defined( _WIN32 )
#define TMODULE HINSTANCE
#define TEXTUS_GET_ADDR(handle, symbol, proc, type ) \
	proc = (type) GetProcAddress((HINSTANCE) handle, symbol)
#define TEXTUS_LOAD_MOD(x, flag) LoadLibrary(x)
#define TEXTUS_FREE_MOD(x) FreeLibrary(x)
#define TEXTUS_MOD_DLERROR ""
#define TEXTUS_MOD_SUFFIX ".dll"

#elif defined(__hpux)
/* HPUX requires shl_* routines */
#include <dl.h>
#define TMODULE shl_t
#define TEXTUS_GET_ADDR(handle, symbol, proc, type) \
	 (shl_findsym(&handle, symbol, (short) TYPE_PROCEDURE, (void *) &proc) == 0)

#define TEXTUS_LOAD_MOD(x, flag) shl_load(x, BIND_DEFERRED|BIND_VERBOSE|DYNAMIC_PATH, 0L)
#define TEXTUS_FREE_MOD(x) shl_unload((shl_t)x)
#define TEXTUS_MOD_DLERROR ""
#define TEXTUS_MOD_SUFFIX ".sl"

#else
#include <dlfcn.h>
#define TMODULE void *
#define TEXTUS_GET_ADDR(handle, symbol, proc, type) \
	proc = (type) dlsym(handle, symbol)
#define TEXTUS_LOAD_MOD(x, flag) dlopen(x, RTLD_NOW | flag)
#define TEXTUS_FREE_MOD(x) dlclose(x)
#define TEXTUS_MOD_DLERROR dlerror()
#define TEXTUS_MOD_SUFFIX ".so"

#endif 

