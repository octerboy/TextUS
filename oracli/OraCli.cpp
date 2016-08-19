/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Oracle Connector
 Build: created by octerboy, 2006/10/30, Guangzhou
 $Header: /textus/oracli/OraCli.cpp 36    12-04-04 16:59 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: OraCli.cpp $"
#define TEXTUS_MODTIME  "$Date: 12-04-04 16:59 $"
#define TEXTUS_BUILDNO  "$Revision: 36 $"
/* $NoKeywords: $ */

#include <assert.h>
#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "textus_string.h"
#include "DBFace.h"
#include "PacData.h"
#if defined( USE_DYNAMIC_ORACLE )
#include "ora_api.h"
#include "textus_load_mod.h"
#else
#include "oci.h"
#endif
#if !defined(_WIN32)
#include <unistd.h>
#endif
#define INLINE inline
#define BUFF_SIZE 256
class OraCli: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	OraCli();
	~OraCli();
#include "wlog.h"
	Amor::Pius dopac_pius, end_ps;  //仅用于向左边传回数据
	bool inThread;
	const char *so_file;
	
	INLINE int handle();
	INLINE int fetch(ub4 many, bool just=false);
	INLINE void copyRowValue();
	INLINE void copyRowSet(int);
#define ROWSET_LEFT	0
#define ROWS_OBTAINED	1

	INLINE void logon();
	INLINE void logout();
	INLINE void myalloc();
	INLINE bool hasError(sword status);
	INLINE ub2 getdty(DBFace::DataType type);
	INLINE long getRowsetSize();
	INLINE void setRowsCount();

	char conn_str[BUFF_SIZE];
	const char *username, *password;

	OCIEnv *envhp;		/* 所有对象共享的环境 */
	OCIError *errhp;
	bool isPoineer;

	OCISvcCtx *svchp;	/* 会话参数 */
	OCISession *authp;
	OCIServer *srvhp;

	bool shared_session;	/* 是否共享同一个会话 */
	bool *isTalking;	/* 一个会话是否进行 */
	
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	PacketObj rowset_pac;	/* 存放查询行集中的一行 */
	DBFace *face;

	OCIStmt *stmthp;
	
	long	cRowsObtained;	/* 结果集返回记录数 */
	long	cRowsLeft;	/* 行集中还有多少记录未取 */
#define STMT_MAX	1024
#define PARA_MAX 	256
	char proc_stmt[STMT_MAX];
	OCIDefine *defnp[PARA_MAX];	/* the defining handles */
	OCIBind  *bndp[PARA_MAX];	/* the binding handle */

	ub2 *rlenp;
	ub2 rlen_sz;

#define PUT_FSND(FLD,BUF,LEN)	\
		if ( snd_pac && face ) 	\
			snd_pac->input(face->FLD, (unsigned char*)BUF, LEN);

#define PUT_FSND_STR(FLD,BUF) PUT_FSND(FLD,BUF,strlen(BUF))
};

#if defined( USE_DYNAMIC_ORACLE )
static bool isLoaded = false;
#endif
void OraCli::ignite(TiXmlElement *cfg) 
{
	bool inThread =true;
	const char *ip, *port, *svcname;
	char *default_port = "1521";
	TiXmlElement *con_ele;

	const char *comm_str;
	inThread =  ( (comm_str= cfg->Attribute("cocurrent")) && strcasecmp(comm_str, "yes") == 0 );
	shared_session =  ( (comm_str= cfg->Attribute("session")) && strcasecmp(comm_str, "shared") == 0 );
	so_file= cfg->Attribute("lib");

	con_ele = cfg->FirstChildElement("connect");
	if ( !con_ele ) 
		return;
	ip = con_ele->Attribute("ip");
	port = con_ele->Attribute("port");
	if (!port )
		port = default_port;
	svcname = con_ele->Attribute("service");
	memset(conn_str, 0 , BUFF_SIZE);
	TEXTUS_SNPRINTF(conn_str, BUFF_SIZE-1, "(DESCRIPTION=(ADDRESS_LIST=(ADDRESS=(PROTOCOL=TCP)(HOST=%s)(PORT=%s)))(CONNECT_DATA=(SERVICE_NAME=%s)))", ip,port,svcname);
//	TEXTUS_SNPRINTF(conn_str, BUFF_SIZE-1, "%s:%s/%s",ip,port,svcname);

	username = con_ele->Attribute("user");
	password = con_ele->Attribute("password");

	return;
}

#define CHECK_ENV()	\
	if (!isLoaded ) 	\
	{			\
		WLOG(INFO, "not load oci yet.");\
		break;		\
	}			\
	if ( !face )		\
	{			\
		WLOG(INFO, "no dbface yet.");	\
		break;				\
	}

bool OraCli::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	TMODULE ext=NULL;
	sword rc=0;
	int retnum = 0;

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		CHECK_ENV()

		if ( !(*isTalking)) logon();

		if ( (*isTalking))
			retnum = handle();

		WBUG("handle end");
		if ( retnum <=0 )
			aptus->sponte(&dopac_pius);
		break;

	case Notitia::CMD_DBFETCH:
		WBUG("facio CMD_DBFETCH");
		CHECK_ENV()

		if ( (*isTalking))
		{
			retnum = fetch(face->rowset == 0 ? 1 : face->rowset->chunk);
			if (retnum <=0 )
				aptus->sponte(&dopac_pius);
		} else {
			WLOG(INFO, "NO Talking");
		}
		break;

	case Notitia::CMD_SET_DBFACE:
		WBUG("facio CMD_SET_DBFACE");
		face = (DBFace *)pius->indic;
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
#if defined( USE_DYNAMIC_ORACLE )

		if ( isLoaded )
			goto WORK;

#define GETORAADDR(FUN, type) \
		FUN = 0;	\
		TEXTUS_GET_ADDR(ext, #FUN, FUN, type); \
		if ( !FUN ) WLOG(ERR, "Failed to find %s\n", #FUN);

		if (so_file && (ext =TEXTUS_LOAD_MOD( so_file, 0)) )
		{
#include "sym.c"
			isLoaded = true;
		} else {
			if ( !so_file)
			{
				WLOG(ERR, "Not set OCI library");
			} else {
				WLOG(ERR, "Failed to load the OCI library %s", TEXTUS_MOD_DLERROR);
			}
			goto LAST;
		}
#else
		isLoaded = true;	/* 静态的, 这里设置标志 */
#endif
WORK:
		rc = OCIEnvCreate ( &envhp, (inThread ? OCI_OBJECT | OCI_THREADED : OCI_OBJECT ) ,
			    (dvoid *)0, NULL, NULL, NULL, 0, (dvoid **) 0 );
		WBUG("OCIEnvCreate rc = %d", rc);
		if (hasError(rc))
			break;

  		(void) OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &errhp, OCI_HTYPE_ERROR,
                	(size_t) 0, (dvoid **) 0);
		myalloc();
		hasError(OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &stmthp,
			OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0));

		isPoineer = true;
LAST:
#if 0
	{
		unsigned char out[OCI_NLS_MAXBUFSZ ];
		size_t outLen;
		if ( OCINlsEnvironmentVariableGet )
		{
		if ( !hasError(OCINlsEnvironmentVariableGet ( out, OCI_NLS_MAXBUFSZ , OCI_NLS_CHARSET_ID , 0, &outLen)))
		{
		out[outLen] = 0;
		printf( "%02x %02x \n", out[0], out[1]);
		printf("len %d, NLS_LANG %s\n", outLen, out);
		exit(0);
		}
		}
	}
#endif
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		logout();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		if (!isLoaded )
			break;
		if ( !shared_session )
		{
			myalloc();
		}
		hasError(OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &stmthp,
			OCI_HTYPE_STMT, (size_t) 0, (dvoid **) 0));

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

	default:
		WBUG("facio Notitia::%lu", pius->ordo);
		return false;
	}

	return true;
}

bool OraCli::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_DBCONN:
		WBUG("facio CMD_SET_DBCONN");
		if ( pius->indic )
			TEXTUS_SNPRINTF(conn_str, BUFF_SIZE-1, "%s", (char*)pius->indic);
		break;
		
	default:
		return false;
	}
	return true;
}

OraCli::OraCli()
{
	so_file = 0;

	envhp = 0;
	errhp = 0;
	stmthp = 0;

	inThread = false;
	isPoineer = false;
	shared_session = false;
	username = password = 0;
	face = 0;
	memset(defnp, 0, PARA_MAX*sizeof(OCIDefine *));
	memset(bndp, 0, PARA_MAX*sizeof(OCIBind *));
	dopac_pius.ordo = Notitia::PRO_UNIPAC;
	dopac_pius.indic = 0;

	end_ps.ordo = Notitia::MULTI_UNIPAC_END;
	end_ps.indic = 0;
	
	rlenp = (ub2*) 0;
	rlen_sz = (ub2) 0;

	rowset_pac.produce(PARA_MAX);
}

OraCli::~OraCli() 
{ 
	logout();
	if ( stmthp)
	{
		(void) OCIHandleFree( (dvoid *)stmthp, OCI_HTYPE_STMT);
		stmthp = 0;
	}
	if ( isPoineer )
	{
		if ( envhp )
		{
			(void) OCIHandleFree((dvoid *) envhp, OCI_HTYPE_ENV); 
			envhp = 0;
		}
	} else if ( !shared_session)
	{
		(void) OCIHandleFree( (dvoid *) srvhp, OCI_HTYPE_SERVER);
		(void) OCIHandleFree( (dvoid *) svchp, OCI_HTYPE_SVCCTX);
		(void) OCIHandleFree( (dvoid *) authp, OCI_HTYPE_SESSION);
		srvhp = 0;
		svchp = 0;
		authp = 0;
	}
}

Amor* OraCli::clone()
{
	OraCli *child = new OraCli();
#define INH(X) child->X = X;
	INH(envhp);
	INH(errhp);
	INH(shared_session);

	if ( shared_session)
	{
		INH(svchp);
		INH(srvhp);
		INH(authp);
		INH(isTalking);
	} else {
		INH(username);
		INH(password);
	}
	memcpy(child->conn_str, conn_str, BUFF_SIZE);
	return (Amor*)child;
}

INLINE int OraCli::handle()
{
	int retnum, m_chunk;
	unsigned int slen = 0;
	unsigned oTotal;
	sword status;
	unsigned int i;
	unsigned int outNum;	/* 输出参数个数, 对于DBPROC、FUNC, 如果本身没有输出参数, 则NO_DATA不算错 */
	ub4 execute_mode;	/* 执行模式 */

	unsigned int defNum, bindNum;

	retnum = 0;
	if ( face->num < 0 )	/* 至少得定义状态值 */
		return retnum;

	cRowsObtained = 0;
	switch ( face->pro )
	{
	case DBFace::DBPROC:
	case DBFace::FUNC:
		WBUG("handle DBPROC/FUNC para num %d", face->num);
		switch ( face->pro )
		{
		case DBFace::DBPROC:
			slen = TEXTUS_SNPRINTF(proc_stmt, sizeof(proc_stmt), "Begin %s(", face->sentence);
			for ( i =1 ; i < face->num; i++)
			{
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, ":f%d,", i);
			}
			if ( face->num >= 1 )
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, ":f%d); END;", i);
			else
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, "); END;");
		
			WBUG("procedure statement: %s", proc_stmt);
			break;

		case DBFace::FUNC:
			
			if ( face->num >= 1 )
				slen = TEXTUS_SNPRINTF(proc_stmt, sizeof(proc_stmt), "Begin :fret := %s", face->sentence);
			else 
				slen = TEXTUS_SNPRINTF(proc_stmt, sizeof(proc_stmt), "Begin %s", face->sentence);
			if ( face->num >=2 )
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, "%s", "(");
			for ( i =2 ; i < face->num; i++)
			{
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, ":f%d,", i);
			}
			if ( face->num >= 2 )
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, ":f%d); END;", i);
			else 
				slen += TEXTUS_SNPRINTF(&proc_stmt[slen], sizeof(proc_stmt)-slen, "%s", "; END;");
		
			WBUG("function statement: %s", proc_stmt);
			break;

		default:
			break;
		}
		
		if ( hasError(OCIStmtPrepare(stmthp, errhp, (const OraText*)&proc_stmt[0], (ub4) slen,
			(ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT)) ) break;

		/* 计算一下总输出长度, 使得snd_pac空间固定 */
		oTotal = 0, outNum = 0;
		for ( i = 0; i < face->num && i < PARA_MAX; ++i )
		{	
			unsigned long blen = 0;
			DBFace::Para &para = face->paras[i];
			int rNo = para.fld + face->offset;
	
			assert(para.pos == i );
			switch ( para.inout )
			{
			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				blen = para.outlen;
				outNum++;
				break;

			case DBFace::PARA_INOUT:
				assert(rNo <= snd_pac->max );
				blen = para.outlen == 0 ? rcv_pac->fld[rNo].range : para.outlen ;
				outNum++;
				break;

			default:
				break;
			}
			if ( blen < 0 ) blen = 0;
			oTotal += blen;
		}
		snd_pac->grant(oTotal);	/* 扩张出足够的空间 */

		/* 输入输出值的绑定 */
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			unsigned char *buf = (unsigned char*) 0;
			unsigned long blen = 0;
			ub2 dty;
			ub2 *alenp = (ub2*) 0; /* actual length  */
			DBFace::Para &para = face->paras[i];
			int rNo = para.fld + face->offset;
	
			assert(para.pos == i );
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				buf = rcv_pac->fld[rNo].val;
				blen = rcv_pac->fld[rNo].range;
				alenp = (ub2*) &(rcv_pac->fld[rNo].range);	/* 这是有问题的, 从一个ulong 到 ushort, 
										对于高位在前，低位在后的机器会有问题 */
				WBUG("in parameter");
				break;

			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				blen = para.outlen;
				//snd_pac->grant(blen);
				buf = snd_pac->buf.point;
				WBUG("out snd_pac commit");
				snd_pac->commit(rNo, blen);
				memset(buf, 0, blen);
				alenp = (ub2*) &(snd_pac->fld[rNo].range); /* 这是有问题的, 从一个ulong 到 ushort, 
										对于高位在前，低位在后的机器会有问题 */
				break;

			case DBFace::PARA_INOUT:
				assert(rNo <= snd_pac->max );
				blen = para.outlen == 0 ? rcv_pac->fld[rNo].range : para.outlen ;
				WBUG("inout snd_pac input blen %ld, fld[%d].range %ld, outlen %ld", blen, rNo, rcv_pac->fld[rNo].range, para.outlen);
				snd_pac->input(rNo, rcv_pac->fld[rNo].val, blen);
				buf = snd_pac->fld[rNo].val;
				alenp = (ub2*) &(snd_pac->fld[rNo].range); /* 这是有问题的, 从一个ulong 到 ushort, 
										对于高位在前，低位在后的机器会有问题 */
				break;

			default:
				break;
			}

			if ( !(dty =getdty(para.type)) )
				goto ENDDBPROC;

			if ( hasError(OCIBindByPos(stmthp, &bndp[i], errhp, i+1, (dvoid *) buf, blen, dty, 
				(dvoid *) 0, alenp, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT)))
				break;
		}

		/* 执行 */
		execute_mode = (ub4) OCI_COMMIT_ON_SUCCESS;

		WBUG("executing proc/func ..... ");
		if ((status = OCIStmtExecute(svchp, stmthp, errhp, (ub4) 1, (ub4) 0,
               		(CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, execute_mode)))
		{
			if ( !(outNum ==0 && status == OCI_NO_DATA ) )	/* 本身无输出参数, 当然 NO_DATA */
				hasError(status);
			else
				setRowsCount();
		} else
			setRowsCount();

		/* cleanup  */
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			if(bndp[i])
			{
			/*
			sb4	count;
				if (!hasError( OCIAttrGet ( bndp[i], OCI_HTYPE_BIND, &count, 0, OCI_ATTR_MAXDATA_SIZE, errhp)) )
					printf("return bytes %d\n", count);
			*/
				(void) OCIHandleFree((dvoid *) bndp[i], OCI_HTYPE_BIND);
				bndp[i] = 0;
			} else
				break;
		}

	ENDDBPROC:	
		break;

	case DBFace::QUERY:
		WBUG("handle QUERY para num %d", face->num);
		slen = TEXTUS_SNPRINTF(proc_stmt, sizeof(proc_stmt), "%s", face->sentence);
		WBUG("query statement: %s", proc_stmt);

		/* cleanup  */
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			if(bndp[i])
			{
				(void) OCIHandleFree((dvoid *) bndp[i], OCI_HTYPE_BIND);
				bndp[i] = 0;
			}

			if(defnp[i])
			{
				(void) OCIHandleFree((dvoid *) defnp[i], OCI_HTYPE_DEFINE);
				defnp[i] = 0;
			}
		}

		if ( hasError(OCIStmtPrepare(stmthp, errhp, (const OraText*)&proc_stmt[0], (ub4) slen,
			(ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT)) ) break;

		/* 计算一下总输出长度, 使得rowset_pac空间固定 */
		oTotal = face->outSize, outNum = face->outNum;
		if ( outNum > PARA_MAX )
			WLOG(WARNING, "out parameter exceeds %d", PARA_MAX);

		rowset_pac.reset();
		if( face->rowset ) 
			m_chunk = face->rowset->chunk;
		else
			m_chunk = 1;
		rowset_pac.grant(oTotal*m_chunk);	/* 扩张出足够的空间 */
		if ( m_chunk * outNum > rlen_sz )
		{
			rlen_sz = m_chunk * outNum;
			if ( rlenp ) delete [] rlenp;
			rlenp = new ub2 [rlen_sz];
			memset(rlenp, 0, rlen_sz*sizeof(ub2));
		}

		/* 输入输出值的绑定 */
		defNum = 0; bindNum = 0;
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			unsigned char *buf = (unsigned char*) 0;
			unsigned int blen = 0;
			ub2 dty;
			ub2 *alenp; /* actual length  */
			DBFace::Para &para = face->paras[i];
			long rNo = para.fld + face->offset;
	
			assert(para.pos == (unsigned int)i );
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				buf = rcv_pac->fld[rNo].val;
				blen = rcv_pac->fld[rNo].range;
				alenp = (ub2*) &(rcv_pac->fld[rNo].range); /* 这是有问题的, 从一个ulong 到 ushort, 
										对于高位在前，低位在后的机器会有问题 */

				bindNum++;
				break;

			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				blen = para.outlen;
				buf = rowset_pac.buf.point;
				WBUG("rowset_pac rNo %ld, blen %d", rNo, blen);
				rowset_pac.commit(rNo, blen*m_chunk);
				memset(buf, 0, blen*m_chunk);

				defNum++;
				break;

			default:
				break;
			}

			if ( !(dty =getdty(para.type)) )
				goto ENDQUERY;

			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				if ( hasError(OCIBindByPos(stmthp, &bndp[i], errhp, bindNum, (dvoid *) buf, blen, dty, 
					(dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT)))
					goto ENDQUERY;
				break;

			case DBFace::PARA_OUT:
				alenp = &rlenp[(defNum-1)*m_chunk];
				if ( hasError(OCIDefineByPos(stmthp, &defnp[i], errhp, defNum, (dvoid *) buf, blen, dty, 
					(dvoid *) 0, alenp, (ub2 *) 0, OCI_DEFAULT)))
					goto ENDQUERY;
				break;

			default:
				break;
			}
		}

		/* 执行 */
		execute_mode = (ub4) OCI_STMT_SCROLLABLE_READONLY;
		//execute_mode = (ub4) OCI_DEFAULT;

		WBUG("executing query bind %d, define %d ..... ", bindNum, defNum);
		retnum = 0;
		if (hasError(status = OCIStmtExecute(svchp, stmthp, errhp, (ub4) 0, (ub4) 0,
               		(CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, execute_mode)))
		{
			goto ENDQUERY;
		}

		if ( face->cRows_field  >= 0) 
		{
			cRowsLeft = getRowsetSize();
			if (  cRowsLeft <= 0 ) 
				goto ENDQUERY;
		}

		retnum = fetch(m_chunk, true);

		/* query执行完以后不清除bind和define句柄, 以便fetch的需要 */
	ENDQUERY:	
		break;

	case DBFace::DML:
		WBUG("handle DML para num %d", face->num);

		slen = TEXTUS_SNPRINTF(proc_stmt, sizeof(proc_stmt), "%s", face->sentence);
		WBUG("statement: %s", proc_stmt);
		if ( hasError(OCIStmtPrepare(stmthp, errhp, (const OraText*)&proc_stmt[0], (ub4) slen,
			(ub4) OCI_NTV_SYNTAX, (ub4) OCI_DEFAULT)) ) break;

		/* 输入值的绑定 */
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			unsigned char *buf;
			unsigned int blen;
			ub2 dty;
			DBFace::Para &para = face->paras[i];
			long rNo = para.fld + face->offset;
	
			assert(para.pos == (unsigned int)i );
			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				buf = rcv_pac->fld[rNo].val;
				blen = rcv_pac->fld[rNo].range;
				break;

			default:
				continue;
			}

			if ( !(dty =getdty(para.type)) )
				goto ENDDBPROC;

			if ( hasError(OCIBindByPos(stmthp, &bndp[i], errhp, i+1, (dvoid *) buf, blen, dty, 
				(dvoid *) 0, (ub2 *) 0, (ub2 *) 0, (ub4) 0, (ub4 *) 0, OCI_DEFAULT)))
				break;
		}

		/* 执行 */
		execute_mode = (ub4) OCI_COMMIT_ON_SUCCESS;

		if ((status = OCIStmtExecute(svchp, stmthp, errhp, (ub4) 1, (ub4) 0,
               		(CONST OCISnapshot *) NULL, (OCISnapshot *) NULL, execute_mode)))
		{
			if ( status != 1  )	/*  */
			{
				hasError(status);
			} else
				setRowsCount();
		} else
			setRowsCount();

		/* cleanup  */
		for ( i = 0; i < face->num && i < PARA_MAX; i++ )
		{
			if(bndp[i])
			{
				(void) OCIHandleFree((dvoid *) bndp[i], OCI_HTYPE_BIND);
				bndp[i] = 0;
			} else
				break;
		}
		break;

	default:
		break;
	}
	
	return retnum;
}

int OraCli::fetch(ub4 many, bool just)
{
	int i = 0;
	sword sc;
	ub2 orientation = just ? OCI_FETCH_ABSOLUTE : OCI_FETCH_NEXT ;
	
	/* 这里的(sb4) 1对于 OCI_FETCH_ABSOLUTE 是必要的, 而对于OCI_FETCH_NEXT 无用 */
	sc = OCIStmtFetch2(stmthp, errhp, many, orientation, (sb4) 1, OCI_DEFAULT);
	if ( sc == OCI_NO_DATA)
	{
		/* 取得的记录数可能会少于many, 这时sc == OCI_NO_DATA.  需要进一步判断是否一条记录都没有 
		   这对于OLEDB则不是这样的情况. OLEDB只有在没有一条记录的情况下才报告错误。
		*/
		cRowsObtained = 0;
		if ( hasError(OCIAttrGet((CONST void *) stmthp, OCI_HTYPE_STMT, 
			(void *) &cRowsObtained, 0, OCI_ATTR_ROWS_FETCHED, errhp)))
			goto END;
			
		if ( cRowsObtained == 0 )
		{	/* 的确没有任何记录 */
 			hasError(sc);
			goto END;
		} 

	} else if ( hasError(sc)) 
	{	/* 其它错误 */
		goto END;
	} else {
		/* 刚好取得所要的记录数 */
		cRowsObtained = many;
	}

	if ( face->cRows_field  >= 0 )
	{	/* 设置还有的记录数 */
		cRowsLeft -= cRowsObtained;
		PUT_FSND(cRows_field, &cRowsLeft, sizeof(cRowsLeft));
	}

	/* 设置已获得的记录数 */
	PUT_FSND(cRowsObt_fld, &cRowsObtained, sizeof(cRowsObtained));

	for ( i = 0 ; i < cRowsObtained; i++ )
	{	/* 取得每条记录 */
		copyRowSet(i);
		aptus->sponte(&dopac_pius);	
	}

	if ( face->rowset  && face->rowset->useEnd )
		aptus->sponte(&end_ps);	/* MULTI_UNIPAC_END */
END:
	return i;	/* 发了多少条记录? */
}

void OraCli::copyRowValue()
{
	int i ;
	for ( i  = 0 ; i < PARA_MAX; i++)
	{
		struct FieldObj &fobj = rowset_pac.fld[i];
		if ( fobj.no >= 0 )
		{
			snd_pac->input(fobj.no, fobj.val, fobj.range);
		}
	}
}

void OraCli::copyRowSet(int rowi)
{
	unsigned int i ;
	unsigned int defi;
	int m_chunk;
	ub2 blen, rlen;

	if( face->rowset ) 
		m_chunk = face->rowset->chunk;
	else
		m_chunk = 1;

	defi = 0;
	for ( i = 0; i < face->num && defi <= PARA_MAX; ++i )
	{
		DBFace::Para &para = face->paras[i];
		assert(para.pos == (unsigned int)i );

		if (  para.inout == DBFace::PARA_OUT )
		{
			int rNo = para.fld + face->offset;
			struct FieldObj &fobj = rowset_pac.fld[rNo];

			assert(rNo <= snd_pac->max );
			blen = (ub2) para.outlen;
			rlen = rlenp[defi*m_chunk + rowi] ;
			if ( rlen && fobj.no > 0 )
				snd_pac->input(fobj.no, &(fobj.val[ rowi*blen ]), rlen);
			defi++;
		}
	}
}

void OraCli::logout()
{
	if ( *isTalking )
	{
		hasError(OCISessionEnd (svchp,errhp, authp, OCI_DEFAULT));
		hasError(OCIServerDetach (srvhp, errhp, OCI_DEFAULT)); 
		*isTalking = false;
	}
}
void OraCli::logon()
{
	
	if ( hasError(OCIServerAttach( srvhp, errhp, (text *)conn_str, strlen(conn_str), 0)))  
		return;

	WBUG("OCIServerAttach successfully!");
	/* set attribute server context in the service context */
	(void) OCIAttrSet( (dvoid *) svchp, OCI_HTYPE_SVCCTX, (dvoid *)srvhp,
		(ub4) 0, OCI_ATTR_SERVER, (OCIError *) errhp);

	*isTalking = !hasError(OCISessionBegin ( svchp,  errhp, authp, OCI_CRED_RDBMS, (ub4) OCI_DEFAULT));
	if ( !*isTalking)
	{
		OCIServerDetach (srvhp, errhp, OCI_DEFAULT); 
		return;
	}

	(void) OCIAttrSet((dvoid *) svchp, (ub4) OCI_HTYPE_SVCCTX,
			(dvoid *) authp, (ub4) 0, (ub4) OCI_ATTR_SESSION, errhp);

	WBUG("logon to a server successfully!");
}

void OraCli::myalloc()
{
	/* server contexts */
	isTalking = new bool;
	*isTalking = false;
	(void) OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &srvhp, OCI_HTYPE_SERVER,
		(size_t) 0, (dvoid **) 0);

	(void) OCIHandleAlloc( (dvoid *) envhp, (dvoid **) &svchp, OCI_HTYPE_SVCCTX,
		(size_t) 0, (dvoid **) 0);
	
	(void) OCIHandleAlloc((dvoid *) envhp, (dvoid **)&authp,
		(ub4) OCI_HTYPE_SESSION, (size_t) 0, (dvoid **) 0);

	(void) OCIAttrSet((dvoid *) authp, (ub4) OCI_HTYPE_SESSION,
		(dvoid *) username, (ub4) strlen((char *)username),
		(ub4) OCI_ATTR_USERNAME, errhp);

	(void) OCIAttrSet((dvoid *) authp, (ub4) OCI_HTYPE_SESSION,
		(dvoid *) password, (ub4) strlen((char *)password),
		(ub4) OCI_ATTR_PASSWORD, errhp);
}

INLINE ub2 OraCli::getdty(DBFace::DataType type)
{
	ub2 dty;
	switch ( type )
	{
	case DBFace::Integer:
		dty = SQLT_INT;
		break;

	case DBFace::Char:
		dty = SQLT_CHR;
		break;

	case DBFace::String:
		dty = SQLT_STR;
		break;

	case DBFace::Decimal:
	case DBFace::Numeric:
		dty = SQLT_NUM;
		break;

	default:
		WLOG(CRIT, "Unknown data type %d!", type);
		dty = 0;
	}
	return dty;
}

long OraCli::getRowsetSize()
{
	long position;
	if ( hasError( OCIStmtFetch2(stmthp, errhp, (ub4) 1, OCI_FETCH_LAST, (sb4) 0, OCI_DEFAULT) ) )
		return 0;

	if (hasError( OCIAttrGet ( stmthp, OCI_HTYPE_STMT, 
		&position, NULL, OCI_ATTR_CURRENT_POSITION, errhp)) )
		return 0;

	WBUG("the Rowset size is %ld", position);
	return position;
}

void OraCli::setRowsCount()
{
	long count;

	if (hasError(OCIAttrGet(stmthp, OCI_HTYPE_STMT, (dvoid*)&count, (ub4 *)0, OCI_ATTR_ROW_COUNT, errhp)))
		return ;

	WBUG("get OCI_ATTR_ROW_COUNT is %ld", count);
	PUT_FSND (cRows_field, &count, sizeof(count));
}

bool OraCli::hasError(sword status)
{
	text errbuf[512];
	sb4 errcode = 0;
	bool ret = true;
	int len;

	PUT_FSND (errCode_field, &status, sizeof(status));

	switch (status)
	{
	case OCI_SUCCESS:
		ret = false;
		break;

	case OCI_SUCCESS_WITH_INFO:
		WLOG(ERR, "OCI_SUCCESS_WITH_INFO=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_SUCCESS_WITH_INFO");
		break;

	case OCI_NEED_DATA:
		WLOG(ERR, "OCI_NEED_DATA=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_NEED_DATA");
		break;

	case OCI_NO_DATA:
		WLOG(ERR, "OCI_NO_DATA=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_NO_DATA");
		break;

	case OCI_ERROR:
		(void) OCIErrorGet((dvoid *)errhp, (ub4) 1, (text *) NULL, &errcode,
				errbuf, (ub4) sizeof(errbuf), OCI_HTYPE_ERROR);
		len = strlen((char*)errbuf);
		if ( errbuf[len-1] == '\r' || errbuf[len-1] == '\n' )
		{
			len--;
			errbuf[len] = 0;
		}
		PUT_FSND(errStr_field, errbuf, len);

		WLOG(ERR, "OCI_ERROR %.*s", 512, errbuf);
		break;

	case OCI_INVALID_HANDLE:
		WLOG(ERR, "OCI_INVALID_HANDLE=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_INVALID_HANDLE");
		break;

	case OCI_STILL_EXECUTING:
		WLOG(ERR, "OCI_STILL_EXECUTE=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_STILL_EXECUTE");
		break;

	case OCI_CONTINUE:
		WLOG(ERR, "OCI_CONTINUE=%d", status);
		PUT_FSND_STR(errStr_field, "OCI_CONTINUE");
		break;

	default:
		WLOG(ERR, "Error=%d", status);
		PUT_FSND_STR(errStr_field, "UNKONW ERROR");
		break;
	}
	return ret;
}

#include "hook.c"
