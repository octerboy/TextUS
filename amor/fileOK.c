#include "pinkey.h"
#include "textus_string.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#define LINE_MAXLEN 1024
static inline unsigned int validate(char* file_name, int row, const char *sum);
/* ����ֵΪ0: ��ʾOK 
	   1: ����
	   2: У���
	   3: ���ݴ���
	   4: �ļ�����
	 >10: �������ƴ���
 */
unsigned int validate(char* filename, int row, const char *sum)
{
  char *p;
  int i,t,check,encrypt,runtimes,expired;
  char offset[17];
  time_t mtime, expired_time;
  unsigned char lrcblock[8];
  int lrc;
	
  int pinkeyA[64], pinkeyB[64], pinkeyC[64];
  int Chiper_text[64],Clear_text[64];

	if ( !sum ) 
		return 2;

  for ( i = 0 ; i < 64 ; i ++ ) pinkeyA[i] = pinkeys[0][i];
  for ( i = 0 ; i < 64 ; i ++ ) pinkeyB[i] = pinkeys[1][i];
  for ( i = 0 ; i < 64 ; i ++ ) pinkeyC[i] = pinkeys[2][i];

  p = (char*) sum;
  for ( i=0; i<16; i++ )
  {
     t = (p[i] > '9' ? p[i]-'A'+10: p[i] -'0');
     	     Chiper_text[i*4] = t/8;
     t %= 8; Chiper_text[i*4+1] = t/4;
     t %= 4; Chiper_text[i*4+2] = t/2;
     t %= 2; Chiper_text[i*4+3] = t;
  }

  /* sum���� */
  xdes(Chiper_text,pinkeyA,Clear_text,-1,0);
  /* lrc ΪУ��� */
  lrc = 0;
  for ( i=0; i<16; i++ )
  {
     t=	Clear_text[i*4]*8 + Clear_text[i*4+1]*4 +
     	Clear_text[i*4+2]*2 + Clear_text[i*4+3];
     if( i < 15 ) lrc ^= t;

     offset[i] = ObtainX(t);
  }
  offset[16] = '\0';

  /* offsetΪʮ�����Ʊ�� */
  if ( lrc != t || offset [0] != offset[1] ) { return 2; }
  switch ( offset[0] ) {
	case  '1':
	/* ��ʱ������,�޴�������,����ȫ��У�� */
		expired =1;
		runtimes = 0;
		check = 0;
		encrypt = 0;
		break;
	case  '2':
	/* ��ʱ������,�޴�������,��ȫ��У�� */
		expired =1;
		runtimes = 0;
		check = 1;
		encrypt = 0;
		break;
	case  '3':
 	/* ����ʱ������,�޴�������,����ȫ��У�� */
		expired =0;
		runtimes = 0;
		check = 0; 
		encrypt = 0;
		/* ���˰�,���ַ������ǽ�ֹ�ĺ� */
			return 3;
		break;
	case  '4':
 	/* ����ʱ������, �޴�������,��ȫ��У�� */
		expired = 0;
		runtimes = 0;
		check = 1;
		encrypt = 0;
		break;
	case  '5':
	/* ��ʱ������,����ȫ��У��,�����д������� */
		expired =1;
		check = 0;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '6':
	/* ��ʱ������,��ȫ��У��,�����д������� */
		expired =1;
		check = 1;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '7':
	/* ����ʱ������,����ȫ��У��,�����д������� */
		expired = 0;
		check = 0;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '8':
	/* ����ʱ������,��ȫ��У��,�����д������� */
		expired = 0;
		check = 1;
		runtimes = 1;
		encrypt = 0;
		break;
	default :
		return 2;
  }

  /* ��Ч�ڼ��� */
  if ( expired ) {
	expired_time = 0;
	/* ����Ԥ��ʱ�� */
	for ( i = 2; i < 10; i ++ ) {
	    expired_time = expired_time*16 + 
	     (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
	}
	if ( time(&mtime) > expired_time ) { return 1; }
  }
  /* ��Ч�ڼ������ */

  /* ���д������ */
  if ( runtimes ) {
	runtimes = 0;
	/* ����Ԥ��ʱ�� */
	for ( i = 10; i < 13; i ++ ) {
	    runtimes = runtimes*16 + 
	     (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
	}
  }
  
  if ( !check ) { return runtimes;}
  /* �����ܵ�8���ֽ���ΪУ���ĳ�ʼ�� */
  for ( i = 0 ; i < 8 ; i ++ ) {
	lrcblock [i] = 
	  16*(offset[2*i] > '9' ? offset[2*i]-'A'+10: offset[2*i] -'0')
	   + (offset[2*i+1] > '9' ? offset[2*i+1]-'A'+10: offset[2*i+1] -'0');
  }
  if ( strcmp( sumfile(filename, row, 0, lrcblock,1),&sum[16]) != 0 )
	return 2; 
  else 
	return runtimes;
}

