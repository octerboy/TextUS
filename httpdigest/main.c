#include "md5.h"
int main (int argc, char* argv[])
{
      MD5_CTX Md5Ctx;
 	char *str="asdfas3423asdfasasde34cv239;'aa'sdla'sdlk;";
	unsigned char md[16];
	int i;

      MD5Init(&Md5Ctx);
      MD5Update(&Md5Ctx, str, strlen(str));
      MD5Update(&Md5Ctx, str, strlen(str));
      MD5Final(md, &Md5Ctx);
	for ( i = 0 ; i < 16; i++)
	printf("%02x", md[i]);
	printf("\n");
	return 0;

}
