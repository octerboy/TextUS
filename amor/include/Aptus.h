/**
 ID: Textus-Amor-Aptus.h
 Title: Aptus class definition
 Build: B01
*/
#ifndef APTUS_H
#define APTUS_H
#include "Amor.h"

class TEXTUS_AMOR_STORAGE Aptus: public Amor {
public:
	Aptus() { canAccessed = need_spo = need_fac = need_lae = need_dex = false; index_positor = 0; ap_tag = 0; series = 0; };

	virtual void ignite	(TiXmlElement *wood) { return; } ;
	virtual void ignite_t	(TiXmlElement *wood, TiXmlElement *tag) = 0 ;
	virtual bool facio	(Amor::Pius *) { return false;} ;
	virtual bool facio_n	(Amor::Pius *, unsigned int ) { return false;} ;
	virtual bool sponte	(Amor::Pius *pius) { return false;} ;
	virtual bool sponte_n	(Amor::Pius *pius, unsigned int ) { return false;} ;
	virtual Amor *clone() = 0;
	virtual Aptus *clone_p	(Aptus *p) { return (Aptus*) this->clone();} ;

	void inherit(Aptus *child) {
		#define  INHER(X) child->X = X;
		INHER(canAccessed) INHER(need_fac)  INHER(need_spo)  INHER(need_lae)  INHER(need_dex)
	} ;
	
	virtual bool dextra(Amor::Pius *, unsigned int) { return false;} ;
	virtual bool laeve( Amor::Pius *pius, unsigned int) { return false;} ;
	
	Aptus *prius;	
	Aptus **compactor;
	unsigned int duco_num;

	Amor *owner;	
	Amor* (*genero)();		/* the function pointer to create owner  */
	void (*casso)(Amor *);		/* the function pointer to destroy owner */
	TiXmlElement *carbo;	
	TiXmlDocument *out_cfg_doc;	/* the file of xml */
	const char *ap_tag;
	
	bool  canAccessed;		/* true: The Aptus extension is saved by Animus */
	bool need_fac;	/* true: call Aptus extension when owner calls facio() */
	bool need_spo;	/* true: call Aptus extension when owner calss sponte() */
	bool need_lae;	/* true: call Aptus before owner->sponte() */
	bool need_dex;	/* true: class Aptus before owner->facio() */

	int series;
	unsigned int index_positor;            /* the array of index of positor */
};
#endif
