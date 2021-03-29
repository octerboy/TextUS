/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: OLE DB Connector
 Build: created by octerboy, 2007/04/02, Guangzhou
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

#include <oledb.h>
#include <olectl.h>       // IConnectionPoints interface
#include <oledberr.h>
#include <msdasc.h>        // OLE DB Service Component header

#define BUFF_SIZE 1024
#define BUFF_SIZE_2 2048

#define ROUNDUP_AMOUNT		8
#define ROUNDUP_(size,amount)	(((ULONG)(size)+((amount)-1))&~((amount)-1))
#define ROUNDUP(size)		ROUNDUP_(size, ROUNDUP_AMOUNT)
#define MOLE_Release(pv)	{ ((IUnknown*)pv)->Release(); (pv) = NULL; }

class Consumer: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	Consumer();
	~Consumer();
#include "wlog.h"
	Amor::Pius dopac_pius, end_ps;	//仅用于向左边传回数据
	bool inThread;
	
	unsigned TEXTUS_LONG handle();	/* 0:需要向左dopac_pius; > 0, 不管, 对于有行集返回时用 */
	void start_session();
	void end_session();
	DBTYPE getdty(DBFace::DataType type);
	DBFace::Para *findDefOut(int &from );

	WCHAR connect_wstr[BUFF_SIZE_2];
	const char *conn_str;

	bool isPoineer;

	struct _GSess {
		bool isTalking;	/* 一个会话是否进行 */
		IDataInitialize		*pIDataInitialize ;
		IDBInitialize		*pIDBInitialize    ;
		IDBCreateSession	*pIDBCreateSession ;
		IDBCreateCommand	*pIDBCreateCommand;
	
		inline _GSess () {
			isTalking = false;
			pIDataInitialize = NULL;
			pIDBInitialize 	 = NULL;
			pIDBCreateSession=NULL;
			pIDBCreateCommand=NULL;
		};

	} *gSess;

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	DBFace *dbface;

#define STMT_MAX	2048
#define PARA_MAX 	256
	WCHAR proc_stmt[STMT_MAX];

	ICommandText		*pICommandText;
//输入输出参数
	IAccessor	*pIAccessor;
	HACCESSOR	hAccessor_para;
	
	DBPARAMS	dbParams;
	TBuffer 	dbData;
	DBBINDING	bnds[PARA_MAX];

//结果集
	IAccessor* 	pIAccessor_qry; // Pointer to the accessor
	HACCESSOR	hAccessor_qry;
	DBROWCOUNT	cRowsAffected;

	TBuffer 	rowData;
	IRowset		*pIRowset;
	DBBINDING	defns[PARA_MAX];

	ULONG 		defNum ;
	DBBINDSTATUS 	DBBindStatus[PARA_MAX];
	HROW		*rghRows; // Row handles
	ULONG 		ghrow_sz;	/* how many Row handles */

	bool getQueryInfo(int para_offset);
	DBCOUNTITEM fetch(int para_offset); 	/* 0, 发生错误, > 0, 有行返回 */
	void showStatus(DBCOUNTITEM row, DBCOUNTITEM col, DBSTATUS st);

//Error Handle
	bool hasError(const char* msg, HRESULT hr);
	const char* getErrorName(HRESULT hr);
	void handleError(HRESULT hrReturned);
	HRESULT displayErrorRecord(HRESULT hrReturned, ULONG iRecord, IErrorRecords* pIErrorRecords);
	HRESULT displayErrorInfo(HRESULT hrReturned, IErrorInfo* pIErrorInfo);
	HRESULT getSqlErrorInfo(ULONG iRecord, IErrorRecords* pIErrorRecords, BSTR* pBstr, LONG* plNativeError);

//other
	char  tmp_str[BUFF_SIZE];
};

void Consumer::ignite(TiXmlElement *cfg) 
{
	bool inThread =true;
	TiXmlElement *con_ele;

	const char *comm_str;
	inThread =  ( (comm_str= cfg->Attribute("cocurrent")) && strcasecmp(comm_str, "yes") == 0 );

	con_ele = cfg->FirstChildElement("connect");
	if ( con_ele ) 
		conn_str = con_ele->GetText();

	if ( gSess == 0 )
		gSess = new _GSess();
	return;
}

bool Consumer::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	HRESULT rc=S_OK;
	TEXTUS_LONG retnum;

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");

		if ( !dbface || !gSess ) 
		{ 
			WLOG(ERR, "no dbface or has no property.");
			break;
		}
		if ( !(gSess->isTalking)) 
			start_session();

		if ( !(gSess->isTalking))
		{
			aptus->sponte(&dopac_pius);
			break;
		}

		retnum = handle();
		WBUG("handle end " TLONG_FMT, retnum);
		if ( retnum == 0 ) 
			aptus->sponte(&dopac_pius);

		break;

	case Notitia::CMD_DBFETCH:
		WBUG("facio CMD_DBFETCH");
		if ( !dbface || !gSess )
		{ 
			WLOG(ERR, "no dbface or has no property.");
			break;
		}
		if ( (gSess->isTalking))
		{
			if ( (retnum = fetch(dbface->rowset == 0 ? 0 : dbface->rowset->para_pos)) == 0 )
				aptus->sponte(&dopac_pius);
		} else {
			WLOG(INFO, "No Session.");
		}
		break;

	case Notitia::CMD_SET_DBFACE:
		WBUG("facio CMD_SET_DBFACE");
		dbface = (DBFace *)pius->indic;
		if ( dbface->rowset )
		{
			if ( dbface->rowset->chunk > ghrow_sz )
			{
				ghrow_sz = dbface->rowset->chunk;
				delete [] rghRows;
				rghRows = new HROW [ghrow_sz];
			}
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		// Initialize the COM library 
		rc = CoInitialize(NULL);

		//WBUG("CoInitialize rc = %d", rc);
		if ( rc == S_OK )
			goto NEXT;

		if ( rc == S_FALSE )
		{
			WLOG(INFO, "The COM library is already initialized on this thread");
			goto NEXT;
		}

		if ( rc == RPC_E_CHANGED_MODE  )
		{
			WLOG(INFO, "A previous call to CoInitializeEx specified the concurrency model for this thread as multithread apartment (MTA). ");
			goto NEXT;
		}

		hasError("CoInitialize", rc);
		break;
	NEXT:	
		// Initialize OLEDB 
		rc = CoCreateInstance(CLSID_MSDAINITIALIZE, NULL, CLSCTX_INPROC_SERVER, IID_IDataInitialize,
   			reinterpret_cast<LPVOID *>(&(gSess->pIDataInitialize)));

		if (hasError("MSDAINITIALIZE failed", rc))
			break;

		isPoineer = true;

		memset(connect_wstr, 0, sizeof(connect_wstr));
        	if ( !MultiByteToWideChar(CP_ACP, 0,  conn_str, -1, connect_wstr, 2048) )
			WLOG_OSERR("Convert conn_str");
		
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		end_session();
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");

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
		WBUG("facio Notitia::" TLONG_FMT, pius->ordo);
		return false;
	}

	return true;
}

bool Consumer::sponte( Amor::Pius *pius)
{
	switch ( pius->ordo )
	{
	case Notitia::CMD_SET_DBCONN:
		WBUG("facio CMD_SET_DBCONN");
		if ( pius->indic )
		{
			const char *c_str = (const char*)pius->indic;
        		if ( !MultiByteToWideChar(CP_ACP, 0,  c_str, -1, connect_wstr, 2048) )
				WLOG_OSERR("convert conn_str");
		}
		break;
		
	default:
		return false;
	}
	return true;
}

Consumer::Consumer()
{
	isPoineer = false;
	dbface = 0;
	dopac_pius.ordo = Notitia::PRO_UNIPAC;
	dopac_pius.indic = 0;

	end_ps.ordo = Notitia::MULTI_UNIPAC_END;
	end_ps.indic = 0;

	gSess = 0;

	pICommandText	= NULL;

	pIAccessor 	= NULL;
	hAccessor_para	= NULL;

	pIAccessor_qry	= NULL;
	hAccessor_qry	= NULL;
	pIRowset	= NULL;
	ghrow_sz	= 1;
	rghRows = new HROW [ghrow_sz];

	memset(defns, 0, PARA_MAX*sizeof( DBBINDING));
	memset(bnds, 0, PARA_MAX*sizeof( DBBINDING));
	memset(connect_wstr, 0, sizeof(connect_wstr));
}

Consumer::~Consumer() 
{ 
	if ( rghRows !=NULL )
		delete[] rghRows;

	if ( isPoineer )
	{
		end_session();

		gSess->pIDataInitialize ->Release();
		gSess->pIDataInitialize= NULL;
	} 
}

Amor* Consumer::clone()
{
	Consumer *child = new Consumer();
#define INH(X) child->X = X;
	INH(conn_str)   ;

	memcpy(child->connect_wstr, connect_wstr, sizeof(connect_wstr));

	INH(gSess)  ;
	return (Amor*)child;
}

unsigned TEXTUS_LONG Consumer::handle()
{
	HRESULT rc=S_OK;
	DBCOUNTITEM  cBindings, cBindOut;

	ULONG i, j, ulOffset = 0;
	unsigned TEXTUS_LONG retnum=0;

	int len ;
	IDBCreateCommand	*pIDBSession;
	bool sNeo = false;

	if ( pICommandText )
	{
		rc = pICommandText->GetDBSession(IID_IDBCreateCommand, (IUnknown**)&pIDBSession);
		sNeo =  (pIDBSession != gSess->pIDBCreateCommand ); /* 判断会话是否已经被更新了 */
	}

	if ( pIRowset || sNeo) 	/* 关闭结果集 */
	{
		if ( hAccessor_qry )
		{
			if ( pIAccessor_qry )
				pIAccessor_qry->ReleaseAccessor(hAccessor_qry, NULL);
			hAccessor_qry = NULL;
		}
		MOLE_Release( pIAccessor_qry );

		MOLE_Release( pIRowset);
		MOLE_Release( pIAccessor );
		MOLE_Release( pICommandText );
	}

	if ( pICommandText == NULL )
	{
		//创建命令，返回ICommandText接口的指针
		rc = gSess->pIDBCreateCommand->CreateCommand(NULL,IID_ICommandText,(IUnknown**)&pICommandText);
		if (hasError("Create command failed", rc))
		{
			end_session();
			return retnum;
		}

		rc = pICommandText->QueryInterface(IID_IAccessor, (void **)&pIAccessor);
		if ( hasError("IAccessor initialize", rc) )
		{
			end_session();
			return retnum;
		}

		WBUG("new Command Text successfully!");
	}

	switch ( dbface->pro )
	{
	case DBFace::QUERY:
		WBUG("handle QUERY para num %d", dbface->num);
		WBUG("statement: %s", dbface->sentence);
        	if ( !MultiByteToWideChar(CP_ACP, 0,  dbface->sentence, -1, proc_stmt, STMT_MAX) )
		{
			WLOG_OSERR("Convert QUERY statement");
			break;
		}

		if ( hasError("Set query text", pICommandText->SetCommandText(DBGUID_DBSQL, proc_stmt)))
			break;

		dbData.reset();	/* 参数绑定区清空 */

		/* 计算一下输入输出内容的长度, 以确定dbData空间. 
		   因为rcv_pac的内容要复制到dbData中, 而dbData中的返回数据要复制到snd_pac中 */

		cBindings = 0;
		ulOffset = 0;
		for ( i = 0; i < dbface->num && i < PARA_MAX; i++ )
		{	
			unsigned char *buf;
			ULONG 	blen, space;
			DBTYPE	dty;
			size_t 	gap;

			DBFace::Para &para = dbface->paras[i];
			int rNo = para.fld + dbface->offset;
			DBSTATUS dbStatus;
	
			assert(para.pos == (unsigned int)i );
			blen = 0;
			space = 0;
			buf = 0;
			if ( !(dty =getdty(para.type)) )
				goto ENDQUERY;

			bnds[cBindings].iOrdinal	= cBindings+1;
			bnds[cBindings].obStatus	= ulOffset;
			bnds[cBindings].obLength	= ulOffset + sizeof(DBSTATUS);
			bnds[cBindings].obValue		= ulOffset + sizeof(ULONG) + sizeof(DBSTATUS);
			
			bnds[cBindings].pTypeInfo	= NULL;
			bnds[cBindings].pBindExt	= NULL;
	
			bnds[cBindings].dwPart		= DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;			
			bnds[cBindings].dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
			bnds[cBindings].dwFlags		= 0;
		
			bnds[cBindings].bPrecision	= para.precision;
			bnds[cBindings].bScale		= para.scale;
				
			bnds[cBindings].pObject		= NULL;
			bnds[cBindings].wType		= dty;

			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				assert(rNo <= rcv_pac->max );
				if ( para.inout == DBFace::PARA_IN )
				{
					bnds[cBindings].eParamIO	= DBPARAMIO_INPUT;
					space = (ULONG) rcv_pac->fld[rNo].range ;
				} else {
					bnds[cBindings].eParamIO	= DBPARAMIO_INPUT | DBPARAMIO_OUTPUT;
					space =(ULONG) (para.outlen == 0 ? rcv_pac->fld[rNo].range : para.outlen) ;
				}

				if ( rcv_pac->fld[rNo].no < 0 )
				{
					dbStatus = DBSTATUS_S_ISNULL;
				} else {
					dbStatus = DBSTATUS_S_OK;
					blen = (ULONG) rcv_pac->fld[rNo].range;
					buf = rcv_pac->fld[rNo].val;
				}

				break;

			default:
				continue;
			}

			dbData.input( (unsigned char*)&dbStatus, sizeof(DBSTATUS));
			dbData.input( (unsigned char*)&blen, sizeof(ULONG));
			if ( blen > 0 && buf ) 
			{	/* input */
				dbData.input(buf, blen);
			} 

			if ( space > blen ) {
				/* output */
				dbData.grant(space - blen);
				dbData.commit(space - blen);
				bnds[cBindings].cbMaxLen	= space;
			} else {
				/* 对于input  output的参数, 输出空间少于输入的, 以输入为准 */
				bnds[cBindings].cbMaxLen	= blen;
			}
			
			/* 为下一个参数操作准备 */
			ulOffset = ROUNDUP(bnds[cBindings].obValue + bnds[cBindings].cbMaxLen);
			gap = ulOffset - bnds[cBindings].obValue - bnds[cBindings].cbMaxLen;
			if ( gap > 0  )
			{	/* aligned */
				dbData.grant(gap);
				dbData.commit(gap);
			}

			cBindings++;
		}

		if ( hAccessor_para )
		{
			pIAccessor->ReleaseAccessor(hAccessor_para, NULL);
			hAccessor_para = NULL;
		}

		if ( cBindings > 0 ) 
		{
			if ( hasError("CreateAccessor", pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA, 
				cBindings, bnds, ulOffset, &hAccessor_para, NULL)))
				break;

			dbParams.hAccessor	= hAccessor_para;
			dbParams.cParamSets	= 1;			//Numer of Parameter sets
			dbParams.pData		= dbData.base;		//Source Data
		}

		/* 执行 */
		if ( hasError("Execute", pICommandText->Execute(NULL, IID_IRowset,  cBindings > 0 ? &dbParams:NULL, 
			&cRowsAffected, (IUnknown**)&pIRowset)))
			break;

		WBUG("cRowsAffected " TLONG_FMT ", pIRowset %p", cRowsAffected, pIRowset);

		if ( snd_pac && dbface->cRows_field >= 0
			&& snd_pac->max >= dbface->cRows_field) 
		{
			snd_pac->input((unsigned int) dbface->cRows_field, (unsigned char*)&cRowsAffected, sizeof(cRowsAffected));
		}

		/* 输出数据, 全在pIRowset中  */
		if ( pIRowset )
		{
			if ( getQueryInfo(0) )
				retnum = fetch(0);
		}
	ENDQUERY:	
		break;

	case DBFace::FUNC:
		WBUG("handle Function para num %d", dbface->num);
		if ( dbface->num < 1 || dbface->paras[0].inout != DBFace::PARA_OUT )
		{
			WLOG(ERR, "function has no return parameter");
			break;
		}

		len = 8;
		memcpy(tmp_str, "{?=call ", len);
		j = 1;
		goto CASE_DBPROC;

	case DBFace::DBPROC:
		WBUG("handle procedure para num %d", dbface->num);
		len = 6;
		memcpy(tmp_str, "{call ", len);
		j = 0;

	    CASE_DBPROC:
		memcpy(&tmp_str[len], dbface->sentence, strlen(dbface->sentence));
		len += (int)strlen(dbface->sentence);
		memcpy(&tmp_str[len], "(", 1);
		len++;
		for ( i = j; (i+1) < dbface->num && (i+1) < PARA_MAX; i++ )
		{	
			if ( dbface->paras[i].inout != DBFace::UNKNOWN)
			{
				memcpy(&tmp_str[len], "?,", 2);
				len +=2;
			}
		}

		if ( dbface->num > j )
		{
				memcpy(&tmp_str[len], "?", 1);
				len++;
		}

		memcpy(&tmp_str[len], ")}", 2);
		len +=2;
		tmp_str[len] = 0;
		WBUG("statement: %s", tmp_str);
        	if ( !MultiByteToWideChar(CP_ACP, 0,  tmp_str, len, proc_stmt, STMT_MAX) )
		{
			WLOG_OSERR("Convert procedure statement");
			break;
		}
		goto HERE_DBProcess;

	case DBFace::DML:
		WBUG("handle DML para num %d", dbface->num);
		WBUG("statement: %s", dbface->sentence);

        	if ( !MultiByteToWideChar(CP_ACP, 0,  dbface->sentence, -1, proc_stmt, STMT_MAX) )
		{
			WLOG_OSERR("Convert DML statement");
			break;
		}

	   HERE_DBProcess:
		if ( hasError("SetCommandText", pICommandText->SetCommandText(DBGUID_DBSQL, proc_stmt)))
			break;

		dbData.reset();	/* 参数绑定区清空 */
		/* 计算一下输入输出内容的长度, 以确定dbData空间. 
		   因为rcv_pac的内容要复制到dbData中, 而dbData中的返回数据要复制到snd_pac中 */

		cBindings = 0;
		ulOffset = 0;
		for ( i = 0; i < dbface->num && i < PARA_MAX; i++ )
		{	
			unsigned char *buf;
			ULONG 	blen, space;
			size_t 	gap;
			DBTYPE	dty;

			DBFace::Para &para = dbface->paras[i];
			int rNo = para.fld + dbface->offset;
			DBSTATUS dbStatus;
	
			assert(para.pos == (unsigned int)i );
			blen = 0;
			space = 0;
			buf = 0;
			if ( !(dty =getdty(para.type)) )
				goto ENDDBPROC;

			bnds[cBindings].iOrdinal	= cBindings+1;
			bnds[cBindings].obStatus	= ulOffset;
			bnds[cBindings].obLength	= ulOffset + sizeof(DBSTATUS);
			bnds[cBindings].obValue		= ulOffset + sizeof(ULONG) + sizeof(DBSTATUS);
			
			bnds[cBindings].pTypeInfo	= NULL;
			bnds[cBindings].pBindExt	= NULL;
	
			bnds[cBindings].dwPart		= DBPART_VALUE | DBPART_LENGTH | DBPART_STATUS;			
			bnds[cBindings].dwMemOwner	= DBMEMOWNER_CLIENTOWNED;
			bnds[cBindings].dwFlags		= 0;
		
			bnds[cBindings].bPrecision	= para.precision;
			bnds[cBindings].bScale		= para.scale;
				
			bnds[cBindings].pObject		= NULL;
			bnds[cBindings].wType		= dty;

			switch ( para.inout )
			{
			case DBFace::PARA_IN:
			case DBFace::PARA_INOUT:
				assert(rNo <= rcv_pac->max );
				if ( para.inout == DBFace::PARA_IN )
				{
					bnds[cBindings].eParamIO	= DBPARAMIO_INPUT;
					space = (ULONG)rcv_pac->fld[rNo].range ;
				} else {
					bnds[cBindings].eParamIO	= DBPARAMIO_INPUT | DBPARAMIO_OUTPUT;
					space = (ULONG) (para.outlen == 0 ? rcv_pac->fld[rNo].range : para.outlen) ;
				}

				if ( rcv_pac->fld[rNo].no < 0 )
				{
					dbStatus = DBSTATUS_S_ISNULL;
				} else {
					dbStatus = DBSTATUS_S_OK;
					blen = (ULONG) rcv_pac->fld[rNo].range;
					buf = rcv_pac->fld[rNo].val;
				}

				break;

			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				bnds[cBindings].eParamIO	= DBPARAMIO_OUTPUT;
				space = para.outlen;
				blen = 0;
				buf = (unsigned char *)0;

				break;

			default:
				continue;
				break;
			}

			dbData.input( (unsigned char*)&dbStatus, sizeof(DBSTATUS));
			dbData.input( (unsigned char*)&blen, sizeof(ULONG));
			if ( blen > 0 && buf ) 
			{	/* input */
				dbData.input(buf, blen);
			} 

			if ( space > blen ) {
				/* output */
				dbData.grant(space - blen);
				dbData.commit(space - blen);
				bnds[cBindings].cbMaxLen	= space;
			} else {
				/* 对于input  output的参数, 输出空间少于输入的, 以输入为准 */
				bnds[cBindings].cbMaxLen	= blen;
			}
			
			/* 为下一个参数操作准备 */
			ulOffset = ROUNDUP(bnds[cBindings].obValue + bnds[cBindings].cbMaxLen);
			gap = ulOffset - bnds[cBindings].obValue - bnds[cBindings].cbMaxLen;
			if ( gap > 0  )
			{	/* aligned */
				dbData.grant(gap);
				dbData.commit(gap);
			}

			cBindings++;
		}

		if ( hAccessor_para )
		{
			pIAccessor->ReleaseAccessor(hAccessor_para, NULL);
			hAccessor_para = NULL;
		}

		if ( cBindings > 0 ) 
		{
			if ( hasError("CreateAccessor", pIAccessor->CreateAccessor(DBACCESSOR_PARAMETERDATA, 
				cBindings, bnds, ulOffset, &hAccessor_para, NULL)))
				break;

			dbParams.hAccessor	= hAccessor_para;
			dbParams.cParamSets	= 1;			//Numer of Parameter sets
			dbParams.pData		= dbData.base;		//Source Data
		}

		/* 执行 */
		if ( hasError("Execute", 
			pICommandText->Execute(NULL, dbface->pro == DBFace::DML ? IID_NULL:IID_IRowset,
  			cBindings > 0 ? &dbParams:NULL, &cRowsAffected, (IUnknown**)&pIRowset)))
			break;

		WBUG("cRowsAffected " TLONG_FMT ", pIRowset %p", cRowsAffected, pIRowset);

		if ( snd_pac && dbface->cRows_field >= 0
			&& snd_pac->max >= dbface->cRows_field) 
		{
			snd_pac->input((unsigned int) dbface->cRows_field, (unsigned char*)&cRowsAffected, sizeof(cRowsAffected));
		}
		/* 输出数据, 全在dbData中  */
		cBindOut = 0;
		for ( i = 0; i < dbface->num && i < PARA_MAX; ++i )
		{
			unsigned char *buf;
			ULONG 	blen;
			DBSTATUS dbStatus;

			DBFace::Para &para = dbface->paras[i];
			int rNo = para.fld + dbface->offset;
	
			assert(para.pos == (unsigned int)i );

			switch ( para.inout )
			{
			case DBFace::PARA_IN:
				break;

			case DBFace::PARA_INOUT:
			case DBFace::PARA_OUT:
				assert(rNo <= snd_pac->max );
				assert( (cBindOut+1) == bnds[cBindOut].iOrdinal);
				assert(bnds[cBindOut].eParamIO	& DBPARAMIO_OUTPUT);

				memcpy(&dbStatus, bnds[cBindOut].obStatus + dbData.base,  sizeof(DBSTATUS));
				if ( dbStatus == DBSTATUS_S_OK ||  dbStatus == DBSTATUS_S_TRUNCATED )
				{
					memcpy(&blen, bnds[cBindOut].obLength + dbData.base,  sizeof(ULONG));
					buf = bnds[cBindOut].obValue + dbData.base;
					snd_pac->input(rNo, buf, blen);
				}
				break;

			default:
				continue;
				break;
			}

			cBindOut++;
		}
		if ( pIRowset )
		{
			if ( getQueryInfo(dbface->rowset == 0 ? 0 : dbface->rowset->para_pos) )
				retnum = fetch(dbface->rowset == 0 ? 0 : dbface->rowset->para_pos);
		}

	ENDDBPROC:
		break;

	default:
		break;
	}
	return retnum;
}

void Consumer::end_session()
{
	if ( !gSess )
		return;
	
	MOLE_Release( gSess->pIDBCreateCommand );
	MOLE_Release( gSess->pIDBCreateSession );
		
	if ( gSess->pIDBInitialize )
	{
		gSess->pIDBInitialize->Uninitialize();
		gSess->pIDBInitialize 	= NULL;
	}

	gSess->isTalking = false;
}

void Consumer::start_session()
{
	HRESULT rc=S_OK;
	if ( gSess && gSess->isTalking ) 
		return;
	
	//wprintf(L"connect_str %s\n", connect_wstr);
	rc = gSess->pIDataInitialize->GetDataSource( NULL, CLSCTX_INPROC_SERVER, connect_wstr,
		IID_IDBInitialize, reinterpret_cast<IUnknown **>(&(gSess->pIDBInitialize)));	

	if (hasError("GetDataSource failed",rc))
		return;
		
	rc = gSess->pIDBInitialize->Initialize();
	if (hasError("DBInitialize failed",rc))
		return;
		
	//获得指向IDBCreateSession接口的指针
	rc = gSess->pIDBInitialize->QueryInterface(IID_IDBCreateSession,(void**)&(gSess->pIDBCreateSession));
	if (hasError("Session initialization failed", rc))
		return;
		
	//创建会话，返回IDBCreateCommand接口的指针
	rc = gSess->pIDBCreateSession->CreateSession(NULL,IID_IDBCreateCommand,(IUnknown**)&(gSess->pIDBCreateCommand));

	if (hasError("Create session failed", rc))
	{
		end_session();
		return;
	}

	gSess->isTalking = true;
	WBUG("connected to a server successfully!");
}

DBTYPE Consumer::getdty(DBFace::DataType type)
{
	DBTYPE dty;
	switch ( type )
	{
	case DBFace::Integer:
		dty = DBTYPE_I4;
		break;

	case DBFace::Byte:
		dty = DBTYPE_BYTES;
		break;

	case DBFace::String:
	case DBFace::Char:
		dty = DBTYPE_STR;
		break;

	case DBFace::Decimal:
		dty = DBTYPE_DECIMAL ;
		break;

	case DBFace::Numeric:
		dty = DBTYPE_NUMERIC;
		break;

	default:
		WLOG(CRIT, "Unknown data type %d!", type);
		dty = 0;
	}
	return dty;
}

#define CHECK_HR(hr)			\
	if(FAILED(hr))				\
		goto CLEANUP

bool Consumer::getQueryInfo(int para_offset)
{
	bool ret;
	HRESULT hr;

	DBCOLUMNINFO* pColumnsInfo = NULL;
	OLECHAR* pColumnStrings = NULL;

	DBORDINAL nCols;
	unsigned TEXTUS_LONG j;
	int k;
	ULONG ulOffset = 0;

	IColumnsInfo* pIColumnsInfo = NULL;

	ret = true;
	//取得列信息
	if (hasError("ColumnsInfo initialize", 
		pIRowset->QueryInterface(IID_IColumnsInfo, (void**) &pIColumnsInfo) ))
	{
		return false;
	}

	hr = pIColumnsInfo->GetColumnInfo(&nCols, &pColumnsInfo, &pColumnStrings);
	if (hasError("GetColumnInfo", hr) )
	{
		nCols = 0;
		return false;
	}
	//wprintf(L"ColumnStrings %s\n", pColumnStrings);

	//创建绑定
	rowData.reset();	/* 行结果集输出区清空 */
	k = para_offset; defNum = 0;
	WBUG("nCols " TLONG_FMT ", para_offset %d, PARA_MAX %d", nCols, para_offset, PARA_MAX);
	for (j = 0 ; j < nCols && defNum + para_offset < PARA_MAX; j++)
	{
		DBFace::Para *para;
		DBTYPE	dty;
		/* 找一个已经定义的 */
		para = findDefOut(k);
		if ( !para ) break;
		//k++;		/* 为下一个参数准备 */

		if ( !(dty =getdty(para->type)) )
			break;

		defns[defNum].iOrdinal	= pColumnsInfo[j].iOrdinal;
		defns[defNum].obStatus	= ulOffset;
		defns[defNum].obLength	= ulOffset + sizeof(DBSTATUS);;
		defns[defNum].obValue	= ulOffset + sizeof(DBSTATUS) + sizeof(ULONG);

		defns[defNum].pTypeInfo	= NULL;
		defns[defNum].pBindExt	= NULL;

		defns[defNum].dwPart	= DBPART_VALUE |DBPART_LENGTH|DBPART_STATUS;
		defns[defNum].dwMemOwner= DBMEMOWNER_CLIENTOWNED;
		defns[defNum].eParamIO	= DBPARAMIO_NOTPARAM;

		defns[defNum].dwFlags	= 0;
		//defns[defNum].bPrecision= pColumnsInfo[j].bPrecision;
		defns[defNum].bPrecision= para->precision;
		//defns[defNum].bScale	= pColumnsInfo[j].bScale;
		defns[defNum].bScale	= para->scale;

		defns[defNum].pObject	= NULL;
		//defns[defNum].wType	= pColumnsInfo[j].wType;
		defns[defNum].wType	= dty;
		//defns[defNum].cbMaxLen	= pColumnsInfo[j].ulColumnSize;
		defns[defNum].cbMaxLen	= para->outlen;

		//Account for the NULL terminator
		if(defns[defNum].wType == DBTYPE_STR)
			defns[defNum].cbMaxLen	+= sizeof(CHAR);

		if(defns[defNum].wType == DBTYPE_WSTR)
			defns[defNum].cbMaxLen	+= sizeof(WCHAR);
		
		ulOffset = ROUNDUP(defns[defNum].obValue + defns[defNum].cbMaxLen);
		if ( !(pColumnsInfo[j].dwFlags & DBCOLUMNFLAGS_ISLONG ) )
			defNum++;
		//wprintf(OLESTR("colname %s\n"), pColumnsInfo[j].pwszName);
	}

	rowData.grant(ulOffset);
	if ( pIAccessor_qry )
	{
		pIAccessor_qry->Release();
		pIAccessor_qry = NULL;
	}

	pIRowset->QueryInterface(IID_IAccessor, (void**) &pIAccessor_qry);
	if (hasError("pIAccessor_qry initialize", hr) )
	{
		ret = false;
		goto CLEAN;
	}

	//创建访问器
	if ( hAccessor_qry )
	{
		pIAccessor_qry->ReleaseAccessor(hAccessor_qry, NULL);
		hAccessor_qry = NULL;
	}

	WBUG("Rowset defNum %d", defNum);
	hr = pIAccessor_qry->CreateAccessor( DBACCESSOR_ROWDATA, defNum, defns, 0, &hAccessor_qry, &DBBindStatus[0] );
	if (hasError("hAccessor_qry create", hr) )
	{
		ret = false;
	}
CLEAN:
	CoTaskMemFree(pIColumnsInfo);
	CoTaskMemFree(pColumnStrings);
	return ret;
}

/* 返回实际取得的行数 */
DBCOUNTITEM Consumer::fetch(int para_offset)
{
	int k;
	DBCOUNTITEM cRowsObtained = 0, iRow; // Count of rows,  Row count
	ULONG j;
	HRESULT hr;
	ULONG chunk_rows;

	if ( dbface->rowset ) 
	{
		chunk_rows = dbface->rowset->chunk;
	} else 	/* 未设定为行集, 只取一行, 就象一般的存储过程的输出参数一样 */
		chunk_rows = 1;

	hr = pIRowset->GetNextRows( 0, 0, chunk_rows, &cRowsObtained, &rghRows ) ;

	WBUG("get " TLONG_FMT " row(s)", cRowsObtained);
	// All done; there are no more rows left to get.
	if ( snd_pac && dbface->cRowsObt_fld >= 0
		&& snd_pac->max >= dbface->cRowsObt_fld) 
	{
		snd_pac->input((unsigned int) dbface->cRowsObt_fld, (unsigned char*)&cRowsObtained, sizeof(cRowsObtained));
	}

	if ( hr == DB_S_ENDOFROWSET  && cRowsObtained > 0 )
	{
		hasError("Rowset GetNextRows", S_OK);		//仅仅是为了让snd_pac记一下S_OK这个返回码
	} else  {
		if ( hasError("Rowset GetNextRows", hr ) )	//一条记录都没有的话, 这里返DB_S_ENDOFROWSET错误
			goto END;
	}
	// Loop over rows obtained, getting data for each.
	for (iRow=0; iRow < cRowsObtained; iRow++)
	{
		rowData.reset();
		k = para_offset; 
		if ( hasError("Rowset GetData", pIRowset->GetData(rghRows[iRow], hAccessor_qry, rowData.base)))
			break;
		for (j = 0; j < defNum; j++)
		{
			DBFace::Para *para;
			int rNo ;

			unsigned char *buf;
			ULONG 	blen;
			DBSTATUS dbStatus;

			/* 找一个已经定义的 */
			para = findDefOut(k);
			if ( !para ) break;
			//k++;		/* 为下一个参数准备 */

			memcpy(&dbStatus, defns[j].obStatus + rowData.base,  sizeof(DBSTATUS));
			if ( dbStatus == DBSTATUS_S_OK ||  dbStatus == DBSTATUS_S_TRUNCATED )
			{
				memcpy(&blen, defns[j].obLength + rowData.base,  sizeof(ULONG));
				buf = defns[j].obValue + rowData.base;

				rNo = para->fld + dbface->offset;
				snd_pac->input(rNo, buf, blen);
			}  else  if ( dbStatus != DBSTATUS_S_ISNULL )
				showStatus(iRow+1, j+1, dbStatus);
		}
		aptus->sponte(&dopac_pius);
	}

	if ( dbface->rowset  && dbface->rowset->useEnd  )
		aptus->sponte(&end_ps);

	// 释放行句柄
	pIRowset->ReleaseRows(cRowsObtained, rghRows, NULL, NULL, NULL);
END:
	return  cRowsObtained;
}

/* 找一个定义输出的 */
DBFace::Para *Consumer::findDefOut(int &from )
{
	int k = from;
	DBFace::Para *pa = 0;

	for (; k < PARA_MAX && k < (TEXTUS_LONG) dbface->num ;k++ )
	{
		pa = & (dbface->paras[k]);
		if (  pa->inout == DBFace::PARA_OUT )
			break;
	}

	if ( k == PARA_MAX || k == (TEXTUS_LONG) dbface->num ) 	/* 没有找到相应的定义 */
		pa = 0;

	from = k+1;	/* 下一个 */
	return pa;
}

void Consumer::showStatus( DBCOUNTITEM row, DBCOUNTITEM col, DBSTATUS st)
{
#define PRINT(X) if ( st == X ) { WLOG(ERR, "GetData row " TLONG_FMT ", column " TLONG_FMT ", status is  %s\n", row, col, #X); return; }
	PRINT( DBSTATUS_S_OK );
	PRINT( DBSTATUS_E_BADACCESSOR );
	PRINT( DBSTATUS_E_CANTCONVERTVALUE );
	PRINT( DBSTATUS_S_ISNULL );
	PRINT( DBSTATUS_S_TRUNCATED );
	PRINT( DBSTATUS_E_SIGNMISMATCH );
	PRINT( DBSTATUS_E_DATAOVERFLOW );
	PRINT( DBSTATUS_E_CANTCREATE );
	PRINT( DBSTATUS_E_UNAVAILABLE );
	PRINT( DBSTATUS_E_PERMISSIONDENIED );
	PRINT( DBSTATUS_E_INTEGRITYVIOLATION );
	PRINT( DBSTATUS_E_SCHEMAVIOLATION );
	PRINT( DBSTATUS_E_BADSTATUS );
	PRINT( DBSTATUS_S_DEFAULT );
	PRINT( MDSTATUS_S_CELLEMPTY );
	PRINT( DBSTATUS_S_IGNORE );
	PRINT( DBSTATUS_E_DOESNOTEXIST );
	PRINT( DBSTATUS_E_INVALIDURL );
	PRINT( DBSTATUS_E_RESOURCELOCKED );
	PRINT( DBSTATUS_E_RESOURCEEXISTS );
	PRINT( DBSTATUS_E_CANNOTCOMPLETE );
	PRINT( DBSTATUS_E_VOLUMENOTFOUND );
	PRINT( DBSTATUS_E_OUTOFSPACE );
	PRINT( DBSTATUS_S_CANNOTDELETESOURCE );
	PRINT( DBSTATUS_E_READONLY );
	PRINT( DBSTATUS_E_RESOURCEOUTOFSCOPE );
	PRINT( DBSTATUS_S_ALREADYEXISTS );
	PRINT( DBSTATUS_E_CANCELED );
	PRINT( DBSTATUS_E_NOTCOLLECTION );
	PRINT( DBSTATUS_S_ROWSETCOLUMN );
}

bool Consumer::hasError(const char*msg, HRESULT hr)
{
	bool ret = true;
	const char *err_name;

	if ( snd_pac && dbface->errCode_field >= 0
		&& snd_pac->max >= dbface->errCode_field) 
	{
		snd_pac->input((unsigned int) dbface->errCode_field, (unsigned char*)&hr, sizeof(hr));
	}

	if (hr == DB_S_ENDOFROWSET)
	{
		if ( snd_pac && dbface->errStr_field >= 0
			&& snd_pac->max >= dbface->errStr_field) 
		{
			snd_pac->input((unsigned int) dbface->errStr_field, (unsigned char*)" ", 1);
		}
		WLOG(NOTICE, "%s of Error=[0x%08x]", msg, hr);
		return true;
	}
	if ( hr == S_OK )
		ret = false;
	else 
	{
		ret = true;
		err_name = getErrorName(hr);
		if ( err_name )
		{
			WLOG(ERR,"%s of %s [0x%08x]", msg, err_name,hr);
		} else {
			WLOG(ERR, "%s of Error=[0x%08x]", msg, hr);
		}
		handleError(hr);
	}
	return ret;
}

#define VALUE_CHAR(value) value, #value
#define NUMELE(rgEle) (sizeof(rgEle) / sizeof(rgEle[0]))
////////////////////////////////////////////////////////////////////////
// ERRORMAP
//
////////////////////////////////////////////////////////////////////////
typedef struct _ERRORMAP
{
	HRESULT		hr;			// HRESULT
	char*		pwszError;	// Name
} ERRORMAP;

ERRORMAP rgErrorMap[] =
{
	VALUE_CHAR(NULL),

	//System Errors
	VALUE_CHAR(E_FAIL),
	VALUE_CHAR(E_INVALIDARG),
	VALUE_CHAR(E_OUTOFMEMORY),
	VALUE_CHAR(E_NOINTERFACE),
	VALUE_CHAR(REGDB_E_CLASSNOTREG),
	VALUE_CHAR(CLASS_E_NOAGGREGATION),
	VALUE_CHAR(E_UNEXPECTED),
	VALUE_CHAR(E_NOTIMPL),
	VALUE_CHAR(E_POINTER),
	VALUE_CHAR(E_HANDLE),
	VALUE_CHAR(E_ABORT),
	VALUE_CHAR(E_ACCESSDENIED),
	VALUE_CHAR(E_PENDING),

	//OLE DB Errors
	VALUE_CHAR(DB_E_BADACCESSORHANDLE),
	VALUE_CHAR(DB_E_ROWLIMITEXCEEDED),
	VALUE_CHAR(DB_E_READONLYACCESSOR),
	VALUE_CHAR(DB_E_SCHEMAVIOLATION),
	VALUE_CHAR(DB_E_BADROWHANDLE),
	VALUE_CHAR(DB_E_OBJECTOPEN),
	VALUE_CHAR(DB_E_CANTCONVERTVALUE),
	VALUE_CHAR(DB_E_BADBINDINFO),
	VALUE_CHAR(DB_SEC_E_PERMISSIONDENIED),
	VALUE_CHAR(DB_E_NOTAREFERENCECOLUMN),
	VALUE_CHAR(DB_E_NOCOMMAND),
	VALUE_CHAR(DB_E_BADBOOKMARK),
	VALUE_CHAR(DB_E_BADLOCKMODE),
	VALUE_CHAR(DB_E_PARAMNOTOPTIONAL),
	VALUE_CHAR(DB_E_BADCOLUMNID),
	VALUE_CHAR(DB_E_BADRATIO),
	VALUE_CHAR(DB_E_ERRORSINCOMMAND),
	VALUE_CHAR(DB_E_CANTCANCEL),
	VALUE_CHAR(DB_E_DIALECTNOTSUPPORTED),
	VALUE_CHAR(DB_E_DUPLICATEDATASOURCE),
	VALUE_CHAR(DB_E_CANNOTRESTART),
	VALUE_CHAR(DB_E_NOTFOUND),
	VALUE_CHAR(DB_E_NEWLYINSERTED),
	VALUE_CHAR(DB_E_UNSUPPORTEDCONVERSION),
	VALUE_CHAR(DB_E_BADSTARTPOSITION),
	VALUE_CHAR(DB_E_NOTREENTRANT),
	VALUE_CHAR(DB_E_ERRORSOCCURRED),
	VALUE_CHAR(DB_E_NOAGGREGATION),
	VALUE_CHAR(DB_E_DELETEDROW),
	VALUE_CHAR(DB_E_CANTFETCHBACKWARDS),
	VALUE_CHAR(DB_E_ROWSNOTRELEASED),
	VALUE_CHAR(DB_E_BADSTORAGEFLAG),
	VALUE_CHAR(DB_E_BADSTATUSVALUE),
	VALUE_CHAR(DB_E_CANTSCROLLBACKWARDS),
	VALUE_CHAR(DB_E_MULTIPLESTATEMENTS),
	VALUE_CHAR(DB_E_INTEGRITYVIOLATION),
	VALUE_CHAR(DB_E_ABORTLIMITREACHED),
	VALUE_CHAR(DB_E_DUPLICATEINDEXID),
	VALUE_CHAR(DB_E_NOINDEX),
	VALUE_CHAR(DB_E_INDEXINUSE),
	VALUE_CHAR(DB_E_NOTABLE),
	VALUE_CHAR(DB_E_CONCURRENCYVIOLATION),
	VALUE_CHAR(DB_E_BADCOPY),
	VALUE_CHAR(DB_E_BADPRECISION),
	VALUE_CHAR(DB_E_BADSCALE),
	VALUE_CHAR(DB_E_BADID),
	VALUE_CHAR(DB_E_BADTYPE),
	VALUE_CHAR(DB_E_DUPLICATECOLUMNID),
	VALUE_CHAR(DB_E_DUPLICATETABLEID),
	VALUE_CHAR(DB_E_TABLEINUSE),
	VALUE_CHAR(DB_E_NOLOCALE),
	VALUE_CHAR(DB_E_BADRECORDNUM),
	VALUE_CHAR(DB_E_BOOKMARKSKIPPED),
	VALUE_CHAR(DB_E_BADPROPERTYVALUE),
	VALUE_CHAR(DB_E_INVALID),
	VALUE_CHAR(DB_E_BADACCESSORFLAGS),
	VALUE_CHAR(DB_E_BADSTORAGEFLAGS),
	VALUE_CHAR(DB_E_BYREFACCESSORNOTSUPPORTED),
	VALUE_CHAR(DB_E_NULLACCESSORNOTSUPPORTED),
	VALUE_CHAR(DB_E_NOTPREPARED),
	VALUE_CHAR(DB_E_BADACCESSORTYPE),
	VALUE_CHAR(DB_E_WRITEONLYACCESSOR),
	VALUE_CHAR(DB_SEC_E_AUTH_FAILED),
	VALUE_CHAR(DB_E_CANCELED),
	VALUE_CHAR(DB_E_BADSOURCEHANDLE),
	VALUE_CHAR(DB_E_PARAMUNAVAILABLE),
	VALUE_CHAR(DB_E_ALREADYINITIALIZED),
	VALUE_CHAR(DB_E_NOTSUPPORTED),
	VALUE_CHAR(DB_E_MAXPENDCHANGESEXCEEDED),
	VALUE_CHAR(DB_E_BADORDINAL),
	VALUE_CHAR(DB_E_PENDINGCHANGES),
	VALUE_CHAR(DB_E_DATAOVERFLOW),
	VALUE_CHAR(DB_E_BADHRESULT),
	VALUE_CHAR(DB_E_BADLOOKUPID),
	VALUE_CHAR(DB_E_BADDYNAMICERRORID),
	VALUE_CHAR(DB_E_PENDINGINSERT),
	VALUE_CHAR(DB_E_BADCONVERTFLAG),
	VALUE_CHAR(DB_S_ROWLIMITEXCEEDED),
	VALUE_CHAR(DB_S_COLUMNTYPEMISMATCH),
	VALUE_CHAR(DB_S_TYPEINFOOVERRIDDEN),
	VALUE_CHAR(DB_S_BOOKMARKSKIPPED),
	VALUE_CHAR(DB_S_ENDOFROWSET),
	VALUE_CHAR(DB_S_COMMANDREEXECUTED),
	VALUE_CHAR(DB_S_BUFFERFULL),
	VALUE_CHAR(DB_S_NORESULT),
	VALUE_CHAR(DB_S_CANTRELEASE),
	VALUE_CHAR(DB_S_DIALECTIGNORED),
	VALUE_CHAR(DB_S_UNWANTEDPHASE),
	VALUE_CHAR(DB_S_UNWANTEDREASON),
	VALUE_CHAR(DB_S_COLUMNSCHANGED),
	VALUE_CHAR(DB_S_ERRORSRETURNED),
	VALUE_CHAR(DB_S_BADROWHANDLE),
	VALUE_CHAR(DB_S_DELETEDROW),
	VALUE_CHAR(DB_S_STOPLIMITREACHED),
	VALUE_CHAR(DB_S_LOCKUPGRADED),
	VALUE_CHAR(DB_S_PROPERTIESCHANGED),
	VALUE_CHAR(DB_S_ERRORSOCCURRED),
	VALUE_CHAR(DB_S_PARAMUNAVAILABLE),
	VALUE_CHAR(DB_S_MULTIPLECHANGES),
};


////////////////////////////////////////////////////////////////////////
// WCHAR* GetErrorName
//
////////////////////////////////////////////////////////////////////////
const char* Consumer::getErrorName(HRESULT hr)
{
	for(ULONG i=0; i<NUMELE(rgErrorMap); i++)	
	{
		if(hr == rgErrorMap[i].hr) 
			return rgErrorMap[i].pwszError;
	}

	//Otherwise just return Unknown
	return rgErrorMap[0].pwszError;
}


// myHandleResult
//
//	This function is called as part of the XCHECK_HR macro; it takes a
//	HRESULT, which is returned by the method called in the XCHECK_HR
//	macro, and the file and line number where the method call was made.
//	If the method call failed, this function attempts to get and display
//	the extended error information for the call from the IErrorInfo,
//	IErrorRecords, and ISQLErrorInfo interfaces.
//
////////////////////////////////////////////////////////////////////////
void Consumer::handleError ( HRESULT	hrReturned )
{
	HRESULT		hr;
	IErrorInfo *	pIErrorInfo	= NULL;
	IErrorRecords *	pIErrorRecords	= NULL;
	ULONG		cRecords;
	ULONG		iErr;

	// If the method called as part of the XCHECK_HR macro failed,
	// we will attempt to get extended error information for the call
	if( FAILED(hrReturned) )
	{
		// Obtain the current Error object, if any, by using the
		// OLE Automation GetErrorInfo function, which will give
		// us back an IErrorInfo interface pointer if successful
		hr = GetErrorInfo(0, &pIErrorInfo);

		// We've got the IErrorInfo interface pointer on the Error object
		if( SUCCEEDED(hr) && pIErrorInfo )
		{
			// OLE DB extends the OLE Automation error model by allowing
			// Error objects to support the IErrorRecords interface; this
			// interface can expose information on multiple errors.
			hr = pIErrorInfo->QueryInterface(IID_IErrorRecords, 
						(void**)&pIErrorRecords);
			if( SUCCEEDED(hr) )
			{
				// Get the count of error records from the object
				CHECK_HR(hr = pIErrorRecords->GetRecordCount(&cRecords));
				
				// Loop through the set of error records and
				// display the error information for each one
				for( iErr = 0; iErr < cRecords; iErr++ )
				{
					displayErrorRecord(hrReturned, iErr, pIErrorRecords);
				}
			}
			// The object didn't support IErrorRecords; display
			// the error information for this single error
			else
			{
				displayErrorInfo(hrReturned, pIErrorInfo);
			}
		}
	}

CLEANUP:
	if( pIErrorInfo )
		pIErrorInfo->Release();
	if( pIErrorRecords )
		pIErrorRecords->Release();
}


////////////////////////////////////////////////////////////////////////
// myDisplayErrorRecord
//
//	This function displays the error information for a single error
//	record, including information from ISQLErrorInfo, if supported
//
////////////////////////////////////////////////////////////////////////
HRESULT Consumer::displayErrorRecord
	(
	HRESULT		hrReturned, 
	ULONG		iRecord, 
	IErrorRecords *	pIErrorRecords 
	)
{
	HRESULT		hr;
	IErrorInfo *	pIErrorInfo	= NULL;
	BSTR		bstrDescription	= NULL;
	BSTR		bstrSource	= NULL;
	BSTR		bstrSQLInfo	= NULL;

	static LCID	lcid		= GetUserDefaultLCID();

	LONG		lNativeError	= 0;
	ERRORINFO	ErrorInfo;
	char str1[1024], str2[1024], str3[1024];

	// Get the IErrorInfo interface pointer for this error record
	CHECK_HR(hr = pIErrorRecords->GetErrorInfo(iRecord, lcid, &pIErrorInfo));
	
	// Get the description of this error
	CHECK_HR(hr = pIErrorInfo->GetDescription(&bstrDescription));
		
	// Get the source of this error
	CHECK_HR(hr = pIErrorInfo->GetSource(&bstrSource));

	// Get the basic error information for this record
	CHECK_HR(hr = pIErrorRecords->GetBasicErrorInfo(iRecord, &ErrorInfo));

	// If the error object supports ISQLErrorInfo, get this information
	getSqlErrorInfo(iRecord, pIErrorRecords, &bstrSQLInfo, &lNativeError);

	// Display the error information to the user
	if( bstrSQLInfo )
	{
		/* 
		wprintf(L"\nErrorRecord:  HResult: 0x%08x\nDescription: %s\n"
			L"SQLErrorInfo: %s\nSource: %s\n", 
			ErrorInfo.hrError, 
			bstrDescription, 
			bstrSQLInfo, 
			bstrSource);
		*/
		WideCharToMultiByte( CP_ACP, 0, bstrDescription, -1, str1, 1024, NULL, NULL );	
		WideCharToMultiByte( CP_ACP, 0, bstrSQLInfo, -1, str2, 1024, NULL, NULL );	
		WideCharToMultiByte( CP_ACP, 0, bstrSource, -1, str3, 1024, NULL, NULL );	
		if ( snd_pac && dbface->errStr_field >= 0
			&& snd_pac->max >= dbface->errStr_field) 
		{
			snd_pac->input((unsigned int) dbface->errStr_field, (unsigned char*)str1, strlen(str1));
		}
		WLOG(ERR, "HRESULT: 0x%08x,  Description: %s SQLErrorInfo: %s,  Source: %s",
			ErrorInfo.hrError, str1, str2, str3);
	} else {
		/*
		wprintf(L"\nErrorRecord:  HResult: 0x%08x\nDescription: %s\n"
			L"Source: %s\n", 
			ErrorInfo.hrError, 
			bstrDescription, 
			bstrSource);
		*/
		WideCharToMultiByte( CP_ACP, 0, bstrDescription, -1, str1, 1024, NULL, NULL );	
		WideCharToMultiByte( CP_ACP, 0, bstrSource, -1, str3, 1024, NULL, NULL );	
		if ( snd_pac && dbface->errStr_field >= 0
			&& snd_pac->max >= dbface->errStr_field) 
		{
			snd_pac->input((unsigned int) dbface->errStr_field, (unsigned char*)str1, strlen(str1));
		}
		WLOG(ERR, "ErrorRecord:  HResult: 0x%08x,  Description: %s  Source: %s",
			ErrorInfo.hrError, str1, str3);
	}

CLEANUP:
	if( pIErrorInfo )
		pIErrorInfo->Release();
	SysFreeString(bstrDescription);
	SysFreeString(bstrSource);
	SysFreeString(bstrSQLInfo);
	return hr;
}


////////////////////////////////////////////////////////////////////////
// myDisplayErrorInfo
//
//	This function displays basic error information for an error object
//	that doesn't support the IErrorRecords interface
//
////////////////////////////////////////////////////////////////////////
HRESULT Consumer::displayErrorInfo
	(
	HRESULT		hrReturned, 
	IErrorInfo *	pIErrorInfo
	)
{
	HRESULT		hr;
	BSTR		bstrDescription	= NULL;
	BSTR		bstrSource	= NULL;
	char str1[1024], str3[1024];

	// Get the description of the error
	CHECK_HR(hr = pIErrorInfo->GetDescription(&bstrDescription));
		
	// Get the source of the error -- this will be the window title
	CHECK_HR(hr = pIErrorInfo->GetSource(&bstrSource));

	// Display this error information
	/*
	wprintf(L"\nErrorInfo:  HResult: 0x%08x, Description: %s\nSource: %s\n",
		hrReturned, bstrDescription, bstrSource);
	*/
	WideCharToMultiByte( CP_ACP, 0, bstrDescription, -1, str1, 1024, NULL, NULL );	
	WideCharToMultiByte( CP_ACP, 0, bstrSource, -1, str3, 1024, NULL, NULL );	
	if ( snd_pac && dbface->errStr_field >= 0
		&& snd_pac->max >= dbface->errStr_field) 
	{
		snd_pac->input((unsigned int) dbface->errStr_field, (unsigned char*)str1, strlen(str1));
	}
	WLOG(ERR, "HResult: 0x%08x,  Description: %s  Source: %s",
		hrReturned, str1, str3);

CLEANUP:
	SysFreeString(bstrDescription);
	SysFreeString(bstrSource);
	return hr;
}


////////////////////////////////////////////////////////////////////////
// myGetSqlErrorInfo
//
//	If the error object supports ISQLErrorInfo, get the SQL error
//	string and native error code for this error
//
////////////////////////////////////////////////////////////////////////
HRESULT Consumer::getSqlErrorInfo
	(
	ULONG					iRecord, 
	IErrorRecords *			pIErrorRecords, 
	BSTR *					pBstr, 
	LONG *					plNativeError
	)
{
	HRESULT					hr;
	ISQLErrorInfo *			pISQLErrorInfo				= NULL;
	LONG					lNativeError				= 0;

	// Attempt to get the ISQLErrorInfo interface for this error
	// record through GetCustomErrorObject. Note that ISQLErrorInfo
	// is not mandatory, so failure is acceptable here
	CHECK_HR(hr = pIErrorRecords->GetCustomErrorObject(
				iRecord,						//iRecord
				IID_ISQLErrorInfo,				//riid
				(IUnknown**)&pISQLErrorInfo		//ppISQLErrorInfo
				));

	// If we obtained the ISQLErrorInfo interface, get the SQL
	// error string and native error code for this error
	if( pISQLErrorInfo )
		hr = pISQLErrorInfo->GetSQLInfo(pBstr, &lNativeError);

CLEANUP:
	if( plNativeError )
		*plNativeError = lNativeError;
	if( pISQLErrorInfo )
		pISQLErrorInfo->Release();
	return hr;
}

#include "hook.c"
