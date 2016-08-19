/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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
 $Header: /textus/sybcli/SybCli.cpp 9     12-04-25 11:47 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: SybCli.cpp $"
#define TEXTUS_MODTIME  "$Date: 12-04-25 11:47 $"
#define TEXTUS_BUILDNO  "$Revision: 9 $"
/* $NoKeywords: $ */

#include <assert.h>
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "DBFace.h"
#include "PacData.h"
#include <ctpublic.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

#ifndef TINLINE
#define TINLINE inline
#endif


#define BUFF_SIZE 256
#define DEFAULT_MAX_RET_COLUMNS 256
#define DEFAULT_PARAM_MAX 256
#define STR_STATUS_RESULT "STATUS RESULT"

class SybCli: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	SybCli();
	~SybCli();
#include "wlog.h"
	inline void logerr ( char *str, char *msg) {
		WLOG(ERR, "%s %s", str, msg);
	};
	Amor::Pius dopac_pius, end_ps;  //仅用于向左边传回数据
	bool inThread;
	const char *so_file;
	
	void handle_pac(bool isDBFetch=false);
	CS_RETCODE handle_result( bool has_back = true);
	CS_RETCODE fetch_data ( int res_type );
	CS_RETCODE bind_columns ( int res_type );
	TINLINE void copyRowValue();
	TINLINE void copyRowSet(int);
#define ROWSET_LEFT	0
#define ROWS_OBTAINED	1

	TINLINE void logon();
	TINLINE void logout();
	TINLINE void myalloc();
	TINLINE CS_INT getdty(DBFace::DataType type);
	TINLINE long getRowsetSize();
	bool hasError(CS_RETCODE ret, bool *isErr = 0);

	bool has_config;
	struct G_CFG {
		bool shared_session;	/* 是否共享同一个会话 */
		char interface[BUFF_SIZE];
		char server_name[BUFF_SIZE];
		char user_name[BUFF_SIZE];
		char password[BUFF_SIZE];
		unsigned int param_max;
		unsigned int ret_col_max;
		CS_INT sec_challenge;
		CS_INT sec_encryption;
		CS_INT sec_negotiate;
		CS_INT sec_appdefined;

		inline G_CFG() {
			memset(interface, 0, sizeof(interface));
			memset(server_name, 0, sizeof(server_name));
			memset(user_name, 0, sizeof(user_name));
			memset(password, 0, sizeof(password));
			shared_session = false;
			param_max = DEFAULT_PARAM_MAX;
			ret_col_max = DEFAULT_MAX_RET_COLUMNS;
			sec_challenge  = CS_FALSE;
			sec_encryption = CS_FALSE;
			sec_negotiate  = CS_FALSE;
			sec_appdefined = CS_FALSE;
		};
	};
	struct G_CFG *gcfg;


	CS_CONTEXT	*cntx_ptr;	/* 所有对象共享的context */
	CS_CONNECTION   *conn_ptr ;	/* connection, 可以多个子对象共享, 或者不是 */

	bool isPoineer;

	bool *isTalking;	/* 一个会话是否进行 */
	
	PacketObj *rcv_pac, *snd_pac;	/* 来自左节点的PacketObj */
	PacketObj rowset_pac;	/* 存放查询行集中的一次所取数据 */
	DBFace *face;

	CS_COMMAND	*cmd_ptr ;
	bool hcmd_idle;		//最近命令是否为空
	DBFace::PROCTYPE last_pro; //最近的处理类型


	CS_INT	cRowsObtained;	/* 结果集返回记录数 */
	long	cRowsLeft;	/* 行集中还有多少记录未取 */
#define STMT_MAX	1024
	char proc_stmt[STMT_MAX];
	CS_DATAFMT  *bndfmt;       /* the binding datafmt */
	CS_DATAFMT  *retfmt;       /* the return datafmt */

	CS_INT *rlenp;
	int *fldNo_arr;		/* rowset时, 记住这些保存了返回数据的域号 */
	unsigned int rlen_sz;
	void ready_alloc();

#define PUT_FSND(FLD,BUF,LEN)	\
		if ( snd_pac && face ) 	\
			snd_pac->input(face->FLD, (unsigned char*)BUF, LEN);

#define PUT_FSND_STR(FLD,BUF) PUT_FSND(FLD,BUF,strlen(BUF))
};

typedef struct _OBJList {
	CS_CONNECTION *connection;
	SybCli *client;	
	struct _OBJList *prev, *next;
	inline _OBJList () {
		connection = 0;
		client = 0;
		prev = 0;
		next = 0;
	};

	inline void put ( struct _OBJList *neo ) {
		if( !neo ) return;
		neo->next = next;
		neo->prev = this;
		if ( next != 0 )
			next->prev = neo;
		next = neo;
	};

	inline struct _OBJList * find( void *data, int mode=0 ) {
		struct _OBJList *obj;
		if( !data ) return 0;
		obj = next;
		while ( obj && data != (mode==0 ? (void*)obj->connection: (void*)obj->client) )
		{
			obj = obj->next;
		}
		return obj;
	};

	inline struct _OBJList * remove ( CS_CONNECTION *conn ) {
		struct _OBJList *obj;
		obj = find(conn);
		if ( !obj ) return 0;	/* 没有一个有这样的connection*/
		/* 至此, 一个obj, 该connection符合条件, 这个obj要去掉  */
		obj->prev->next = obj->next; 
		if ( obj->next )
			obj->next->prev  =  obj->prev;
		return obj;
	};

	inline struct _OBJList * remove ( SybCli *cli ) {
		struct _OBJList *obj;
		obj = find(cli, 1);
		if ( !obj ) return 0;	/* 没有一个有这样的client*/
		/* 至此, 一个obj, 该client符合条件, 这个obj要去掉  */
		obj->prev->next = obj->next; 
		if ( obj->next )
			obj->next->prev  =  obj->prev;
		return obj;
	};
} OBJList ;

OBJList objlist;

 /* 客户端消息处理程序 */
static CS_RETCODE client_msg_handler(CS_CONTEXT *context,  CS_CONNECTION *connection, CS_CLIENTMSG *errmsg)
{
	OBJList *obj = objlist.find(connection);
	SybCli *cli;
	char str[512], *cstr=&str[0];

	if ( !obj ) goto END;
	cli= obj->client;
	TEXTUS_SNPRINTF(str, sizeof(str), "Client message : layer = (%ld), origin = (%ld), severity = (%ld), number = (%ld)", CS_LAYER(errmsg->msgnumber), CS_ORIGIN(errmsg->msgnumber), CS_SEVERITY(errmsg->msgnumber), CS_NUMBER(errmsg->msgnumber));
	cli->logerr(cstr, errmsg->msgstring);
        if (errmsg->osstringlen > 0)
        {
		cli->logerr(errmsg->osstring, "");
        }
END:
        return CS_SUCCEED;
}

/* 服务器消息回叫程序 */
static CS_RETCODE server_msg_handler(CS_CONTEXT *context, CS_CONNECTION *connection, CS_SERVERMSG *errmsg)
{
	OBJList *obj = objlist.find(connection);
	SybCli *cli;
	char str[512], *cstr=&str[0];
	char procmsg[60];

	if ( !obj ) goto END;
	cli= obj->client;
	if (errmsg->proclen > 0)
		TEXTUS_SNPRINTF(procmsg, sizeof(procmsg), " at procedure '%s'", errmsg->proc);
	else 
		procmsg[0] = '\0';

	TEXTUS_SNPRINTF(str, sizeof(str), "Server%s message (%ld), severity(%ld), state(%ld), line(%ld) ", procmsg, errmsg->msgnumber, errmsg->severity, errmsg->state, errmsg->line);
	cli->logerr(cstr, errmsg->text);
END:
        return CS_SUCCEED;
}

void SybCli::ignite(TiXmlElement *cfg) 
{
	TiXmlElement *con_ele, *sec_ele;
	const char *comm_str;

	if (!cfg) return;
	if ( !gcfg ) 
	{
		gcfg = new struct G_CFG();
		has_config = true;
	}

	gcfg->param_max = DEFAULT_PARAM_MAX;
	gcfg->ret_col_max = DEFAULT_MAX_RET_COLUMNS;
	cfg->QueryIntAttribute("max_parameter_num", (int*)&(gcfg->param_max));
	cfg->QueryIntAttribute("max_column_num", (int*)&(gcfg->ret_col_max));
	ready_alloc();
	gcfg->shared_session =  ( (comm_str= cfg->Attribute("session")) && strcasecmp(comm_str, "shared") == 0 );
	comm_str = cfg->Attribute("interface");
	if ( comm_str)
		TEXTUS_STRNCPY(gcfg->interface, comm_str, sizeof(gcfg->interface)-1);

	sec_ele = cfg->FirstChildElement("security");
	if ( sec_ele )
	{
		#define SET_SEC(X) \
		comm_str = sec_ele->Attribute(#X);				\
		if (comm_str) {							\
			if (strcasecmp(comm_str, "true") == 0 )			\
				gcfg->sec_##X = CS_TRUE;			\
			else if (strcasecmp(comm_str, "false") == 0 )		\
				gcfg->sec_##X = CS_FALSE;			\
		}

		SET_SEC(challenge)
		SET_SEC(encryption)
		SET_SEC(negotiate)
		SET_SEC(appdefined)
	}

	con_ele = cfg->FirstChildElement("connect");
	if ( !con_ele ) 
		return;

	comm_str = con_ele->Attribute("user");
	if ( comm_str)
		TEXTUS_STRNCPY(gcfg->user_name, comm_str, sizeof(gcfg->user_name)-1);
	comm_str = con_ele->Attribute("password");
	if ( comm_str)
		TEXTUS_STRNCPY(gcfg->password, comm_str, sizeof(gcfg->password)-1);

	comm_str = con_ele->Attribute("server");
	if ( comm_str)
		TEXTUS_STRNCPY(gcfg->server_name, comm_str, sizeof(gcfg->server_name)-1);
	return;
}

#define CHECK_ENV()	\
	if ( !face )		\
	{			\
		WLOG(INFO, "no dbface yet.");	\
		break;				\
	}

bool SybCli::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	CS_RETCODE      retcode ;

#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); break; }
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		CHECK_ENV()

		if ( !(*isTalking)) logon();
		if ( (*isTalking)) 
		{ 
			handle_pac();
			WBUG("handle end");
		}
		break;

	case Notitia::CMD_DBFETCH:
		WBUG("facio CMD_DBFETCH");
		CHECK_ENV()

		if ( (*isTalking))
		{
			handle_pac(true);	//handle_pac的fetch方式
		} else {
			WLOG(INFO, "no talking");
		}
		break;

	case Notitia::CMD_SET_DBFACE:
		WBUG("facio CMD_SET_DBFACE");
		face = (DBFace *)pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		/* allocate a context */
		retcode = cs_ctx_alloc (CS_VERSION_125, &cntx_ptr );
		ERR_IF ( retcode, "cs_ctx_alloc ");

		/* initialize the library */
		retcode = ct_init (cntx_ptr, CS_VERSION_125);
		ERR_IF ( retcode, "ct_init ");

		/*  interface file */
		if ( strlen(gcfg->interface) > 0 )
		retcode = ct_config (cntx_ptr, CS_SET, CS_IFILE, gcfg->interface,
				CS_NULLTERM, NULL );
		if (retcode != CS_SUCCEED) 
		{
			WLOG(ERR,"error in ct_config: CS_IFILE(%s)",gcfg->interface);
			break;
		}

		/* install the server message callback */
		retcode = ct_callback ( cntx_ptr, NULL, CS_SET, CS_SERVERMSG_CB,
			(CS_VOID *)server_msg_handler) ;
		ERR_IF ( retcode, "ct_callback: CS_SERVERMSG_CB");

		/* install the client message callback */
		retcode = ct_callback (cntx_ptr, NULL, CS_SET, CS_CLIENTMSG_CB,
			(CS_VOID *)client_msg_handler);
		ERR_IF ( retcode, "ct_callback: CS_CLIENTMSG_CB");

		myalloc();

		/* Allocate a command structure */
		retcode = ct_cmd_alloc (conn_ptr, &cmd_ptr );
		ERR_IF ( retcode, "ct_cmd_alloc ");

		isPoineer = true;
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		logout();
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		logon();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		if ( !gcfg->shared_session )
		{
			myalloc();
		}
		/* Allocate a command structure */
		retcode = ct_cmd_alloc (conn_ptr, &cmd_ptr );
		ERR_IF ( retcode, "ct_cmd_alloc ");

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
		retcode = ct_cancel(NULL, cmd_ptr, CS_CANCEL_ALL);
		if ( retcode == CS_SUCCEED )
			hcmd_idle = true;
		WBUG("facio CMD_DB_CANCEL cmd_ptr %p, ct_cancel retcode %lu", cmd_ptr, retcode);
		break;

	default:
		WBUG("facio Notitia::%lu", pius->ordo);
		return false;
	}

	return true;

#undef ERR_IF
}

bool SybCli::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_DBCONN:
		WBUG("facio CMD_SET_DBCONN");
		if ( pius->indic )
			TEXTUS_SNPRINTF(gcfg->server_name, sizeof(gcfg->server_name)-1, "%s", (char*)pius->indic);
		break;
		
	default:
		return false;
	}
	return true;
}

SybCli::SybCli()
{
	gcfg = 0;
	has_config = false;

	cntx_ptr = 0;
	conn_ptr = 0;
	cmd_ptr = 0;
	hcmd_idle = true;
	last_pro = (DBFace::PROCTYPE)-1;	

	isPoineer = false;
	face = 0;
	dopac_pius.ordo = Notitia::PRO_UNIPAC;
	dopac_pius.indic = 0;

	end_ps.ordo = Notitia::MULTI_UNIPAC_END;
	end_ps.indic = 0;
	
	bndfmt = 0;
	retfmt = 0;

	rlen_sz = 0;
	rlenp = 0;
	fldNo_arr = 0;
}

SybCli::~SybCli() 
{ 
	CS_RETCODE	retcode ;
	OBJList *obj;
	obj = objlist.remove(this);
	delete obj;
#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); }

	if ( bndfmt ) delete bndfmt;
	if ( retfmt ) delete retfmt;
	if ( cmd_ptr )
	{
		retcode = ct_cmd_drop(cmd_ptr);
		ERR_IF ( retcode, "~SybCli:cs_cmd_drop ");
		cmd_ptr = 0;
	}

	if ( has_config ) delete gcfg;
	if ( isPoineer )
	{
		/* close and cleanup connection to the server */
		retcode = ct_exit (cntx_ptr, CS_FORCE_EXIT);

		/* drop the context */
		retcode = cs_ctx_drop (cntx_ptr) ;
		ERR_IF ( retcode, "~SybCli:cs_drop ");
		cntx_ptr = 0;

	} else if ( !gcfg->shared_session)
	{
		logout();
		delete isTalking;
		retcode = ct_con_drop(conn_ptr);
		ERR_IF ( retcode, "~SybCli:ct_con_drop ");
		conn_ptr = 0;
	}
#undef ERR_IF
}

Amor* SybCli::clone()
{
	SybCli *child = new SybCli();
#define INH(X) child->X = X;
	INH( cntx_ptr);
	INH( gcfg);
	
	child->ready_alloc();
	if ( !gcfg ) goto END;

	if ( gcfg->shared_session)
	{
		INH(conn_ptr);
		INH(isTalking);
	} else {
	}

END:
	return (Amor*)child;
}

TINLINE void SybCli::handle_pac(bool isDBFetch)
{
	unsigned int i;
	bool err_before_ret=false;	//假定在handle_result之前没有错

	CS_RETCODE	retcode=CS_SUCCEED ;
	cRowsObtained = 0;
	PUT_FSND (errCode_field, &retcode, sizeof(retcode));	 //首先假定结果OK，把值设到返回域中
	if ( isDBFetch)
	{
		fetch_data ( CS_ROW_RESULT ) ;	//取结果集的
		return ;
	}

	if ( last_pro == DBFace::QUERY && !hcmd_idle ) 	//凡是最近QUERY的, 要cancel
	{
		retcode = ct_cancel(NULL, cmd_ptr, CS_CANCEL_ALL);
		if ( retcode == CS_SUCCEED )
			hcmd_idle = true;
	}
	last_pro = face->pro;

	switch ( face->pro )
	{
	case DBFace::DBPROC:
	case DBFace::FUNC:
		WBUG("handle DBPROC/FUNC (%s) para num %d", face->sentence, face->num);
 		retcode = ct_command ( cmd_ptr, CS_RPC_CMD, (CS_CHAR *)face->sentence, CS_NULLTERM, CS_NO_RECOMPILE) ;
		if ( hasError(retcode, &err_before_ret) )
			goto END_HANDLE;

		hcmd_idle= false;
		/* 输入输出值的设定 */
		for ( i = 0; i < face->num && i < gcfg->param_max; i++ )
		{
			unsigned char *buf = (unsigned char*) 0;
			unsigned long blen = 0;
			DBFace::Para &para = face->paras[i];
			int rNo = para.fld + face->offset;
	
			assert(para.pos == i );
			memset(&bndfmt[i],0,sizeof(CS_DATAFMT));
			if ( (bndfmt[i].datatype =getdty(para.type)) == CS_ILLEGAL_TYPE || !para.name)
			{
				WLOG(ERR,"error in getdty(%d), para.type %d, para.name is %s", i, para.type, para.name);
				err_before_ret = true;
				goto END_HANDLE;
			}

			strcpy(bndfmt[i].name,para.name);
			bndfmt[i].namelen = CS_NULLTERM ;
			bndfmt[i].scale = para.scale;
			bndfmt[i].precision = para.precision;
			bndfmt[i].locale = NULL ;

			/* ct_param */
			retcode = CS_SUCCEED;	/* 先假定成功, 这样对PARA_INOUT的, 就可以放过去 */
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				buf = rcv_pac->fld[rNo].val;
				blen = rcv_pac->fld[rNo].range;
				bndfmt[i].maxlength = CS_UNUSED;
				bndfmt[i].status = CS_INPUTVALUE ;
				WBUG("in parameter %s", para.name);
				retcode = ct_param ( cmd_ptr, &bndfmt[i], (CS_VOID *)buf, blen, CS_UNUSED);
				break;

			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				bndfmt[i].maxlength = para.outlen;
				bndfmt[i].status = CS_RETURN ;
				WBUG("out parameter %s", para.name);
				retcode = ct_param ( cmd_ptr, &bndfmt[i], (CS_VOID *)0,  0, -1);
				break;

			default:
				break;
			}

			if ( hasError(retcode, &err_before_ret) )
			{
				WLOG(ERR,"error in ct_param(%d), para name is %s", i, para.name);
				goto END_HANDLE;
			}
		}

		/* 执行 */
		retcode = ct_send ( cmd_ptr );
		if ( hasError(retcode, &err_before_ret) )
		{
			WLOG(ERR,"error in ct_send (%s)", face->sentence);
			goto END_HANDLE;
		}
		WBUG("ct_send rpc seccessfully!");
		handle_result();

		break;

	case DBFace::DML:
		WBUG("handle DML (\"%s\") param num %d", face->sentence, face->num);
		goto NOWSQL;
	case DBFace::QUERY:
		WBUG("handle QUERY (\"%s\") param num %d", face->sentence, face->num);
	NOWSQL:
 		retcode = ct_command(cmd_ptr, CS_LANG_CMD, (CS_CHAR *)face->sentence, CS_NULLTERM,CS_UNUSED) ;
		if ( hasError(retcode, &err_before_ret) )
			goto END_HANDLE;
		hcmd_idle = false;
		/* 输入输出值的设定 */
		for ( i = 0; i < face->num && i < gcfg->param_max; i++ )
		{
			unsigned char *buf = (unsigned char*) 0;
			unsigned long blen = 0;
			DBFace::Para &para = face->paras[i];
			int rNo = para.fld + face->offset;
	
			assert(para.pos == i );
			memset(&bndfmt[i],0,sizeof(CS_DATAFMT));
			if ( (bndfmt[i].datatype =getdty(para.type)) == CS_ILLEGAL_TYPE || !para.name)
			{
				WLOG(ERR,"error in getdty(%d), para.type %d, para.name is %s", i, para.type, para.name == 0 ? "(null)":para.name );
				err_before_ret = true;
				goto END_HANDLE;
			}

			strcpy(bndfmt[i].name,para.name);
			bndfmt[i].namelen = CS_NULLTERM ;
			bndfmt[i].scale = para.scale;
			bndfmt[i].precision = para.precision;
			bndfmt[i].locale = NULL ;

			/* ct_param */
			retcode = CS_SUCCEED;	/* 先假定成功, 这样对PARA_INOUT的, 就可以放过去 */
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				buf = rcv_pac->fld[rNo].val;
				blen = rcv_pac->fld[rNo].range;
				bndfmt[i].maxlength = CS_UNUSED;
				bndfmt[i].status = CS_INPUTVALUE ;
				WBUG("in parameter %s", para.name);
				retcode = ct_param ( cmd_ptr, &bndfmt[i], (CS_VOID *)buf, blen, CS_UNUSED);
				break;

			default:
				break;
			}

			if ( hasError(retcode, &err_before_ret) )
			{
				WLOG(ERR,"error in ct_param(%d), para name is %s", i, para.name);
				goto END_HANDLE;
			}
		}

		/* 执行 */
		retcode = ct_send ( cmd_ptr );
		if ( hasError(retcode, &err_before_ret) )
		{
			WLOG(ERR,"error in ct_send (%s)", face->sentence);
			goto END_HANDLE;
		}
		WBUG("ct_send dml/query seccessfully!");
		handle_result();

		break;

	default:
		break;
	}
END_HANDLE:
	if ( err_before_ret)
		aptus->sponte(&dopac_pius);
}

void SybCli::copyRowValue()
{
	unsigned int i ;
	struct FieldObj *fobj;
	for ( i  = 0 ; i < gcfg->ret_col_max  ; i++)
	{
		if ( fldNo_arr[i] < 0 ) break;
		fobj = &rowset_pac.fld[fldNo_arr[i]];
		if ( fobj->no >= 0 )
		{
			snd_pac->input(fobj->no, fobj->val, fobj->range);
		}
	}
}

void SybCli::copyRowSet(int rowi)
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

	for ( i  = 0 ; i < gcfg->ret_col_max  ; i++)
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

void SybCli::logout()
{
	CS_RETCODE	retcode ;
#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); return; }
	if ( *isTalking )
	{
		retcode = ct_close(conn_ptr, CS_FORCE_CLOSE);
		ERR_IF ( retcode, "logout:ct_close ");
		*isTalking = false;
	}
#undef ERR_IF
	return ; 
}

void SybCli::logon()
{
	CS_RETCODE	retcode ;
#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); return; }

	*isTalking = false;

	/* connect to the server */
	retcode = ct_connect ( conn_ptr, gcfg->server_name, CS_NULLTERM ) ;
	ERR_IF ( retcode, "logon: ct_connect ") ;

	*isTalking = true;

#undef ERR_IF
	WBUG("logon to a server successfully!");
}

void SybCli::myalloc()
{
	CS_RETCODE	retcode ;
	OBJList *obj;
#define ERR_IF(a,b) if (a != CS_SUCCEED) { WLOG(ERR,"error in %s\n",b); return; }
	isTalking = new bool;
	*isTalking = false;

	/* Allocate a connection pointer */
	retcode = ct_con_alloc (cntx_ptr, &conn_ptr );
	ERR_IF ( retcode, "myalloc: ct_con_alloc ");
	obj = new OBJList();
	obj->connection = conn_ptr;
	obj->client = this;
	objlist.put(obj);
	

	/* Set the username and password properties */
	retcode = ct_con_props ( conn_ptr, CS_SET, CS_USERNAME, gcfg->user_name,
				CS_NULLTERM, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: username ");

	retcode = ct_con_props ( conn_ptr, CS_SET, CS_PASSWORD, gcfg->password,
				CS_NULLTERM, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: password"); 

	retcode = ct_con_props ( conn_ptr, CS_SET, CS_SEC_CHALLENGE, (CS_VOID *)&gcfg->sec_challenge,
				CS_UNUSED, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: sec_challenge"); 

	retcode = ct_con_props ( conn_ptr, CS_SET, CS_SEC_ENCRYPTION,(CS_VOID *)&gcfg->sec_encryption,
				CS_UNUSED, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: sec_encryption"); 

	retcode = ct_con_props ( conn_ptr, CS_SET, CS_SEC_NEGOTIATE, (CS_VOID *)&gcfg->sec_negotiate,
				CS_UNUSED, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: sec_negotiate"); 

	retcode = ct_con_props ( conn_ptr, CS_SET, CS_SEC_APPDEFINED,(CS_VOID *)&gcfg->sec_appdefined,
				CS_UNUSED, NULL );
	ERR_IF ( retcode, "myalloc: ct_con_props: sec_appdefined"); 
#undef ERR_IF
}

TINLINE CS_INT SybCli::getdty(DBFace::DataType type)
{
	CS_INT dty;
	switch ( type )
	{
	case DBFace::Integer:
		dty = CS_INT_TYPE;
		break;

	case DBFace::SmallInt:
		dty = CS_SMALLINT_TYPE;
		break;

	case DBFace::TinyInt:
		dty = CS_TINYINT_TYPE;
		break;

	case DBFace::Char:
		dty = CS_CHAR_TYPE;
		break;

	case DBFace::String:
		dty = CS_VARCHAR_TYPE;
		break;

	case DBFace::Decimal:
		dty = CS_DECIMAL_TYPE;
		break;

	case DBFace::Numeric:
		dty = CS_NUMERIC_TYPE;
		break;

	case DBFace::Currency:
		dty = CS_MONEY_TYPE;
		break;

	case DBFace::Double:
		dty = CS_FLOAT_TYPE;
		break;

	case DBFace::VarBinary:
		dty = CS_VARBINARY_TYPE;
		break;

	case DBFace::Long:
		dty = CS_LONG_TYPE;
		break;

	case DBFace::Text:
		dty = CS_TEXT_TYPE;
		break;

	case DBFace::Binary:
		dty = CS_BINARY_TYPE;
		break;

	case DBFace::Date:
		dty = CS_DATETIME_TYPE;
		break;

	case DBFace::LongBinary:
		dty = CS_LONGBINARY_TYPE;
		break;

	case DBFace::Boolean:
		dty = CS_BIT_TYPE;
		break;

	default:
		WLOG(CRIT, "Unknown data type %d!", type);
		dty = CS_ILLEGAL_TYPE;
	}
	return dty;
}

long SybCli::getRowsetSize()
{
	long position=0;

	WBUG("the Rowset size is %ld", position);
	return position;
}

/* 返回ct_results的状态, 如果到了CS_END_RESULTS, 则不再继续*/
TINLINE CS_RETCODE SybCli::handle_result( bool has_back )
{
	CS_RETCODE	results_ok, retcode ;
	CS_INT		result_type ;
	int count;
	int many =0;	/* 0： 没有返回任何数据, 正常
      			>0:  返回了数据
      			<0:  发生错误 */

	/* Process all returned result types */
	while ((results_ok = ct_results( cmd_ptr, &result_type)) == CS_SUCCEED )
	{
		switch (result_type)
		{
		case CS_ROW_RESULT:
			WBUG("ct_results: CS_ROW_RESULT");
			if ( !hasError(bind_columns ( result_type )) )
			{
				many = 1; /* 不用再管返回数据了*/
				results_ok = fetch_data ( result_type ) ;
				goto END;
			} else {
				many = -1;	
			}
			break ;

		case CS_CMD_SUCCEED:	
			WBUG("ct_results: CS_CMD_SUCCEED");
			break ;

		case CS_CMD_DONE:
			WBUG("ct_results: CS_CMD_DONE");
			retcode = ct_res_info(cmd_ptr, CS_ROW_COUNT, &count, CS_UNUSED, NULL);
			if ( hasError(retcode) )
			{
				WLOG(ERR,"handle_result: ct_res_info(CS_ROW_COUNT) failed.");
			} else {
				WBUG("get CS_ROW_COUNT is %d", count);
				PUT_FSND (cRows_field, &count, sizeof(count));
			}
			break ;

		case CS_CMD_FAIL:
			WBUG("ct_results: CS_CMD_FAIL");
			hasError(result_type);
			many = -1;
			break ;

		case CS_STATUS_RESULT:
			WBUG("ct_results: CS_STATUS_RESULT");
			goto NOW_PARAM;

		case CS_PARAM_RESULT:
			WBUG("ct_results: CS_PARAM_RESULT");
		NOW_PARAM:
			if ( !hasError(bind_columns ( result_type )))
			{
				results_ok = fetch_data ( result_type );
				many =1;	/* 调用者不用再管返回了 */
			} else {
				many = -1;	
			}
			break ;

		case CS_COMPUTE_RESULT:
			WBUG("ct_results: CS_COMPUTE_RESULT");
			break ;

		default:
			WBUG("ct_results: unknown results %d", (int)result_type);
			break ;
		}
		
		if ( results_ok != CS_SUCCEED ) break;	/* 这一条不是多余, 因为本函数调用了fetch_data, 其中再调用本函数, 
								这样, ct_results 就可能发生改变, 所以这里再判断一下。以避免错误信息.*/
	}

	if ( results_ok != CS_END_RESULTS )
	{
		WBUG("ct_results: !CS_END_RESULTS");
		hasError ( results_ok,0);
	} else {
		WBUG("ct_results: CS_END_RESULTS");
	}
END:
	if ( many <=0 && has_back )		/* 还没有返回数据, 这里返回一下 */
		aptus->sponte(&dopac_pius);
	return results_ok;
}

TINLINE CS_RETCODE SybCli::bind_columns ( int res_type )
{
	CS_RETCODE	retcode;
	CS_INT		num_cols;
	unsigned int i,k, m_chunk = 0;

	unsigned oTotal;
	unsigned int outNum, defNum;	/* 输出参数个数 */
	int para_offset;

	para_offset = 0;	/* 一般为0, 对于ROW_RESULT, 再作另外定义  */
	retcode = ct_res_info(cmd_ptr, CS_NUMDATA, &num_cols, CS_UNUSED, NULL);
	if (retcode != CS_SUCCEED)
	{
		WLOG(ERR,"bind_columns: ct_res_info() failed.");
		return retcode;
	}
	WBUG("bind_columns: the num_cols is %d", (int)num_cols);
	if (num_cols <= 0)
	{
		WBUG("bind_columns: ct_res_info() returned zero columns.");
		return CS_SUCCEED;
	}

	/* 计算一下总输出长度, 使得snd_pac空间固定 */
	switch ( res_type )
	{
	case CS_STATUS_RESULT:
		snd_pac->grant(8);	/* 顶多8个字节就够了 */
		break;

	case CS_PARAM_RESULT:
		oTotal = face->outSize, outNum = face->outNum;
		if ( outNum > gcfg->param_max )
			WLOG(WARNING, "out parameter exceeds %d", gcfg->param_max);
		snd_pac->grant(oTotal);	/* 扩张出足够的空间 */
		break;

	case CS_ROW_RESULT:
		/* 计算一下总输出长度, 使得rowset_pac空间固定 */
		para_offset = face->rowset == 0 ? 0 : face->rowset->para_pos ;
		oTotal = face->outSize, outNum = face->outNum;
		if ( outNum > gcfg->ret_col_max )
			WLOG(WARNING, "out columns exceeds %d", gcfg->ret_col_max);

		rowset_pac.reset();
		if( face->rowset ) 
			m_chunk = face->rowset->chunk;
		else
			m_chunk = 1;
		rowset_pac.grant(oTotal*m_chunk);	/* 扩张出足够的空间, m_chunk下面还要用到 */
		memset(fldNo_arr, 0xff, (gcfg->ret_col_max)*sizeof(int));
		if ( m_chunk * outNum > rlen_sz )
		{
			rlen_sz = m_chunk * outNum;
			if ( rlenp ) delete [] rlenp;
			rlenp = new CS_INT [rlen_sz];
			memset(rlenp, 0, rlen_sz*sizeof(CS_INT));
		}

		break;

	default:
		break;
	}

	retcode = CS_SUCCEED ;	/* 假定结果成功 */
	defNum = 0;		/* 尚无有效输出列 */
	for (i = 0; i < (unsigned int)num_cols && i < gcfg->ret_col_max; i++)
	{
		unsigned char *buf = (unsigned char*) 0;
		CS_INT nlen = 0;
		int rNo ;
		DBFace::Para *para = 0;

		/** Get the column description.  ct_describe() fills the
			datafmt parameter with a description of the column.
		*/
		retcode = ct_describe(cmd_ptr, (i + 1), &retfmt[i]);
		if (retcode != CS_SUCCEED)
		{
			WLOG(ERR,"bind_columns: ct_describe() failed");
			break;
		}
		WBUG("output parameter is \"%s\", type is %d", retfmt[i].name, (int)retfmt[i].datatype);

		nlen =  retfmt[i].namelen ;  

		/* 寻找此变量与定义中相应的行(根据变量名) */
		for ( k = para_offset; k < face->num ; k++ )
		{
			para = &(face->paras[k]);
			assert(para->pos == k );
			/* 非输出变量者略过 */
			if ( para->inout != DBFace::PARA_OUT || para->namelen != nlen) continue;

			/* 匹配变量名 */
			if ( ( (res_type == CS_PARAM_RESULT || res_type == CS_ROW_RESULT)
				&& memcmp( retfmt[i].name, para->name, nlen) == 0 )
		    		|| (res_type == CS_STATUS_RESULT 
				&& memcmp (STR_STATUS_RESULT, para->name, nlen) == 0 ) )
			{	
 				/* 找到一行有定义 */
				WBUG("matched definition line %d, para name \"%s\", type %d",k, para->name, para->type);
				break;
			}
		}
		
		if ( k >= face->num ) continue;	 /* 没有对应的输出参数定义, 则处理下一列 */
		
		/* 至此, para是对应的参数定义了  */
		/* update the datafmt structure to indicate that we want */
		if ( para->type != DBFace::UNKNOWN_TYPE )
		{
			CS_INT dtype; 
			dtype = getdty(para->type);
			if (  dtype == CS_ILLEGAL_TYPE )
			{
				WLOG(ERR, "error in getdty(%d), para.type %d, para.name is %s", i, para->type, para->name);
				continue;		/* 数据类型不合法, 处理下一列 */
			}
			retfmt[i].datatype = dtype;
			retfmt[i].maxlength = para->outlen;
			retfmt[i].scale = para->scale;
			retfmt[i].precision = para->precision;
		}

		/* 判定结束, 至此, 这一列为可取的数据 */
		defNum++;	/* 有效的需要输出的列, 此值最小为1 */
		retfmt[i].count = m_chunk;		/* 一次取多少行, 对于CS_STATUS_RESULT和CS_PARAM_RESULT, m_chunk为0, 而对于CS_ROW_RESULT, m_chunk至少为1 */
		retfmt[i].format = CS_FMT_UNUSED;   /* 不补任何字符, 象CS_CHAR, 就不以NULL结尾了*/

		/* Now bind. */
		rNo = para->fld + face->offset;
		if (  res_type == CS_ROW_RESULT )
		{
			/* 如果ROW_RESULT, 则要使用rowset_pac */
			unsigned int blen = 0;
			buf = rowset_pac.buf.point;
			WBUG("rowset_pac rNo %d, outlen %ld", rNo, para->outlen);
			blen = para->outlen*m_chunk;
			rowset_pac.commit(rNo, blen);
			memset(buf, 0, blen);
			//retfmt[i].maxlength = blen ; /* 真的要这个总最大长度? */
			retcode = ct_bind(cmd_ptr, (i + 1), &retfmt[i], buf, (CS_INT*)&(rlenp[(defNum-1)*m_chunk]), 0);
			fldNo_arr[(defNum-1)] = rNo;
		} else {
			buf = snd_pac->buf.point;
			snd_pac->commit(rNo, para->outlen);
			memset(buf, 0, para->outlen);
			retcode = ct_bind(cmd_ptr, (i + 1), &retfmt[i], buf, (CS_INT*)&(snd_pac->fld[rNo].range), 0);
		}

		if (retcode != CS_SUCCEED)
		{	/* bind failed, 不再处理 */
			WLOG(ERR, "error in ct_bind(%d), para.type %d, para.name is %s", i, para->type, para->name);
			break;
		}
	}

	WBUG("bind_columns: ct_bind %s (retcode=%ld).", retcode == CS_SUCCEED ? "succeeded" : "failed", retcode);
	return retcode;
}

/* 
retnum 0:数据已经取完, 继续ct_results, 对于res_type 为 CS_STATUS_RESULT 和 CS_PARAM_RESULT, 则必然为0
    -1:发生错误,
    >0: 可能还有数据, 需要再一次fetch_data
返回的results_ok 是handle_result中的ct_results的返回值。 handle_returns中调用本函数时, 可判断ct_results到了什么状态。
*/
TINLINE CS_RETCODE SybCli::fetch_data( int res_type )
{
	CS_RETCODE	retcode = CS_SUCCEED, results_ok=CS_SUCCEED;
	CS_INT		rows_read;
	int		retnum = -1;
	unsigned int m_chunk = 0;

	switch ( res_type )
	{
	case CS_STATUS_RESULT:
	case CS_PARAM_RESULT:
		while (((retcode = ct_fetch(cmd_ptr, CS_UNUSED, CS_UNUSED, CS_UNUSED,
			&rows_read)) == CS_SUCCEED) || (retcode == CS_ROW_FAIL))
		{
			/* Check if we hit a recoverable error. */
			if (retcode == CS_ROW_FAIL)
			{
				WBUG("fetch_data: ct_fetch() encount CS_ROW_FAIL ");
				continue;
			} else {	//取结果成功
				WBUG("fetch_data: read %s parameter success", res_type == CS_PARAM_RESULT ? "parameter" : "status");
				cRowsObtained = rows_read;
				PUT_FSND(cRowsObt_fld, &cRowsObtained, sizeof(cRowsObtained));
			}
		} /* End of while for ct_fetch all row, 实际上就取了一条 */

		if ( retcode != CS_END_DATA )	/* 加入错误码 */
			hasError(retcode);
		
		if ( res_type == CS_PARAM_RESULT )	//如果是参数，就向左边返回数据
			aptus->sponte(&dopac_pius);	

		if ( retcode == CS_END_DATA )
		{
        		/*  Everything went fine.	*/
			WBUG("fetch_data:All done processing rows.");
			results_ok = handle_result(false);	//从这里调用, 不需要再返回数据了
		}
		break;

	case CS_ROW_RESULT:
		if( face->rowset ) 
			m_chunk = face->rowset->chunk;
		else
			m_chunk = 1;
		cRowsObtained = 0;
		PUT_FSND(cRowsObt_fld, &cRowsObtained, sizeof(cRowsObtained));
		retcode = ct_fetch(cmd_ptr, CS_UNUSED, CS_UNUSED, CS_UNUSED, &cRowsObtained);
		/* 设置已获得的记录数 */
		PUT_FSND(cRowsObt_fld, &cRowsObtained, sizeof(cRowsObtained));
		WBUG("fetch_data: read %ld line%s",  cRowsObtained, cRowsObtained > 1 ? "s" :" ");

		if (retcode == CS_ROW_FAIL)
		{
			WLOG(ERR, "fetch_data: ct_fetch() encount CS_ROW_FAIL.");
			retnum = 1; /* 可能还有数据 */
		} else if ( retcode ==  CS_SUCCEED)
		{
			if (cRowsObtained < (CS_INT) m_chunk )	/* 使其CS_END_DATA */
			{
				retcode = ct_fetch(cmd_ptr, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);
				retnum = 0;
			} else 
				retnum = 1;
		}
		hasError(retcode);

		if ( retcode == CS_END_DATA )
		{
        		/*  Everything went fine.	*/
			WBUG("fetch_data(CS_END_DATA):All done processing rows.");
			results_ok = handle_result(false);
		}

		for (int i = 0 ; i < cRowsObtained; i++ )
		{	/* 取得每条记录 */
			copyRowSet(i);
			aptus->sponte(&dopac_pius);	//每条向左返回一下数据
		}

		if ( face->rowset  && face->rowset->useEnd )
			aptus->sponte(&end_ps);	/* MULTI_UNIPAC_END */
		else if ( cRowsObtained <=0 )	/* 没有行返回, 也没有结束标记, 则也返回响应 */
			aptus->sponte(&dopac_pius);

		break;
	default:
		break;
	}
	/*
	** We're done processing rows.  Let's check the final return
	** value of ct_fetch().
	*/
	switch (retcode)
	{
	case CS_SUCCEED:
	case CS_ROW_FAIL:	/* 前面已经处理过了,这里不再 */
	case CS_END_DATA:
		break;

	case CS_FAIL:
		/*
		** Something terrible happened.
		*/
		retnum = -1;
		WBUG("fetch_data: ct_fetch() encount CS_FAIL.");
                break;

	default:
		/*
		** We got an unexpected return value.
		*/
		retnum = -1;
		WBUG("fetch_data: ct_fetch() encount an expected retcode %d", (int)retcode);
                break;
	}

	return results_ok;
}


bool SybCli::hasError(CS_RETCODE retcode, bool *isErr)
{
	bool ret = true; 
	switch (retcode)
	{
	case CS_SUCCEED:
	case CS_END_DATA:
		ret = false;
		break;

	case CS_FAIL:
		WLOG(ERR, "retcode Error=CS_FAIL");
		PUT_FSND_STR(errStr_field, "CS_FAIL");
		break;

	case CS_CMD_FAIL:
		WLOG(ERR, "retcode Error=CS_CMD_FAIL");
		PUT_FSND_STR(errStr_field, "CS_CMD_FAIL");
		break;

	default:
		WLOG(ERR, "retcode Error=%ld", retcode);
		PUT_FSND_STR(errStr_field, "UNKONW ERROR");
		break;
	}
	if ( ret )
		PUT_FSND (errCode_field, &retcode, sizeof(retcode));
	if ( isErr )
		*isErr = ret;
	return ret;
}

void SybCli::ready_alloc()
{
	if ( !gcfg ) return;
	if ( !bndfmt) bndfmt = new CS_DATAFMT[gcfg->param_max*sizeof(CS_DATAFMT)];
	if ( !retfmt) retfmt = new CS_DATAFMT[gcfg->ret_col_max*sizeof(CS_DATAFMT)];
	rowset_pac.produce(gcfg->ret_col_max);
	if ( !fldNo_arr) fldNo_arr = new int [gcfg->ret_col_max];
	memset(fldNo_arr, 0xff, (gcfg->ret_col_max)*sizeof(int));
}
#include "hook.c"
