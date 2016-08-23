/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title:Internet Protocol Head pro
 Build: created by octerboy, 2007/08/02, Panyu
 $Header: /textus/montcp/NetTcp.cpp 8     10-12-11 15:29 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: NetTcp.cpp $"
#define TEXTUS_MODTIME  "$Date: 10-12-11 15:29 $"
#define TEXTUS_BUILDNO  "$Revision: 8 $"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>

#if !defined (_WIN32)
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
 #if !defined(__linux__) && !defined(_AIX)
 #include <strings.h>
 #endif
#else
#include <windows.h>
#define in_addr_t u_long
#endif 

#ifndef TINLINE
#define TINLINE inline
#endif

class NetTcp: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
		
	NetTcp();
	~NetTcp();

	enum FLAG { FIN=0x01, SYN=0x02, RST=0x04, PSH=0x08, ACK=0x10, URG=0x20 };

#include "PacData.h"
#include "wlog.h"
private:
	Amor::Pius pro_unipac, start_ps, end_ps;

	PacketObj *rcv_pac, *snd_pac;	/* 来自左(右)节点的PacketObj  */
	PacketObj first_pac, second_pac;	/* 向右(左)传递 */
	PacketObj *pa[3];

	TINLINE bool handle(PacketObj *rpac);
	TINLINE bool handle_sim(PacketObj *rpac);
	TINLINE bool match(PacketObj *mpac);
	bool initConn(NetTcp *fa, PacketObj *rpac);
	TINLINE void data_pro();
	TINLINE void endConn();
	void deliver(Notitia::HERE_ORDO aordo);
	
	/* 处理每一个已建立连接的通讯 */
	in_addr_t server_addr, client_addr;
	unsigned short int   server_port, client_port;

	struct G_CFG {
		int srcip_fld;		/* 源IP 域号 */
		int dstip_fld;		/* 目标IP 域号 */
		int srcport_fld;	/* 源PORT 域号 */
		int dstport_fld;	/* 目标PORT 域号 */

		int offset_fld;		/* offset 域号 */
		int flags_fld;		/* flags 域号 */
		
		int data_fld;		/* 数据域号， 包括option， 经过本模块处理后，这个域将不包括option*/
		int option_fld;		/* option域号, 如果TCP头中有option，处理后，该域有这个数据 */

		int direction_fld;	/* 表示方向的域号, 这个域为1字节, 0x01: 向server, 0x02: 向client */

		bool lonely;		/* true: 只一次性监听一个TCP连接; false: 监听多个TCP连接 */
		bool simply;		/* true: 直接显示, 判断目标IP、PORT与设定的服务器ip、port相符就输出*/
		
		int serv_num;
		struct L_SERV {
			unsigned short int server_port;
			in_addr_t server_ip;
			bool used;
			inline L_SERV() {
				server_port = 0;
				memset(&server_ip, 0, sizeof(server_ip));
				used = false;
			};
		} ;
		struct L_SERV *servs;

		NetTcp **filius;
		int fil_size;
		int top;

		inline void setServ(const char *srv_ip, int port, struct L_SERV &serv)
		{
			serv.server_port  = htons(port);

			if ( srv_ip )
			{
				struct in_addr me;
#if defined (_WIN32)
#define	HAS_ADDR (me.s_addr = inet_addr(srv_ip)) == INADDR_NONE
#else
#define HAS_ADDR inet_aton(srv_ip, &me) == 0
#endif
				if ( HAS_ADDR )
				{
					struct hostent* he;
					he = gethostbyname(srv_ip);
					if ( he != (struct hostent*) 0 )
		   			{
		   		 		(void) memcpy( &(serv.server_ip), he->h_addr, he->h_length );
		   	 		}
		  		} else {
		   		 	(void) memcpy(&(serv.server_ip), &me.s_addr, sizeof(serv.server_ip));
		   		}
				serv.used = true;
			}
		}

		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			TiXmlElement *av;
			int tmp_i;
			
                        offset_fld  = -1;		
                        flags_fld  = -1;		
			                    			                    
			data_fld  = -1;					                    
			option_fld  = -1;
			
			serv_num = 0;
			servs = 0;
			cfg->QueryIntAttribute("service_num", &(serv_num));
			if ( serv_num < 0 )
				serv_num = 0;
			
			av = cfg->FirstChildElement("service");
			for (; av; av = av->NextSiblingElement("service") ) 
			{
				serv_num++;
			}
			serv_num++;
			servs = new struct L_SERV[serv_num];

			tmp_i = 0;
			cfg->QueryIntAttribute("server_port", &(tmp_i));
			comm_str = cfg->Attribute("server_ip");
			if ( comm_str != (const char*) 0 && tmp_i > 0  )
			{
				setServ( comm_str, tmp_i, servs[0]);
			}

			av = cfg->FirstChildElement("service");
			for (; av; av = av->NextSiblingElement("service") ) 
			{
				addService( av->Attribute("ip"), av->Attribute("port"));
			}


			cfg->QueryIntAttribute("source_ip", &(srcip_fld));
			cfg->QueryIntAttribute("destination_ip", &(dstip_fld));
			cfg->QueryIntAttribute("source_port", &(srcport_fld));
			cfg->QueryIntAttribute("destination_port", &(dstport_fld));

			cfg->QueryIntAttribute("offset", &(offset_fld));
			cfg->QueryIntAttribute("flags", &(flags_fld));
			cfg->QueryIntAttribute("data", &(data_fld));
			cfg->QueryIntAttribute("option", &(option_fld));
			cfg->QueryIntAttribute("direction", &(direction_fld));

			lonely = false;	//默认多个连接
			if ( (comm_str = cfg->Attribute("lonely") ) && strcmp(comm_str, "yes") ==0 )
				lonely = true; /* 在建立一个连接后, 即关闭 */

			simply = true;	//默认简单方式
			if ( (comm_str = cfg->Attribute("simply") ) && strcmp(comm_str, "no") ==0 )
				simply = false; /* 分析TCP的连接 */

			top = 0;
			fil_size = 8;
			filius = new NetTcp* [fil_size];
			memset(filius, 0, sizeof(NetTcp*)*fil_size);
		};

		inline ~G_CFG() { 
			delete[] filius;
			top = 0;
		};

		inline void put(NetTcp *p) {
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
				NetTcp **tmp;
				tmp = new NetTcp *[fil_size*2];
				memcpy(tmp, filius, fil_size*(sizeof(NetTcp *)));
				fil_size *=2;
				delete[] filius;
				filius = tmp;
			}
		};

		inline NetTcp* get(PacketObj *pac) {
			NetTcp *p;
			int i;
			for (i =0; i < top; i++)
			{
				p = filius[i];
				if ( p->match(pac) )
					return p;
			}
			return (NetTcp *) 0;
		};

		inline void remove(NetTcp *p) {
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
					filius[j] = (NetTcp *) 0;
					k++;
				}
			}
			top -= k;
		};

		inline bool isService( unsigned char *ip_val, unsigned char *port_val) 
		{
			int i;
			
			for (i =0; i < serv_num; i++)
			{
				struct L_SERV &a_serv = servs[i];
				if ( a_serv.used 
					&& memcmp(ip_val, &a_serv.server_ip, sizeof(a_serv.server_ip) ) == 0
					&& memcmp(port_val, &a_serv.server_port, sizeof(a_serv.server_port) ) == 0
				)
					return true;
			}
			return false;
		};

		inline void rmService( unsigned char *ip_val, unsigned char *port_val) 
		{
			int i;
			
			for (i =0; i < serv_num; i++)
			{
				struct L_SERV &a_serv = servs[i];
				if ( a_serv.used 
					&& memcmp(ip_val, &a_serv.server_ip, sizeof(a_serv.server_ip) ) == 0
					&& memcmp(port_val, &a_serv.server_port, sizeof(a_serv.server_port) ) == 0
				)
					a_serv.used = false;
			}
			return ;
		};

		inline void addService( const char *ip, const char *port) 
		{
			int i, tmp_i;
			struct L_SERV *a_serv = 0;
			
			for (i =0; i < serv_num; i++)
			{
				a_serv = &servs[i];
				if ( !a_serv->used )
					break;
			}

			if ( i == serv_num)
				return;

			a_serv->used = true;
			tmp_i = 0;
			tmp_i = atoi(port);
			setServ( ip, tmp_i, *a_serv);
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;

};

#include <assert.h>

void NetTcp::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}

NetTcp::NetTcp()
{
	rcv_pac = 0;
	snd_pac = 0;

	pro_unipac.ordo = Notitia::PRO_UNIPAC;
	pro_unipac.indic = 0;

	end_ps.ordo = Notitia::END_SESSION;
	end_ps.indic = 0;

	start_ps.ordo = Notitia::START_SESSION;
	start_ps.indic = 0;

	pa[0] = &first_pac;
	pa[1] = &second_pac;
	pa[2] = 0;

	gCFG = 0;
	has_config = false;

	server_addr =  client_addr = 0;
	server_port =  client_port = 0;
}

NetTcp::~NetTcp() 
{
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

Amor* NetTcp::clone()
{
	NetTcp *child = new NetTcp();
	child->gCFG = gCFG;
	return (Amor*) child;
}

bool NetTcp::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	TiXmlElement *cfg;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		if ( gCFG->simply)
		{
			handle_sim(rcv_pac);
		} else {
			handle(rcv_pac);
		}
		break;

	case Notitia::CMD_NEW_SERVICE:
		WBUG("facio CMD_NEW_SERVICE");
		cfg = (TiXmlElement *)(pius->indic);
		if( !cfg)
		{
			WLOG(WARNING, "CMD_NEW_SERVICE cfg is null");
			break;
		}
		gCFG->addService( cfg->Attribute("ip"), cfg->Attribute("port"));
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

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		deliver(Notitia::SET_UNIPAC);	
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		deliver(Notitia::SET_UNIPAC);	
		break;

	default:
		return false;
	}
	return true;
}

bool NetTcp::sponte( Amor::Pius *pius)
{
	PacketObj **tmp;
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:
		WBUG("sponte PRO_UNIPAC");
		if ( handle(snd_pac) )
			aptus->sponte(&pro_unipac);
		break;

	case Notitia::SET_UNIPAC:
		WBUG("sponte SET_UNIPAC");

		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "sponte SET_UNIPAC snd_pac null");

			aptus->sponte(pius);
		} else 
			WLOG(WARNING, "sponte SET_UNIPAC null");

		break;
	default:
		return false;
	}
	return true;
}

#define ASSERTPAC(Y)	\
	assert(Y);	\
	assert(gCFG->srcip_fld >= 0 );	\
	assert(gCFG->dstip_fld >=0  );	\
	assert(gCFG->srcport_fld >=0 );	\
	assert(gCFG->dstport_fld >=0 );	\
	assert(gCFG->direction_fld >=0 );	\
						\
	assert(gCFG->srcip_fld <= Y->max );	\
	assert(gCFG->dstip_fld <= Y->max );	\
	assert(gCFG->srcport_fld <= Y->max );	\
	assert(gCFG->dstport_fld <= Y->max );	\
	assert(gCFG->direction_fld <= Y->max );


#define ValidPAC(Y)	\
	struct FieldObj &srcip = Y->fld[gCFG->srcip_fld];	\
	struct FieldObj &srcpt = Y->fld[gCFG->srcport_fld];	\
	struct FieldObj &dstip = Y->fld[gCFG->dstip_fld];	\
	struct FieldObj &dstpt = Y->fld[gCFG->dstport_fld];	\
	if ( 	srcip.no != gCFG->srcip_fld		\
		|| srcpt.no != gCFG->srcport_fld	\
		|| dstip.no != gCFG->dstip_fld		\
		|| dstpt.no != gCFG->dstport_fld	\
		|| srcip.range != sizeof(client_addr)	\
		|| srcpt.range != sizeof(client_port)	\
		|| dstip.range != sizeof(server_addr)	\
		|| dstpt.range != sizeof(server_port)	\
	)						\
		return false;


TINLINE bool NetTcp::handle(PacketObj *rpac)
{
	unsigned char flag;
	NetTcp *neo = 0 ;
	unsigned short offset=0, opt_len = 0;
	FieldObj *fld_tmp = 0 , *fld_d = 0;
	
	assert(gCFG->flags_fld >= 0 );
	assert(gCFG->flags_fld <= rpac->max );
	
	if (rpac->fld[gCFG->flags_fld].no != gCFG->flags_fld )
		return false;
		
	flag = rpac->fld[gCFG->flags_fld].val[0];

	neo = gCFG->get(rpac); 
	if ( (flag & SYN ) && !(flag & ACK) )
	{	/* 正常情况下，这是由client向server第一次发起的 */
		/* 如果前一次连接没有断开, 这里再次发起, 则取原来的对象, 从新使用 */
		ASSERTPAC(rpac);
		ValidPAC(rpac);

		if ( !gCFG->isService(dstip.val, dstpt.val) )
			return false;

		WLOG(INFO, "TCP SYN sent. ");
		if ( gCFG->lonely)
		{
			gCFG->rmService(dstip.val, dstpt.val);	
		}

		if ( !neo )
		{
			Amor::Pius tmp_p;
			
			tmp_p.ordo = Notitia::CMD_ALLOC_IDLE;
			tmp_p.indic = this;
			aptus->sponte(&tmp_p);
			/* 请求空闲的子实例 */
			if ( !(tmp_p.indic) || this == ( Amor* ) (tmp_p.indic) )
			{	/* 实例没有增加, 已经到达最大连接数，故关闭刚才的连接 */
				WLOG(NOTICE, "limited tcp connections to max");
				return false;
			} else {
				neo =  (NetTcp *)(tmp_p.indic);
				gCFG->put(neo); /* 入本地队列 */
			}
		} 

		neo->initConn(this, rpac);	/* 新建连接初始化 */
		neo->facio(&start_ps);

	} else if ( (flag & FIN ) || (flag & RST) ) {
		/* 断开连接 */
		if ( neo )
		{	/* 释放对象 */
			neo->endConn();
			gCFG->remove(neo);
		}
	} else if (neo ) {
		/* 一般数据 */
		assert(gCFG->offset_fld >=0 );
		assert(gCFG->data_fld >=0 );
		
		fld_d = &(rpac->fld[gCFG->data_fld]);
		fld_tmp = &(rpac->fld[gCFG->offset_fld]);
		if ( fld_d->no == gCFG->data_fld 
		   && fld_tmp->no == gCFG->offset_fld
		) {
			
			offset = ( fld_tmp->val[0] ) & 0xF0 ;
			offset = offset >> 2;
			opt_len = offset - 20;
			if ( opt_len > 0 && fld_d->range >= opt_len)
			{
				fld_d->val += opt_len;
				fld_d->raw = fld_d->val;
				fld_d->range -= opt_len;
				fld_d->_rlen = fld_d->range;
				fld_d->len = fld_d->range;
			}
			
			if ( fld_d->range > 0 )
			{
				FieldObj  *tmp_f; 	
				if ( neo->first_pac.max < 0 )
					neo->first_pac.produce(rcv_pac->max);	

				tmp_f = neo->first_pac.fld;
				neo->first_pac.fld = rcv_pac->fld;
				rcv_pac->fld = tmp_f;
				
				TBuffer::exchange(neo->first_pac.buf, rcv_pac->buf);
				neo->data_pro();
			}
		}
	}
	
	return true;
}

TINLINE bool NetTcp::handle_sim(PacketObj *rpac)
{
	unsigned short offset=0, opt_len = 0;
	FieldObj *fld_tmp = 0 , *fld_d = 0;
	unsigned char dir = 0x00;
	unsigned char flag;
	ASSERTPAC(rpac);
	ValidPAC(rpac);
	
	assert(gCFG->offset_fld >=0 );
	assert(gCFG->data_fld >=0 );
	assert(gCFG->flags_fld >= 0 );
	assert(gCFG->flags_fld <= rpac->max );

	if (rpac->fld[gCFG->flags_fld].no != gCFG->flags_fld )
		return false;
		
	if ( gCFG->isService(dstip.val, dstpt.val) )
	{	/* to Server */
		dir = 0x01;
	} else if ( gCFG->isService(srcip.val, srcpt.val) )
	{	/* to Client */
		dir = 0x02;
	}

	if ( dir == 0x00 )
	{
		return false;
	}

	rpac->input(gCFG->direction_fld, &dir, 1);

	fld_d = &(rpac->fld[gCFG->data_fld]);
	fld_tmp = &(rpac->fld[gCFG->offset_fld]);
	flag = rpac->fld[gCFG->flags_fld].val[0];

	if ( (flag & SYN ) && !(flag & ACK) )
	{
		WLOG(INFO, "TCP SYN sent. ");
		goto END;
	} else if ( (flag & FIN ) || (flag & RST) ) 
	{ 
		WLOG(INFO, "TCP FIN or RST.");
		goto END;
	} else if ( fld_d->no == gCFG->data_fld && fld_tmp->no == gCFG->offset_fld) 
	{
		FieldObj  *tmp_f; 	
		offset = ( fld_tmp->val[0] ) & 0xF0 ;
		offset = offset >> 2;
		opt_len = offset - 20;
		if ( opt_len > 0 && fld_d->range >= opt_len)
		{
			fld_d->val += opt_len;
			fld_d->raw = fld_d->val;
			fld_d->range -= opt_len;
			fld_d->_rlen = fld_d->range;
			fld_d->len = fld_d->range;
		}
			
		if ( fld_d->range <= 0 )
		{
			return false;
		}

		if ( first_pac.max < 0 )
			first_pac.produce(rcv_pac->max);	

		tmp_f = first_pac.fld;	
		first_pac.fld = rcv_pac->fld;
		rcv_pac->fld = tmp_f;
				
		TBuffer::exchange(first_pac.buf, rcv_pac->buf);
		data_pro();
	}
END:
	return true;
}

bool NetTcp::initConn(NetTcp *fa, PacketObj *rpac)
{
	ASSERTPAC(rpac);
	ValidPAC(rpac);

	WLOG(INFO, "TCP init Connection. ");
	memcpy(&client_addr, srcip.val, sizeof(client_addr));
	memcpy(&client_port, srcpt.val, sizeof(client_port));
	memcpy(&server_addr, dstip.val, sizeof(server_addr));
	memcpy(&server_port, dstpt.val, sizeof(server_port));
	return true;	
}

TINLINE bool NetTcp::match(PacketObj *mpac)
{
	unsigned char dir = 0x00;
	ASSERTPAC(mpac);
	ValidPAC(mpac);
	
	if ( memcmp(srcip.val, &client_addr, sizeof(client_addr) ) == 0 
		&& memcmp(srcpt.val, &client_port, sizeof(client_port) ) == 0
		&& memcmp(dstip.val, &server_addr, sizeof(server_addr) ) == 0
		&& memcmp(dstpt.val, &server_port, sizeof(server_port) ) == 0
	) {	/* to Server */
		dir = 0x01;
	} else if ( memcmp(dstip.val, &client_addr, sizeof(client_addr) ) == 0 
		&& memcmp(dstpt.val, &client_port, sizeof(client_port) ) == 0
		&& memcmp(srcip.val, &server_addr, sizeof(server_addr) ) == 0
		&& memcmp(srcpt.val, &server_port, sizeof(server_port) ) == 0
	) {	/* to Client */
		dir = 0x02;
	}

	if ( dir != 0x00 )
	{
		mpac->input(gCFG->direction_fld, &dir, 1);
		return true;
	}

	return false;
}

TINLINE void NetTcp::data_pro()
{
	aptus->facio(&pro_unipac);
}

TINLINE void NetTcp::endConn()
{
	Amor::Pius tmp_p;
	WLOG(INFO, "TCP FIN or RST, will end connection.");
	aptus->facio(&end_ps);
	server_addr =  client_addr = 0;
	server_port =  client_port = 0;
	tmp_p.ordo = Notitia::CMD_FREE_IDLE;
	tmp_p.indic = this;
	aptus->sponte(&tmp_p);
}

/* 向接力者提交 */
void NetTcp::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
		case Notitia::SET_UNIPAC:
			WBUG("deliver SET_UNIPAC");
			tmp_pius.indic = &pa[0];
			break;

		default:
			WBUG("deliver Notitia::%d",aordo);
			break;
	}
	aptus->facio(&tmp_pius);
	return ;
}
#include "hook.c"
