/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#if defined(_WIN32)
#include <windows.h>
__declspec(dllimport) int textus_animus_start(char *);
#else
#include <stdio.h>
 extern int textus_animus_start(char *);
#endif
#include "tinyxml/tinyxml.h"
int main (int argc, char *argv[])
{
	TiXmlDocument doc;	
        if (argc <=1 ) 
        {
#if defined(_WIN32)        	
        	MessageBox(NULL, "Usage: main xmlfile", TEXT("Error"), MB_OK); 
#else
		printf("Usage:%s xmlfile\n",argv[0]);
#endif
        	return 1;
        }
	//return textus_animus_start(argv[1]);
        //return 0;
}
