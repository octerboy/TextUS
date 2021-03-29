/**
 ID: 		Textus-TBuffer.h
 Title: 	������
 Description:	ϣ����Ū�����õ�. ���಻Ҫ���̳�,ֻ����Ϊһ�����ݶ���,ʡ�����
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
	
	unsigned char * base;	//�������ĵ�
	unsigned char * point;	//�����ǰ, ����Ч����, ���(���˵�)�Ժ��ǿյ�.
	unsigned char * limit;	//�������Ķ�

	TBINLINE void grant(unsigned TEXTUS_LONG space); //��֤����ռ���space��ô��
	TBINLINE void input(unsigned char *val, unsigned TEXTUS_LONG len); /* ����val����, lenΪ�ֽ��� */
	/* ���ݶ���(��)��Ĵ���, ��������ô�, ���������������� */
	TBINLINE TEXTUS_LONG commit(TEXTUS_LONG len);	/* len >0, ��������, lenΪ���ݵĳ���
					   len <0, ��������, -lenΪ���ݵĳ���
					����Buffer��ʣ��Ŀռ��Сֵ */
	void reset();
	static void exchange(TBuffer &a, TBuffer &b); /* ��a��b�����ռ� */
	static void pour(TBuffer &dst, TBuffer &src); /* ��src�е����ݵ��뵽dst�� */
	static void pour(TBuffer &dst, TBuffer &src, unsigned TEXTUS_LONG n); /* ��src�е�n�ֽ����ݵ��뵽dst�� */
private:
	TBINLINE void expand(unsigned TEXTUS_LONG extraSize);
};
#endif
