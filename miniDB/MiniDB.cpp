/**
 标题:统一使用内存数据库
 标识:Textus-MiniDB.cpp
 版本:B01
	B01:created by octerboy 2005/06/20
*/

#include "MiniDB.h"
#include "SecLog.h"
#include "BTool.h"
#ifndef NDEBUG
#include <iostream.h>
#endif

dbDatabase *MiniDB::dbh = 0;
char* MiniDB::dbfile = 0;

#define HaveValue(x) ((x) \
	&& (x)->FirstChild() \
	&& (x)->FirstChild()->ToText() \
	&& (x)->FirstChild()->ToText()->Value())

#define GetValue(x) ((x)->FirstChild()->ToText()->Value())

void MiniDB::ignite(TiXmlElement *cfg)
{
	SecLog *log = SecLog::instance();
	TiXmlElement *dbfile_ele;

	dbfile_ele = cfg->FirstChildElement("dbfile");
	
	if ( !dbfile ) 
	{
		dbfile = new char[256];
		*dbfile = '\0';
	}
	if ( HaveValue(dbfile_ele) )
	{
#ifndef NDEBUG
		cout << APPSTR << "::dbfile_ele "<< GetValue(dbfile_ele) << endl;
#endif
		strncpy(dbfile, GetValue(dbfile_ele), 255);
	}
#ifndef NDEBUG
	cout << APPSTR << "::dbfile "<< dbfile << endl;
#endif

	BTool::voco("MiniDB Ver 1.0.0");
}

bool MiniDB::facio( Amor::Pius *pius)
{
	return false;
}

bool MiniDB::sponte( Amor::Pius *pius)
{
	return false;
}

MiniDB::MiniDB()
{ }

MiniDB* MiniDB::clone()
{
	MiniDB *child = new MiniDB();
	return child;
}

MiniDB::~MiniDB()
{
	if(dbh && dbh->isOpen() ) 
	{
		dbh->close();
		delete (dbDatabase *) dbh;
	}
}

dbDatabase *MiniDB::hand()
{
	SecLog *log = SecLog::instance();

	if ( dbh ) return dbh;
	dbh = new dbDatabase();
	if ( !dbh->open(dbfile))
	{
		log->w(SecLog::EMERG, -1, " MiniDB::Opening database failed from %s", dbfile);
#ifndef NDEBUG
		cout<<"MiniDB"<<"::Opening database failed from " << dbfile <<".fdb"<<endl; 
#endif
		delete (dbDatabase *)dbh;
		dbh = (dbDatabase *) 0;
		return 0;
	}

	return dbh;
}

#define AMOR_CLS_TYPE MiniDB
#include "hook.c"


