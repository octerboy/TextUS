/**
 标题:MiniDB类的定义
 标识:Textus-MiniDB.h
 版本:B01
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
