/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include <string.h>
#include <stdio.h>

/* $NoKeywords: $ */

#if defined( _WIN32 )
#include <windows.h>
#include <process.h>
#include <conio.h>

#define GETPID _getpid()
#else
#include <sys/types.h>
#include <unistd.h>
#define GETPID getpid()
#endif
#ifndef TEXTUS_STRING
#define TEXTUS_STRING
#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
	// Microsoft visual studio, version 2005 and higher.
	#define TEXTUS_STRCAT strcat_s
	#define TEXTUS_SPRINTF sprintf_s
	#define TEXTUS_STRNCPY strncpy_s
	#define TEXTUS_STRCPY(x,y) strcpy_s(x, strlen(y)+1, y)

	#define TEXTUS_SNPRINTF(x,y,...)  _snprintf_s(x,y+1,y,__VA_ARGS__)
	#define TEXTUS_VSNPRINTF(w,x,y,z) vsnprintf_s(w,x+1,x,y,z)
	#define TEXTUS_SSCANF  sscanf_s
	#define TEXTUS_FOPEN(x,y,z)  fopen_s(&x,y,z)
#else
 #if defined(_MSC_VER) && (_MSC_VER >= 1200 )
	// Microsoft visual studio, version 6 and higher.
	//#pragma message( "Using _sn* functions." )

	#define TEXTUS_SNPRINTF _snprintf
	#define TEXTUS_VSNPRINTF _vsnprintf
	#define TEXTUS_SNSCANF  _snscanf
 #else
	#define TEXTUS_SNPRINTF snprintf
	#define TEXTUS_VSNPRINTF vsnprintf
	#define TEXTUS_SSCANF  sscanf
 #endif

	#define TEXTUS_STRCAT strcat
	#define TEXTUS_SPRINTF sprintf
	#define TEXTUS_STRNCPY strncpy
	#define TEXTUS_STRCPY strcpy
	#define TEXTUS_FOPEN(x,y,z)  x = fopen(y,z)
#endif

#endif
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

#if !defined(RTLD_GLOBAL) 
#define RTLD_GLOBAL        0x00100
#endif

#if defined( _WIN32 )
static void error_pro(const char* so_file) 
{ 
    char errstr[1024], dispstr[2048];
    DWORD dw = GetLastError(); 

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errstr, 1024, NULL );

    wsprintf(dispstr, "Load module(%s) failed with error %d: %s", (char*)so_file, dw, errstr); 
    MessageBox(NULL, (const char*)dispstr, TEXT("Error"), MB_OK); 
}
#else
static void error_pro (const char *so_file)
{
	fprintf(stderr, "Textus cannot load library %s : %s\n", so_file, TEXTUS_MOD_DLERROR);	
}
#endif

static char* r_share(const char *so_file)
{
	static char r_file[1024];
	int l = 0;
	memset(r_file, 0, sizeof(r_file));
	l = strlen(so_file);
	if (l > 512 ) l = 512;
	memcpy(r_file, so_file, l);
	memcpy(&r_file[l], TEXTUS_MOD_SUFFIX, strlen(TEXTUS_MOD_SUFFIX));
	return r_file;
}

int main (int argc, char *argv[])
{
	TMODULE ext=NULL;
	typedef int (*Start_fun)(int , char *[]);
	Start_fun run;
        if (argc <=1 ) 
        {
#if defined(_WIN32)        	
        	MessageBox(NULL, "Usage: main xmlfile", TEXT("Error"), MB_OK); 
#else
		printf("Usage:%s xmlfile\n",argv[0]);
#endif
        	return 1;
        }
	if ( (  ext =TEXTUS_LOAD_MOD(r_share("libanimus"), RTLD_GLOBAL) ) 
		&& (TEXTUS_GET_ADDR(ext, "textus_animus_start", run, Start_fun) ) )
	{ 
		int ret = run(argc, argv); 
		fprintf(stderr, "process %d exited with %d.\n", GETPID, ret);
	} else {
		error_pro("libanimus");
	}
	return 0;
}
