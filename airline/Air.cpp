/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: 航空线
 Desc: Aptus扩展, 实现多个模块对于特定ordo，将Pius转向某个模块。
	这些模块在clone出新对象时，是同步的。即这些模块子对象数目相同, 并且同时出现。
	这些特定ordo的Pius将在同一阶（相同子对象序数号）对象之间传递。
 Build: created by octerboy, 2008/01/16, Guangzhou
 $Header: /textus/airline/Air.cpp 3     13-10-04 17:23 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: Air.cpp $"
#define TEXTUS_MODTIME  "$Date: 13-10-04 17:23 $"
#define TEXTUS_BUILDNO  "$Revision: 3 $"
/* $NoKeywords: $ */

#include "Aptus.h"
#include "casecmp.h"
#include "Notitia.h"
#include "textus_string.h"

typedef struct _Array {
	void **filius;
	int fil_size;
	int top;

	inline _Array () {
		top = 0;
		fil_size = 4;
		filius = new void* [fil_size];
		memset(filius, 0, sizeof(void*)*fil_size);
	};

	
	inline void put(void *p, int i) {
		if ( i >= top )
			return;
			
		 filius[i] = p;
	};

	inline void put(void *p) {
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
			void **tmp;
			tmp = new void *[fil_size*2];
			memcpy(tmp, filius, fil_size*(sizeof(void *)));
			fil_size *=2;
			delete[] filius;
			filius = tmp;
		}
	};

	inline void* get(int i) { return filius[i]; };

	inline void remove(void *p) {
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
				filius[j] = (void *) 0;
				k++;
			}
		}
		top -= k;
	};

} Array; 
		
static Array *groups = 0;
static int g_type_ref = 0;	/* 全局类型参考值, ignite时将此值加1, 复制给typeID */

class Air: public Aptus {
public:
	/* 以下是Amor 类定义的 */
	void ignite_t (TiXmlElement *wood, TiXmlElement *);	
	Amor *clone();

	/* 这是在Aptus中定义过的。*/
	bool sponte_n ( Amor::Pius *, unsigned int );
	bool facio_n  ( Amor::Pius *, unsigned int );

	/* 以下为本类特别定义 */
	Air();
	~Air();
	
	typedef struct _List { 
		Air *me; 
		_List* next; 
		inline _List (){
			me = 0;
			next = 0;
		}
	} List;


protected:
	/* Group 表示一个组，这个组包括原来一个Cross线的功能, 有多个起飞点和多个终点
		(这在ends变量中体现出来) */
	typedef struct _MGroup {
		char id_name[256];
		bool h_causation;/* true: 有因果关系参考, false:无 */
		bool caused;	/* true: 有因,有前面的作为因的实例产生, 应发生果, 并置其为false */
		List g_lines;	/* 所有顶层父实例 */
		inline _MGroup( const char *nm) 
		{
			id_name[255] = '\0';
			id_name[0] = '\0';
			if ( nm )
			{
				int l = strlen(nm);
				memcpy(id_name, nm,  l > 254 ? 254: l+1);
			}
			h_causation = false;
			caused = false;
		};
	} Group;
	List *ele_g;	/* 本地一个单元,为加入groups->g_lines */

	Array ends; 	/* 航线终点, 仅对于航线起点的实例有意义. 有多个终点 */
	char levelStr[256];	/* 层次标识, ignite的为"", clone时, child的为前者的levelStr+"-"+id  */
	unsigned long serial_no;		/* 在同一个层次中的序列 */
	List ele_r;	/* 一个单元,为加入roll */

	bool isPoineer;

	typedef struct _Flyer {
		TEXTUS_ORDO ordo;	/* 被抓的ordo */
		Amor::Pius another;	/* -1:用原来的, >=0 用这里的 */
		bool rev;	/* 是否逆转 */
		bool stop;	/* 是否继续原有的调用 */
		inline _Flyer () 
		{
			ordo = Notitia::TEXTUS_RESERVED;
			another.ordo = Notitia::TEXTUS_RESERVED;
			another.indic = 0;
			rev = false;
			stop = true;
		}
	} Flyer; 

	typedef struct _GCFG {
		int typeID;		/* ignite时得到新值, clone时传递 */
		int child_num;		/* clone时加一传递到子实例的serial_no */

		char grp_name[256];	/* 所在群的名称, xml中属性名称为"group" */

		bool isCause;	/* true: 这里因, 置caused为true; false: 这里是果 */
		bool isLast;	/* true: 这里是最后的果, 置caused为false; false: 并不置caused */
		bool del_noCause;/* true: 非因果删除; false: 保留 */

		List roll;	/* 子实例表, 从一个ignite的出发, 只有一个 */
		bool isEnd;	/* 航线是否为终点的标志 */

		unsigned int num_spo;	/* sponte函数的ordo数 */
		Flyer *fly_fac;

		unsigned int num_fac;	/* facio函数的ordo数 */
		Flyer *fly_spo;

		inline _GCFG( TiXmlElement *sz_ele, Group *here_grp) 
		{
			const char *comm_str;

			typeID = 0;
			child_num = 0;

			grp_name[0] = '\0';

			num_spo = num_fac = 0;	
			fly_spo = fly_fac = (Flyer *) 0;

			isEnd = false;

			isCause = false;
			isLast = false;
			del_noCause= false;

			comm_str = sz_ele->Attribute("where");
			if ( comm_str ) 
				isEnd = ( strcmp(comm_str, "end") == 0  );

			comm_str = sz_ele->Attribute("cause");
			if ( comm_str )
			{ 
				here_grp->h_causation = true;
				/* true, 这里是起因, 有它才有别的 */
				isCause = ( strcmp(comm_str, "yes") == 0  );
			}

			comm_str = sz_ele->Attribute("effect");
			if ( comm_str )
			{ 
				here_grp->h_causation = true;
				/* true, 这里是最后的果 */
				isLast = ( strcmp(comm_str, "last") == 0 );
			}

			comm_str = sz_ele->Attribute("noCause") ;
			if ( comm_str ) 
			{
				here_grp->h_causation = true;
				/* true: 非因果关系的, 删除之 */
				del_noCause = ( strcmp(comm_str, "del") == 0 );
			}
		};

		inline ~_GCFG() {
			if ( fly_spo ) delete [] fly_spo;
			if ( fly_fac ) delete [] fly_fac;
		};

		inline void set_fly( TiXmlElement *sz_ele)
		{
			const char *comm_str;
			TiXmlElement *spo_ele, *fac_ele;
			int i;
			/* sponte方法中需要飞出的ordo */
			spo_ele = sz_ele->FirstChildElement("sponte"); num_spo = 0;
			while(spo_ele)
			{
				spo_ele = spo_ele->NextSiblingElement("sponte");
				num_spo++;
			}
			if ( num_spo > 0 )
				fly_spo = new Flyer [num_spo];
	
			spo_ele = sz_ele->FirstChildElement("sponte"); i = 0;
			for( ; spo_ele; spo_ele = spo_ele->NextSiblingElement("sponte"), i++ )
			{
				//comm_str = spo_ele->Attribute("ordo");
				//Notitia::get_textus_ordo(&fly_spo[i].ordo, comm_str);
				fly_spo[i].ordo = Notitia::get_ordo(spo_ele->Attribute("ordo"));

				//comm_str = spo_ele->Attribute("another");
				//Notitia::get_textus_ordo(&fly_spo[i].another.ordo, comm_str);
				fly_spo[i].another.ordo = Notitia::get_ordo(spo_ele->Attribute("another"));
	
				comm_str = spo_ele->Attribute("to");
				if ( comm_str && strcasecmp(comm_str, "FACIO") == 0 ) 
				fly_spo[i].rev = true;

				if ( comm_str && strcasecmp(comm_str, "DEXTRA") == 0 ) 
				fly_spo[i].rev = true;

				comm_str = spo_ele->Attribute("stop");
				if ( comm_str && strcasecmp(comm_str, "no") == 0 ) 
				fly_spo[i].stop = false;
			}

			/* facio方法中需要飞出的ordo */
			fac_ele = sz_ele->FirstChildElement("facio"); num_fac = 0;
			while(fac_ele)
			{
				fac_ele = fac_ele->NextSiblingElement("facio");
				num_fac++;
			}

			if ( num_fac > 0 )
				fly_fac = new Flyer [num_fac];
	
			fac_ele = sz_ele->FirstChildElement("facio"); i = 0;
			for(; fac_ele; fac_ele = fac_ele->NextSiblingElement("facio"), i++ )
			{
				//comm_str = fac_ele->Attribute("ordo");
				//Notitia::get_textus_ordo(&fly_fac[i].ordo, comm_str);
				fly_fac[i].ordo = Notitia::get_ordo(fac_ele->Attribute("ordo"));

				//comm_str = fac_ele->Attribute("another");
				//Notitia::get_textus_ordo(&fly_fac[i].another.ordo, comm_str);
				fly_fac[i].another.ordo = Notitia::get_ordo(fac_ele->Attribute("another"));

				comm_str = fac_ele->Attribute("to");
				if ( comm_str && strcasecmp(comm_str, "SPONTE") == 0 ) 
					fly_fac[i].rev = true;

				if ( comm_str && strcasecmp(comm_str, "LAEVE") == 0 ) 
					fly_fac[i].rev = true;

				comm_str = fac_ele->Attribute("stop");
				if ( comm_str && strcasecmp(comm_str, "no") == 0 ) 
					fly_fac[i].stop = false;
			}
		};
	} GCFG;

	GCFG *gcfg;
	bool has_config;

	void append(List *l, List *e) {
		List *n;	/* 表的头一元素的me始终为空 */
		if ( !l || !e ) return;
		n = l;
		while (n->next) { 
			if ( n->next == e ) return;	/* 已有, 不再加载 */
			n = n->next;
		}
		n->next  = e;
	}

	void put_end(Air *tor);
	Group* get_grp(const char *nm);
	void joint(Group *);

	Amor::Pius an_pius;
	#include "tbug.h"
};

void Air::ignite_t (TiXmlElement *cfg, TiXmlElement *sz_ele)
{	/* 从cfg获取hostname参数, aliusID等从carbo获得 */
	Group *here_grp;
	
	if ( !groups )
		groups = new Array;

	WBUG("this %p , prius %p, aptus %p, cfg %p, owner %p", this, prius, aptus, cfg, owner);

	if ( !sz_ele) return;

	/* 识别所在的 群 */
	here_grp = get_grp(sz_ele->Attribute("group"));
	if ( !gcfg )
	{
		gcfg = new GCFG(sz_ele, here_grp);
		has_config = true;
	}
	memcpy(gcfg->grp_name, here_grp->id_name, sizeof(gcfg->grp_name));
	
	canAccessed = true;	/* 至此可以认为此模块需要Air */
	isPoineer = true;

	TEXTUS_STRCPY(levelStr, "");	/* 最初的层次 */
	g_type_ref++;	
	gcfg->typeID = g_type_ref;	/* 父实例与子实例共享一个空间, 所以一起改变 */

	if ( !ele_g ) 
	{
		ele_g = new List;
		ele_g->me = this; 
	}
	append(&(here_grp->g_lines), ele_g);	/* 加入到全局的顶层父实例表 */
	append(&gcfg->roll, &ele_r);	/* 父实例加入到花名册, joint仅对roll处理就可以了 */
	joint(here_grp);	/* 连接航线 */

	if ( !gcfg->isEnd ) 	/* 对于航线起点, 才设置起飞件 */
		gcfg->set_fly(sz_ele);

	WBUG("this ordos_num_spo %d", gcfg->num_spo);
	WBUG("this ordos_num_fac %d", gcfg->num_fac);
	need_spo = ( gcfg->num_spo > 0 );
	need_fac = ( gcfg->num_fac > 0 );
	
	return ;
}

Amor *Air::clone() 
{
	char idStr[10];
	Air *child = 0;
	Group *here_grp = (Group*) 0;

	child = new Air();
	Aptus::inherit((Aptus*) child);
	child->gcfg = gcfg;

	/* 以下的for, 子实例先与本实例有相同的航线终点集, 而后在joint()中更新. 
	这一点很重要,因为:
	对于航线起点, 有可能层次比终点处于更深的层次上, 这样在joint函数中是
	不会被画线的. 在实际上有一种可能:同一个连接中同时包含更多的交易, 这样,
	一个连接是一个对象, 而交易却有多个对象，通常合理的做法是：多个交易对象对应
	同一个连接对象.
	*/
	for (int i = 0 ; i < ends.top; i++)
		child->ends.put(ends.get(i));

	here_grp = get_grp(gcfg->grp_name);	/* 取得所在的群 */
	if ( here_grp->h_causation ) 
	{
		if ( gcfg->isCause )
			here_grp->caused = true;
		else {
			if ( here_grp->caused )
			{
				if ( gcfg->isLast )
					here_grp->caused = false;
			} else {
				child->canAccessed = !(gcfg->del_noCause);
				goto End;
			}
		}
	}

	TEXTUS_STRCPY(child->levelStr, levelStr);
	TEXTUS_SPRINTF(idStr, "-%lx", serial_no);
	TEXTUS_STRCAT(child->levelStr, idStr);	/* 子实例的层次标识为父实例的levelStr+"-"+serial_no */
	child->serial_no = ++(gcfg->child_num);	/* 这样, 第一个子实例的serial_no为1, 而父实例的为0 */
	append(&gcfg->roll, &(child->ele_r));	/* 将子实例加入花名册, 不通过clone函数所得的新实例, 则不在册中 */
	child->joint(here_grp);		/* 子实例作连接 */
End:
	return  (Amor*)child;
}

void Air::put_end(Air *tor)
{
	/* 查一下, ends中是否已有相同typeID, 若是替换,否则增加 */
	Air *rel; int j;
	WBUG("put_end tor %p", tor);
	for ( j = 0 ; j < ends.top; j++ )
	{
		rel = (Air*) ends.get(j);
		WBUG("rel %p, typeID %d, tor typeID %d, tor.owner %p", rel, rel->gcfg->typeID, tor->gcfg->typeID, tor->owner);
		if ( rel->gcfg->typeID == tor->gcfg->typeID )
		{
			ends.put(tor, j);	/* 替换原来的rel */
			WBUG("update to rel %p, typeID %d, tor typeID %d, tor.owner %p", ends.get(j), ((Air*) ends.get(j))->gcfg->typeID, tor->gcfg->typeID, tor->owner);
			break;
		}
	}

	if ( j == ends.top )	/* 新的, 加进去 */
		ends.put(tor);
}

/* 航线连接 */
void Air::joint(Group *here_grp)
{
	List *n, *m;
	Air *p, *q;
	n = here_grp->g_lines.next;	/* 第一个me为空 */

	/* 遍历所有起点及终点 */
	while (n) 
	{
		q = n->me; /* q为Air对象, q是顶层父实例 */
		if ( q && q->gcfg->isEnd != gcfg->isEnd ) 
		{	/* 还是判断一下, 同是起点或同是终点的略过, 当然会避免自己与自己连 */
			m = q->gcfg->roll.next;
			while ( m )	/* m遍历所有实例, 父实例也在内 */
			{
				p = m->me;
				if ( strcmp(p->levelStr, levelStr) == 0 
					&& p->serial_no == serial_no )
				{	/* 找到对应者了, 可以画一条线了 */
					if ( p->gcfg->isEnd && !gcfg->isEnd )
					{	/* 找到一个终点, 而自身是起点 */
						put_end(p);
						//end = p;
					}
	
					if ( !(p->gcfg->isEnd) && gcfg->isEnd )
					{	/* 找到一个起点, 而自身是终点 */
						p->put_end(this);
						//p->end = this;
					}
				}
				m = m->next;
			}
		}
		n = n->next;	/* 下一个 */
	}
}

bool Air::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向左发出数据前的处理 */
	WBUG("sponte ordo %d, owner %p", pius->ordo, owner);

	if (ends.top > 0 )
	for ( unsigned int i =0; i < gcfg->num_spo; i++)
	{
		if ( pius->ordo == gcfg->fly_spo[i].ordo )
		{
			bool isOther = ( gcfg->fly_spo[i].another.ordo != Notitia::TEXTUS_RESERVED );
			if ( gcfg->fly_spo[i].rev )
			{
				/* 模拟end的右节点进入 */
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air*) ends.get(j);
					((Aptus*)(end->aptus))->dextra( isOther ? &(gcfg->fly_spo[i].another) : pius, 0); 
				}
			} else  
			{ 	/* 模拟end的左节点进入 */
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air *)ends.get(j);
					((Aptus*)(end->aptus))->laeve( isOther ? &(gcfg->fly_spo[i].another) : pius, 0); 
				}
			}
			return gcfg->fly_spo[i].stop;
		}
	}
	return false;
}

bool Air::facio_n ( Amor::Pius *pius, unsigned int from)
{	/* 在owner向右发出数据前的处理 */
	WBUG("facio ordo %d, owner %p, serial_no %ld", pius->ordo, owner, serial_no);

	if( ends.top > 0) 
	for ( unsigned int i =0; i < gcfg->num_fac; i++)
	{
		if ( pius->ordo == gcfg->fly_fac[i].ordo )
		{
			bool isOther = ( gcfg->fly_fac[i].another.ordo != Notitia::TEXTUS_RESERVED );
			
			if ( gcfg->fly_fac[i].rev )
			{	/* 模拟end的右节点进入 */
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air*) ends.get(j);
					((Aptus *)(end->aptus))->laeve(isOther ? &(gcfg->fly_fac[i].another) : pius, 0); 
				}
			} else  
			{	/* 模拟end的左节点进入 */ 
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air*) ends.get(j);
					((Aptus *)(end->aptus))->dextra(isOther ? &(gcfg->fly_fac[i].another) : pius, 0); 
				}
			}
			return gcfg->fly_fac[i].stop;
		}
	}
	return false;
}

Air::Air() {
	WBUG("new this %p, groups %p, ele_r %p", this, groups, &ele_r);
	ele_g = 0;
	ele_r.me = this;

	serial_no = 0;

	gcfg = 0;
	has_config = false;
	isPoineer = false;
}

Air::~Air() {
	WBUG("delete this %p, ele_r.me %p", this, ele_r.me);
	if ( isPoineer )
	{
		if ( ele_g ) delete ele_g;
	}

	if ( has_config )
		delete gcfg;
}

Air::Group* Air::get_grp( const char *grp_str )
{
	/* 寻找已有的group, 根据id_name */
	int j= 0;
	Group *here_grp = (Group*) 0;
	if ( !groups )
		goto RETURN;

	/* 寻找已有的group, 根据id_name */
	for ( j = 0 ; j < groups->top; j++)
	{
		here_grp = (Group*) groups->get(j);		
		if ( grp_str == 0 &&  here_grp->id_name[0] == '\0' )
			break;
		if ( grp_str && strcmp(grp_str, here_grp->id_name) == 0 ) 
			break;
	}

	if ( j == groups->top )
	{	/* 还没有这样的group, 所以加进去 */
		here_grp = new Group(grp_str);
		groups->put(here_grp);
	} 

RETURN:	
	return here_grp;
}

#define TEXTUS_APTUS_TAG {'F', 'l', 'y', 0};
#include "hook.c"
