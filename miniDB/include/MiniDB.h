/**
 ����:MiniDB��Ķ���
 ��ʶ:Textus-MiniDB.h
 �汾:B01
	B01:created by octerboy 2005/06/20
*/
#ifndef MINIDB_H
#define	MINIDB_H
#include "fastdb.h"
#include "Amor.h"
#include "Aptus.h"

class MiniDB :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	MiniDB *clone();
	Aptus *aptus;

	static dbDatabase *hand();
	MiniDB();
	~MiniDB();

protected:
	static dbDatabase *dbh;

private:
	static char *dbfile;
};
#endif
