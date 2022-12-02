#include <stdarg.h>
#ifndef TEXTUS_LOGGER_H
#define TEXTUS_LOGGER_H
class TEXTUS_AMOR_STORAGE Logger {
public:
	unsigned int instance_id;	/* for every instance */
	Amor *log_apt;	//to Jor actually
	/* Amor objects communicate by Pius object */
	struct PiDat {
		Logger *me;
		unsigned int instance_id;	//copy from this obj
		void *log_apt;	//copy from this obj
		va_list* h_va;
		char *msg;
	};

#if defined( _MSC_VER ) && (_MSC_VER >= 1400 ) || defined(__GNUC__) && (__GNUC__ >= 3 ) || defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x560) ||  defined(__clang_major__) && (__clang_major__ >= 2)
#if !defined (WBUG)
#ifndef NDEBUG
	#define WBUG(...) { Amor::Pius amor_log_pius; \
		struct PiDat pidat;	\
		char amor_log_errMsg[2314]; \
		char amor_log_msg[2048]; \
		memset(amor_log_msg,0,sizeof(amor_log_msg)); \
		memset(amor_log_errMsg,0,sizeof(amor_log_errMsg)); \
		TEXTUS_SNPRINTF(amor_log_msg, sizeof(amor_log_msg)-1, __VA_ARGS__); \
		TEXTUS_SNPRINTF(amor_log_errMsg, sizeof(amor_log_errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, amor_log_msg); \
		amor_log_pius.ordo = Notitia::LOG_DEBUG; \
		pidat.me = this; \
		pidat.instance_id = this->instance_id; \
		pidat.log_apt = this->log_apt; \
		pidat.h_va = 0; \
		pidat.msg = amor_log_errMsg; \
		amor_log_pius.indic = &pidat; \
		log_apt->sponte(&amor_log_pius); \
		}
#else
#define WBUG(...)
#endif

#endif
	inline void _OWN_WLOG(int lev, ...) 
	{ 
		struct PiDat pidat;
		Amor::Pius amor_log_pius; 
		va_list va;
		va_start(va, lev);
		amor_log_pius.ordo = lev;
		pidat.me = this; 
		pidat.instance_id = this->instance_id; 
		pidat.log_apt = this->log_apt; 
		pidat.h_va = &va; 
		pidat.msg = 0; 
		amor_log_pius.indic = &pidat; 
		log_apt->sponte(&amor_log_pius); 
		va_end(va);
	};

#define WLOG(Z,...) _OWN_WLOG(Notitia::LOG_VAR_##Z, __VA_ARGS__)

#else	/* for < C99 */
	enum HERELOGLEV { 
		EMERG 	= Notitia::LOG_EMERG, 
		ALERT	= Notitia::LOG_ALERT,
		CRIT	= Notitia::LOG_CRIT,
		ERR	= Notitia::LOG_ERR,
		WARNING	= Notitia::LOG_WARNING,
		NOTICE	= Notitia::LOG_NOTICE,
		INFO	= Notitia::LOG_INFO
	} ;

	void WLOG(int lev, const char *format, ...) 
	{ 
		struct PiDat pidat;
		Amor::Pius amor_log_pius; 
		char amor_errMsg[1024]; 
		va_list va;
		va_start(va, format);
		memset(amor_errMsg, 0, 1024);
		TEXTUS_VSNPRINTF(amor_errMsg, sizeof(amor_errMsg)-1,format,va); 
		va_end(va);
		amor_log_pius.ordo = lev;
		pidat.me = this; 
		pidat.instance_id = this->instance_id; 
		pidat.log_apt = this->log_apt; 
		pidat.h_va = 0; 
		pidat.msg = &amor_errMsg[0]; 
		amor_log_pius.indic = &pidat; 
		log_apt->sponte(&amor_log_pius); 
	};

#ifndef NDEBUG
	void inline WBUG(const char *format, ...) 
	{ 
		struct PiDat pidat;	
		Amor::Pius amor_log_pius; 
		//char amor_log_errMsg[2314]; 
		char amor_log_msg[2048]; 
		va_list va;
		va_start(va, format);
		memset(amor_log_msg,0,sizeof(amor_log_msg));
		TEXTUS_VSNPRINTF(amor_log_msg, sizeof(amor_log_msg)-1, format, va); 
		va_end(va);
		//TEXTUS_SNPRINTF(amor_log_errMsg, sizeof(amor_log_errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, amor_log_msg); 
		amor_log_pius.ordo = Notitia::LOG_DEBUG; 
		//amor_log_pius.indic = &amor_log_errMsg[0]; 
		//amor_log_pius.indic = &amor_log_msg[0]; 
		pidat.me = this; 
		pidat.instance_id = this->instance_id; 
		pidat.log_apt = this->log_apt; 
		pidat.h_va = 0; 
		pidat.msg = &amor_errMsg[0]; 
		amor_log_pius.indic = &pidat; 
		log_apt->sponte(&amor_log_pius);
	};
#else
	void inline WBUG(const char *format,...) { };
#endif

#endif

#if !defined (WLOG_OSERR)
#if defined (_WIN32 )
#define WLOG_OSERR(X) { \
	struct PiDat pidat;	\
	Amor::Pius amor_log_pius; \
	char amor_errMsg[1024]; \
	memset(amor_errMsg,0,sizeof(amor_errMsg)); \
	char *s; \
	char error_string[1024]; \
	DWORD dw = GetLastError(); \
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw, \
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) error_string, 1024, NULL );\
	s= strstr(error_string, "\r\n") ; \
	if (s )  *s = '\0';  \
	TEXTUS_SNPRINTF(amor_errMsg, 1024, "%s errno %d, %s", X,dw, error_string);\
	amor_log_pius.ordo = Notitia::LOG_ERR; \
	pidat.me = this; \
	pidat.instance_id = this->instance_id; \
	pidat.log_apt = this->log_apt; \
	pidat.h_va = 0; \
	pidat.msg = &amor_errMsg[0]; \
	amor_log_pius.indic = &pidat; \
	log_apt->sponte(&amor_log_pius); \
	}
#else
#define WLOG_OSERR(X)  {		\
	struct PiDat pidat;	\
	Amor::Pius amor_log_pius; 	\
	char amor_errMsg[1024]; 	\
	memset(amor_errMsg,0,sizeof(amor_errMsg)); \
	TEXTUS_SNPRINTF(amor_errMsg, 1024, "%s errno %d, %s.", X, errno, strerror(errno));	\
	amor_log_pius.ordo = Notitia::LOG_ERR; \
	pidat.me = this; \
	pidat.instance_id = this->instance_id; \
	pidat.log_apt = this->log_apt; \
	pidat.h_va = 0; \
	pidat.msg = &amor_errMsg[0]; \
	amor_log_pius.indic = &pidat; \
	log_apt->sponte(&amor_log_pius); \
	}
#endif	/* for _WIN32 */
#endif
};
#endif
