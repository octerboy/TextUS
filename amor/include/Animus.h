/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: The class definition of Animus
 Build: created by octerboy, 2006/03/10, Shenyang
 $Header: /textus/amor/include/Animus.h 27    10-12-11 21:07 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef ANIMUS_H
#define ANIMUS_H
#include "Aptus.h"

class TEXTUS_AMOR_STORAGE Animus: public Aptus {
public:
	/* from Aptus class definition */
	void ignite(TiXmlElement *wood);	
	void ignite_t (TiXmlElement *wood, TiXmlElement *tag) { return; }
	bool facio(Amor::Pius *);
	bool facio_n(Amor::Pius *, unsigned int);
	bool to_dextra(Amor::Pius *, unsigned int);
	bool sponte( Amor::Pius *);
	bool sponte_n( Amor::Pius *, unsigned int);
	Amor *clone();
	Aptus *clone_p(Aptus *);

	bool dextra(Amor::Pius *, unsigned int);	/* it will call the child node */
	bool laeve( Amor::Pius *pius, unsigned int);	/* it will be called by the child node */

	Animus();
	~Animus();

	/* this class special definition */
	static char ver_buf[128];	/* version infomation */
	char module_tag[128];		/* often is "Module" */

	struct  Aptus_Positor  {		/* the structure of Aptus extension module */
		Aptus* (*genero)();		/* the function pointer to create Aptus object */
		void (*casso)(Aptus *);		/* the function pointer to destroy Aptus object */
		TiXmlElement *carbo;
		const char *tag;		/* the tag of xml element for load module */
		int must;			/* should ignite() */
		inline  Aptus_Positor () {
			genero = 0;
			casso  = 0;
			carbo = 0;
			tag = 0;
			must = 0;
		}
	};
			
	static struct Aptus_Positor *positor ; 
	static unsigned int num_extension;	/* the number of Aptus extension modules. -1: Not loadded */
	static bool pst_added;			/* false: not load the aptus yet */

	int refer_count;
	bool ignite_info_ready;		//whether info IGNITE_ALL_READY
	void info(Amor::Pius &);
	void destroy_right();
	
	Aptus **consors;		/* the array of Aptus objects */
	unsigned int num_real_ext;		/* the number of Aptus objects and index of positor */

	enum BRA_DIRECT {BRA_LAEVE= 0, BRA_SPONTE = 1, BRA_DEXTRA = 2 };
	enum BRA_ACT { 	ACCEPT_BRA = 0, REJECT_BRA = 1, SET_BRA = 2 };

	struct Branch {			/* branch, a node will accept only a pius which has an ordo & sub if it matches */
		TEXTUS_ORDO ordo;	/* if a pius.ordo == ordo, the node only accept it when the pius.subdo==sub (>0) */
		int sub;
		enum BRA_ACT act;
		inline Branch() {
			ordo = 0;
			sub = -1;
			act = ACCEPT_BRA;
		};
	};
	struct Branch *branch_dex, *branch_lae, *branch_spo;
	unsigned int  bran_num_dex, bran_num_lae, bran_num_spo;
	int  unique_sub;	/* the sub only to this module */
	Animus **unisub_branch;
	int unisub_max;

private:
	void stipes(const char*);	/* set branch according to pius */
	void stipes_unisub();		/* set branch according to pius */
	void tolero(const char*);	/* load Amor module */
	void emunio(const char*);	/* Load Aptus extension module */
	inline void scratch(Aptus * dst);
	inline bool need(Aptus * dst);
	
	Aptus **cons_spo;
	unsigned int num_spo;
	
	Aptus **cons_fac;
	unsigned int num_fac;

	Aptus **cons_lae;
	unsigned int num_lae;

	Aptus **cons_dex;
	unsigned int num_dex;	

	bool isTunnel;
	inline bool branch_pro(Amor::Pius *pius, enum BRA_DIRECT);
};
#endif
