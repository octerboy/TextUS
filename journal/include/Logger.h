#include <stdarg.h>
#ifndef TEXTUS_LOGGER_H
#define TEXTUS_LOGGER_H
static Amor **logger_journal_arr=0;	//to Jor actually
static unsigned int logger_apt_many= 0;
static unsigned int logger_journal_cur= 0;

class TEXTUS_AMOR_STORAGE TusLogger {
public:
	unsigned int instance_id;	/* for every instance */
	unsigned int which_jor;	/* some instance have the same object */
	/* Amor objects communicate by Pius object */
	struct PiDat {
		TusLogger *me;
		unsigned int instance_id;	//copy from this obj
		va_list* h_va;
		char *msg;
	};
	void give_logger( Amor *jor) {
		if (logger_journal_arr==0 ) {
			logger_apt_many = 16;
			logger_journal_arr = new Amor *[logger_apt_many];
			memset(logger_journal_arr, 0, sizeof(Amor *)*logger_apt_many);
			logger_journal_cur=0;
		} 
		if ( logger_journal_cur == logger_apt_many )
		{
			Amor **logger_journal_arr_tmp= new Amor *[logger_apt_many + 16];
			memset(logger_journal_arr, 0, sizeof(Amor *)*(logger_apt_many+16));
			memcpy(logger_journal_arr_tmp, logger_journal_arr, sizeof(Amor*)*logger_apt_many);
			logger_apt_many += 16 ;
			delete []logger_journal_arr;
			logger_journal_arr = logger_journal_arr_tmp;
		}
		logger_journal_arr[logger_journal_cur] = jor; logger_journal_cur++;
	};
	TusLogger::TusLogger() {
		which_jor = 0;
		instance_id = 0;
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
		pidat.h_va = 0; \
		pidat.msg = amor_log_errMsg; \
		amor_log_pius.indic = &pidat; \
		logger_journal_arr[which_jor]->sponte(&amor_log_pius); \
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
		pidat.h_va = &va; 
		pidat.msg = 0; 
		amor_log_pius.indic = &pidat; 
		logger_journal_arr[which_jor]->sponte(&amor_log_pius); 
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
		pidat.h_va = 0; 
		pidat.msg = &amor_errMsg[0]; 
		amor_log_pius.indic = &pidat; 
		logger_journal_arr[which_jor]->sponte(&amor_log_pius); 
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
		pidat.h_va = 0; 
		pidat.msg = &amor_errMsg[0]; 
		amor_log_pius.indic = &pidat; 
		logger_journal_arr[which_jor]->sponte(&amor_log_pius);
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
	pidat.h_va = 0; \
	pidat.msg = &amor_errMsg[0]; \
	amor_log_pius.indic = &pidat; \
	logger_journal_arr[which_jor]->sponte(&amor_log_pius); \
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
	pidat.h_va = 0; \
	pidat.msg = &amor_errMsg[0]; \
	amor_log_pius.indic = &pidat; \
	logger_journal_arr[which_jor]->sponte(&amor_log_pius); \
	}
#endif	/* for _WIN32 */
#endif
};
#endif
