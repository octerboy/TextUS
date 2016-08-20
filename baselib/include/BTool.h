/**
 ID: Textus-BTool.h
 Title: 基本工具
 Build: B01
 Modify history:
        B01:created by octerboy, 2005/06/10
*/
#if defined(_WIN32) 
#include <windows.h>
#endif 

#if !defined(TEXTUS_AMOR_STORAGE)
#if defined(_WIN32) 
#define TEXTUS_AMOR_STORAGE __declspec(dllimport) 
#else
#define TEXTUS_AMOR_STORAGE
#endif
#endif

#ifndef BTOOL__H
#define BTOOL__H
class TEXTUS_AMOR_STORAGE BTool
{
public:
	static	char *getaddr ( const char *filename, const char *key);
	static	char *getaddr ( const char *filename, const char *key, const char *split, int no);
	static	bool putaddr ( const char* filename, const char*key, const char* value);
	static	bool putaddr ( const char* filename, const char*key, const char* split, const char* value,int no);
	/* base64编码与解码函数,返回已解码或编码内容的长度 */
	static int base64_encode(char* encoded, const unsigned char * plain, int len);	
	static int base64_decode(const char* encoded, unsigned char* plain, int size);

	/* Remove "\" escapes from s..., to t...., return the length of t */
	static unsigned int unescape( const char *s, unsigned char *t);
};
#endif
