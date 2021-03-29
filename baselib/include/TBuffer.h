/**
 ID: 		Textus-TBuffer.h
 Title: 	缓冲器
 Description:	希望能弄个更好的. 该类不要被继承,只是作为一个数据对象,省点代码
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

#if !defined(TEXTUS_LONG)
#if defined(__LP64__)
// LP64 machine, OS X or Linux
#define TEXTUS_LONG long
#elif defined(__LLP64__) ||  defined(_M_X64)  ||  defined(_WIN64)
// LLP64 machine, Windows
#define TEXTUS_LONG __int64
#else
// 32-bit machine, Windows or Linux or OS X
#define TEXTUS_LONG long
#endif
#endif

#ifndef TBUFFER__H
#define TBUFFER__H
#define DEFAULT_TBUFFER_SIZE     4096
#define TBINLINE  
class TEXTUS_AMOR_STORAGE TBuffer
{
public:
	TBuffer(unsigned TEXTUS_LONG size = DEFAULT_TBUFFER_SIZE);
	~TBuffer();
	
	unsigned char * base;	//缓冲区的底
	unsigned char * point;	//这点以前, 是有效数据, 这点(及此点)以后是空的.
	unsigned char * limit;	//缓冲区的顶

	TBINLINE void grant(unsigned TEXTUS_LONG space); //保证空余空间有space那么大
	TBINLINE void input(unsigned char *val, unsigned TEXTUS_LONG len); /* 输入val内容, len为字节数 */
	/* 数据读入(出)后的处理, 如果不调用此, 则数据仍留在其中 */
	TBINLINE TEXTUS_LONG commit(TEXTUS_LONG len);	/* len >0, 增加数据, len为数据的长度
					   len <0, 减少数据, -len为数据的长度
					返回Buffer还剩余的空间大小值 */
	void reset();
	static void exchange(TBuffer &a, TBuffer &b); /* 将a与b交换空间 */
	static void pour(TBuffer &dst, TBuffer &src); /* 将src中的数据倒入到dst中 */
	static void pour(TBuffer &dst, TBuffer &src, unsigned TEXTUS_LONG n); /* 将src中的n字节数据倒入到dst中 */
private:
	TBINLINE void expand(unsigned TEXTUS_LONG extraSize);
};
#endif
