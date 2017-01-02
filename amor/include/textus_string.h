/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 $Date: 14-01-23 10:36 $
 $Revision: 8 $
*/

/* $NoKeywords: $ */

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
 #endif

	#define TEXTUS_SSCANF  sscanf
	#define TEXTUS_STRCAT strcat
	#define TEXTUS_SPRINTF sprintf
	#define TEXTUS_STRNCPY strncpy
	#define TEXTUS_STRCPY strcpy
	#define TEXTUS_FOPEN(x,y,z)  x = fopen(y,z)
#endif

#define Obtainx(s)   "0123456789abcdef"[s]
#define ObtainX(s)   "0123456789ABCDEF"[s]
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static char* byte2hex(const unsigned char *byte, size_t blen, char *hex) 
{
	size_t i;
	for ( i = 0 ; i < blen ; i++ )
	{
		hex[2*i] =  ObtainX((byte[i] & 0xF0 ) >> 4 );
		hex[2*i+1] = ObtainX(byte[i] & 0x0F );
	}
	return hex;
}

static unsigned char* hex2byte(unsigned char *byte, size_t blen, const char *hex)
{
	size_t i;
	const char *p ;	

	p = hex; i = 0;

	while ( i < blen )
	{
		byte[i] =  (0x0F & Obtainc( hex[2*i] ) ) << 4;
		byte[i] |=  Obtainc( hex[2*i+1] ) & 0x0f ;
		i++;
		p +=2;
	}
	return byte;
}

#endif
