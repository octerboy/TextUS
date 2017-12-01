#include "pinkey.h"
#include "textus_string.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#define LINE_MAXLEN 1024
static inline unsigned int validate(char* file_name, int row, const char *sum);
/* 返回值为0: 表示OK 
	   1: 过期
	   2: 校验错
	   3: 内容错误
	   4: 文件错误
	 >10: 运行限制次数
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

  /* sum解密 */
  xdes(Chiper_text,pinkeyA,Clear_text,-1,0);
  /* lrc 为校验和 */
  lrc = 0;
  for ( i=0; i<16; i++ )
  {
     t=	Clear_text[i*4]*8 + Clear_text[i*4+1]*4 +
     	Clear_text[i*4+2]*2 + Clear_text[i*4+3];
     if( i < 15 ) lrc ^= t;

     offset[i] = ObtainX(t);
  }
  offset[16] = '\0';

  /* offset为十六进制表达 */
  if ( lrc != t || offset [0] != offset[1] ) { return 2; }
  switch ( offset[0] ) {
	case  '1':
	/* 受时间限制,无次数限制,不作全文校验 */
		expired =1;
		runtimes = 0;
		check = 0;
		encrypt = 0;
		break;
	case  '2':
	/* 受时间限制,无次数限制,作全文校验 */
		expired =1;
		runtimes = 0;
		check = 1;
		encrypt = 0;
		break;
	case  '3':
 	/* 不受时间限制,无次数限制,不作全文校验 */
		expired =0;
		runtimes = 0;
		check = 0; 
		encrypt = 0;
		/* 算了吧,这种方法还是禁止的好 */
			return 3;
		break;
	case  '4':
 	/* 不受时间限制, 无次数限制,作全文校验 */
		expired = 0;
		runtimes = 0;
		check = 1;
		encrypt = 0;
		break;
	case  '5':
	/* 受时间限制,不作全文校验,作运行次数限制 */
		expired =1;
		check = 0;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '6':
	/* 受时间限制,作全文校验,作运行次数限制 */
		expired =1;
		check = 1;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '7':
	/* 不受时间限制,不作全文校验,作运行次数限制 */
		expired = 0;
		check = 0;
		runtimes = 1;
		encrypt = 0;
		break;
	case  '8':
	/* 不受时间限制,作全文校验,作运行次数限制 */
		expired = 0;
		check = 1;
		runtimes = 1;
		encrypt = 0;
		break;
	default :
		return 2;
  }

  /* 有效期检验 */
  if ( expired ) {
	expired_time = 0;
	/* 计算预定时间 */
	for ( i = 2; i < 10; i ++ ) {
	    expired_time = expired_time*16 + 
	     (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
	}
	if ( time(&mtime) > expired_time ) { return 1; }
  }
  /* 有效期检验结束 */

  /* 运行次数检查 */
  if ( runtimes ) {
	runtimes = 0;
	/* 计算预定时间 */
	for ( i = 10; i < 13; i ++ ) {
	    runtimes = runtimes*16 + 
	     (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
	}
  }
  
  if ( !check ) { return runtimes;}
  /* 将解密的8个字节作为校验块的初始块 */
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

