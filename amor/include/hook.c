#include "hook.h"
#ifdef AMOR_CLS_TYPE

extern "C"  {

#ifdef TEXTUS_APTUS_TAG
TEXTUS_AMOR_EXPORT char textus_aptus_tag[64] = TEXTUS_APTUS_TAG;
TEXTUS_AMOR_EXPORT void textus_pfun_ignite_t(Amor* p, TiXmlElement *wood,TiXmlElement *t) 
{
	return (AMOR_CLS_TYPE*)p->ignite(wood, t);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_dextra(Amor* p, Amor::Pius *pius ,size_t from) 
{
	return (AMOR_CLS_TYPE*)p->dextra(pius, from);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_laeve(Amor* p, Amor::Pius *pius, size_t from) 
{
	return (AMOR_CLS_TYPE*)p->laeve(pius, from);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_facio_n(Amor* p, Amor::Pius *pius ,size_t from) 
{
	return (AMOR_CLS_TYPE*)p->facio_n(pius, from);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_sponte_n(Amor* p, Amor::Pius *pius, size_t from) 
{
	return (AMOR_CLS_TYPE*)p->sponte_n(pius, from);
}
#endif

#ifdef TEXTUS_APTUS_MUST_IGNITE
TEXTUS_AMOR_EXPORT int textus_aptus_must_ignite = TEXTUS_APTUS_MUST_IGNITE;
#endif

TEXTUS_AMOR_EXPORT Amor* TEXTUS_CREATE_AMOR() 
{
	return  new AMOR_CLS_TYPE;
}

TEXTUS_AMOR_EXPORT void TEXTUS_DESTROY_AMOR(Amor* p) 
{
	delete (AMOR_CLS_TYPE*)p;
}

TEXTUS_AMOR_EXPORT Amor* textus_pfun_clone(Amor* p) 
{
	return (AMOR_CLS_TYPE*)p->clone();
}

TEXTUS_AMOR_EXPORT void textus_pfun_ignite(Amor* p, TiXmlElement *wood) 
{
	return (AMOR_CLS_TYPE*)p->ignite(wood);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_facio(Amor* p, Amor::Pius *pius) 
{
	return (AMOR_CLS_TYPE*)p->facio(pius);
}

TEXTUS_AMOR_EXPORT bool textus_pfun_sponte(Amor* p, Amor::Pius *pius) 
{
	return (AMOR_CLS_TYPE*)p->sponte(pius);
}

#ifdef VAN_CLS_TYPE
TEXTUS_AMOR_EXPORT void* textus_pfun_neovan (size_t *sz) 
{
	if ( sz != 0 )
		*sz = sizeof(struct AMOR_CLS_TYPE::VAN_CLS_TYPE);
	return new AMOR_CLS_TYPE::VAN_CLS_TYPE;
}
#endif

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
/*
#if defined(TEXTUS_LOGGER_H) && !defined(TEXTUS_JOURNAL)
#define HERE_VAN(X) AMOR_CLS_TYPE_##X
void AMOR_CLS_TYPE::give_logger( Amor *jor) {
	((HERE_VAN(Van)*)van)->tusLogger = jor;	
}
#endif 
*/
#endif /* end ifdef AMOR_CLS_TYPE */
