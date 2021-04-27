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
#define TBINLINE  inline
#include <string.h>

class TEXTUS_AMOR_STORAGE TBuffer
{
public:
	TBuffer(unsigned TEXTUS_LONG size = DEFAULT_TBUFFER_SIZE);
	~TBuffer();
	
	unsigned char * base;	//�������ĵ�
	unsigned char * point;	//�����ǰ, ����Ч����, ���(���˵�)�Ժ��ǿյ�.
	unsigned char * limit;	//�������Ķ�

	void grant(unsigned TEXTUS_LONG space) {
		if ( point +space > limit ) this->expand(space);
	}; //��֤����ռ���space��ô��
	void input(unsigned char *p, unsigned TEXTUS_LONG n) {
		if ( point +n > limit ) this->expand(n);
		memcpy(point, p, n);
		point += n;
	}; /* ����val����, lenΪ�ֽ��� */
	/* ���ݶ���(��)��Ĵ���, ��������ô�, ���������������� */
	TEXTUS_LONG commit(TEXTUS_LONG len);	/* len >0, ��������, lenΪ���ݵĳ���
					   len <0, ��������, -lenΪ���ݵĳ���
					����Buffer��ʣ��Ŀռ��Сֵ */
	void commit_ack(TEXTUS_LONG len) { point += len; } ; /* ȷ�����ύ */
	void reset() { point = base;};
	static void exchange(TBuffer &a, TBuffer &b) { register unsigned char * med;
		med = b.base; b.base=a.base; a.base = med;
		med = b.point; b.point=a.point; a.point = med;
		med = b.limit; b.limit=a.limit; a.limit = med;
	}; /* ��a��b�����ռ� */
	static void pour(TBuffer &dst, TBuffer &src); /* ��src�е����ݵ��뵽dst�� */
	static void pour(TBuffer &dst, TBuffer &src, unsigned TEXTUS_LONG n); /* ��src�е�n�ֽ����ݵ��뵽dst�� */
private:
	TBINLINE void expand(unsigned TEXTUS_LONG extraSize);
};
#endif
