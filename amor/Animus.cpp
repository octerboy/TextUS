/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Main loader
 desc: Loading all modules, deliver message between modules. 
 Build: created on 2006/03/10 in Shenyang
 $Header: /textus/amor/Animus.cpp 1    13-10-04 17:21 Octerboy $
*/
#define SCM_MODULE_ID  "$Id: Animus.cpp $"
#define TEXTUS_MODTIME  "$Date: 13-10-04 17:21 $"
#define TEXTUS_BUILDNO  "$Revision: 96 $"
/* $NoKeywords: $ */

#ifndef NDEBUG
	#if defined( _MSC_VER )
	#include "textus_string.h"
	#include <windows.h>
	#define WBUG2(x,y) { \
		char errMsg[2314]; \
		char msg[2048]; \
		TEXTUS_SNPRINTF(msg, sizeof(msg)-1, x,y); \
		TEXTUS_SNPRINTF(errMsg, sizeof(errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, msg); \
		printf("%s\n", errMsg); \
		OutputDebugString(errMsg); \
		}
	#define WBUG3(x,y,z) { \
		char errMsg[2314]; \
		char msg[2048]; \
		TEXTUS_SNPRINTF(msg, sizeof(msg)-1, x,y,z); \
		TEXTUS_SNPRINTF(errMsg, sizeof(errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, msg); \
		printf("%s\n", errMsg); \
		OutputDebugString(errMsg); \
		}
	#define WBUG4(x,y,z,w) { \
		char errMsg[2314]; \
		char msg[2048]; \
		TEXTUS_SNPRINTF(msg, sizeof(msg)-1, x,y,z,w); \
		TEXTUS_SNPRINTF(errMsg, sizeof(errMsg)-1, "%s(%d) %s", __FILE__, __LINE__, msg); \
		printf("%s\n", errMsg); \
		OutputDebugString(errMsg); \
		}
	#else
	#include <stdio.h>
	#define WBUG2(x,y) {printf("%s(%d) ", __FILE__, __LINE__); printf (x,y); printf("\n");}
	#define WBUG3(x,y,z) {printf("%s(%d) ", __FILE__, __LINE__); printf (x,y,z); printf("\n");}
	#define WBUG4(x,y,z,w) {printf("%s(%d) ", __FILE__, __LINE__); printf (x,y,z,w); printf("\n");}
	#endif
#else
#define WBUG2(x,y)
#define WBUG3(x,y,z)
#define WBUG4(x,y,z,w)
#endif

#include "Animus.h"
#include "Notitia.h"
#include "textus_string.h" 
#include "textus_load_mod.h"
#include "casecmp.h" 
#include "hook.h" 
#include "fileOK.c" 
#include <stdio.h>
#include <assert.h>
#if !defined(RTLD_GLOBAL) 
#define RTLD_GLOBAL        0x00100
#endif

#if defined( _WIN32 )
#define DispErrStr(x) MessageBox(NULL, (const char*)x, TEXT("Error"), MB_OK); 
#else
#define DispErrStr(x) fprintf(stderr, "%s\n", x)
#endif

static void error_pro(const char* so_file);
static char* r_share(const char *so_file);
static TiXmlElement* get_out_prop(TiXmlElement *cfg);
static const char *ld_lib_path = 0;
static unsigned int all_runtimes = 0;
#define RUN_PERIOD 100000000
static unsigned int period_runtimes = RUN_PERIOD;

unsigned int Animus::num_extension = 0;	
bool Animus::pst_added= false;	/* Not loaded any aptus module yet */
struct  Animus::Aptus_Positor *Animus::positor= 0;

/* Loading all extensions of Aptus */
void Animus::emunio(const char *ext_mod)
{
	TiXmlElement *amod;
	int mod_num, index = 0 ;
	const char* disable_str;
	unsigned char block[8];

	num_extension = 0;
	/* how many extensions of Aptus? mod_num.  */
	mod_num = 0; amod = carbo->FirstChildElement(ext_mod);
	for (; amod; amod = amod->NextSiblingElement(ext_mod) ) 
	{
		disable_str= amod->Attribute("disable");
		if ( !(disable_str && strcmp(disable_str, "yes") ==0 ) )  
			mod_num ++;
	}
	WBUG2("Animus has %d Attachments.", mod_num);

	pst_added = true;
	if ( mod_num == 0 ) 	/* No Aptus extension */
		return;
	
	memcpy(block, sum_file_vector, 8);

	/* Loading the so/dll file of Aptus extension. positor is the array */
	positor = new struct Aptus_Positor [mod_num];
	memset(positor, 0, mod_num * sizeof(struct Aptus_Positor));
	index = 0;  amod = carbo->FirstChildElement(ext_mod); 
	for (; amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		const char* so_file, *sum; 
		TMODULE ext=NULL;
		const char*so_flag;
		int flag = 0;

		typedef Aptus* (*Create_fun)();
		typedef void (*Destroy_fun)(Aptus*);
		
		Create_fun create_let;
		Destroy_fun destroy_let;
		const char *ap_tag_sym;
		int *pmust = 0;

		/* not enabled, skip it */	
		disable_str= amod->Attribute("disable");
		if ( disable_str && strcmp(disable_str, "yes") ==0  ) 
			continue;

		so_file= amod->Attribute("name");
		sum = amod->Attribute("sum");
		if ( so_file && sum && strcmp(sum, "no") != 0 && 
			strncmp( sumfile(r_share(so_file), 0, 1, block,0), sum,4) != 0  )
		{
			char msg[128];
			TEXTUS_SPRINTF(msg, "Attachment %s checksum error!", so_file);
			DispErrStr(msg);
			continue;
		}

		so_flag= amod->Attribute("flag");
		if ( so_flag && strcmp (so_flag, "GLOB") == 0 )
			flag = RTLD_GLOBAL;

		if (  so_file 
			&& (  ext =TEXTUS_LOAD_MOD(r_share(so_file), flag) )
			&& (TEXTUS_GET_ADDR(ext, TEXTUS_CREATE_AMOR_STR, create_let, Create_fun) )
			&& (TEXTUS_GET_ADDR(ext, "textus_aptus_tag", ap_tag_sym, const char*) )
			&& (TEXTUS_GET_ADDR(ext, TEXTUS_DESTROY_AMOR_STR, destroy_let, Destroy_fun) )
		) {
			WBUG4("Load '%s\' %s is %p", ext_mod, so_file,  ext);
			positor[index].genero	= create_let;
			positor[index].casso	= destroy_let;
			positor[index].carbo	= amod;
			if( amod->Attribute("tag"))	/* prefer the tag attribute of Attachment */
				ap_tag_sym = amod->Attribute("tag");
			positor[index].tag	= ap_tag_sym;

			TEXTUS_GET_ADDR(ext, "textus_aptus_must_ignite", pmust, int*) ;
			if ( pmust ) 
			{	
				positor[index].must	= *pmust;
			}
			index++;
		} else {
			if (!ext) 
				error_pro(so_file);
		}
	}
	num_extension = index;	/* num_extension are  really avaliable  */
	return ;	
}

 /* Load the modules of a child node */
void Animus::tolero(const char *ext_mod)
{
	TiXmlElement *amod;
	const char* disable_str;
	int mod_num =0, index = 0;
	unsigned char block[8];

	duco_num = 0;

	/* how many of so files? mod_num. */
	for (mod_num = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		disable_str= amod->Attribute("disable");
		if ( !(disable_str && strcmp(disable_str, "yes") ==0 ) )
			mod_num ++;
	}
	WBUG3("%s has %d Modules.", carbo->Attribute("name")==(const char*)0 ? carbo->Value(): carbo->Attribute("name"), mod_num);
	if ( mod_num == 0 )
		return;
	
	memcpy(block, sum_file_vector, 8);

	/* Load every module */
	compactor = new Aptus* [mod_num];
	memset(compactor, 0, sizeof(Aptus*) * mod_num );
	for ( index = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		const char  *cfg_file;
		const char* so_file, *sum; 
		TMODULE ext=NULL;
		const char*so_flag;
		int flag = 0;

		typedef Amor* (*Create_it)();
		typedef void (*Destroy_it)(Amor*);
		
		Create_it create_let;
		Destroy_it destroy_let;
		TiXmlDocument *another;
		TiXmlElement *realMod;

		another = 0;
		cfg_file = amod->Attribute("extend");
		if ( cfg_file )
		{	/* configured by another xml file */
			another = new TiXmlDocument();
			if ( !another->LoadFile(cfg_file) || another->Error() )
			{
				char msg[1024];
				TEXTUS_SNPRINTF(msg, 1023, "Animus loading %s file failed %s", cfg_file, another->ErrorDesc());
				DispErrStr(msg);
				delete another;
				continue ;
			} 
			realMod = another->RootElement();
		} else
			realMod = amod;

		/* skip it if not enabled */	
		disable_str= realMod->Attribute("disable");
		if ( disable_str && strcmp(disable_str, "yes") ==0  )
			continue;

		so_file= realMod->Attribute("name");
		sum = realMod->Attribute("sum");
		if ( so_file && sum && strcmp(sum, "no") != 0 && 
			strncmp( sumfile(r_share(so_file), 0, 1, block,0), sum,4) != 0  )
		{
			char msg[128];
			TEXTUS_SPRINTF(msg, "Module %s checksum error!", so_file);
			DispErrStr(msg);
			continue;
		}

		so_flag= realMod->Attribute("flag");
		if ( so_flag && strcmp (so_flag, "GLOB") == 0 )
			flag = RTLD_GLOBAL;

		if ( (  so_file= realMod->Attribute("name")   )
			&& (  ext =TEXTUS_LOAD_MOD( r_share(so_file), flag) )
			&& (TEXTUS_GET_ADDR (ext, TEXTUS_CREATE_AMOR_STR, create_let, Create_it) )
			&& (TEXTUS_GET_ADDR (ext, TEXTUS_DESTROY_AMOR_STR, destroy_let, Destroy_it))
		) {
			WBUG4("Load '%s' %s is %p", ext_mod, so_file, ext);
			compactor[index] = new Animus();			
			memcpy(((Animus*)(compactor[index]))->module_tag, module_tag, sizeof(module_tag));
			compactor[index]->prius	= this;
			compactor[index]->genero= create_let;
			compactor[index]->casso	= destroy_let;
			compactor[index]->carbo	= realMod;
			if ( another )
				compactor[index]->out_cfg_doc	= another;
			index++;
		} else  {
			error_pro(so_file);
			if (ext)
				TEXTUS_FREE_MOD(ext);
		}
	}
	duco_num = index;	/* duco_num is really number of compactor */
	return ;
}

 /* get branches */
void Animus::stipes(const char *bran_tag)
{
	TiXmlElement *aps, *bran_ele;
	const char *dex_tag="dextra", *lae_tag="laeve";
	int index = 0;

	#define IS_BRAN(x) strcasecmp(x, "accept_dextra") ==0  \
		|| strcasecmp(x, "reject_dextra") ==0 \
		|| strcasecmp(x, "accept_laeve") ==0 \
		|| strcasecmp(x, "reject_laeve") ==0

	bran_num = 0;
	aps = carbo->FirstChildElement(bran_tag);
	if ( !aps ) return;
	/* how many of pius accept? bran_num. */
	for ( bran_ele = aps->FirstChildElement();
		bran_ele; bran_ele = bran_ele->NextSiblingElement() )
	{
		if ( IS_BRAN(bran_ele->Value()) )
			bran_num++;
	}

	if ( bran_num == 0 )
		return;
	branch = new struct Branch[bran_num];
	
	index = 0;
	for ( bran_ele = aps->FirstChildElement();
		bran_ele; bran_ele = bran_ele->NextSiblingElement() )
	{
		if ( strcasecmp(bran_ele->Value(), "accept_dextra") == 0  )
		{
			branch[index].coni = BRA_DEXTRA;
			branch[index].act = ACCEPT_BRA;
		} else if ( strcasecmp(bran_ele->Value(), "reject_dextra") == 0  )
		{
			branch[index].coni = BRA_DEXTRA;
			branch[index].act = REJECT_BRA;
		} else if ( strcasecmp(bran_ele->Value(), "accept_laeve") == 0  )
		{
			branch[index].coni = BRA_LAEVE; 
			branch[index].act = ACCEPT_BRA;
		} else if ( strcasecmp(bran_ele->Value(), "reject_laeve") == 0  )
		{
			branch[index].coni = BRA_LAEVE;
			branch[index].act = REJECT_BRA;
		} else if ( strcasecmp(bran_ele->Value(), "set_sponte") == 0  )
		{
			branch[index].coni = BRA_SPONTE;
			branch[index].act = SET_BRA;
		} else {
			continue;
		}
		branch[index].ordo = Notitia::get_ordo(bran_ele->Attribute("ordo"));			
		if ( aps->Attribute("sub"))
			branch[index].sub = atoi(aps->Attribute("sub"));			
		
		index++;
	}

	return ;
}

void Animus::ignite(TiXmlElement *cfg)
{
	unsigned int i,j, k, real_space;
	unsigned int l;
	TiXmlElement *out_cfg;
	TiXmlElement *ap_cfg;
	
	assert(cfg);
	WBUG4("this %p , prius %p, cfg %p", this, prius, cfg);
	carbo = cfg;
	if ( carbo->Attribute("tunnel") && strcasecmp(carbo->Attribute("tunnel"), "no") == 0 )
		isTunnel = false;

	if ( !pst_added )	/* only produce the positor */
		emunio("Attachment");
	
	if (genero) 
	{	/* ignite owner */
		/*Property is the parameter of application*/
		owner = genero();
		owner->aptus = this;
		out_cfg = carbo->FirstChildElement("PropertyExternal");
		if ( !out_cfg )
			out_cfg = carbo->FirstChildElement("ExternalProperty");
		if (out_cfg ) 
			owner->ignite(get_out_prop(out_cfg));
		else
			owner->ignite(carbo->FirstChildElement("Property"));
	}

	tolero(module_tag); /* Just load modules, owner not generated yet */
	stipes("Stipes"); /* obtain branches */
		
	for (l = 0 ; l < duco_num; l++)
		compactor[l]->ignite(compactor[l]->carbo);

	/* cacu the really number of aptus */
#define VALID_AP_TAG(X) \
	strcmp(X, "PropertyExternal") != 0 && strcmp(X, "ExternalProperty") != 0 && strcmp(X, "Property") != 0 && strcmp(X, module_tag) != 0

	num_real_ext = 0;
	for ( ap_cfg = carbo->FirstChildElement(); ap_cfg; ap_cfg = ap_cfg->NextSiblingElement())
	{
		if ( VALID_AP_TAG(ap_cfg->Value()) )
			num_real_ext++;
	}
	
#define	CCO(X)	X = new Aptus* [num_real_ext];
	if ( num_real_ext > 0 )
	{
		CCO(consors)   CCO(cons_fac)   CCO(cons_spo)   CCO(cons_lae)   CCO(cons_dex)
		memset( consors, 0, sizeof(Aptus*) * num_real_ext) ;
	}
	
	k= 0;	/* k is last the num of Aptus objects created really */
	/* 好了, 下面根据Aptus的tag 来创建， 也不用排序 ..... */
	for ( i = 0; i < num_extension; i++ )
	{	/* i is index of Aptus extension，j is index of the instances available really */
		char apTagExtern[2048];
		int tmpLen;

		tmpLen = strlen(positor[i].tag);
		if ( tmpLen > 2000 ) tmpLen = 2000;
		memcpy(apTagExtern, positor[i].tag, tmpLen);
		memcpy(&apTagExtern[tmpLen], "_External", 9);
		apTagExtern[tmpLen+9] = 0;

		if ( positor[i].must == 1 )	/* for version Aptus etc. */
		{
			Aptus *tmp = positor[i].genero();
			tmp->ignite(carbo);
			positor[i].casso(tmp);	/* delete it */
		}
	
		j = 0;	/* j indexs the position for Aptus ele */
		for ( ap_cfg = carbo->FirstChildElement(); ap_cfg; ap_cfg = ap_cfg->NextSiblingElement())
		{
			TiXmlElement *tag_ele ;
			unsigned int sub;
			tag_ele = 0 ;

			if ( VALID_AP_TAG(ap_cfg->Value()) ) j++;
			if ( strcmp(ap_cfg->Value(), apTagExtern) == 0 )
			{
				/* External Config */
				tag_ele = get_out_prop(ap_cfg);

			} else if ( strcmp(ap_cfg->Value(), positor[i].tag) == 0 )
			{
				/* Local Config */
				tag_ele = ap_cfg;
			} else 
				continue;		/* it will left to be a hole if none matches it */

			/* OK, now the Aptus tag matched, */
			sub = j -1;	/* for j++ previously */
			if ( consors[sub] )
			{	/* Sure that some Aptus have the same tag */
				void (*delCon)(Aptus *) = positor[consors[sub]->index_positor].casso;
				delCon(consors[sub]);	/* delete it first */
				k--;
			}

			consors[sub] = positor[i].genero();
			scratch(consors[sub]);
			consors[sub]->ap_tag = positor[i].tag;
			consors[sub]->series = j;
			consors[sub]->ignite_t(positor[i].carbo, tag_ele); 
			consors[sub]->index_positor = i;		/* save the index of positor of casso */
			if ( consors[sub]->canAccessed ) 
			{
				k++; 	/* save it */
			} else {
				positor[i].casso(consors[sub]);	/* delete it */
				consors[sub] = 0;
			}
		}
	}

	/* squeeze the holes */
	real_space = num_real_ext;
	for ( i = 0 ; i < real_space; i ++ )
	{
		while ( consors[i] == (Aptus *)0 )
		{
			num_real_ext--;
			for ( j = i ; j < real_space -1; j ++ )
			{
				consors[j] = consors[j+1];
			}
			if (num_real_ext == k ) break;	/* no holes left */
		}
		if (num_real_ext == k ) break;	/* no holes left */
	}

	for ( i = k ; i < real_space; i ++ )
		consors[i] = 0;

	num_fac = num_spo = num_lae = num_dex = 0;
	/* cons_fac, cons_spo, cons_dex, cons_lae  may need it, just copy the pointer from consors */
	for ( i = 0 ; i < num_real_ext; i++ )
		need(consors[i]);
}

Amor *Animus::clone()
{
	return (Amor*) clone_p(prius);
}

Aptus *Animus::clone_p(Aptus *ma) /* clone self first, then clone the left node */
{
	unsigned int i,j;
	Animus *child = 0;
	Amor *owner_child = 0;

	if ( !owner ) goto Next; /* avoid the root element */
	owner_child = owner->clone();
	if ( owner_child == (Amor*) 0) 
		return (Animus*) 0; /* owner don't allow, so do here */
	
	if ( owner_child == owner ) 
	{	/* Just refer it, increase count */
		refer_count++;
		return this;
	}

Next:
	/* out_cfg_doc is not heritted ; */
	child = new Animus();
	child->isTunnel	= isTunnel;
	child->prius	= ma;	/* new left node */
	child->genero	= genero;
	child->casso	= casso;
	child->carbo	= carbo;
	child->owner	= owner_child;	/* new owner  */
	if ( child->owner ) child->owner->aptus = child;

	if ( duco_num > 0 )
		child->compactor = new Aptus* [duco_num];
	for ( i=0, j=0; i < duco_num; i++ )
	{	/* i indexes the father's compactor, j indexes the child's */
		Aptus *neo = (Aptus*)((Animus*)(compactor[i]))->clone_p(child);
		if ( neo == 0 ) /* not allowed, it will be not used */
			continue;
		else {	
			child->compactor[j] = neo; /* may be the same as the father's */
			j++;
		}
	}
	child->duco_num	= j;	/* j is real number of compactor */

#define	NCO(X)	child->X = new Aptus* [num_real_ext];
	if ( num_real_ext > 0 ) 
	{
		NCO(consors) NCO(cons_fac) NCO(cons_spo) NCO(cons_lae) NCO(cons_dex) 	
	}
	
	j= child->num_fac = child->num_spo = child->num_lae =child->num_dex = 0;
	for (  i = 0; i < num_real_ext; i++)
	{	/* i indexes the father's Aptus extension, j indexes the child's */
		child->consors[j] = consors[i]->clone_p(child);
		child->consors[j]->ap_tag = consors[i]->ap_tag;
		child->consors[j]->index_positor = consors[i]->index_positor;
		child->consors[j]->series = consors[i]->series;

		if ( !(child->consors[j]) || child->consors[j] == consors[i] )	/* must new it! */
			child->consors[j] = positor[consors[i]->index_positor].genero();

		child->scratch(child->consors[j]); 
		if ( child->need(child->consors[j]) )
			j++;  	/* j points the next */
		else
			positor[consors[i]->index_positor].casso(child->consors[j]);	/* delete it */
	}
	child->num_real_ext = j;	/*j is the really number of the attachments of the Animus child */

	return  (Aptus *)child;
}

/* info: from the innerest leaf in a tree and the first tree */
void Animus::info(Amor::Pius &piu)
{
	unsigned int i;
	for (i = 0; i < duco_num; i++)  ((Animus*)compactor[i])->info(piu);

	for (i = 0; i < num_real_ext; i++) consors[i]->facio(&piu);

	if ( owner ) owner->facio(&piu);
}

void Animus::scratch(Aptus *dst)
{
#define  SCRA(X) dst->X = X;
	dst->aptus 	= this;
	SCRA(prius)	SCRA(compactor)	SCRA(duco_num)	SCRA(owner)	SCRA(genero)
	SCRA(casso)	SCRA(carbo)	SCRA(out_cfg_doc)
}

bool Animus::need(Aptus *dst)
{
#define CONS(X) if ( dst->need_##X ) {cons_##X [ num_##X ] = dst; num_##X++; }
	if ( dst->canAccessed) { CONS(fac) CONS(spo) CONS(lae) CONS(dex) }
	return dst->canAccessed;
}

Animus::Animus()
{
	refer_count = 1;	/* intialized as 1, if zero, it should be deleted immediately */

	prius =	0;
	compactor = (Aptus**) 0;
	duco_num = 0;
	owner = 0;
	out_cfg_doc = 0;
	carbo	= 0;

	genero 	= 0;
	casso 	= 0;

	aptus = 0;
	consors = 0;	
	num_real_ext = 0;
	memset(module_tag, 0, sizeof(module_tag));
	
	cons_spo = cons_fac = cons_lae = cons_dex = (Aptus **)0;
	num_spo = num_fac = num_lae = num_dex = 0;
	
	canAccessed = true;	/* it is not usefull although */
	isTunnel = true;	/* if a owner does not proccess a pius, the pius wiil be passed through. */
	branch = 0;
	bran_num = 0;
}

void Animus::destroy_right()
{
	unsigned int i;
	for (i = 0 ; i < duco_num; i++ )
	{
		Animus* pc = (Animus*)compactor[i];
		pc->refer_count--;
		if (pc->refer_count == 0) delete pc;
	}
	duco_num = 0;
	if ( compactor ) delete []compactor;	
	compactor = 0;
}

Animus::~Animus()
{
	unsigned int i;
	destroy_right();
		
	if( casso) casso(owner);
	owner = 0;	
	for (i = 0 ; i < num_real_ext; i++ )
	{
		void (*delCon)(Aptus *) = positor[consors[i]->index_positor].casso;
		delCon(consors[i]);
	}
	
	if ( consors ) 		delete []consors;
	if ( cons_fac ) 	delete []cons_fac;
	if ( cons_spo ) 	delete []cons_spo;
	if ( cons_lae ) 	delete []cons_lae;
	if ( cons_dex ) 	delete []cons_dex;
	if ( branch )		delete (struct Branch *) branch;
}

/* owner call this function */
inline bool Animus::sponte( Amor::Pius *pius)
{
	if (  branch ) 
	{
		if ( !branch_pro(pius,BRA_SPONTE)) 
			goto LAST;
	}
	switch ( pius->ordo )
	{
	case Notitia::CMD_GET_OWNER:
		pius->indic = owner;
		break;

	case Notitia::SET_SAME_PRIUS:
		if ( pius->indic )
		{
			Aptus *n;
			n = (Aptus*)(((Amor * )pius->indic)->aptus) ;
			n->prius = prius;
		}
		break;
	default:
		return sponte_n(pius, 0);
		break;
	}
LAST:
	return true;
}

inline bool Animus::sponte_n( Amor::Pius *pius, unsigned int from)
{
	unsigned int i;
	for ( i = from ; i < num_spo; i++ )
		if (cons_spo[i]->sponte_n(pius, i))  
			return true;
	
	if( prius)  
		return prius->laeve(pius, 0) ;
	return true;
}

/* the child node calls this function. */
inline bool Animus::laeve( Amor::Pius *pius, unsigned int from)
{
	unsigned int i;
	if (  branch ) 
	{
		if ( !branch_pro(pius,BRA_LAEVE)) 
			goto LAST;
	}
	for (i = from; i < num_lae; i++)
		if (cons_lae[i]->laeve(pius, i))  
			return true;
	if ( !owner) 
	{
		if( prius)  
			return prius->laeve(pius, 0) ;
		else
			return true;
	}

	if ( isTunnel )
	{
		if ( !(owner->sponte(pius)))  /* pass it to the prius if owner does not process the pius */
			sponte_n(pius, 0);
	} else
		owner->sponte(pius);
LAST:	
	return true;			
}

/*owner calls this function */
inline bool Animus::facio(Amor::Pius *pius)
{
	register unsigned int i;
	if ( all_runtimes != 0 ) 	/* for all_runtimes == 0 , it will not run out */
	{ 	
		if ( period_runtimes == 0 )
		{
			all_runtimes--;
			period_runtimes = RUN_PERIOD;
		} else 
			period_runtimes--;

		if ( all_runtimes == 0 ) 	/* run out */
		{
			DispErrStr("TextUS run out!\n");
			exit(1);
		}
	}

	for (i = 0 ; i < num_fac; i++ )
		if ( cons_fac[i]->facio_n(pius,i) ) 
			return true;

	return to_dextra (pius, 0);
}

inline bool Animus::facio_n(Amor::Pius *pius, unsigned int from)
{
	register unsigned int i;

	for (i = from ; i < num_fac; i++ )
		if ( cons_fac[i]->facio_n(pius,i) ) 
			return true;

	return to_dextra (pius, 0);
}

bool Animus::to_dextra(Amor::Pius *pius, unsigned int from)
{
	unsigned int i;
	Aptus **tor;
	if ( from < duco_num )
	for (i= from,tor = &compactor[from], aptus = (Aptus *)0; i < duco_num; i++, tor++)
	{
		(*tor)->dextra(pius, 0);
		if (aptus != (Aptus *)0 ) 
		{
			aptus->facio(pius);	/* the control follow turned to the Aptus extension */
			aptus = (Aptus *)0;
			break;
		}
	}
	return true;
}

/* the parent node call this function */
inline bool Animus::dextra(Amor::Pius *pius, unsigned int from)
{
	unsigned int i;
	if (  branch ) 
	{
		if ( !branch_pro(pius,BRA_DEXTRA)) 
			goto LAST;
	}
	for ( i = from;  i < num_dex;  i++ )
		if (cons_dex[i]->dextra(pius, i)) 
			return true;
	if ( !owner) 
	{
		return	facio_n (pius, 0);/* pass it to the child node if the owner does not proccess it */
	}

	if ( isTunnel )
	{
		if ( !(owner->facio(pius)) ) 
			facio_n (pius, 0);/* pass it to the child node if the owner does not proccess it */
	} else
		owner->facio(pius);
LAST:	
	return true;
}

inline bool Animus::branch_pro( Amor::Pius *pius, enum BRA_DIRECT dir)
{
	bool can;
	unsigned int i;
	can = true;
	for ( i = 0; i < bran_num; i++ )
	{
		if (branch[i].coni != dir || pius->ordo != branch[i].ordo  ) continue; 

		if ( branch[i].act == SET_BRA)
		{
			can = true;
			pius->subor = branch[i].sub ;
		} else if ( branch[i].sub == -1 || pius->subor == branch[i].sub ) 
		{
			can = ( branch[i].act == ACCEPT_BRA);
		} else {
			can = !( branch[i].act == ACCEPT_BRA);
		}
		break;
	}

	return can;
}

char Animus::ver_buf[] = "";
static TiXmlDocument doc;
static Animus *apt=0;
static  int go(char *xml_file, Amor::Pius &para)
{
	TiXmlElement *root;
	int ret = 0;

	char time_str[512], *p;
	char ver_str[64];
	char scmid_str[64];

	Amor::Pius ready = {Notitia::IGNITE_ALL_READY, 0, 0};

#define GETAPRK(X,Y)				\
	p = (char*)X;				\
	while ( *p && *p != ':' ) p++;		\
	p++; while ( *p && *p == ' ' ) p++;	\
	memset(Y, 0, sizeof(Y));		\
	memcpy(Y, p, strlen(p)-1);

	GETAPRK(SCM_MODULE_ID, scmid_str)
	GETAPRK(TEXTUS_BUILDNO, ver_str)
	GETAPRK(TEXTUS_MODTIME, time_str)

	p = scmid_str; while ( *p != '.' ) p++; *p = '\0';
	TEXTUS_SNPRINTF(Animus::ver_buf, sizeof(Animus::ver_buf)-1, "%s\n%s\n%s", scmid_str, time_str, ver_str);

	doc.SetTabSize( 8 );
	if ( !doc.LoadFile (xml_file) || doc.Error()) {
		char msg[1024];
		TEXTUS_SNPRINTF(msg, sizeof(msg), "Loading %s file failed in row %d and column %d, %s\n", xml_file, doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
		DispErrStr(msg);
		return -1;
	} 
	apt = new Animus;
	root = doc.RootElement();
	if ( root) 
	{
		all_runtimes = validate(xml_file, root->Row(), root->Attribute("sum"));
		if( all_runtimes != 0 && all_runtimes < 10 )
		{
			char msg[128];
			TEXTUS_SPRINTF(msg, "Invalid System Definition! Err#: %d", all_runtimes);
			DispErrStr(msg);
			goto END;
		}
		TEXTUS_STRCPY(apt->module_tag, "Module");
		if ( root->Attribute("tag"))
			TEXTUS_STRNCPY(apt->module_tag, root->Attribute("tag"), sizeof(apt->module_tag)-2);

		ld_lib_path = root->Attribute("path");
		apt->ignite(root);
		apt->info(ready);
		apt->facio(&para);
	} else 
		ret = 1;
END:
	//delete apt;
	return ret;
}

extern "C" TEXTUS_AMOR_STORAGE int textus_animus_start(int argc, char *argv[])
{
	void *ps[3];
	Amor::Pius para = {Notitia::MAIN_PARA, 0, 0};
	ps[0] = &argc;
	ps[1] = argv;
	ps[2] = 0;
	para.indic = ps;
	return go ( argv[1], para);
}

char* r_share(const char *so_file)
{
	static char r_file[1024];
	int l = 0, n = 0;
	memset(r_file, 0, sizeof(r_file));
	if ( ld_lib_path && so_file[0] != '\\' && so_file[0] != '/' 
		&& !( strlen(so_file) > 2 && so_file[1] == ':' && 
			(( so_file[0] >= 'a' && so_file[0] <= 'z') || ( so_file[0] >= 'A' && so_file[0] <= 'Z')) ) )
	{
		l = strlen(ld_lib_path);
		if (l > 512 ) l = 512;
	 	memcpy(r_file, ld_lib_path, l);
		n = l;
	}
	l = strlen(so_file);
	if (l > 512 ) l = 512;
	memcpy(&r_file[n], so_file, l);
	TEXTUS_STRCAT(r_file, TEXTUS_MOD_SUFFIX);
	return r_file;
}

/* For a module, get the Property element from an outer xml file */
TiXmlElement* get_out_prop(TiXmlElement *ex_prop)
{
	const char *file_name;
	TiXmlDocument *ou = new TiXmlDocument();
	TiXmlElement *rt, *ele;

	file_name = ex_prop->Attribute("document");
	if (!file_name ) return 0;

	ou->SetTabSize( 8 );
	if ( !ou->LoadFile(file_name) || ou->Error() )
	{
		char msg[1024];
		memset(msg, 0, 1024);
		TEXTUS_SNPRINTF(msg, sizeof(msg), "Loading outer config %s document failed in row %d and column %d: %s\n", file_name, ou->ErrorRow(), ou->ErrorCol(), ou->ErrorDesc());
		DispErrStr(msg);
		delete ou;
		return 0;
	} 
	rt = ou->RootElement();
	ele = ex_prop->FirstChildElement();
	while ( ele )
	{
		rt = rt->FirstChildElement(ele->Value());
		ele = ele->FirstChildElement();
	}
	return rt;
}

#if defined(_WIN32) 
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) { return TRUE; }

extern "C" TEXTUS_AMOR_STORAGE int textus_animus_winstart(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR cmd, int  show)
{
	char *xmlfile, *p;
	void *ps[5];
	Amor::Pius para = {Notitia::WINMAIN_PARA, 0, 0};
	ps[0] = hInst;
	ps[1] = hPrev;
	ps[2] = cmd;
	ps[3] = &show;
	ps[4] = 0;
	para.indic = ps;
	xmlfile = new char[strlen(cmd)+1];
	TEXTUS_STRCPY(xmlfile, cmd);
	p = strstr(xmlfile, " ");	
	if( p ) *p = '\0';
	return go (xmlfile, para);
}

void error_pro(const char* so_file) 
{ 
    char errstr[1024], dispstr[2048];
    DWORD dw = GetLastError(); 

    FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        errstr, 1024, NULL );

    wsprintf(dispstr, "Load module(%s) failed with error %d: %s", (char*)so_file, dw, errstr); 
    MessageBox(NULL, (const char*)dispstr, TEXT("Error"), MB_OK); 
}
#else
void error_pro (const char *so_file)
{
	fprintf(stderr, "Textus cannot load library %s : %s\n", so_file, TEXTUS_MOD_DLERROR);	
}
#endif

unsigned long Notitia::get_ordo(const char *comm_str)
{
	unsigned long ret_ordo;
	if ( !comm_str  )
		return TEXTUS_RESERVED;
	
#define WHAT_ORDO(X,Y) if ( comm_str && strcasecmp(comm_str, #X) == 0 ) Y = X 
#define GET_ORDO(Y) 	\
	Y = Notitia::TEXTUS_RESERVED;	\
	WHAT_ORDO(WINMAIN_PARA , Y); \
	WHAT_ORDO(CMD_MAIN_EXIT , Y); \
	WHAT_ORDO(CLONE_ALL_READY , Y); \
	WHAT_ORDO(CMD_GET_OWNER , Y); \
	WHAT_ORDO(WHO_AM_I, Y); \
	WHAT_ORDO(LOG_EMERG , Y); \
	WHAT_ORDO(LOG_ALERT , Y); \
	WHAT_ORDO(LOG_CRIT , Y); \
	WHAT_ORDO(LOG_ERR , Y); \
	WHAT_ORDO(LOG_WARNING , Y); \
	WHAT_ORDO(LOG_NOTICE , Y); \
	WHAT_ORDO(LOG_INFO , Y); \
	WHAT_ORDO(LOG_DEBUG , Y); \
	WHAT_ORDO(FAC_LOG_EMERG , Y); \
	WHAT_ORDO(FAC_LOG_ALERT , Y); \
	WHAT_ORDO(FAC_LOG_CRIT , Y); \
	WHAT_ORDO(FAC_LOG_ERR , Y); \
	WHAT_ORDO(FAC_LOG_WARNING , Y); \
	WHAT_ORDO(FAC_LOG_NOTICE , Y); \
	WHAT_ORDO(FAC_LOG_INFO , Y); \
	WHAT_ORDO(FAC_LOG_DEBUG , Y); \
	WHAT_ORDO(CMD_GET_VERSION , Y); \
	WHAT_ORDO(CMD_GET_PIUS , Y); \
	WHAT_ORDO(DMD_CONTINUE_SELF , Y); \
	WHAT_ORDO(DMD_STOP_NEXT , Y); \
	WHAT_ORDO(DMD_CONTINUE_NEXT , Y); \
	WHAT_ORDO(CMD_ALLOC_IDLE , Y); \
	WHAT_ORDO(CMD_FREE_IDLE , Y); \
	WHAT_ORDO(DMD_CLONE_OBJ , Y); \
	WHAT_ORDO(CMD_INCR_REFS , Y); \
	WHAT_ORDO(CMD_DECR_REFS , Y); \
	WHAT_ORDO(JUST_START_THREAD , Y); \
	WHAT_ORDO(FINAL_END_THREAD , Y); \
	WHAT_ORDO(NEW_SESSION , Y); \
	WHAT_ORDO(END_SERVICE , Y); \
	WHAT_ORDO(CMD_RELEASE_SESSION , Y); \
	WHAT_ORDO(CHANNEL_TIMEOUT , Y); \
	WHAT_ORDO(CMD_NEW_SERVICE , Y); \
	WHAT_ORDO(START_SERVICE , Y); \
	WHAT_ORDO(DMD_END_SERVICE , Y); \
	WHAT_ORDO(DMD_BEGIN_SERVICE , Y); \
	WHAT_ORDO(END_SESSION , Y); \
	WHAT_ORDO(DMD_END_SESSION , Y); \
	WHAT_ORDO(START_SESSION , Y); \
	WHAT_ORDO(DMD_START_SESSION , Y); \
	WHAT_ORDO(SET_TBUF , Y); \
	WHAT_ORDO(PRO_TBUF , Y); \
	WHAT_ORDO(GET_TBUF , Y); \
	WHAT_ORDO(ERR_FRAME_LENGTH , Y); \
	WHAT_ORDO(ERR_FRAME_TIMEOUT , Y); \
	WHAT_ORDO(FD_SETRD , Y); \
	WHAT_ORDO(FD_SETWR , Y); \
	WHAT_ORDO(FD_SETEX , Y); \
	WHAT_ORDO(FD_CLRRD , Y); \
	WHAT_ORDO(FD_CLRWR , Y); \
	WHAT_ORDO(FD_CLREX , Y); \
	WHAT_ORDO(FD_PRORD , Y); \
	WHAT_ORDO(FD_PROWR , Y); \
	WHAT_ORDO(FD_PROEX , Y); \
	WHAT_ORDO(TIMER , Y); \
	WHAT_ORDO(DMD_SET_TIMER , Y); \
	WHAT_ORDO(DMD_CLR_TIMER , Y); \
	WHAT_ORDO(DMD_SET_ALARM , Y); \
	WHAT_ORDO(PRO_HTTP_HEAD , Y); \
	WHAT_ORDO(CMD_HTTP_GET , Y); \
	WHAT_ORDO(CMD_HTTP_SET , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADBUF , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADOBJ , Y); \
	WHAT_ORDO(CMD_SET_HTTP_HEAD , Y); \
	WHAT_ORDO(PRO_HTTP_REQUEST , Y); \
	WHAT_ORDO(PRO_HTTP_RESPONSE , Y); \
	WHAT_ORDO(HTTP_Request_Complete , Y); \
	WHAT_ORDO(HTTP_Response_Complete , Y); \
	WHAT_ORDO(HTTP_Request_Cleaned , Y); \
	WHAT_ORDO(GET_COOKIE , Y); \
	WHAT_ORDO(SET_COOKIE , Y); \
	WHAT_ORDO(GET_DOMAIN , Y); \
	WHAT_ORDO(HTTP_ASKING , Y); \
	WHAT_ORDO(WebSock_Start , Y); \
	WHAT_ORDO(SET_TINY_XML , Y); \
	WHAT_ORDO(PRO_TINY_XML , Y); \
	WHAT_ORDO(PRO_SOAP_HEAD , Y); \
	WHAT_ORDO(PRO_SOAP_BODY , Y); \
	WHAT_ORDO(ERR_SOAP_FAULT , Y); \
	WHAT_ORDO(CMD_GET_FD , Y); \
	WHAT_ORDO(CMD_SET_PEER , Y); \
	WHAT_ORDO(CMD_GET_PEER , Y); \
	WHAT_ORDO(CMD_GET_SSL , Y); \
	WHAT_ORDO(CMD_GET_CERT_NO , Y); \
	WHAT_ORDO(SET_WEIGHT_POINTER , Y); \
	WHAT_ORDO(TRANS_TO_SEND, Y); \
	WHAT_ORDO(TRANS_TO_RECV, Y); \
	WHAT_ORDO(TRANS_TO_HANDLE, Y); \
	WHAT_ORDO(CMD_BEGIN_TRANS, Y); \
	WHAT_ORDO(CMD_CANCEL_TRANS, Y); \
	WHAT_ORDO(CMD_FAIL_TRANS, Y); \
	WHAT_ORDO(CMD_RETAIN_TRANS, Y); \
	WHAT_ORDO(CMD_END_TRANS, Y); \
	WHAT_ORDO(CMD_FORK , Y); \
	WHAT_ORDO(FORKED_PARENT , Y); \
	WHAT_ORDO(FORKED_CHILD , Y); \
	WHAT_ORDO(NEW_HOLDING , Y); \
	WHAT_ORDO(AUTH_HOLDING , Y); \
	WHAT_ORDO(HAS_HOLDING , Y); \
	WHAT_ORDO(CMD_SET_HOLDING , Y); \
	WHAT_ORDO(CMD_CLR_HOLDING , Y); \
	WHAT_ORDO(CLEARED_HOLDING , Y); \
	WHAT_ORDO(SET_UNIPAC , Y); \
	WHAT_ORDO(PRO_UNIPAC , Y); \
	WHAT_ORDO(ERR_UNIPAC_COMPOSE , Y); \
	WHAT_ORDO(ERR_UNIPAC_RESOLVE , Y); \
	WHAT_ORDO(ERR_UNIPAC_INFO, Y); \
	WHAT_ORDO(MULTI_UNIPAC_END,Y); \
	WHAT_ORDO(CMD_SET_DBFACE , Y); \
	WHAT_ORDO(CMD_SET_DBCONN , Y); \
	WHAT_ORDO(CMD_DBFETCH , Y); \
	WHAT_ORDO(CMD_GET_DBFACE , Y); \
	WHAT_ORDO(CMD_DB_CANCEL , Y); \
	WHAT_ORDO(PRO_DBFACE , Y); \
	WHAT_ORDO(IC_DEV_INIT_BACK, Y); \
	WHAT_ORDO(IC_DEV_INIT, Y); \
	WHAT_ORDO(IC_DEV_QUIT, Y); \
	WHAT_ORDO(IC_OPEN_PRO, Y); \
	WHAT_ORDO(IC_CLOSE_PRO, Y); \
	WHAT_ORDO(IC_PRO_COMMAND, Y); \
	WHAT_ORDO(IC_SAM_COMMAND, Y); \
	WHAT_ORDO(IC_RESET_SAM, Y); \
	WHAT_ORDO(IC_PRO_PRESENT, Y); \
	WHAT_ORDO(ICC_Authenticate, Y); \
	WHAT_ORDO(ICC_Read_Sector, Y); \
	WHAT_ORDO(ICC_Write_Sector, Y); \
	WHAT_ORDO(ICC_Reader_Version, Y); \
	WHAT_ORDO(ICC_Led_Display, Y); \
	WHAT_ORDO(ICC_Audio_Control , Y); \
	WHAT_ORDO(ICC_GetOpInfo, Y); \
	WHAT_ORDO(ICC_Get_Card_RFID, Y); \
	WHAT_ORDO(ICC_Get_CPC_RFID , Y); \
	WHAT_ORDO(ICC_Get_Flag_RFID, Y); \
	WHAT_ORDO(ICC_Get_Power_RFID, Y); \
	WHAT_ORDO(ICC_Set433_Mode_RFID, Y); \
	WHAT_ORDO(ICC_Get433_Mode_RFID, Y); \
	WHAT_ORDO(ICC_CARD_open, Y); \
	WHAT_ORDO(URead_ReLoad_Dll, Y); \
	WHAT_ORDO(URead_UnLoad_Dll, Y); \
	WHAT_ORDO(URead_Load_Dll, Y); \
	if ( Y == Notitia::TEXTUS_RESERVED && comm_str && atoi(comm_str) >= 0) 	\
		Y = atoi(comm_str);

	GET_ORDO(ret_ordo);
	return ret_ordo;
#undef GET_ORDO
#undef WHAT_ORDO
}
