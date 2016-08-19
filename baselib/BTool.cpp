/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: Basic Tool
 Build: Created by octerboy 2005/6/9
 $Header: /textus/baselib/BTool.cpp 24    13-09-12 14:26 Octerboy $
*/

#define TEXTUS_MODTIME  "$Date: 13-09-12 14:26 $"
#define TEXTUS_BUILDNO  "$Revision: 24 $"
/* $NoKeywords: $ */
//#include "version_1.c"
#include "Notitia.h"
#include "BTool.h"
#include "textus_string.h"
#include "casecmp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <errno.h>
#endif

char* BTool::getaddr ( const char *filename, const char *key)
{
	return getaddr ( filename, key, "=", 1);
}

char * BTool::getaddr ( const char *filename, const char*key, const char* split,int no)
{
	int current = 0;
	FILE *fp;
	char fline[512];
	static char ret[512];
	char *name,*nend;
	char *value;
	TEXTUS_FOPEN (fp, filename,"r");
	if ( !fp ) 
	{
#ifndef NDEBUG
#if defined(_WIN32)
		printf("BTool::getaddr open file %s,  error: %d\n" ,filename ,GetLastError() );
#else
		printf("BTool::getaddr open file %s,  error: %s\n" ,filename ,strerror(errno) );
#endif
#endif
		return (char*)0;
	}
	while ( fgets(fline,512,fp)) 
	{
		name = fline;
		name +=  strspn( fline, " \t" );
		nend = value=strpbrk(name, split);
		if ( !value) continue;
		*value++ = '\0';
		nend --;
		while ( nend > name && (
			*nend == '\t'
			|| *nend == ' '
			) )
		{
			nend --;
		}
		
		nend++;	
		*nend = '\0';
		if ( strcmp(name,key) == 0 )
		{
			value +=  strspn( value, " \t" );
			TEXTUS_STRCPY(ret,value);
			current++;
			if ( current == no ) break;
		}
 	}

 	fclose(fp);
 	if ( current != no ) 
	{
		memset (ret,0,80);
		return (char*) 0;
	} else {
		value = &ret[strlen(ret)];
		value --;
		while ( value > ret && (
			*value == '\r'
			|| *value == '\n'
			|| *value == '\t'
			|| *value == ' '
			) )
		{
			value --;
		}
		
		value++;	
		*value = '\0';
	}
 	return ret;
}

bool  BTool::putaddr ( const char* filename, const char*key, const char* value) 
{
	return (putaddr ( filename, key, "=",value,1));
}

#define MAXLINE 1000
bool  BTool::putaddr ( const char* filename, const char*key, const char* split, const char* value,int no) 
{
	FILE *fp;
	char fline[513][MAXLINE];
	char aline[513];
	char *name, *nend;
	char *oldvalue,oldchar;
	int i = 0,total_line=0; 
	int current = 0 , found = 0;
	if (!key || !split ) return false;
	TEXTUS_FOPEN (fp, filename,"r");
	if ( !fp ) 
	{
#ifndef NDEBUG
#if defined(_WIN32)
		printf("BTool::putaddr open file %s,  error: %d\n" ,filename , GetLastError() );
#else
		printf("BTool::putaddr open file %s,  error: %s\n" ,filename ,strerror(errno) );
#endif
#endif
		return false;
	}

	for ( ;i < MAXLINE && fgets(fline[i],512,fp);i++ ) 
	{
		memcpy ( aline, fline[i], 513);
		name = aline;
		name +=  strspn( aline, " \t" );
		nend = oldvalue = strpbrk(name,split);
		if ( !oldvalue) continue;
		oldchar=*oldvalue;
		*oldvalue = '\0';
		nend --;
		while ( nend > name && (
			*nend == '\t'
			|| *nend == ' '
			) )
		{
			nend --;
		}
		
		nend++;	
		*nend = '\0';
		if ( strcmp(name,key) == 0 )
		{
			current++;
			if ( current == no || no == 0 ) 
			{
				if ( value )
				{
					TEXTUS_SPRINTF(fline[i],"%s%s%s\n",name,split,value);
				} else
				{
					fline[i][0] = '\0';
				}
				found = 1; 
			}
			else
				*oldvalue = oldchar;
		} else
			*oldvalue = oldchar;
 	}
 	total_line = i;

	fclose(fp);

 	if (  !found && value ) 
 	{
		TEXTUS_SPRINTF(fline[total_line],"%s%s%s\n",key,split,value);
		total_line++;
 	}

 	TEXTUS_FOPEN (fp, filename,"w");
	if ( !fp ) 
	{
#ifndef NDEBUG
#if defined(_WIN32)
		printf("BTool::putaddr write file %s,  error: %d\n" ,filename ,GetLastError());
#else
		printf("BTool::putaddr write file %s,  error: %s\n" ,filename ,strerror(errno) );
#endif
#endif
		return false;
	}
 	for ( i =0; i < total_line; i++ )
	{
		fputs(fline[i],fp);
 	}
 	fclose(fp);
 	return true;
}

void BTool::get_textus_ordo(unsigned long *ordop, const char *comm_str)
{
	if ( !ordop || !comm_str  )
		return;
	
#define WHAT_ORDO(X,Y) if ( comm_str && strcasecmp(comm_str, #X) == 0 ) Y = Notitia::X 
#define GET_ORDO(Y) 	\
	Y = Notitia::TEXTUS_RESERVED;	\
	WHAT_ORDO(WINMAIN_PARA , Y); \
	WHAT_ORDO(CMD_MAIN_EXIT , Y); \
	WHAT_ORDO(CLONE_ALL_READY , Y); \
	WHAT_ORDO(CMD_GET_OWNER , Y); \
	WHAT_ORDO(WHO_AM_I, Y); \
	WHAT_ORDO(LOG_EMERG , Y); \
	WHAT_ORDO(LOG_ALERT , Y); \
	WHAT_ORDO(LOG_CRIT , Y); \
	WHAT_ORDO(LOG_ERR , Y); \
	WHAT_ORDO(LOG_WARNING , Y); \
	WHAT_ORDO(LOG_NOTICE , Y); \
	WHAT_ORDO(LOG_INFO , Y); \
	WHAT_ORDO(LOG_DEBUG , Y); \
	WHAT_ORDO(FAC_LOG_EMERG , Y); \
	WHAT_ORDO(FAC_LOG_ALERT , Y); \
	WHAT_ORDO(FAC_LOG_CRIT , Y); \
	WHAT_ORDO(FAC_LOG_ERR , Y); \
	WHAT_ORDO(FAC_LOG_WARNING , Y); \
	WHAT_ORDO(FAC_LOG_NOTICE , Y); \
	WHAT_ORDO(FAC_LOG_INFO , Y); \
	WHAT_ORDO(FAC_LOG_DEBUG , Y); \
	WHAT_ORDO(CMD_GET_VERSION , Y); \
	WHAT_ORDO(CMD_GET_PIUS , Y); \
	WHAT_ORDO(DMD_CONTINUE_SELF , Y); \
	WHAT_ORDO(DMD_STOP_NEXT , Y); \
	WHAT_ORDO(DMD_CONTINUE_NEXT , Y); \
	WHAT_ORDO(CMD_ALLOC_IDLE , Y); \
	WHAT_ORDO(CMD_FREE_IDLE , Y); \
	WHAT_ORDO(DMD_CLONE_OBJ , Y); \
	WHAT_ORDO(CMD_INCR_REFS , Y); \
	WHAT_ORDO(CMD_DECR_REFS , Y); \
	WHAT_ORDO(JUST_START_THREAD , Y); \
	WHAT_ORDO(FINAL_END_THREAD , Y); \
	WHAT_ORDO(NEW_SESSION , Y); \
	WHAT_ORDO(END_SERVICE , Y); \
	WHAT_ORDO(CMD_RELEASE_SESSION , Y); \
	WHAT_ORDO(CHANNEL_TIMEOUT , Y); \
	WHAT_ORDO(CMD_NEW_SERVICE , Y); \
	WHAT_ORDO(START_SERVICE , Y); \
	WHAT_ORDO(DMD_END_SERVICE , Y); \
	WHAT_ORDO(DMD_BEGIN_SERVICE , Y); \
	WHAT_ORDO(END_SESSION , Y); \
	WHAT_ORDO(DMD_END_SESSION , Y); \
	WHAT_ORDO(START_SESSION , Y); \
	WHAT_ORDO(DMD_START_SESSION , Y); \
	WHAT_ORDO(SET_TBUF , Y); \
	WHAT_ORDO(PRO_TBUF , Y); \
	WHAT_ORDO(GET_TBUF , Y); \
	WHAT_ORDO(ERR_FRAME_LENGTH , Y); \
	WHAT_ORDO(ERR_FRAME_TIMEOUT , Y); \
	WHAT_ORDO(FD_SETRD , Y); \
	WHAT_ORDO(FD_SETWR , Y); \
	WHAT_ORDO(FD_SETEX , Y); \
	WHAT_ORDO(FD_CLRRD , Y); \
	WHAT_ORDO(FD_CLRWR , Y); \
	WHAT_ORDO(FD_CLREX , Y); \
	WHAT_ORDO(FD_PRORD , Y); \
	WHAT_ORDO(FD_PROWR , Y); \
	WHAT_ORDO(FD_PROEX , Y); \
	WHAT_ORDO(TIMER , Y); \
	WHAT_ORDO(DMD_SET_TIMER , Y); \
	WHAT_ORDO(DMD_CLR_TIMER , Y); \
	WHAT_ORDO(DMD_SET_ALARM , Y); \
	WHAT_ORDO(PRO_HTTP_HEAD , Y); \
	WHAT_ORDO(CMD_HTTP_GET , Y); \
	WHAT_ORDO(CMD_HTTP_SET , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADBUF , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADOBJ , Y); \
	WHAT_ORDO(CMD_SET_HTTP_HEAD , Y); \
	WHAT_ORDO(PRO_HTTP_REQUEST , Y); \
	WHAT_ORDO(PRO_HTTP_RESPONSE , Y); \
	WHAT_ORDO(HTTP_Request_Complete , Y); \
	WHAT_ORDO(HTTP_Response_Complete , Y); \
	WHAT_ORDO(HTTP_Request_Cleaned , Y); \
	WHAT_ORDO(GET_COOKIE , Y); \
	WHAT_ORDO(SET_COOKIE , Y); \
	WHAT_ORDO(SET_TINY_XML , Y); \
	WHAT_ORDO(PRO_TINY_XML , Y); \
	WHAT_ORDO(PRO_SOAP_HEAD , Y); \
	WHAT_ORDO(PRO_SOAP_BODY , Y); \
	WHAT_ORDO(ERR_SOAP_FAULT , Y); \
	WHAT_ORDO(CMD_GET_FD , Y); \
	WHAT_ORDO(CMD_SET_PEER , Y); \
	WHAT_ORDO(CMD_GET_PEER , Y); \
	WHAT_ORDO(CMD_GET_SSL , Y); \
	WHAT_ORDO(CMD_GET_CERT_NO , Y); \
	WHAT_ORDO(SET_WEIGHT_POINTER , Y); \
	WHAT_ORDO(COMPLEX_PIUS , Y); \
	WHAT_ORDO(CMD_FORK , Y); \
	WHAT_ORDO(FORKED_PARENT , Y); \
	WHAT_ORDO(FORKED_CHILD , Y); \
	WHAT_ORDO(NEW_HOLDING , Y); \
	WHAT_ORDO(AUTH_HOLDING , Y); \
	WHAT_ORDO(HAS_HOLDING , Y); \
	WHAT_ORDO(CMD_SET_HOLDING , Y); \
	WHAT_ORDO(CMD_CLR_HOLDING , Y); \
	WHAT_ORDO(CLEARED_HOLDING , Y); \
	WHAT_ORDO(SET_UNIPAC , Y); \
	WHAT_ORDO(PRO_UNIPAC , Y); \
	WHAT_ORDO(ERR_UNIPAC_COMPOSE , Y); \
	WHAT_ORDO(ERR_UNIPAC_RESOLVE , Y); \
	WHAT_ORDO(ERR_UNIPAC_INFO, Y); \
	WHAT_ORDO(MULTI_UNIPAC_END,Y); \
	WHAT_ORDO(CMD_SET_DBFACE , Y); \
	WHAT_ORDO(CMD_SET_DBCONN , Y); \
	WHAT_ORDO(CMD_DBFETCH , Y); \
	WHAT_ORDO(CMD_GET_DBFACE , Y); \
	WHAT_ORDO(CMD_DB_CANCEL , Y); \
	WHAT_ORDO(PRO_DBFACE , Y); \
	if ( Y == Notitia::TEXTUS_RESERVED && comm_str && atoi(comm_str) >= 0) 	\
		Y = atoi(comm_str);

	GET_ORDO((*ordop));
}

unsigned int BTool::unescape( const char *s, unsigned char *t)
{
/*
 * Remove "\" escapes from s..., to t....
 */
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )
	unsigned char	*p; const char *q;
	short i;
	if ( !s || !t ) 
		return 0;

	p = t; *p ='\0';
	while( *s )
	{
		if ( *s == '\\' ) 	/*  "\" */
		{
			q = s; ++q;
			/* character after "\"  */
			switch (*q )
			{
			case '\\':
				*p++ = '\\';
				break;

			case 'a':
				*p++ = '\a';
				break;

			case 'b':
				*p++ = '\b';
				break;

			case 'f':
				*p++ = '\f';
				break;

			case 'r':
				*p++ = '\r';
				break;

			case 'n':
				*p++ = '\n';
				break;

			case 't':
				*p++ = '\t';
				break;

			case 'v':
				*p++ = '\v';
				break;

			case '0':		/* \0NNN, character with octal value NNN (0 to 3 digits) */
				for ( 	*p=0, ++q, i= 0;
					*q >= '0' && *q <= '7' && i < 3;	/* for ASCII or EBCDIC */
					++q, i++)	
				{
					*p = ((*p) << 3) | ((*q - '0') & 0x07L);
				}

				p++;	/* p to next */
				q--;	/* back a character, for s=q and ++s will skip it */
				break;

			case 'x':		/* \xNN, byte with hexadecimal value NN (1 to 2 digits) */
				for ( 	*p=0, ++q, i= 0;
					i < 2  && (
					   (*q >= '0' && *q <= '9')
					|| (*q >= 'a' && *q <= 'f' )
					|| (*q >= 'A' && *q <= 'F' ));
						/* for ASCII or EBCDIC */
					++q, i++)	
				{
					*p = ((*p) << 4)  | ( (Obtainc(*q)) & 0x0FL );
				}

				if ( i > 0 )
					p++;	/* p to next */

				q--;	/* back a character, for s=q and ++s will skip it */
				break;

			default:
				if ( *q )
					*p++ = *q;
				break;
			}
			
			s = q;
		} else {
			*p++ = *s;
		}

		if ( *s)
			++s;
		else
			break;
	}

	*p = '\0';
	return ( p-t);
}

/* base64 program  
** Copyright 1999,2000 by Jef Poskanzer <jef@acme.com>.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
*/

static const char *b64_code_table="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int BTool::base64_encode(char *encoded, const unsigned char *string, int len)
{
	int i;
	char *p;
  
	p = encoded;
      for (i = 0; i < len - 2; i += 3) {
  	*p++ = b64_code_table[(string[i] >> 2) & 0x3F];
  	*p++ = b64_code_table[((string[i] & 0x3) << 4) |
  	                ((int) (string[i + 1] & 0xF0) >> 4)];
  	*p++ = b64_code_table[((string[i + 1] & 0xF) << 2) |
  	                ((int) (string[i + 2] & 0xC0) >> 6)];
  	*p++ = b64_code_table[string[i + 2] & 0x3F];
      }
      if (i < len) {
  	*p++ = b64_code_table[(string[i] >> 2) & 0x3F];
  	if (i == (len - 1)) {
  	    *p++ = b64_code_table[((string[i] & 0x3) << 4)];
  	    *p++ = '=';
  	}
  	else {
  	    *p++ = b64_code_table[((string[i] & 0x3) << 4) |
  	                    ((int) (string[i + 1] & 0xF0) >> 4)];
  	    *p++ = b64_code_table[((string[i + 1] & 0xF) << 2)];
  	}
  	*p++ = '=';
      }
  
      *p++ = '\0';
      return p - encoded;
}

/* Base-64 decoding.  This represents binary data as printable ASCII
** characters.  Three 8-bit binary bytes are turned into four 6-bit
** values, like so:
**
**   [11111111]  [22222222]  [33333333]
**
**   [111111] [112222] [222233] [333333]
**
** Then the 6-bit values are represented using the characters "A-Za-z0-9+/".
*/

static int b64_decode_table[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,  /* 20-2F */
    52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,  /* 30-3F */
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,  /* 40-4F */
    15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,  /* 50-5F */
    -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,  /* 60-6F */
    41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,  /* 70-7F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1   /* F0-FF */
    };

/* Do base-64 decoding on a string.  Ignore any non-base64 bytes.
** Return the actual number of bytes generated.  The decoded size will
** be at most 3/4 the size of the encoded, and may be smaller if there
** are padding characters (blanks, newlines).
*/
int BTool::base64_decode( const char* str, unsigned char* space, int size )
{
	const char* cp;
	int space_idx, phase;
    	int d, prev_d = 0;
    	unsigned char c;

    	space_idx = 0;
    	phase = 0;
    	for ( cp = str; *cp != '\0'; ++cp )
	{
		d = b64_decode_table[(int)*cp];
		if ( d != -1 )
	    	{
	    		switch ( phase )
			{
			case 0:
				++phase;
				break;
			case 1:
				c = ( ( prev_d << 2 ) | ( ( d & 0x30 ) >> 4 ) );
				if ( space_idx < size )
		    			space[space_idx++] = c;
				++phase;
				break;
			case 2:
				c = ( ( ( prev_d & 0xf ) << 4 ) | ( ( d & 0x3c ) >> 2 ) );
				if ( space_idx < size )
		    			space[space_idx++] = c;
				++phase;
				break;
			case 3:
				c = ( ( ( prev_d & 0x03 ) << 6 ) | d );
				if ( space_idx < size )
		    			space[space_idx++] = c;
				phase = 0;
				break;
			}
	   		prev_d = d;
		}
	}
   	return space_idx;
}
