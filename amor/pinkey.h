static  int pinkeys[3][64] = {
	{ 
		1,0,1,1,1,0,0,1,
		0,1,0,0,1,1,0,1,
		1,1,1,1,0,0,0,1,
		0,1,1,0,0,1,1,0,
		0,0,0,0,0,1,1,1,
		0,1,1,1,1,0,0,0,
		1,1,1,1,1,0,0,0,
	},
	{ 
		1,0,1,1,0,0,1,1,
		0,1,0,0,0,1,0,1,
		1,0,1,1,0,0,0,1,
		0,1,1,0,1,1,1,0,
		0,1,0,0,1,1,0,1,
		0,1,0,1,0,1,1,0,
		1,1,1,1,1,0,0,0,
	},
	{
		1,0,1,0,1,0,0,1,
		0,1,0,1,1,1,0,1,
		1,1,0,1,1,0,0,1,
		0,1,0,0,1,1,1,0,
		0,0,0,1,0,1,1,1,
		0,1,0,0,1,1,1,0,
		1,1,1,1,1,0,0,0,
	}
};

static  unsigned char sum_file_vector[8] = {9,75,1,139,4,5,6,99 };

#define ObtainHex(s, X)   ( (s) > 9 ? (s)-10+X :(s)+'0')
#define Obtainx(s)   ObtainHex(s, 'a')
#define ObtainX(s)   ObtainHex(s, 'A')

#include "xdes.c"
static const char* sumfile (char *filename, int row, int keyIndex, unsigned char lrcblock[], int mode) 
{
  FILE *fp;
  int hasRead;
  static char sum[17];
  int pinkeyA[64];
  int Chiper_text[64],Clear_text[64];
  unsigned char block[8];
  int i,t;

  memset(sum, '0', sizeof(sum));
  sum[16] = '\0';
  TEXTUS_FOPEN (fp, filename,"rb") ;
  if(!fp) 
	goto End;

  for (; row > 0 ; row-- ) {
	char buf[1024];
	fgets(buf, 1024, fp);
  }
   /* 全文校验 */ 
  while (1 )
  {	
	hasRead = fread ( block ,1,8,fp);
	if ( hasRead != 8 ) 
	{
		if ( mode == 0 ) 
			goto DESENC;
		else
			break;
	}

        for ( i = 0 ; i < 8 ; i++ ) 
        	lrcblock[i] ^=  block[i];

	if ( mode == 0 ) continue;
DESENC:
   	for ( i = 0; i < 8 ; i ++ ) {
		t = lrcblock[i];
      	      	Clear_text[i*8] = t/128;
      	   t %= 128; Clear_text[i*8+1] = t/64;
      	   t %= 64; Clear_text[i*8+2] = t/32;
      	   t %= 32; Clear_text[i*8+3] = t/16;
      	   t %= 16; Clear_text[i*8+4] = t/8;
      	   t %= 8; Clear_text[i*8+5] = t/4;
      	   t %= 4; Clear_text[i*8+6] = t/2;
      	   t %= 2; Clear_text[i*8+7] = t;
     }
  	/* 校验结果加密 */
 	for ( i = 0 ; i < 64 ; i ++ ) pinkeyA[i] = pinkeys[keyIndex][i];
  	xdes(Clear_text, pinkeyA, Chiper_text,1,0);

	/* 替换 */
	for ( i = 0 ; i < 8 ; i ++ ) {	
               	lrcblock [i] = Chiper_text[i*8]*128
			 + Chiper_text[i*8+1]*64
			 + Chiper_text[i*8+2]*32
			 + Chiper_text[i*8+3]*16
			 + Chiper_text[i*8+4]*8
			 + Chiper_text[i*8+5]*4
			 + Chiper_text[i*8+6]*2
			 + Chiper_text[i*8+7]*1;
	}
	if ( mode ==0 ) break;
  }
  for ( i = 0; i < 8; i ++ )
  {
	sum[i*2] = ObtainX((lrcblock[i] & 0xF0) >> 4  );
        sum[i*2+1] = ObtainX((lrcblock[i] & 0x0F));
  }
  fclose(fp); 
End:
  return sum;
}

