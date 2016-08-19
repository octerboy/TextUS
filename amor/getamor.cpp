#include "tinyxml.h"
#define MOD_SUFFIX ".so"
static const char *ld_lib_path = 0;

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
	strcat(r_file, MOD_SUFFIX);
	return r_file;
}

void tolero(const char *ext_mod, TiXmlElement *carbo, char *snumber)
{
	TiXmlElement *amod;
	const char* disable_str;
	int mod_num =0, index = 0;
	int duco_num = 0;
	char *loc_snumber = (char*) 0;

	/* how many of so files? mod_num. */
	for (mod_num = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		disable_str= amod->Attribute("disable");
		if ( !(disable_str && strcmp(disable_str, "yes") ==0 ) )
			mod_num ++;
	}
	
	/* Load every module */
	printf ("void Animus%s::tolero(const char *ext_mod) {\n", snumber);
	printf ("\tduco_num=%d", mod_num);
	loc_snumber = new char [strlen(snumber)+3];
	if ( mod_num == 0 )
		goto PEND;

	printf ("\tcompactor = new Aptus* [%d];\n", mod_num);
	printf ("\tmemset(compactor, 0, sizeof(Aptus*) * %d);\n", mod_num );
	printf ("\tTMODULE ext=NULL;\n");
	printf ("\ttypedef Amor* (*Create_it)();\n");
	printf ("\ttypedef void (*Destroy_it)(Amor*);\n");
		
	printf ("\tCreate_it create_let;\n");
	printf ("\tDestroy_it destroy_let;\n");

	for ( index = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		const char  *cfg_file;
		const char* so_file;
		const char*so_flag;
		int flag = 0;

		TiXmlDocument *another;
		TiXmlElement *realMod;

		another = 0;
		realMod = amod;

		/* skip it if not enabled */	
		disable_str= realMod->Attribute("disable");
		if ( disable_str && strcmp(disable_str, "yes") ==0  )
			continue;

		so_file= realMod->Attribute("name");

		so_flag= realMod->Attribute("flag");
		if ( so_flag && strcmp (so_flag, "GLOB") == 0 )
			so_flag = "RTLD_GLOBAL";
		else
			so_flag ="";

			printf("\text =TEXTUS_LOAD_MOD( \"%s\", %s);\n", r_share(so_file), so_flag);
			printf("\tTEXTUS_GET_ADDR(ext, TEXTUS_CREATE_AMOR_STR, create_let, Create_it);\n");
			printf("\tTEXTUS_GET_ADDR(ext, TEXTUS_DESTROY_AMOR_STR, destroy_let, Destroy_it);\n");
			sprintf(loc_snumber, "%s%d",  snumber, index);
			printf("\tcompactor[%d] = new Animus%s();\n", index, loc_snumber);			
			printf("\tstrcpy(((Animus*)(compactor[%d]))->module_tag, \"Module\");\n", index);
			printf("\tcompactor[%d]->prius	= this;\n", index);
			printf("\tcompactor[%d]->genero= create_let;\n",index);
			printf("\tcompactor[%d]->casso	= destroy_let;\n", index);
			printf("\tcompactor[%d]->carbo	= realMod;\n", index);
			index++;
	}
	duco_num = index;	/* duco_num is really number of compactor */

PEND:
	printf("}\n\n");
	for ( index = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		/* skip it if not enabled */	
		disable_str= amod->Attribute("disable");
		if ( disable_str && strcmp(disable_str, "yes") ==0  )
			continue;

		sprintf(loc_snumber, "%s%d",  snumber, index);
		//printf("loc_snumber %s\n", loc_snumber);
		tolero(ext_mod, amod, loc_snumber);
		index++;
	}

	if ( loc_snumber)
		delete [] loc_snumber;
	return ;
}

void pre_tolero(const char *ext_mod, TiXmlElement *carbo, char *snumber)
{
	TiXmlElement *amod;
	const char* disable_str;
	int mod_num =0, index = 0;
	int duco_num = 0;
	char *loc_snumber = (char*) 0;

	/* Load every module */
	printf ("class Animus%s:public Animus\n{\nprotected:\n", snumber);
	printf ("\tvoid tolero(const char*);\n");
	printf("};\n\n");
	loc_snumber = new char [strlen(snumber)+3];
	for ( index = 0, amod = carbo->FirstChildElement(ext_mod);
		amod; amod = amod->NextSiblingElement(ext_mod) )
	{
		/* skip it if not enabled */	
		disable_str= amod->Attribute("disable");
		if ( disable_str && strcmp(disable_str, "yes") ==0  )
			continue;

		sprintf(loc_snumber, "%s%d",  snumber, index);
		//printf("loc_snumber %s\n", loc_snumber);
		pre_tolero(ext_mod, amod, loc_snumber);
		index++;
	}

	if ( loc_snumber)
		delete [] loc_snumber;
	return ;
}

int main ( int argc, char *argv[])
{
	TiXmlDocument doc;
	TiXmlElement *root;
	int ret;

	char *xml_file = argv[1];
	doc.SetTabSize( 8 );
	if ( !doc.LoadFile (xml_file) || doc.Error()) {
		printf("Loading %s file failed in row %d and column %d, %s\n", xml_file, doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
		return -1;
	} 
	root = doc.RootElement();
	if ( root) 
	{
		ld_lib_path = root->Attribute("path");
		if ( argc >=3 )
		{
			pre_tolero(argv[2], root, "s");
			tolero(argv[2], root, "s");
		} else {
			pre_tolero("Module", root, "s");
			tolero("Module", root, "s");
		}
	} else 
		ret = 1;
END:
	return ret;
}

