/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <windows.h>
__declspec(dllimport) int textus_animus_winstart(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int   nCmdShow);

int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR    lpCmdLine, int       nCmdShow)
{
	if ( !lpCmdLine || strlen(lpCmdLine) <= 0  )
	{
        	MessageBox(NULL, "Please use a xml file.", TEXT("Error"), MB_OK); 
        	return 1;
        }
	return textus_animus_winstart(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
