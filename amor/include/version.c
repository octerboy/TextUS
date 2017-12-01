#if !defined(TEXTUS_AMOR_EXPORT)
	#if defined(_WIN32) 
		#define TEXTUS_AMOR_EXPORT __declspec(dllexport) 
	#else	/* OS: Linux/unix, etc */
		#define TEXTUS_AMOR_EXPORT 
	#endif
#endif /* end ifdef _WIN32 */

#include <stdio.h>
#include <string.h>
#include "textus_string.h"
static void textus_base_get_version(char *scm_id, char *time_str, char *ver_no, int len) 
{
	char tmp[1024];
	char *p, *q;
	int flen;

	if ( len > 1023 ) len = 1023;

#define GET_SCM_VERSION_INFO(X,Y)	\
	memset(Y,0,len);					\
	p = (char*)X;							\
	while ( *p && *p != ':' ) p++;				\
	p++; while ( *p == ' ' ) p++;				\
	flen = TEXTUS_SNPRINTF(tmp, sizeof(tmp)-1, "%s", p);	\
	q = &tmp[flen-1];					\
	while ( *q == ' ' || *q == '$' ) { q--; flen--; }	\
	q++;							\
	*q = '\0';						\
	if ( flen > len ) flen = len;				\
	memcpy(Y, tmp, flen+1);				

#ifdef SCM_MODULE_ID
	GET_SCM_VERSION_INFO(SCM_MODULE_ID, scm_id)
	p = strpbrk(scm_id, ".");
	*p = '\0';
#endif

#ifdef TEXTUS_BUILDNO
	GET_SCM_VERSION_INFO(TEXTUS_BUILDNO, ver_no)
#endif

#ifdef TEXTUS_MODTIME
	GET_SCM_VERSION_INFO(TEXTUS_MODTIME, time_str)
#endif
}
