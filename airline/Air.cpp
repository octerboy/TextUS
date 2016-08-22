/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: ������
 Desc: Aptus��չ, ʵ�ֶ��ģ������ض�ordo����Piusת��ĳ��ģ�顣
	��Щģ����clone���¶���ʱ����ͬ���ġ�����Щģ���Ӷ�����Ŀ��ͬ, ����ͬʱ���֡�
	��Щ�ض�ordo��Pius����ͬһ�ף���ͬ�Ӷ��������ţ�����֮�䴫�ݡ�
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
static int g_type_ref = 0;	/* ȫ�����Ͳο�ֵ, igniteʱ����ֵ��1, ���Ƹ�typeID */

class Air: public Aptus {
public:
	/* ������Amor �ඨ��� */
	void ignite_t (TiXmlElement *wood, TiXmlElement *);	
	Amor *clone();

	/* ������Aptus�ж�����ġ�*/
	bool sponte_n ( Amor::Pius *, unsigned int );
	bool facio_n  ( Amor::Pius *, unsigned int );

	/* ����Ϊ�����ر��� */
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
	/* Group ��ʾһ���飬��������ԭ��һ��Cross�ߵĹ���, �ж����ɵ�Ͷ���յ�
		(����ends���������ֳ���) */
	typedef struct _MGroup {
		char id_name[256];
		bool h_causation;/* true: �������ϵ�ο�, false:�� */
		bool caused;	/* true: ����,��ǰ�����Ϊ���ʵ������, Ӧ������, ������Ϊfalse */
		List g_lines;	/* ���ж��㸸ʵ�� */
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
	List *ele_g;	/* ����һ����Ԫ,Ϊ����groups->g_lines */

	Array ends; 	/* �����յ�, �����ں�������ʵ��������. �ж���յ� */
	char levelStr[256];	/* ��α�ʶ, ignite��Ϊ"", cloneʱ, child��Ϊǰ�ߵ�levelStr+"-"+id  */
	unsigned long serial_no;		/* ��ͬһ������е����� */
	List ele_r;	/* һ����Ԫ,Ϊ����roll */

	bool isPoineer;

	typedef struct _Flyer {
		TEXTUS_ORDO ordo;	/* ��ץ��ordo */
		Amor::Pius another;	/* -1:��ԭ����, >=0 ������� */
		bool rev;	/* �Ƿ���ת */
		bool stop;	/* �Ƿ����ԭ�еĵ��� */
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
		int typeID;		/* igniteʱ�õ���ֵ, cloneʱ���� */
		int child_num;		/* cloneʱ��һ���ݵ���ʵ����serial_no */

		char grp_name[256];	/* ����Ⱥ������, xml����������Ϊ"group" */

		bool isCause;	/* true: ������, ��causedΪtrue; false: �����ǹ� */
		bool isLast;	/* true: ���������Ĺ�, ��causedΪfalse; false: ������caused */
		bool del_noCause;/* true: �����ɾ��; false: ���� */

		List roll;	/* ��ʵ����, ��һ��ignite�ĳ���, ֻ��һ�� */
		bool isEnd;	/* �����Ƿ�Ϊ�յ�ı�־ */

		unsigned int num_spo;	/* sponte������ordo�� */
		Flyer *fly_fac;

		unsigned int num_fac;	/* facio������ordo�� */
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
				/* true, ����������, �������б�� */
				isCause = ( strcmp(comm_str, "yes") == 0  );
			}

			comm_str = sz_ele->Attribute("effect");
			if ( comm_str )
			{ 
				here_grp->h_causation = true;
				/* true, ���������Ĺ� */
				isLast = ( strcmp(comm_str, "last") == 0 );
			}

			comm_str = sz_ele->Attribute("noCause") ;
			if ( comm_str ) 
			{
				here_grp->h_causation = true;
				/* true: �������ϵ��, ɾ��֮ */
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
			/* sponte��������Ҫ�ɳ���ordo */
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

			/* facio��������Ҫ�ɳ���ordo */
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
		List *n;	/* ���ͷһԪ�ص�meʼ��Ϊ�� */
		if ( !l || !e ) return;
		n = l;
		while (n->next) { 
			if ( n->next == e ) return;	/* ����, ���ټ��� */
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
{	/* ��cfg��ȡhostname����, aliusID�ȴ�carbo��� */
	Group *here_grp;
	
	if ( !groups )
		groups = new Array;

	WBUG("this %p , prius %p, aptus %p, cfg %p, owner %p", this, prius, aptus, cfg, owner);

	if ( !sz_ele) return;

	/* ʶ�����ڵ� Ⱥ */
	here_grp = get_grp(sz_ele->Attribute("group"));
	if ( !gcfg )
	{
		gcfg = new GCFG(sz_ele, here_grp);
		has_config = true;
	}
	memcpy(gcfg->grp_name, here_grp->id_name, sizeof(gcfg->grp_name));
	
	canAccessed = true;	/* ���˿�����Ϊ��ģ����ҪAir */
	isPoineer = true;

	TEXTUS_STRCPY(levelStr, "");	/* ����Ĳ�� */
	g_type_ref++;	
	gcfg->typeID = g_type_ref;	/* ��ʵ������ʵ������һ���ռ�, ����һ��ı� */

	if ( !ele_g ) 
	{
		ele_g = new List;
		ele_g->me = this; 
	}
	append(&(here_grp->g_lines), ele_g);	/* ���뵽ȫ�ֵĶ��㸸ʵ���� */
	append(&gcfg->roll, &ele_r);	/* ��ʵ�����뵽������, joint����roll����Ϳ����� */
	joint(here_grp);	/* ���Ӻ��� */

	if ( !gcfg->isEnd ) 	/* ���ں������, ��������ɼ� */
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

	/* ���µ�for, ��ʵ�����뱾ʵ������ͬ�ĺ����յ㼯, ������joint()�и���. 
	��һ�����Ҫ,��Ϊ:
	���ں������, �п��ܲ�α��յ㴦�ڸ���Ĳ����, ������joint��������
	���ᱻ���ߵ�. ��ʵ������һ�ֿ���:ͬһ��������ͬʱ��������Ľ���, ����,
	һ��������һ������, ������ȴ�ж������ͨ������������ǣ�������׶����Ӧ
	ͬһ�����Ӷ���.
	*/
	for (int i = 0 ; i < ends.top; i++)
		child->ends.put(ends.get(i));

	here_grp = get_grp(gcfg->grp_name);	/* ȡ�����ڵ�Ⱥ */
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
	TEXTUS_STRCAT(child->levelStr, idStr);	/* ��ʵ���Ĳ�α�ʶΪ��ʵ����levelStr+"-"+serial_no */
	child->serial_no = ++(gcfg->child_num);	/* ����, ��һ����ʵ����serial_noΪ1, ����ʵ����Ϊ0 */
	append(&gcfg->roll, &(child->ele_r));	/* ����ʵ�����뻨����, ��ͨ��clone�������õ���ʵ��, ���ڲ��� */
	child->joint(here_grp);		/* ��ʵ�������� */
End:
	return  (Amor*)child;
}

void Air::put_end(Air *tor)
{
	/* ��һ��, ends���Ƿ�������ͬtypeID, �����滻,�������� */
	Air *rel; int j;
	WBUG("put_end tor %p", tor);
	for ( j = 0 ; j < ends.top; j++ )
	{
		rel = (Air*) ends.get(j);
		WBUG("rel %p, typeID %d, tor typeID %d, tor.owner %p", rel, rel->gcfg->typeID, tor->gcfg->typeID, tor->owner);
		if ( rel->gcfg->typeID == tor->gcfg->typeID )
		{
			ends.put(tor, j);	/* �滻ԭ����rel */
			WBUG("update to rel %p, typeID %d, tor typeID %d, tor.owner %p", ends.get(j), ((Air*) ends.get(j))->gcfg->typeID, tor->gcfg->typeID, tor->owner);
			break;
		}
	}

	if ( j == ends.top )	/* �µ�, �ӽ�ȥ */
		ends.put(tor);
}

/* �������� */
void Air::joint(Group *here_grp)
{
	List *n, *m;
	Air *p, *q;
	n = here_grp->g_lines.next;	/* ��һ��meΪ�� */

	/* ����������㼰�յ� */
	while (n) 
	{
		q = n->me; /* qΪAir����, q�Ƕ��㸸ʵ�� */
		if ( q && q->gcfg->isEnd != gcfg->isEnd ) 
		{	/* �����ж�һ��, ͬ������ͬ���յ���Թ�, ��Ȼ������Լ����Լ��� */
			m = q->gcfg->roll.next;
			while ( m )	/* m��������ʵ��, ��ʵ��Ҳ���� */
			{
				p = m->me;
				if ( strcmp(p->levelStr, levelStr) == 0 
					&& p->serial_no == serial_no )
				{	/* �ҵ���Ӧ����, ���Ի�һ������ */
					if ( p->gcfg->isEnd && !gcfg->isEnd )
					{	/* �ҵ�һ���յ�, ����������� */
						put_end(p);
						//end = p;
					}
	
					if ( !(p->gcfg->isEnd) && gcfg->isEnd )
					{	/* �ҵ�һ�����, ���������յ� */
						p->put_end(this);
						//p->end = this;
					}
				}
				m = m->next;
			}
		}
		n = n->next;	/* ��һ�� */
	}
}

bool Air::sponte_n ( Amor::Pius *pius, unsigned int from)
{	/* ��owner���󷢳�����ǰ�Ĵ��� */
	WBUG("sponte ordo %d, owner %p", pius->ordo, owner);

	if (ends.top > 0 )
	for ( unsigned int i =0; i < gcfg->num_spo; i++)
	{
		if ( pius->ordo == gcfg->fly_spo[i].ordo )
		{
			bool isOther = ( gcfg->fly_spo[i].another.ordo != Notitia::TEXTUS_RESERVED );
			if ( gcfg->fly_spo[i].rev )
			{
				/* ģ��end���ҽڵ���� */
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air*) ends.get(j);
					((Aptus*)(end->aptus))->dextra( isOther ? &(gcfg->fly_spo[i].another) : pius, 0); 
				}
			} else  
			{ 	/* ģ��end����ڵ���� */
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
{	/* ��owner���ҷ�������ǰ�Ĵ��� */
	WBUG("facio ordo %d, owner %p, serial_no %ld", pius->ordo, owner, serial_no);

	if( ends.top > 0) 
	for ( unsigned int i =0; i < gcfg->num_fac; i++)
	{
		if ( pius->ordo == gcfg->fly_fac[i].ordo )
		{
			bool isOther = ( gcfg->fly_fac[i].another.ordo != Notitia::TEXTUS_RESERVED );
			
			if ( gcfg->fly_fac[i].rev )
			{	/* ģ��end���ҽڵ���� */
				for ( int j = 0 ; j < ends.top; j++)
				{
					Air *end = (Air*) ends.get(j);
					((Aptus *)(end->aptus))->laeve(isOther ? &(gcfg->fly_fac[i].another) : pius, 0); 
				}
			} else  
			{	/* ģ��end����ڵ���� */ 
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
	/* Ѱ�����е�group, ����id_name */
	int j= 0;
	Group *here_grp = (Group*) 0;
	if ( !groups )
		goto RETURN;

	/* Ѱ�����е�group, ����id_name */
	for ( j = 0 ; j < groups->top; j++)
	{
		here_grp = (Group*) groups->get(j);		
		if ( grp_str == 0 &&  here_grp->id_name[0] == '\0' )
			break;
		if ( grp_str && strcmp(grp_str, here_grp->id_name) == 0 ) 
			break;
	}

	if ( j == groups->top )
	{	/* ��û��������group, ���Լӽ�ȥ */
		here_grp = new Group(grp_str);
		groups->put(here_grp);
	} 

RETURN:	
	return here_grp;
}

#define TEXTUS_APTUS_TAG {'F', 'l', 'y', 0};
#include "hook.c"
