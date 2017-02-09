#include "hook.h"
#ifdef AMOR_CLS_TYPE

#if defined(_WIN32) 
	//BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	//{
	//	return TRUE;
	//}
	#if !defined(TEXTUS_AMOR_EXPORT)
		#define TEXTUS_AMOR_EXPORT __declspec(dllexport) 
	#endif
#else	/* OS: Linux/unix */
	#if !defined(TEXTUS_AMOR_EXPORT)
		#define TEXTUS_AMOR_EXPORT 
    	#endif
#endif /* end ifdef _WIN32 */

extern "C"  {

#ifdef TEXTUS_APTUS_TAG
TEXTUS_AMOR_EXPORT char textus_aptus_tag[256] = TEXTUS_APTUS_TAG;
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
#endif /* end ifdef AMOR_CLS_TYPE */
