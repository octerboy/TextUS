/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Sybase System Connector
 Build: created by octerboy,  2008/09/22, Panyu
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include <assert.h>
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "DBFace.h"
#include "PacData.h"

#if !defined(_WIN32)
#include <unistd.h>
#endif

#include <mysql.h>
#include <mysqld_error.h>

#define BUFF_SIZE 256
#define DEFAULT_MAX_RET_COLUMNS 256
#define DEFAULT_PARAM_MAX 256
#define STR_STATUS_RESULT "STATUS RESULT"

#define STMT_MAX	1024
typedef struct _OBJList {
	MYSQL_STMT *cmd_ptr;
	MYSQL_BIND *bndfmt;       /* the binding datafmt */
	MYSQL_BIND *retfmt;       /* the return datafmt */
	bool *is_null_arr;
	bool cmd_idle;		//
	long many;

	struct _OBJList *prev, *next;
	_OBJList () {
		cmd_ptr = 0;
		bndfmt = retfmt = 0;
		cmd_idle = true;
		is_null_arr = 0;

		prev = next = this;
	};

	~_OBJList () {
		if ( bndfmt ) delete[] bndfmt;
		if ( retfmt ) delete[] retfmt;
		if ( is_null_arr ) delete[] is_null_arr;
	};

	struct _OBJList *remove()
	{
		DoubleList *list = next;

		next = next->next;
		next->prev = this;

		return ((list != this) ? list : 0);
	};

	void append(struct _OBJList *list)
	{
		list->next = this;
		list->prev = prev;

		prev->next = list;
		prev = list;
	};

	struct _OBJList * look_for_idle() {
		struct _OBJList *obj = this;
		obj = obj->next;
		while ( !obj->cmd_idle && obj->next != this )
		{
			obj = obj->next;
		}
		return  ((obj != this) ? obj : 0);
	};

} OBJList ;

typedef struct _MyFaceExt {
	char proc_str[STMT_MAX];
	int proc_len;
	OBJList olist;
	_MyFaceExt () {
		proc_len = 0;
		memset(proc_str,0,sizeof(proc_str));
	};

	void set_proc_str(DBFace *) {
		int len=0,i;
		if (proc_len > 0 ) return;
		switch ( face->pro )
		{
		case DBFace::DBPROC:
			len = TEXTUS_SNPRINTF(proc_str, sizeof(proc_str), "call %s", face->sentence);	
			goto CASE_DBPROC;

		case DBFace::FUNC:
			len = TEXTUS_SNPRINTF(proc_str, sizeof(proc_str), "select %s", face->sentence);	

	    	CASE_DBPROC:
			len += TEXTUS_SNPRINTF(&proc_str[len], sizeof(proc_str)-len, "%s", "(");
			for ( i = 1; i < face->num-1 && i < PARA_MAX-1; i++ )
			{
				if ( face->paras[i].inout != DBFace::UNKNOWN)
				{
					len += TEXTUS_SNPRINTF(&proc_str[len], sizeof(proc_str)-len, "%s", "?,");
				}
			}

			if ( face->num > i )
			{
				len += TEXTUS_SNPRINTF(&proc_str[len], sizeof(proc_str)-len, "%s", "?");
			}

			len += TEXTUS_SNPRINTF(&proc_str[len], sizeof(proc_str)-len, "%s", ")");
			break;
		default:
			break;
		}
		proc_len = len;
	};
} MyFaceExt;

Amor::Pius sine_ps;
class MyCli: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	MyCli();
	~MyCli();

	void set_for_mysql(int status);
#include "wlog.h"
	inline void logerr ( char *str, char *msg) {
		WLOG(ERR, "%s %s", str, msg);
	};
	Amor::Pius dopac_pius, end_ps;  //仅用于向左边传回数据
	Amor::Pius epl_set_ps, epl_clr_ps;
	Amor::Pius clr_timer_pius, delay_pius;
	DPoll::Pollor pollor; /* 保存事件句柄, 各子实例不同 */
	bool inThread;
	int n_status;
	
	void handle_pac();
	int get_result();
	bool bind_result();
	int fetch_data ( int res_type );
	int bind_columns ( int res_type );
	void copyRowValue();
	void copyRowSet(int);
#define ROWSET_LEFT	0
#define ROWS_OBTAINED	1

	void logon();
	void logout();
	void myalloc(bool, TiXmlElement *);
	enum enum_field_types getdty_in(DBFace::DataType type);
	enum enum_field_types getdty_out(DBFace::DataType type);
	bool hasAsyncError(int ret, const char *pro_str);
	bool hasError(int, const char *pro_str);

	struct G_CFG {
		bool shared_session;	/* 是否共享同一个会话 */
		char c_host[BUFF_SIZE];
		char c_dbnm[BUFF_SIZE];
		char c_sock[BUFF_SIZE];
		unsigned int c_port;
		char c_user[BUFF_SIZE];
		char c_auth[BUFF_SIZE];
		char default_group[BUFF_SIZE];
		unsigned int param_max;
		unsigned int ret_col_max;
		unsigned sec_challenge:1;
		unsigned sec_encryption:1;
		unsigned sec_negotiate:1;
		unsigned sec_appdefined:1;
		unsigned on_start:1;
		unsigned on_start_poineer:1;
		Amor *sch;
		struct DPoll::PollorBase lor; /* 探询 */
		bool isNull;
		int interval;
		void *arr[3];
		Amor::Pius alarm_pius;

		unsigned int transmit_buf_sz;
		TiXmlElement *gxml;
		G_CFG( TiXmlElement *cfg ) {
			const char* on_start_str;
			const char *so_file;
			TiXmlElement *con_ele, *sec_ele;
			const char *comm_str;
			lor.type = DPoll::NotUsed;
			isNull = true;
			sch = 0;
			gxml = cfg;
			#define BZERO(X) memset(X, 0 ,sizeof(X))
			BZERO(c_host);
			BZERO(c_dbnm);
			BZERO(c_sock);
			BZERO(c_user);
			BZERO(c_auth);
			BZERO(default_group);
			c_port = 0;
			shared_session = false;
			param_max = DEFAULT_PARAM_MAX;
			ret_col_max = DEFAULT_MAX_RET_COLUMNS;
			sec_challenge  = sec_encryption = sec_negotiate  = sec_appdefined = false;

			transmit_buf_sz = 128000;
			cfg->QueryIntAttribute("so-sndbuf", (int*)&(transmit_buf_sz));
			cfg->QueryIntAttribute("max_parameter_num", (int*)&(param_max));
			cfg->QueryIntAttribute("max_column_num", (int*)&(ret_col_max));
			shared_session =  ( (comm_str= cfg->Attribute("session")) && strcasecmp(comm_str, "shared") == 0 );
			if ( (on_start_str = cfg->Attribute("start") ) && strcasecmp(on_start_str, "yes") ==0 )
				on_start = false;	/* 并非一开始就启动 */
			else
				on_start = true;

			on_start_poineer = on_start;
			if ( (on_start_str = cfg->Attribute("start_poineer") ) && strcasecmp(on_start_str, "yes") ==0 )
				on_start_poineer = false;	
			else
				on_start_poineer = true;	

/*
			sec_ele = cfg->FirstChildElement("security");
			if ( sec_ele )
			{
	#define SET_SEC(X) \
	comm_str = sec_ele->Attribute(#X);				\
	if (comm_str) {							\
		if (strcasecmp(comm_str, "true") == 0 )			\
			gCFG->sec_##X = CS_TRUE;			\
		else if (strcasecmp(comm_str, "false") == 0 )		\
			gCFG->sec_##X = CS_FALSE;			\
	}

				SET_SEC(challenge)
				SET_SEC(encryption)
				SET_SEC(negotiate)
				SET_SEC(appdefined)
			}
*/

			con_ele = cfg->FirstChildElement("connect");
			if ( !con_ele ) 
				return;

			#define CFG_SET(X,Y) \
			comm_str = con_ele->Attribute(Y);	\
			if ( comm_str)	\
				TEXTUS_STRNCPY(X, comm_str, sizeof(X)-1);
			CFG_SET(c_host, "host")
			CFG_SET(c_dbnm, "db")
			CFG_SET(c_sock, "sock")
			CFG_SET(c_user ,"user")
			CFG_SET(c_auth, "password")
			CFG_SET(default_group, "default_group")

			con_ele->QueryIntAttribute("host", (int*)&(c_port));
			alarm_pius.ordo = Notitia::DMD_SET_ALARM;
			alarm_pius.indic = &arr[0];
			arr[0] = 0;	/* set this later */
			arr[1] = &interval;
			arr[2] = 0;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	enum ASYNC_STEP { Async_OK=0, ConnectIng, Real_QueryIng, Real_FetchIng_Row, Real_FreeIng_Result, Real_NextIng_Result, StmtPreIng,
       			StmtExeIng, StmtResStoreIng, StmtResNextIng, StmtResFreeIng, StmtCloseIng, QueryFetchIng };
	struct MySession {
		MYSQL *mysql_local;
		int ready_status;
		unsigned isTalking:1;	/* 一个会话是否进行 */
		unsigned async_step:4;	/* */
		unsigned should_hand_pac:1;	/* */
		MySession () {
			mysql_local = 0;
			isTalking = false;
			async_step =  Async_OK;
			should_hand_pac = false;
		};
	};
	struct MySession *m_session;

	PacketObj *rcv_pac, *snd_pac;	/* 来自左节点的PacketObj */
	PacketObj rowset_pac;	/* 存放查询行集中的一次所取数据 */
	DBFace *face;
	MYSQL_RES *my_result;
	MYSQL_ROW my_row;


	DBFace::PROCTYPE last_pro; //最近的处理类型

	MYSQL_ROW cRowsObtained;	/* 结果集返回记录数 */
	long	cRowsLeft;	/* 行集中还有多少记录未取 */

	CS_INT *rlenp;
	int *fldNo_arr;		/* rowset时, 记住这些保存了返回数据的域号 */
	unsigned int rlen_sz;
	void ready_alloc();

#define PUT_FSND(FLD,BUF,LEN)	\
		if ( snd_pac && face ) 	\
			snd_pac->input(face->FLD, (unsigned char*)BUF, LEN);

#define PUT_FSND_STR(FLD,BUF) PUT_FSND(FLD,BUF,strlen(BUF))
	OBJList *cur_cmd_obj;
};

void MyCli::ignite(TiXmlElement *cfg) 
{
	TiXmlElement *con_ele, *sec_ele;
	const char *comm_str;

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	return;
}

#define CHECK_ENV()	\
	if ( !face )		\
	{			\
		WLOG(INFO, "no dbface yet.");	\
		break;				\
	}

bool MyCli::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	int retcode ;

#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); break; }
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		CHECK_ENV()

		m_session->should_hand_pac = false;
		if ( !(m_session->isTalking) && !(m_session->connecting) ) { logon(); }
		if ( m_session->isTalking ) 
		{ 
			handle_pac();
			WBUG("handle end");
		} else {
			m_session->should_hand_pac = true;
		}
		break;

	case Notitia::CMD_DBFETCH:
		WBUG("facio CMD_DBFETCH");
		CHECK_ENV()

		if ( m_session->isTalking )
		{
			fetch_data ( CS_ROW_RESULT ) ;	//取结果集的
		} else {
			WLOG(INFO, "no talking");
		}
		break;

	case Notitia::CMD_SET_DBFACE:
		WBUG("facio CMD_SET_DBFACE");
		face = (DBFace *)pius->indic;
		if ( face->pro != DBFace::REAL_QUERY )
		{
			cmd_alloc(face);
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		sine_ps.ordo = Notitia::CMD_GET_SCHED;
		sine_ps.indic = 0;
		aptus->sponte(&sine_ps);	//向tpoll, 取得sched
		gCFG->sch = (Amor*)(sine_ps.indic);
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}

		tmp_p.ordo = Notitia::POST_EPOLL;
		tmp_p.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_p);	//向tpoll, 取得TPOLL
		if ( tmp_p.indic == gCFG->sch )
			gCFG->use_epoll = true;
		else
			gCFG->use_epoll = false;

		if (mysql_library_init(0, NULL, NULL) ) {
			WLOG(ERR, "could not initialize MySQL client library");
  		}


		//ready_alloc();

		myalloc(true, gCFG->gxml);
		break;

	case Notitia::MAIN_PARA:
	case Notitia::WINMAIN_PARA:
		if ( gCFG->on_start_poineer )
			logon();		//开始建立连接
		break;

	case Notitia::WILL_MAIN_EXIT:
		mysql_library_end();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		myalloc(false, gCFG-gxml);
		if ( gCFG->on_start )
			logon();		//开始建立连接
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		logout();
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		logon();
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");

		break;

	case Notitia::CMD_DB_CANCEL:
		if ( m_session->async_step ) {
			int st;
			if ( last_pro == DBFace::QUERY ) {
				st = mariadb_cancel (m_session->mysql);
				if ( st)
				{
					WLOG(ERR, "mariadb_cancel(%p) failed error=%d, errmsg %s", m_session->mysql, mysql_errno(m_session->mysql), mysql_error(m_session->mysql)); 
				} else {
					WLOG(NOTICE, "facio CMD_DB_CANCEL mysql=%p", m_session->mysql);
				}
			} else {
				if( mysql_stmt_reset( cur_cmd_obj->cmd_ptr) )
				{
					WLOG(ERR, "mysql_stmt_reset(%p) failed error=%d, errmsg %s",  cur_cmd_obj->cmd_ptr, mysql_errno(m_session->mysql), mysql_error(m_session->mysql)); 
				} else {
					WLOG(NOTICE, "facio CMD_DB_CANCEL stmt=%p", cur_cmd_obj->cmd_ptr);
				}
			}
			m_session->async_step = Async_OK;
		}
		cur_cmd_obj->cmd_idle = true;
		break;

	case Notitia::FD_PRORD:
		WBUG("facio FD_PRORD");
		m_session->ready_status = MYSQL_WAIT_READ;
		if ( m_session->async_step == ConnectIng ) {	//first time
			logon();
		} else {
			handle_pac();
		}
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		end(true);	//at once
		break;

	case Notitia::ACCEPT_EPOLL:
		WBUG("facio ACCEPT_EPOLL");
#if defined (_WIN32 )	
#else
		m_session->ready_status = MYSQL_WAIT_READ;
		logon();
#endif
		break;


	case Notitia::RD_EPOLL:
		m_session->ready_status = MYSQL_WAIT_READ;
		WBUG("facio RD_EPOLL");
		handle_pac();
		break;

	case Notitia::WR_EPOLL:		//rarely
		WBUG("facio WR_EPOLL");
WR_PRO:
		m_session->ready_status = MYSQL_WAIT_WRITE;
		if ( m_session->async_step == ConnectIng ) {	//first time
			logon();
		} else {
			handle_pac();
		}
		break;

	case Notitia::EOF_EPOLL:
		WBUG("facio EOF_EPOLL");
		//end(false);	//down & close
		break;

	case Notitia::EXCEPT_EPOLL:
		WBUG("facio EXCEPT_EPOLL");
		m_session->ready_status = MYSQL_WAIT_EXCEPT;
		if ( m_session->async_step == ConnectIng ) {	//first time
			logon();
		} else {
			handle_pac();
		}
		break;

	case Notitia::FD_PROWR:
		WBUG("facio FD_PROWR");
		//写, 很少见, 除非系统很忙
		goto WR_PRO;

	case Notitia::FD_PROEX:
		WBUG("facio FD_PROEX");
		m_session->ready_status = MYSQL_WAIT_EXCEPT;
		if ( m_session->async_step == ConnectIng ) {	//first time
			logon();
		} else {
			handle_pac();
		}
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
		end(false);	//down close
		break;

	case Notitia::CMD_RELEASE_SESSION:
		WBUG("facio CMD_RELEASE_SESSION");
		//release();	//针对多进程的 parent
		break;

	case Notitia::CMD_TIMER_TO_RELEASE:
		WBUG("facio CMD_TIMER_TO_RELEASE");
		gCFG->sch->sponte(&delay_pius); /* 定时后关闭 */
		break;

	case Notitia::TIMER:
		WBUG("facio TIMER");
		switch ( m_session->async_step ) {
		case ConnectIng:
			m_session->ready_status = MYSQL_WAIT_TIMEOUT;
			logon();
			break;
		case Real_QueryIng:
		case Real_FetchIng_Row:
		case Real_FreeIng_Result:
		case Real_NextIng_Result:
		case StmtPreIng:
		case StmtExeIng:
		case StmtResStoreIng:
		case StmtResNextIng:
		case StmtResFreeIng:
		case StmtCloseIng:
		case QueryFetchIng:
			m_session->ready_status = MYSQL_WAIT_TIMEOUT;
			handle_pac();
			break;

		default:
			break;
		}
		break;

	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

	default:
		WBUG("facio Notitia::%lu", pius->ordo);
		return false;
	}

	return true;
}


#define Err_Dml(isErr, Pro_Str)	 			\
		if (isErr) {				\
			err_before_ret=true;		\
			int my_err;			\
			my_err = mysql_errno(m_session->mysql_local);				\
			PUT_FSND (errCode_field, &my_err, sizeof(my_err));			\
			PUT_FSND_STR(errStr_field, mysql_error(m_session->mysql_local));	\
			WLOG(ERR, Pro_Str " failed status=%d, error=%d, errmsg is %s", err_status, my_err, mysql_error(m_session->mysql)) ;	\
			goto DML_DONE;			\
		}

void MyCli::handle_pac() {
	my_ulonglong count;
	bool err_before_ret=false;
	int status, err_status;
	unsigned long param_count=0;
	unsigned int i;
	bool err_before_ret=false;	//假定在handle_result之前没有错
	size_t len;
	char  tmp_str[BUFF_SIZE];

	int retus;
	int retcode  =0;
	cRowsObtained = 0;

 	const char *query_string;
	unsigned long query_length;
/*
	if ( last_pro == DBFace::QUERY && !hcmd_idle ) 	//凡是最近QUERY的, 要cancel
	{
		retcode = ct_cancel(NULL, cmd_ptr, CS_CANCEL_ALL);
		if ( retcode == CS_SUCCEED )
			hcmd_idle = true;
	}
	*/
	last_pro = face->pro;

	switch ( face->pro )
	{
	case DBFace::FUNC:
		WBUG("handle FUNC (%s) %d parameters", face->sentence, face->num);
		goto CASE_DBPROC;

	case DBFace::DBPROC:
		WBUG("handle DBPROC (%s) %d parameters", face->sentence, face->num);
	CASE_DBPROC:
		WBUG("statement: %s", ((MyFaceExt *)face->dbext)->proc_str);

		switch ( m_session->async_step ) {
		case StmtPreIng:
			goto DML_PRE_PRO;
		case StmtExeIng:
			goto DML_EXEC_PRO;
		case StmtResStoreIng:
			goto DML_SOTRE_PRO;
		case StmtResNextIng:
			goto DML_NEXT_PRO;
		case StmtResFreeIng:
			goto DML_FREE_RESULT_PRO;
		case ConnectIng:
			logon();
			return ;
		case Async_OK:		//Just start
			break;
		default: 
			return ;
		}
 		query_string =  ((MyFaceExt *)face->dbext)->proc_str;
		query_length =  ((MyFaceExt *)face->dbext)->proc_len;
		goto DML_PRE_START;

	case DBFace::QUERY:
		WBUG("handle QUERY (\"%s\") param num %d", face->sentence, face->num);
		switch ( m_session->async_step ) {
		case StmtPreIng:
			goto DML_PRE_PRO;
		case StmtExeIng:
			goto QUERY_EXEC_PRO;
		case QueryFetchIng:
			goto QUERY_FETCH_PRO;
		case StmtResNextIng:
			goto QUERY_NEXT_PRO;
		case StmtResFreeIng:
			goto DML_FREE_RESULT_PRO;
		case ConnectIng:
			logon();
			return ;
		case Async_OK:		//Just start
			break;
		default: 
			return ;
		}
 		query_string = (const char *)face->sentence;
		query_length = face->sentence_len;
		goto DML_PRE_START;

	QUERY_EXEC_PRO:
		status= mysql_stmt_execute_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtExeIng)
		Err_Dml(err_status != 0, "mysql_stmt_execute_cont")

	QUERY_FETCH_START:
		cur_cmd_obj->many =0;
		if ( !bind_result()) 
			goto DML_LAST;
	QUERY_FETCH_START_2:
  		status= m_session->ready_status = mysql_stmt_fetch_start(&err_status, cur_cmd_obj->cmd_ptr);
		Will_Async(status, QueryFetchIng)
		Err_Dml (err_status != 0, "mysql_stmt_fetch_start")
		goto QUERY_GET_ROW;

	QUERY_FETCH_PRO:
		status= mysql_stmt_fetch_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, QueryFetchIng)
		if ( err_status == MYSQL_NO_DATA ) 
			goto DML_FREE_RESULT_START;
		Err_Dml(err_status != 0, "mysql_stmt_fetch_cont")

	QUERY_GET_ROW:
		cur_cmd_obj->many++;
		aptus->sponte(&dopac_pius);	//put data to back 
		goto QUERY_FETCH_START_2;

	QUERY_NEXT_PRO:
		status= mysql_stmt_next_result_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtResNextIng)
	QUERY_NEXT_OK:	/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		if ( err_status ==0 )  {
			goto QUERY_FETCH_START;
		} else  if ( err_status == -1)  {
			goto DML_LAST;
		} else {
			Err_Dml(true, "mysql_stmt_next_result_cont")
		}

		break;

	case DBFace::DML:
		WBUG("handle DML (\"%s\") param num %d", face->sentence, face->num);
		switch ( m_session->async_step ) {
		case StmtPreIng:
			goto DML_PRE_PRO;
		case StmtExeIng:
			goto DML_EXEC_PRO;
		case StmtResStoreIng:
			goto DML_SOTRE_PRO;
		case StmtResNextIng:
			goto DML_NEXT_PRO;
		case StmtResFreeIng:
			goto DML_FREE_RESULT_PRO;
		case ConnectIng:
			logon();
			return;
		case Async_OK:		//Just start
			break;
		default: 
			return ;
		}
 		query_string = (const char *)face->sentence;
		query_length = face->sentence_len;

	DML_PRE_START:
		status = 0;
		PUT_FSND (errCode_field, &status, sizeof(status));	 //首先假定结果OK，把值设到返回域中
		cur_cmd_obj->many = 0;
  		status= m_session->ready_status = mysql_stmt_prepare_start(&err_status, cur_cmd_obj->cmd_ptr, query_string, query_length);
		Will_Async(status, StmtPreIng)
		Err_Dml (err_status != 0, "mysql_stmt_prepare_start")
		goto DML_EXEC_START;

	DML_PRE_PRO:
		status= mysql_stmt_prepare_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtPreIng)
		Err_Dml(err_status != 0, "mysql_stmt_prepare_cont")

	DML_EXEC_START:
		/* input parameters */
		param_count= mysql_stmt_param_count((cur_cmd_obj->cmd_ptr);
		WBUG("statement has %d parameters", param_count);
		if ( param_count == 0 ) 
			goto DML_EXEC_START_2 ;
		/* 输入 */
		for ( i = 0; i < face->num && i < gCFG->param_max; i++ )
		{
			unsigned char *buf = (unsigned char*) 0;
			unsigned long blen = 0;
			DBFace::Para &para = face->paras[i];
			int rNo = para.fld + face->offset;

			assert(para.pos == i );
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
			case DBFace::PARA_INOUT:
				if ( (cur_cmd_obj->bndfmt[i].buffer_type =getdty_in(para.type)) == MYSQL_TYPE_INVALID )
				{
					WLOG(ERR,"error in getdty(%d), para.type %d, para.name is %s", i, para.type, para.name);
					err_before_ret = true;
					goto END_HANDLE;
				}

				assert(rNo <= rcv_pac->max );
				cur_cmd_obj->bndfmt[i].buffer = (char*) rcv_pac->fld[rNo].val;
				cur_cmd_obj->bndfmt[i].length =  rcv_pac->fld[rNo].range;
				if ( rcv_pac->fld[rNo].val == 0 || rcv_pac->fld[rNo].range == 0 ) 
					cur_cmd_obj->bndfmt[i].is_null =  &gCFG->isNull;
				else
					cur_cmd_obj->bndfmt[i].is_null =  0;
				WBUG("in parameter %s", para.name);
				break;

			default:
				break;
			}
		}

		status =  mysql_stmt_bind_param(cur_cmd_obj->cmd_ptr, cur_cmd_obj->bndfmt) ;
		Err_Dml(status != 0, "mysql_stmt_bind_param");
		/* end of input parameters, then goto exec  */

	DML_EXEC_START_2:
  		status= m_session->ready_status = mysql_stmt_execute_start(&err_status, cur_cmd_obj->cmd_ptr);
		Will_Async(status, StmtExeIng)
		Err_Dml (err_status != 0, "mysql_stmt_execute_start")
		if ( face->pro == DBFace::QUERY )		//it's seldom
			goto QUERY_FETCH_START;
		else
			goto DML_STORE_START;

	DML_EXEC_PRO:
		status= mysql_stmt_execute_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtExeIng)
		Err_Dml(err_status != 0, "mysql_stmt_execute_cont")

	DML_SOTRE_START:
		if ( !bind_result()) 
			goto DML_DONE;
		count = mysql_stmt_affected_rows(cur_cmd_obj->cmd_ptr);
		PUT_FSND (cRows_field, &count, sizeof(count));
  		status= m_session->ready_status = mysql_stmt_store_result_start(&err_status, cur_cmd_obj->cmd_ptr);
		Will_Async(status, StmtResStoreIng)
		Err_Dml (err_status != 0, "mysql_stmt_store_result_start")
		goto DML_GET_RES;

	DML_SOTRE_PRO:
		status= mysql_stmt_store_result_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtResStoreIng)
		Err_Dml(err_status != 0, "mysql_stmt_store_result_cont")
	DML_GET_RES:	
		get_result();

	DML_FREE_RESULT_START:
  		status= m_session->ready_status = mysql_stmt_free_result_start(&err_status, cur_cmd_obj->cmd_ptr);
		Will_Async(status, StmtResFreeIng)
		Err_Dml (err_status > 0, "mysql_stmt_free_result_start")
		goto DML_NEXT_START ;

	DML_FREE_RESULT_PRO:
		status= mysql_stmt_free_result_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtResFreeIng)
		Err_Dml (err_status > 0, "mysql_stmt_free_result_con")

	DML_NEXT_START:
  		status= m_session->ready_status = mysql_stmt_next_result_start(&err_status, cur_cmd_obj->cmd_ptr);
		Will_Async(status, StmtResNextIng)
		Err_Dml (err_status > 0, "mysql_stmt_next_result_start")
		if ( face->pro == DBFace::QUERY )		//it's seldom
			goto QUERY_NEXT_OK;
		else
			goto DML_NEXT_OK ;

	DML_NEXT_PRO:
		status= mysql_stmt_next_result_cont(&err_status, cur_cmd_obj->cmd_ptr, m_session->ready_status);
		Will_Async(status, StmtResNextIng)
	DML_NEXT_OK:	/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		if ( err_status ==0 )  {
			goto DML_STORE_START;
		} else  if ( err_status == -1)  {
			goto DML_LAST;
		} else {
			Err_Dml(true, "mysql_stmt_next_result_cont")
		}
		break;

	DML_LAST:
		WBUG("dml stmt seccessfully!");

	DML_DONE:
		mysql_stmt_reset(cur_cmd_obj->cmd_ptr);
		cur_cmd_obj->cmd_idle = true;
		m_session->async_step = 0;
		if ( cur_cmd_obj->many ==0 )		/* 还没有返回数据, 这里返回一下 */
			aptus->sponte(&dopac_pius);
		break;

	case DBFace::REAL_QUERY:
		WBUG("handle REAL_QUERY (\"%s\") ", face->sentence);
#define Err_Query(isErr, Pro_Str)	 			\
		if (isErr) {					\
			err_before_ret=true;			\
			int my_err;				\
			my_err = mysql_errno(m_session->mysql_local);				\
			PUT_FSND (errCode_field, &my_err, sizeof(my_err));			\
			PUT_FSND_STR(errStr_field, mysql_error(m_session->mysql_local));	\
			WLOG(ERR, Pro_Str " failed status=%d, error=%d, errmsg is %s", err_status, my_err, mysql_error(m_session->mysql)) ;	\
			logout();	\
			goto REAL_QUERY_DONE;			\
		}

		switch ( m_session->async_step ) {
		case Real_QueryIng:
			goto REAL_QUERY_PRO;
		case Real_FetchIng_Row:
			goto REAL_FETCH_PRO;
		case Real_FreeIng_Result:
			goto REAL_FREE_PRO;
		case Real_NextIng_Result:
			goto REAL_NEXT_PRO;
		case ConnectIng:
			logon();
			return;
		case Async_OK:		//Just start
			break;
		default: 
			return ;
		}

		status = 0;
		PUT_FSND (errCode_field, &status, sizeof(status));	 //首先假定结果OK，把值设到返回域中
  		status= m_session->ready_status = mysql_real_query_start(&err_status, m_session->mysql_local, (const char *)face->sentence, face->sentence_len);
		Will_Async(status, Real_QueryIng)
		Err_Query (err_status != 0, "mysql_real_query_start")
		goto REAL_RESULT_START;

	REAL_QUERY_PRO:
		status= mysql_real_query_cont(&err_status, m_session->mysql_local, m_session->ready_status);
		Will_Async(status, Real_QueryIng)
		Err_Query(err_status != 0, "mysql_real_query_cont")

	REAL_RESULT_START:
  		/* This method cannot block. */
		my_result = mysql_use_result(m_session->mysql_local);
		Err_Query( my_result == NULL, "mysql_use_result")
		count = mysql_affected_rows(m_session->mysql);
		PUT_FSND (cRows_field, &count, sizeof(count));

	REAL_FETCH_START:
		my_row = 0;
		status= m_session->ready_status = mysql_fetch_row_start(&my_row, my_result);
		Will_Async(status, Real_FetchIng_Row)
		if ( !my_row ) {
			Err_Query( mysql_errno(m_session->mysql_local) !=0 , "mysql_fetch_row_start")
			goto REAL_FREE_START;
		} else 
			goto GET_ROW;

	REAL_FETCH_PRO:
      		status= mysql_fetch_row_cont(&my_row, my_result, m_session->ready_status);
		Will_Async(status, Real_FetchIng_Row)

    		if ( !my_row) {	//no rows
			Err_Query(mysql_errno(m_session->mysql_local) !=0 , "mysql_fetch_row_cont")
			if ( face->rowset  && face->rowset->useEnd )
				aptus->sponte(&end_ps);	/* MULTI_UNIPAC_END */
			goto REAL_FREE_START;
		} 

	GET_ROW:
		copyRowValue();	
		aptus->sponte(&dopac_pius);
		goto FETCH_START;

	REAL_FREE_START:
		status = mysql_free_result_start(my_result);
		Will_Async(status, Real_FreeIng_Result)
		goto REAL_NEXT_START;

	REAL_FREE_PRO:
		status= mysql_free_result_cont(my_result, m_session->ready_status);
		Will_Async(status, Real_FreeIng_Result)

	REAL_NEXT_START:
  		status= m_session->ready_status = mysql_next_result_start(&err_status, m_session->mysql);
		Will_Async(status, Real_NextIng_Result)
		Err_Query(err_status > 0, "mysql_next_result_start")
		goto REAL_NEXT_OK:

	REAL_NEXT_PRO:
		status= mysql_next_result_cont(&err_status, m_session->mysql, m_session->ready_status);
		Will_Async(status, Real_NextIng_Result)
	REAL_NEXT_OK:	/* more results? -1 = no, >0 = error, 0 = yes (keep looking) */
		if ( err_status ==0 )  {
			goto REAL_RESULT_START;
		} else if ( err_status == -1)  {
			goto REAL_LAST;
		} else {
			Err_Dml(true, "mysql_next_result_cont")
		}
		break;
	REAL_LAST:
		WBUG("async query seccessfully!");

	REAL_QUERY_DONE:
		m_session->async_step = 0;
		break;

	default:
		break;
	}
END_HANDLE:
	if ( err_before_ret)
		aptus->sponte(&dopac_pius);
}

void MyCli::copyRowValue()
{
	unsigned int i,j ;
	unsigned int  num_fields = mysql_num_fields(my_result);
	unsigned long *lengths = mysql_fetch_lengths(my_result);
	for ( i = 0,j =0; j < face->num && i <  num_fields; ++j )
	{
		DBFace::Para &para = face->paras[j];
		assert(para.pos == j );

		if (  para.inout == DBFace::PARA_OUT ||  para.inout == DBFace::PARA_INOUT)
		{
			snd_pac->input(para.fld + face->offset, my_row[i], lengths[i]);
			i++;
		}
	}
}

void MyCli::copyRowSet(int rowi)
{
	unsigned int i; 
	int j ;
	struct FieldObj *fobj;
	int m_chunk;
	unsigned short rlen;

	if( face->rowset ) 
		m_chunk = face->rowset->chunk;
	else
		m_chunk = 1;

	for ( i  = 0 ; i < gCFG->ret_col_max  ; i++)
	{
		unsigned int offset = 0;
		if ( fldNo_arr[i] < 0 ) break;
		for ( j = 0 ; j < rowi; j++)	/* 累计前各行的长度而获得偏移量 */
		{
			offset += rlenp[i*m_chunk + j] ;
		}
		fobj = &rowset_pac.fld[fldNo_arr[i]];
		rlen = rlenp[i*m_chunk + rowi] ;
		if ( rlen && fobj->no > 0 )
			snd_pac->input(fobj->no, &(fobj->val[offset]), rlen);
	}
}

/* 以下一般性的设置, 其实是对MYSQL_WAIT_WRITE的设置  */
void MyCli::set_for_mysql(int status) {

	if ( !(status & MYSQL_WAIT_WRITE) )  {
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;	//等下一次设置POLLIN时不再设
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;	//等下一次设置时不再设
#endif	//for bsd

	} else if ( status & MYSQL_WAIT_WRITE ) {	//还是写阻塞, 再设

#if  defined(__linux__)
		pollor.ev.events |= EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events |= POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_ONESHOT;
#endif	//for bsd

	}

	if ( !(status &  MYSQL_WAIT_EXCEPT) )  {
#if  defined(__linux__)
		pollor.ev.events &= ~EPOLLPRI;	//等下一次设置POLLIN时不再设
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLPRI;	//等下一次设置POLLIN时不再设
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		//How to do ?
#endif	//for bsd

	} else if ( status &  MYSQL_WAIT_EXCEPT ) {

#if  defined(__linux__)
		pollor.ev.events |= EPOLLPRI;
#endif	//for linux

#if  defined(__sun)
		pollor.events |= POLLPRI;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		//How to do ?
#endif	//for bsd

	}

#if !defined(_WIN32 )
	gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 以设置kqueue等
#endif

	if ( status & MYSQL_WAIT_TIMEOUT  )
	{
		gCFG->interval = mysql_get_timeout_value_ms(m_session->mysql_local);
		gCFG->arr[0] = this;
		gCFG->sch->sponte(&gCFG->alarm_pius);
	}
}

/* 以下应为第一次设置  */
void MyCli::set_for_logon_mysql(int status) 
{
	pollor.pro_ps.ordo = Notitia::ACCEPT_EPOLL; //直到logon完成才被changed 
	pollor.fd = mysql_get_socket(m_session->mysql_local);
#if  defined(__linux__)
	pollor.ev.events = EPOLLIN | EPOLLRDHUP | (status & MYSQL_WAIT_WRITE ? EPOLLOUT : 0) | (status & MYSQL_WAIT_EXCEPT ? EPOLLPRI : 0);
	pollor.op = EPOLL_CTL_ADD;
#endif	//for linux

#if  defined(__sun)
	pollor.ev.events = POLLIN | (status & MYSQL_WAIT_WRITE ? POLLOUT : 0) | (status & MYSQL_WAIT_EXCEPT ? POLLPRI : 0);
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
	EV_SET(&(pollor.events[0]), pollor.fd, EVFILT_READ, EV_ADD , 0, 0, &pollor);
	if ( status & MYSQL_WAIT_WRITE )
		EV_SET(&(pollor.events[1]), pollor.fd, EVFILT_WRITE, EV_ADD , 0, 0, &pollor);
	else
		EV_SET(&(pollor.events[1]), pollor.fd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, &pollor);
#endif	//for bsd

	gCFG->sch->sponte(&epl_set_ps);	//向tpoll

#if  defined(__linux__)
	pollor.op = EPOLL_CTL_MOD; //以后操作就是修改了。
#endif	//for linux

	if ( status & MYSQL_WAIT_TIMEOUT  )
	{
		gCFG->interval = mysql_get_timeout_value_ms(m_session->mysql_local);
		gCFG->arr[0] = this;
		gCFG->sch->sponte(&gCFG->alarm_pius);
	}
}

void MyCli::logout()
{
	if ( m_session && m_session->isTalking )
	{
		mysql_close(m_session->mysql_local);
		WLOG(INFO, "mysql_close (%s)", gCFG->c_host);
		m_session->isTalking = false;
		m_session->mysql_local = 0;
	}
	return ; 
}

void MyCli::logon()
{
	MYSQL *ret;
	int status;
	if ( m_session->async_step != ConnectIng ) {	//first time
		status = m_session->ready_status =  mysql_real_connect_start(&ret, m_session->mysql_local, gCFG->c_host,  gCFG->c_user,
						gCFG->c_auth,  gCFG->c_dbnm,  gCFG->c_port, gCFG->c_sock, 0);
		 m_session->async_step = ConnectIng ;
		if ( status ) 
			set_for_logon_mysql(status);
		else 
			goto DONE;
	} else {
		status = m_session->ready_status = mysql_real_connect_cont((&ret, m_session->mysql_local, m_session->ready_status);
		if ( status ) 
			set_for_mysql(status);
		else 
			goto DONE;
	}
	return ;

DONE: // !status  
	m_session->async_step = Async_OK;
	gCFG->sch->sponte(&clr_timer_pius);
	if ( ret ) {	//success
		m_session->isTalking = true;
		establish_done();
	} else {	//error
		m_session->isTalking = false;
		WLOG(ERR, "mysql_real_connect_nonblocking() failed status=%d, error=%d, errmsg is %s", status, mysql_errno(m_session->mysql_local), mysql_error(m_session->mysql)) ;
	}
}

#define ERR_IF(X, Y)  if ( X != 0 ) { WLOG(ERR, "%s() failed error=%d, errmsg is %s", Y, mysql_errno(m_session->mysql_local), mysql_error(m_session->mysql_local)) ; return; }
void MyCli::myalloc(bool isPioneer, TiXmlElement *cfg)
{
	int retcode;
	if ( isPioneer || !gCFG->shared_session )
	{
		m_session = new MySession;
		if (!(m_session->mysql_local = mysql_init(NULL))) {
			WLOG(ERR,"mysql_init() failed");
			return;	
		}
		mysql_options(m_session->mysql_local, MYSQL_OPT_NONBLOCK, 0);
		if ( gCFG->default_group[0] ) 
			mysql_options(m_session->mysql_local, MYSQL_READ_DEFAULT_GROUP, gCFG->default_group);
	}

	rowset_pac.produce(gCFG->ret_col_max);
	if ( !fldNo_arr) 
		fldNo_arr = new int [gCFG->ret_col_max];
	memset(fldNo_arr, 0xff, (gCFG->ret_col_max)*sizeof(int));

	/* Set the username and password properties */
	retcode = mysql_options (m_session->mysql_local, MYSQL_DEFAULT_AUTH,(char*)gCFG->c_user);
	ERR_IF ( retcode, " mysql_options: MYSQL_DEFAULT_AUTH ");

	retcode = mysql_options (m_session->mysql_local, MYSQL_OPT_BIND, (char*)gCFG->c_host);
	ERR_IF ( retcode, " mysql_options: MYSQL_OPT_BIND ");
}
#undef ERR_IF

enum enum_field_types MyCli::getdty_out(DBFace::DataType type)
{
	enum enum_field_types dty;
	switch ( type )
	{
	case DBFace::BigInt:
		dty = MYSQL_TYPE_LONGLONG;
		break;

	case DBFace::Integer:
	case DBFace::Long:
		dty = MYSQL_TYPE_LONG;
		break;

	case DBFace::SmallInt:
		dty = MYSQL_TYPE_SHORT;
		break;

	case DBFace::TinyInt:
		dty = MYSQL_TYPE_TINY;
		break;

	case DBFace::Boolean:
		dty = MYSQL_TYPE_BOOL;
		break;

	case DBFace::Char:
	case DBFace::String:
	case DBFace::Binary:
		dty = MYSQL_TYPE_STRING;
		break;

	case DBFace::Decimal:
	case DBFace::Numeric:
	case DBFace::Currency:
		dty = MYSQL_TYPE_NEWDECIMAL;
		break;

	case DBFace::Float:
	case DBFace::Single:
		dty = MYSQL_TYPE_FLOAT;
		break;

	case DBFace::Double:
		dty = MYSQL_TYPE_DOUBLE;
		break;

	case DBFace::TimeStamp:
		dty = MYSQL_TYPE_TIMESTAMP;
		break;

	case DBFace::Date:
		dty = MYSQL_TYPE_DATE;
		break;

	case DBFace::Time:
		dty = MYSQL_TYPE_TIME;
		break;

	case DBFace::DatTime:
		dty = MYSQL_TYPE_DATETIME;
		break;

	case DBFace::Bit:
		dty = MYSQL_TYPE_BIT;
		break;

	case DBFace::VarBinary:
	case DBFace::LongBinary:
		dty = MYSQL_TYPE_VAR_STRING;
		break;

	case DBFace::Text:
		dty = MYSQL_TYPE_BLOB;
		break;

	case DBFace::LongText:
		dty = MYSQL_TYPE_LONG_BLOB;
		break;

	default:
		WLOG(CRIT, "Unknown data type %d!", type);
		dty = MYSQL_TYPE_INVALID;
	}
	return dty;
}	/* out data type */

enum enum_field_types MyCli::getdty_in(DBFace::DataType type)
{
	enum enum_field_types dty;
	switch ( type )
	{
	case DBFace::BigInt:
		dty = MYSQL_TYPE_LONGLONG;
		break;

	case DBFace::Integer:
	case DBFace::Long:
		dty = MYSQL_TYPE_LONG;
		break;

	case DBFace::SmallInt:
		dty = MYSQL_TYPE_SHORT;
		break;

	case DBFace::TinyInt:
		dty = MYSQL_TYPE_TINY;
		break;

	case DBFace::Boolean:
		dty = MYSQL_TYPE_BOOL;
		break;

	case DBFace::Char:
	case DBFace::String:
	case DBFace::Text:
		dty = MYSQL_TYPE_STRING;
		break;

	case DBFace::Decimal:
		dty = MYSQL_TYPE_DECIMAL;
		break;

	case DBFace::Numeric:
	case DBFace::Currency:
		dty = MYSQL_TYPE_NEWDECIMAL;
		break;

	case DBFace::Float:
	case DBFace::Single:
		dty = MYSQL_TYPE_FLOAT;
		break;

	case DBFace::Double:
		dty = MYSQL_TYPE_DOUBLE;
		break;

	case DBFace::TimeStamp:
		dty = MYSQL_TYPE_TIMESTAMP;
		break;

	case DBFace::Date:
		dty = MYSQL_TYPE_DATE;
		break;

	case DBFace::Time:
		dty = MYSQL_TYPE_TIME;
		break;

	case DBFace::DateTime:
		dty = MYSQL_TYPE_DATETIME;
		break;

	case DBFace::Year:
		dty = MYSQL_TYPE_SHORT:
		break;

	case DBFace::Bit:
		dty = MYSQL_TYPE_BIT;
		break;

	case DBFace::VarBinary:
	case DBFace::Binary:
		dty = MYSQL_TYPE_BLOB;
		break;

	case DBFace::LongText:
	case DBFace::LongBinary:
		dty = MYSQL_TYPE_LONG_BLOB;
		break;

	default:
		WLOG(CRIT, "Unknown data type %d!", type);
		dty = MYSQL_TYPE_INVALID;
	}
	return dty;
}

bool MyCli::bind_result()
{
#define Err_Res(isErr, Pro_Str)	 			\
		if (isErr) {				\
			err_before_ret=true;		\
			int my_err;			\
			my_err = mysql_errno(m_session->mysql_local);				\
			PUT_FSND (errCode_field, &my_err, sizeof(my_err));			\
			PUT_FSND_STR(errStr_field, mysql_error(m_session->mysql_local));	\
			WLOG(ERR, Pro_Str " failed status=%d, error=%d, errmsg is %s", err_status, my_err, mysql_error(m_session->mysql)) ;	\
			goto END;			\
		}
	int rNo, i;
	unsigned oTotal;
	int num_fields;       /* number of columns in result */
	MYSQL_FIELD *fields;  /* for result set metadata */
	MYSQL_RES *rs_metadata;

	/* the column count is > 0 if there is a result set */
	/* 0 if the result is only the final status packet */
	num_fields = mysql_stmt_field_count( cur_cmd_obj->cmd_ptr);
	if ( gCFG->ret_col_max < num_fields  ) {
		char msg[64];
		my_err = 1000;
		PUT_FSND (errCode_field, &my_err, sizeof(my_err));
		TEXTUS_SPRINTF(msg, "num_fields > max (%d)", gCFG->ret_col_max);
		PUT_FSND_STR(errStr_field, msg);
		WLOG(ERR, "%s", msg);
		goto END;
	}

	WBUG("Number of columns in result: %d", (int) num_fields);
	if (num_fields <= 0)
		goto END;

		/* what kind of result set is this? */
    		printf("Data: ");
	if(m_session->mysql_local->server_status & SERVER_PS_OUT_PARAMS) {	//case CS_PARAM_RESULT:
		WBUG("this result set contains OUT/INOUT parameters");
		oTotal = face->outSize, outNum = face->outNum;
		if ( outNum > gCFG->param_max )
			WLOG(WARNING, "out parameter exceeds %d", gCFG->param_max);
		snd_pac->grant(oTotal);	/* 扩张出足够的空间 */

	} else {	// case CS_ROW_RESULT:
		WBUG("this result set is produced by the procedure");
		/* 计算一下总输出长度, 使得rowset_pac空间固定 */
		para_offset = face->rowset == 0 ? 0 : face->rowset->para_pos ;
		oTotal = face->outSize, outNum = face->outNum;
		if ( outNum > gCFG->ret_col_max )
			WLOG(WARNING, "out columns exceeds %d", gCFG->ret_col_max);

		rowset_pac.reset();
		if( face->rowset ) 
			m_chunk = face->rowset->chunk;
		else
			m_chunk = 1;
		rowset_pac.grant(oTotal*m_chunk);	/* 扩张出足够的空间, m_chunk下面还要用到 */
		memset(fldNo_arr, 0xff, (gCFG->ret_col_max)*sizeof(int));
		if ( m_chunk * outNum > rlen_sz )
		{
			rlen_sz = m_chunk * outNum;
			if ( rlenp ) delete [] rlenp;
			rlenp = new CS_INT [rlen_sz];
			memset(rlenp, 0, rlen_sz*sizeof(CS_INT));
		}
	}


	rs_metadata = mysql_stmt_result_metadata(cur_cmd_obj->cmd_ptr);
	Err_Res( !rs_metadata , "mysql_stmt_result_metadata");
	fields = mysql_fetch_fields(rs_metadata);
	memset(cur_cmd_obj->retfmt, 0, sizeof (MYSQL_BIND) * num_fields);

	/* 计算一下总输出长度, 使得snd_pac空间固定 */
	oTotal = 0, outNum = 0;
	for ( i = 0; i < face->num ; ++i )
	{	
		DBFace::Para &para = face->paras[i];
		rNo = para.fld + face->offset;

		assert(para.pos == i );
		switch ( para.inout )
		{
		case DBFace::PARA_OUT:
			assert(rNo <= snd_pac->max );
			oTotal += ( fields[outNum].length >  para.outlen ?  para.outlen : fields[outNum].length );	//取较小者
			outNum++;
			break;

		case DBFace::PARA_INOUT:
			assert(rNo <= snd_pac->max );
			oTotal += (para.outlen == 0 ? rcv_pac->fld[rNo].range : (fields[outNum].length > para.outlen ? para.outlen : fields[outNum].length) );
			outNum++;
			break;

		default:
			break;
		}
	}
	snd_pac->grant(oTotal);	/* 扩张出足够的空间 */
	//memset( snd_pac->buf.point, 0, oTotal);

	/* set up and bind result set output buffers */
	for ( i = 0,j =0; j < face->num && i <  num_fields; ++j )
	{
		DBFace::Para &para = face->paras[j];
		assert(para.pos == j );

		if (  para.inout == DBFace::PARA_OUT ||  para.inout == DBFace::PARA_INOUT)
		{
			rNo = para.fld + face->offset;
			cur_cmd_obj->retfmt[i].buffer_type = fields[i].type;
			if ( getdty_out(para.type) != fields[i].type ) 
			{
				WLOG(ERR, "para[%d].type %d not same as fields[%d].type %d!", j, para.type, i,  fields[i].type);
			}
			cur_cmd_obj->retfmt[i].is_null = &cur_cmd_obj->is_null_arr[i];
	  		cur_cmd_obj->retfmt[i].buffer = (char *) snd_pac->buf.point;
			cur_cmd_obj->retfmt[i].length =  &snd_pac->fld[rNo].range;
			//cur_cmd_obj->retfmt[i].buffer_length = para.outlen;
			if ( para.outlen > 0 ) {
				cur_cmd_obj->retfmt[i].buffer_length =  (fields[i].length > para.outlen ? para.outlen : fields[i].length);	//取较小者
			} else {
				cur_cmd_obj->retfmt[i].buffer_length = rcv_pac->fld[rNo].range ;
			}
			snd_pac->commit(rNo, cur_cmd_obj->retfmt[i].buffer_length); //前有足够空间, 这里就不会变动地址
			i++;
		}
	}
	status = mysql_stmt_bind_result(cur_cmd_obj->cmd_ptr, cur_cmd_obj->retfmt);
	mysql_free_result(rs_metadata); /* free metadata */
	Err_Res( status!=0 , "mysql_stmt_bind_result");
	return true;
END:
	return false;
}

int MyCli::get_result()
{
	unsigned int i;
	int status, my_err;
	/* fetch and display result set rows */
	cur_cmd_obj->many =0;
	while (1)
	{
		status = mysql_stmt_fetch(cur_cmd_obj->cmd_ptr);
		if (status == MYSQL_NO_DATA) {
			break;
		} else if (status == MYSQL_DATA_TRUNCATED ) {
			WLOG(ERR, "mysql_stmt_fetch encounter  MYSQL_DATA_TRUNCATED");
		} else if (status != 0 ) {
			my_err = mysql_errno(m_session->mysql_local);
			PUT_FSND (errCode_field, &my_err, sizeof(my_err));
			PUT_FSND_STR(errStr_field, mysql_error(m_session->mysql_local));
			WLOG(ERR, "mysql_stmt_fetch failed status=%d, error=%d, errmsg is %s", status, my_err, mysql_error(m_session->mysql));	
			return status;
		}
#ifdef LOOK_DATA
		for (i = 0; i < 2; ++i)
		{
			switch (cur_cmd_obj->retfmt[i].buffer_type)
			{
			case MYSQL_TYPE_STRING:
				if (*cur_cmd_obj->retfmt[i].is_null)
					printf(" val[%d] = NULL;", i);
				else
					printf(" val[%d] = %s;", i, (char *) (cur_cmd_obj->retfmt[i].buffer));
            		break;

			case MYSQL_TYPE_LONG:
				if (*cur_cmd_obj->retfmt[i].is_null)
					printf(" val[%d] = NULL;", i);
				else
					printf(" val[%d] = %ld;", i, (long) *((int *) cur_cmd_obj->retfmt[i].buffer));
            		break;

          		default:
				printf("  unexpected type (%d)\n", cur_cmd_obj->retfmt[i].buffer_type);
        		}
		}
			printf("\n");
#endif
		cur_cmd_obj->many++;
		aptus->sponte(&dopac_pius);	//put data to back 
		if ( face->rowset  && face->rowset->useEnd )
			aptus->sponte(&end_ps);	/* MULTI_UNIPAC_END */
	}

	return 0;
}

void MyCli::cmd_alloc( DBFace *face)
{
	OBJList *obj;
	unsigned int i;
	MyFaceExt *mext;
	if ( !face->dbext ) {
		face->dbext = mext = new MyFaceExt;
		mext->set_proc_str(face);
	} else {
		mext =  (MyFaceExt *)face->dbext;
	}
	obj = mext->olist.look_for_idle();
	if ( !obj ) {
		obj = new OBJList();
		obj->bndfmt = new MYSQL_BIND[gCFG->param_max*sizeof(MYSQL_BIND)];
		obj->retfmt = new MYSQL_BIND[gCFG->ret_col_max*sizeof(MYSQL_BIND)];
		obj->is_null_arr = new bool[gCFG->ret_col_max*sizeof(bool)];
		mext->olist.appen(obj);
		for ( i = 0; i < face->num && i < gCFG->param_max; i++ )
		{
			DBFace::Para &para = face->paras[i];
			assert(para.pos == i );
			memset(&obj->bndfmt[i], 0, sizeof(MYSQL_BIND));
			if ( (obj->bndfmt[i].buffer_type = getdty_in(para.type)) == MYSQL_TYPE_INVALID || !para.name)
			{
				WLOG(ERR,"error in getdty(%d), para.type %d, para.name is %s", i, para.type, para.name);
			}
		}
	}
	if (!(obj->cmd_ptr = mysql_stmt_init(m_session->mysql_local))) {
		WLOG(ERR, "mysql_stmt_init() failed error=%d, errmsg is %s", mysql_errno(m_session->mysql_local), mysql_error(m_session->mysql)) ;
		return;
	}
	obj->cmd_idle = false;
	cur_cmd_obj = obj;
}

void MyCli::establish_done()
{
	int value;
	socklen_t len;
	value = 0; len= sizeof(value);
	if ( getsockopt(  mysql_get_socket(m_session->mysql_local),SOL_SOCKET,  SO_SNDBUF,  (void*)&value, &len) < 0)
	{
		WLOG(ERR,"getsockopt SO_RCVBUF");
		goto N_PRO;
	}
	printf("srv ------- buf len %d\n", value);
	if ( value < gCFG->transmit_buf_sz ) {
		value = gCFG->transmit_buf_sz;
		if ( setsockopt(connfd ,SOL_SOCKET,  SO_SNDBUF,  (void*)&value, len) < 0)
		{
			WLOG(ERR,"setsockopt SO_RCVBUF");
		}
	}
N_PRO:
	if ( gCFG->use_epoll ) 
	{
		pollor.pro_ps.ordo = Notitia::RD_EPOLL;
		pollor.fd =  mysql_get_socket(m_session->mysql_local);

#if  defined(__linux__)
		pollor.ev.events = EPOLLIN | EPOLLET |EPOLLONESHOT|EPOLLRDHUP;	//MeriaDB perfect?
		pollor.ev.events &= ~EPOLLOUT;
#endif	//for linux

#if  defined(__sun)
		pollor.events &= ~POLLOUT;
#endif	//for sun

#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		pollor.events[1].flags = EV_ADD | EV_DISABLE;
#endif	//for bsd

		gCFG->sch->sponte(&epl_set_ps);	//向tpoll, 
	} else {
		mytor.scanfd = mysql_get_socket(m_session->mysql_local);
		deliver(Notitia::FD_SETRD);
		deliver(Notitia::FD_CLRWR);
		deliver(Notitia::FD_CLREX);
	}

	gCFG->sch->sponte(&clr_timer_pius); /* 清除定时 */
	WLOG(INFO, "estabish %s%s ok!", gCFG->c_host, gCFG->c_sock);
	deliver(Notitia::START_SESSION); //向接力者发出通知, 本对象开始
	if ( m_session->should_hand_pac ) handle_pac();
}

bool MyCli::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_DBCONN:
		WBUG("facio CMD_SET_DBCONN");
		if ( pius->indic )
			TEXTUS_SNPRINTF(gCFG->server_name, sizeof(gCFG->server_name)-1, "%s", (char*)pius->indic);
		break;
		
	default:
		return false;
	}
	return true;
}

MyCli::MyCli()
{
	gCFG = 0;
	has_config = false;
	m_session = 0;

	cur_cmd_obj = 0;
	last_pro = (DBFace::PROCTYPE)-1;	

	face = 0;
	dopac_pius.ordo = Notitia::PRO_UNIPAC;
	dopac_pius.indic = 0;

	end_ps.ordo = Notitia::MULTI_UNIPAC_END;
	end_ps.indic = 0;
	
	rlen_sz = 0;
	rlenp = 0;
	fldNo_arr = 0;

	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	epl_set_ps.ordo = Notitia::SET_EPOLL;
	epl_set_ps.indic = &pollor;

	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = 0;
}

MyCli::~MyCli() 
{ 
	OBJList *obj;
	obj = objlist.remove(this);
	if ( obj->cmd_ptr) {
		mysql_stmt_close( obj->cmd_ptr);
		obj->cmd_ptr = 0;
	}
	delete obj;
	if ( has_config ) { 
		logout();
		delete m_session;
		delete gCFG;

	} else if ( !gCFG->shared_session)
	{
		logout();
		delete m_session;
		m_session = 0;
	}
}

Amor* MyCli::clone()
{
	MyCli *child = new MyCli();
	child->gCFG = gCFG;
	if ( gCFG->shared_session) {
		child->m_session = m_session;
	}
	return (Amor*)child;
}

#include "hook.c"
