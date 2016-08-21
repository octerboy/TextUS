/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: DataBase Port
 Build: created by octerboy, 2006/11/27, Guangzhou
 $Header: /textus/dbport/DBPort.cpp 39    13-09-23 14:29 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: DBPort.cpp $"
#define TEXTUS_MODTIME  "$Date: 13-09-23 14:29 $"
#define TEXTUS_BUILDNO  "$Revision: 39 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "casecmp.h"
#include "DBFace.h"
#include "BTool.h"
#include "PacData.h"
#include <stdarg.h>

#define INLINE inline
class DBPort: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	DBPort();
	~DBPort();

private:
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	int ref_count_when_rset;
	DBPort *refRowset;
	

	Amor::Pius setdb_ps, dbfetch_ps, setpac_ps, err_ps;
	PacketObj *pacs[3];
	struct G_CFG {
		int fcNum;
		struct DBFace *faces;
		DBPort **filius;
		int fil_size;
		int top;

		int	errCode_field;	/* 错误代码所存放的PacketObj域号, < 0 表示不需要 */
		int	errStr_field;	/* 错误信息所存放的PacketObj域号, < 0 表示不需要 */
	
		int	cRows_field;	/* 记录数所存放的PacketObj域号, < 0 表示不需要 */
		int 	cRowsObt_fld;	/* 取了多少行, 在哪个域 */

		inline G_CFG () {
			faces = 0;
			fcNum=0;
			top = 0;
			fil_size = 8;
			filius = new DBPort* [fil_size];
			memset(filius, 0, sizeof(DBPort*)*fil_size);

			errCode_field = errStr_field =  cRows_field = cRowsObt_fld = -1;
		};

		inline ~G_CFG () {
			delete[] filius;
			top = 0;
			if (faces) 
				delete[] faces;
		};

		inline void put(DBPort *p) {
			int i;
			for (i =0; i < top; i++)
			{
				if ( p == filius[i] ) 
					return ;
			}
			filius[top] = p;
			top++;
			if ( top == fil_size) 
			{
				DBPort **tmp;
				tmp = new DBPort *[fil_size*2];
				memcpy(tmp, filius, fil_size*(sizeof(DBPort *)));
				fil_size *=2;
				delete[] filius;
				filius = tmp;
			}
		};

		inline DBPort* get(DBPort *p) {
			int i;
			for (i =0; i < top; i++)
			{
				if ( p == filius[i] ) 
					return p;
			}
			return (DBPort *) 0;
		};

		inline void remove(DBPort *p) {
			int i, j, k;
			k = 0;
			for (i =0; i < top; i++)
			{
				if ( p == filius[i] ) 
				{
					for (  j = i; j+1 < top; j++)
					{
						filius[j] = filius[j+1];
					}
					filius[j] = (DBPort *) 0;
					k++;
				}
			}
			top -= k;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	void getDef( struct DBFace *, TiXmlElement *cfg, DBFace::WHAT gin, DBFace::WHAT gout, int goffset, int gtrace);
	void handle_face( struct DBFace *face, Amor::Pius *pius);
	INLINE void passto( DBPort *, struct DBFace *);
	INLINE void incrRef();
	INLINE void decrRef();

	Amor::Pius cancel_db_ps;
		
#include "wlog.h"
};

#define WHATPAC(X,Y) \
	comm_str = cfg->Attribute(X);	\
	if ( comm_str )				\
	{					\
		if (strcasecmp(comm_str, "first") == 0 ) 	\
			Y = DBFace::FIRST;		\
								\
		if (strcasecmp(comm_str, "second") == 0 ) 	\
			Y = DBFace::SECOND;		\
	}

void DBPort::ignite(TiXmlElement *cfg) 
{
	int i;
	const char *comm_str;
	TiXmlElement *fc_ele;

	DBFace::WHAT gin, gout;
	int gtrace = -1;
	int goffset = 0;

	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	if ( gCFG->faces)	
		delete[] gCFG->faces;
	fc_ele= cfg->FirstChildElement("face"); gCFG->fcNum = 0;
	while(fc_ele)
	{
		gCFG->fcNum++;
		fc_ele = fc_ele->NextSiblingElement("face");
	}

	if ( gCFG->fcNum == 0 )
		return;
	
	cfg->QueryIntAttribute("trace", &(gtrace));
	gin = DBFace::FIRST;		
	gout = DBFace::SECOND;
	WHATPAC("in", gin)
	WHATPAC("out", gout)

	if ( (comm_str = cfg->Attribute("offset")) && atoi(comm_str) > 0 )
		goffset = atoi(comm_str);

	cfg->QueryIntAttribute("error", &gCFG->errCode_field );
	cfg->QueryIntAttribute("errorInfo", &gCFG->errStr_field );
	cfg->QueryIntAttribute("rowsCount", &gCFG->cRows_field);
	cfg->QueryIntAttribute("rowsFetched", &gCFG->cRowsObt_fld);
	gCFG->faces= new struct DBFace[gCFG->fcNum];
	fc_ele= cfg->FirstChildElement("face"); i = 0;
	while(fc_ele)
	{
		getDef(&gCFG->faces[i], fc_ele, gin, gout, goffset, gtrace);
		i++;
		fc_ele = fc_ele->NextSiblingElement("face");
	}
}

/*  取得某个face的定义 */
void DBPort::getDef(struct DBFace *face, TiXmlElement *cfg, DBFace::WHAT gin, DBFace::WHAT gout, int goffset, int gtrace)
{
	const char *comm_str;
	TiXmlElement *fld_ele, *ref_ele, *rs_ele;
	DBFace::Para *par;

	ref_ele= cfg->FirstChildElement("reference"); 
	if ( ref_ele )
	{
		comm_str = ref_ele->Attribute("field");	/* 参考域: 与此域内容相同即使用此face*/
		if ( comm_str && atoi(comm_str) >= 0)	
		{
			face->ref.fldNo = atoi(comm_str);
			if (  ref_ele->GetText() )
			{
				face->ref.content = new unsigned char[strlen(ref_ele->GetText()) +1 ];
				face->ref.len = BTool::unescape(ref_ele->GetText(), face->ref.content);
			}
		}
	}
	
	rs_ele = cfg->FirstChildElement("rowset");
	if ( rs_ele )
	{
		int m_chunk ;
		face->rowset = new struct DBFace::Rowset;
		if  ( gtrace >= 0 )
			face->rowset->trace_field = gtrace;
		
		rs_ele->Attribute("param_pos", (int*)&(face->rowset->para_pos));
		rs_ele->Attribute("trace", (int*)&(face->rowset->trace_field));

		if ( rs_ele->QueryIntAttribute("chunk", &(m_chunk)) != TIXML_NO_ATTRIBUTE )
		{
			if ( m_chunk > 0 ) 
				face->rowset->chunk = (unsigned int) m_chunk;
		}

		face->rowset->useEnd =  ( (comm_str= rs_ele->Attribute("endQuery")) && strcasecmp(comm_str, "yes") == 0 );
	}
	face->id_name = cfg->Attribute("name");

	comm_str = cfg->Attribute("type");
	if ( comm_str )	
	{
		if (strcasecmp(comm_str, "procedure") == 0 ) 
			face->pro = DBFace::DBPROC;

		if (strcasecmp(comm_str, "function") == 0 ) 
			face->pro = DBFace::FUNC;

		if (strcasecmp(comm_str, "query") == 0 ) 
			face->pro = DBFace::QUERY;

		if (strcasecmp(comm_str, "dml") == 0 ) 
			face->pro = DBFace::DML;

		if (strcasecmp(comm_str, "fetch") == 0 ) 
			face->pro = DBFace::FETCH;

		if (strcasecmp(comm_str, "cursor") == 0 ) 
			face->pro = DBFace::CURSOR;
	}

	face->sentence = cfg->Attribute("statement");
	if ( !face->sentence )
	{
		if ( cfg->FirstChildElement("statement") )
			face->sentence = cfg->FirstChildElement("statement")->GetText();
	}

	/* 对于FETCH这样的face, 还是继续下面的处理, 以便获得errCode_field等, 象cardmaker这样的程序就方便些 */
	//if ( face->pro ==DBFace::FETCH || !face->sentence )
	//	return;

	face->offset = goffset;
	if ( (comm_str = cfg->Attribute("offset")) && atoi(comm_str) > 0 )
		face->offset = atoi(comm_str);

	face->in = gin;
	face->out = gout;
	WHATPAC("in", face->in)
	WHATPAC("out", face->out)

	face->errCode_field	= gCFG->errCode_field ;
	face->errStr_field	= gCFG->errStr_field;
	face->cRows_field	= gCFG->cRows_field;
	face->cRowsObt_fld	= gCFG->cRowsObt_fld ;

	cfg->QueryIntAttribute("error", &face->errCode_field );
	cfg->QueryIntAttribute("errorInfo", &face->errStr_field );
	cfg->QueryIntAttribute("rowsCount", &face->cRows_field);
	cfg->QueryIntAttribute("rowsFetched", &face->cRowsObt_fld);

	if ( face->errCode_field >=0 )
		face->errCode_field += face->offset;

	if ( face->errStr_field >=0 )
		face->errStr_field += face->offset;

	if ( face->cRows_field >=0 )
		face->cRows_field += face->offset;

	if ( face->cRowsObt_fld >=0 )
		face->cRowsObt_fld += face->offset;

	fld_ele= cfg->FirstChildElement(); face->num = 0;
	while(fld_ele)
	{
		if ( fld_ele->Value() )
		{
			if (	strcasecmp(fld_ele->Value(), "in") ==0 
			    ||	strcasecmp(fld_ele->Value(), "out") ==0 
			    ||	strcasecmp(fld_ele->Value(), "inout") ==0 )

				face->num++;
		}
		fld_ele = fld_ele->NextSiblingElement();
	}

	if ( face->num == 0 )
		return;

	face->paras = new DBFace::Para [face->num];

	/* for each parameter  */
	fld_ele= cfg->FirstChildElement();
	par = &(face->paras[0]) ; face->num= 0;
	par->pos = 0;
	while(fld_ele)
	{
		DBFace::DIRECTION i_o;
		i_o = DBFace::UNKNOWN ;
#define WHATDIR(X) if ( strcasecmp(fld_ele->Value(), #X) ==0  ) i_o = DBFace::PARA_##X;
		WHATDIR(IN)
		WHATDIR(OUT)
		WHATDIR(INOUT)
		
		if ( i_o == DBFace::UNKNOWN )	
			goto NEXTELE;

		par->inout = i_o;

		if ( (comm_str = fld_ele->Attribute("field")) && atoi(comm_str) >= 0 )
			par->fld = atoi(comm_str);

		comm_str =  fld_ele->Attribute("name");
		if ( comm_str)
		{
			par->name = comm_str;
			par->namelen = strlen(comm_str);
		}

#define WHATCODE(X)	if ( (comm_str = fld_ele->Attribute("type") ) && strcasecmp(comm_str, #X) ==0 ) \
				par->type = DBFace::X;
				
		WHATCODE(BigInt)
		WHATCODE(SmallInt)
		WHATCODE(TinyInt)
		WHATCODE(Binary)
		WHATCODE(Boolean)
		WHATCODE(Byte)
		WHATCODE(Char)
		WHATCODE(Currency)
		WHATCODE(Date)
		WHATCODE(Decimal)
		WHATCODE(Double)
		WHATCODE(Float)
		WHATCODE(GUID)
		WHATCODE(Integer)
		WHATCODE(Long)
		WHATCODE(LongBinary)
		WHATCODE(Memo)
		WHATCODE(Numeric)
		WHATCODE(Single)
		WHATCODE(Text)
		WHATCODE(Time)
		WHATCODE(TimeStamp)
		WHATCODE(VarBinary)
				
		if ( (comm_str = fld_ele->Attribute("length")) && atoi(comm_str) >= 0 )
			par->outlen = atoi(comm_str);

		if ( (comm_str = fld_ele->Attribute("scale")) && atoi(comm_str) >= 0 )
			par->scale = atoi(comm_str);

		if ( (comm_str = fld_ele->Attribute("precision")) && atoi(comm_str) >= 0 )
			par->precision = atoi(comm_str);

		if ( par->inout == DBFace::PARA_OUT )
		{
			face->outNum  += 1;
			face->outSize += par->outlen;
		}
		par->pos = face->num;

		face->num++;
		par++;

	NEXTELE:
		fld_ele = fld_ele->NextSiblingElement();
	}
}

bool DBPort::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	void **ind;
	const char *fname;
	struct DBFace *face ;
	int i;
	Amor::Pius pac_ps;
	switch ( pius->ordo )
	{
	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("facio SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("facio SET_UNIPAC snd_pac null");
			}

			aptus->facio(pius);
		} else {
			WBUG("facio SET_UNIPAC null");
		}
		break;

	case Notitia::CMD_GET_DBFACE:
		WBUG("facio CMD_GET_DBFACE");
		ind = (void**)pius->indic;
		fname = (const char*)ind[0];
		ind[1] = 0;
		for ( i = 0, face = gCFG->faces; i < gCFG->fcNum; i++, face++ )
		{
			if ( face->id_name )
			{
				if ( strcmp(face->id_name, fname) == 0 ) 
				{
					ind[1] = face;
					break;
				}
			}
		}
		
		break;

	case Notitia::PRO_DBFACE:
		WBUG("facio PRO_DBFACE");
		face = (struct DBFace*)pius->indic;
		pac_ps.ordo = Notitia::PRO_UNIPAC;
		pac_ps.indic = 0;
		for ( i = 0; i < gCFG->fcNum; i++)
		{
			if ( face == &gCFG->faces[i] )	//找一个face地址是本模块内的。不在的， 不处理
				break;
		}
		if ( i < gCFG->fcNum )	
		{
			handle_face(face, &pac_ps);
		}
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");

		if ( !rcv_pac || !snd_pac )
		{
			WLOG(WARNING, "rcv_pac or snd_pac is null");
			break;
		}
		assert(gCFG);

		for ( i = 0, face = gCFG->faces; i < gCFG->fcNum; i++, face++ )
		{
			bool matched = false;	/* 指示是否满足当前参考域 */
			if ( face->ref.fldNo < 0 ) 	/* 无参考即认为满足条件 */
				matched = true;
			else if ( rcv_pac && rcv_pac->max >= face->ref.fldNo )
			{
				FieldObj  *fld = &(rcv_pac->fld[face->ref.fldNo]);
				if ( fld->no >= 0 )
				{
					if ( face->ref.content )
					{
						if ( fld->range == face->ref.len && 
							memcmp(face->ref.content, fld->val, face->ref.len) == 0 )
							matched = true;
					} else	/* 无内容限制 */
						matched = false;
				}
			}

			if ( matched )
			{
				WBUG("matched dbface %d", i);
				handle_face(face, pius);
				break;
			}
		}
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		assert(gCFG);
		if ( refRowset )
		{ /* 先前引用了此行集对象, 减一次引用, 这里不再引用 */
			DBPort *that = 0;
			that = gCFG->get(refRowset);
			if (that)
				that->decrRef();
			refRowset = 0;
		}
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		break;
	default:
		return false;
	}

	return true;
}

void DBPort::handle_face( struct DBFace *face, Amor::Pius *pius)
{
	if ( face->pro == DBFace::FETCH )
	{
		DBPort *that=0, *that2 = 0;
		int &tr_fld = face->rowset->trace_field;

		if ( face->rowset  && tr_fld >= 0 
			&& rcv_pac && tr_fld < rcv_pac->max )
		{
			if ( rcv_pac->fld[tr_fld].no == tr_fld && rcv_pac->fld[tr_fld].range == sizeof(that) )
			{
				memcpy(&that2, rcv_pac->fld[tr_fld].val, sizeof(that));
				/* 寻找确认一下 */
				WBUG("FETCH dbface, rcv_pac.fld[%d] %p, refRowset %p", tr_fld, that2, refRowset);
				that = gCFG->get(that2);
			}

			if ( !that )	/* 已经没有这样的行集了 */
			{
				WLOG(WARNING, "No such rowset handle %p",  that2);
				snd_pac->input(face->errCode_field, (unsigned char*)"DBPort error", 12);
				snd_pac->input(face->errStr_field, (unsigned char*)"No such rowset handle", 21);
				aptus->sponte(&err_ps);
				goto RET;
			}

			if ( refRowset && refRowset != that )
			{	/*这次不同上次了,  上次的引用对象不要了 */
				DBPort *there = 0;
				there = gCFG->get(refRowset);
				if (there)
					there->decrRef();
				refRowset = 0;
			}

			if ( refRowset != that )
			{
				/* 第一次引用这个行集对象 */
				refRowset = that;
				that->incrRef();
			}

			passto( that, face);
		} else {
			that = this;
		}

		if ( that )
			that->aptus->facio(&dbfetch_ps);
	} else {
		setdb_ps.indic = face;	/* 设置DBFACE */

		if ( face->rowset  && face->rowset->trace_field >= 0 )
		{ /* 这是带一个结果集的, 且需要被多次取数, 那就另找一个DBPort对象, 由其处理 */
			Amor::Pius tmp_p;
		
			if ( refRowset )
			{	/* 先前引用了此行集对象, 减一次引用, 这里不再引用 */
				/* 只有在需要新行集时, 才把以前的抛弃, 
			   所以, 当一个行集产生时, 可以同时作其它操作: 
			   比如: 从一个行集取数据, 处理后插入某表 */
				DBPort *that = 0;
				that = gCFG->get(refRowset);
				if (that)
					that->decrRef();
				refRowset = 0;
			}

			tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
			tmp_p.indic = this;
			aptus->sponte(&tmp_p);
			/* 请求空闲的子实例 */
			if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
			{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
				WLOG(NOTICE, "limited rowset handles to max");
				goto RET;
			} else {
				DBPort *here =  (DBPort *)(tmp_p.indic);
				here->ref_count_when_rset = 1;
				gCFG->put(here); /* 入本地队列 */
				passto( here, face);	/* here与本类有相同的prius, 返回数据不操心啦 */
				here->aptus->facio(&setdb_ps);
				here->aptus->facio(pius);	/* 续传PRO_UNIPAC */
			}
		} else  {
			aptus->facio(&setdb_ps);
			aptus->facio(pius);	/* 续传PRO_UNIPAC */
		}
	}
RET:
	return;
}

bool DBPort::sponte( Amor::Pius *pius)
{
	WBUG("sponte %lu, indic %p", pius->ordo, pius->indic);
	return false;
}

DBPort::DBPort()
{
	gCFG=0;
	has_config = false;
	rcv_pac = 0;
	snd_pac = 0;
	ref_count_when_rset = 0;

	setdb_ps.ordo = Notitia::CMD_SET_DBFACE;
	dbfetch_ps.ordo = Notitia::CMD_DBFETCH;
	dbfetch_ps.indic = 0;

	setpac_ps.ordo = Notitia::SET_UNIPAC;
	setpac_ps.indic = pacs;

	err_ps.ordo = Notitia::ERR_UNIPAC_INFO;
	err_ps.indic = 0;
	
	cancel_db_ps.ordo = Notitia::CMD_DB_CANCEL;
	cancel_db_ps.indic = 0;
}

DBPort::~DBPort()
{
	if ( gCFG )
	{
		gCFG->remove(this);
		if ( has_config )
			delete gCFG;
	}
}

Amor* DBPort::clone()
{
	DBPort *child = new DBPort();
	child->gCFG = gCFG;
	return (Amor*)child;
}

void DBPort::passto(DBPort *here, DBFace *face)
{
	Amor::Pius tmp_p;
	pacs[0] = rcv_pac;
	pacs[1] = snd_pac;
	pacs[2] = 0;
	refRowset = here;
	here->facio(&setpac_ps);

	tmp_p.ordo = Notitia::SET_SAME_PRIUS;	/* rowset对象与这里相同的prius */
	tmp_p.indic = here;			/* 当数据返回时, 不用操心啦 */
	aptus->sponte(&tmp_p);

	snd_pac->input((unsigned int) face->rowset->trace_field, 	/* 设定跟踪域 */
		(unsigned char*) (&here), sizeof(here));
}

void DBPort::incrRef()
{
	ref_count_when_rset++;
}

void DBPort::decrRef()
{
	if (ref_count_when_rset > 0 )
	{
		ref_count_when_rset--;
		if (ref_count_when_rset == 0 )
		{
			Amor::Pius tmp_p;
			aptus->facio(&cancel_db_ps);	//通知数据库，取消当前命令结果
			tmp_p.ordo = Notitia::CMD_FREE_IDLE;
			tmp_p.indic = this;
			aptus->sponte(&tmp_p);
			gCFG->remove(this);
		}
	}
}
#include "hook.c"
