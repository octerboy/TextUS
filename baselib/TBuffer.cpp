/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: 	TBuffer
 Build: created by octerboy, 2005/06/10
 $Id$
*/

#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "hook.h"

#if defined(_WIN32) 
#include <windows.h>
	BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		return TRUE;
	}
	#if !defined(TEXTUS_AMOR_EXPORT)
		#define TEXTUS_AMOR_EXPORT __declspec(dllexport) 
	#endif
#else	/* OS: Linux/unix */
	#if !defined(TEXTUS_AMOR_EXPORT)
		#define TEXTUS_AMOR_EXPORT 
    	#endif
#endif /* end ifdef _WIN32 */
/*
extern "C"  {

#include "version.c"
TEXTUS_AMOR_EXPORT void TEXTUS_GET_VERSION(char *scm_id, char *time_str, char *ver_no, int len) 
{
	textus_base_get_version(scm_id, time_str, ver_no, len);
}

#if defined(_WIN32) 
typedef struct _DllVersionInfo {
	DWORD cbSize;
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuildNumber;
	DWORD dwPlatformID;
} DLLVERSIONINFO;
TEXTUS_AMOR_EXPORT HRESULT CALLBACK DllGetVersion( DLLVERSIONINFO *pdvi)
	{
		pdvi->dwMajorVersion = 1;
		pdvi->dwMinorVersion = 1;
		pdvi->dwBuildNumber =1;
		pdvi->dwPlatformID = 1;
		return 0;
	}
#endif
}
*/

/* $NoKeywords: $ */

#include "TBuffer.h"
#include <assert.h>   
#include <string.h>  
#include <stdio.h>  
#define MINIMUM_BUFFER_SIZE     128

TBuffer::TBuffer(unsigned TEXTUS_LONG size)
{
	if (size < MINIMUM_BUFFER_SIZE)
		size = MINIMUM_BUFFER_SIZE;

	base = new unsigned char[size];
	limit = base + size;
	reset();
}

TBuffer::~TBuffer()
{
	if (base)
		delete[] base;
}

void TBuffer::expand(unsigned TEXTUS_LONG extraSize)	//根据point扩充extraSize
{
	unsigned TEXTUS_LONG newSize, size, n, len;

	if (!base) //新建空间, base有可能被清了
	{
		newSize = extraSize;	//就是新空间大小了
		if (newSize < MINIMUM_BUFFER_SIZE)
			newSize = MINIMUM_BUFFER_SIZE;

		base = new unsigned char[newSize];
		limit = base + newSize;
		reset();
		return;
	}

	n = limit - base;
	len = point - base;	//len已有数据长度
	newSize = len + extraSize; 	//这是所期望的空间大小newSize
	if (newSize < n)
		return;		/* never shrink buffer */

	//size将是实际的新尺寸
	if (newSize < 0x1000)
	{
		size = (MINIMUM_BUFFER_SIZE < n) ? n : MINIMUM_BUFFER_SIZE;
		while (size < newSize)
			size <<= 1;
	} else {
		size = (newSize + 0x0FFF) & ~0x0FFF;
	}

	unsigned char * p = base;
	
	base = new unsigned char[size];
	limit = base + size;

	memcpy(base, p, n);
	n = base - p;
	delete[] p;

	point   += n;

	return;
}

TEXTUS_LONG TBuffer::commit(TEXTUS_LONG len)
{
	if( point + len > limit || point + len < base )
	{
#if defined( _MSC_VER ) && (  defined(_M_X64)  ||  defined(_WIN64))
		printf ( "space %lld, alloc %lld\n", (TEXTUS_LONG)(limit - base), (TEXTUS_LONG)(point - base));
		printf ( "blen %lld\n", len);
#else
		printf ( "space %ld, alloc %ld\n", (TEXTUS_LONG)(limit - base), (TEXTUS_LONG)(point - base));
		printf ( "blen %ld\n", len);
#endif
	}
	point += len;

	assert( (point <= limit) && (point >= base ) );
	if ( len < 0  && point > base ) /* 确认已读出数据, 如果数据没有取完的, 移动一下 */
		memmove(base, base - len, point - base);
	return (limit - point);
}

#if 0
TBINLINE void TBuffer::grant(unsigned TEXTUS_LONG space)
{
	if ( point +space > limit )
		expand(space);
	return ;
}
TBINLINE void TBuffer::reset()
{
	point = base;
	return ;
}
void TBuffer::exchange(TBuffer &a, TBuffer &b)
{
	unsigned char * med;
#define EX(X) \
	med = b.X; \
	b.X = a.X; \
	a.X = med;
	
	EX(base);
	EX(point);
	EX(limit);
	return ;
}
void TBuffer::input(unsigned char *p, unsigned TEXTUS_LONG n)
{
	grant(n);
	memcpy(point, p, n);
	commit(n);
}
#endif

void TBuffer::pour(TBuffer &dst, TBuffer &src)
{
	TEXTUS_LONG len;
	if ( dst.point == dst.base )	/* dst is empty */
		TBuffer::exchange(src, dst);
	else {
		/* 只能将数据拷贝，而不能直接交换TBUFFER中的地址 */
		len = src.point - src.base;
		dst.grant(len);
		memcpy(dst.point, src.base, len);
		src.commit(-len);
		dst.commit(len);
	}
	return ;
}

void TBuffer::pour(TBuffer &dst, TBuffer &src, unsigned TEXTUS_LONG n)
{
	unsigned TEXTUS_LONG l;
	TEXTUS_LONG m;
	l = src.point - src.base;
	if ( dst.point == dst.base && l <= n ) /* dst is empty and src to be empty  */
		TBuffer::exchange(src, dst);
	else {
		/* 只能将数据拷贝，而不能直接交换TBUFFER中的地址 */
		m = l < n ? l : n;
		dst.grant(m);
		memcpy(dst.point, src.base, m);
		src.commit(-m);
		dst.commit(m);
	}
	return ;
}

