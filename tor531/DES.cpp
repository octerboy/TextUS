// DES.cpp: implementation of the CDES class.
//
//////////////////////////////////////////////////////////////////////
#include "DES.h"
#include <stdio.h>
#include <math.h>



////////////////////////////////////////////////////////////////////////
// initial permutation IP
const char IP_Table[64] = {
	58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
		62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
		57, 49, 41, 33, 25, 17,  9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
		61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7
};
// final permutation IP^-1 
const char IPR_Table[64] = {
	40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
		38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
		36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
		34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41,  9, 49, 17, 57, 25
};
// expansion operation matrix
const char E_Table[48] = {
	32,  1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,
		8,  9, 10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
		16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25,
		24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32,  1
};
// 32-bit permutation function P used on the output of the S-boxes 
const char P_Table[32] = {
	16, 7, 20, 21, 29, 12, 28, 17, 1,  15, 23, 26, 5,  18, 31, 10,
		2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25
};
// permuted choice table (key) 
const char PC1_Table[56] = {
	57, 49, 41, 33, 25, 17,  9,  1, 58, 50, 42, 34, 26, 18,
		10,  2, 59, 51, 43, 35, 27, 19, 11,  3, 60, 52, 44, 36,
		63, 55, 47, 39, 31, 23, 15,  7, 62, 54, 46, 38, 30, 22,
		14,  6, 61, 53, 45, 37, 29, 21, 13,  5, 28, 20, 12,  4
};
// permuted choice key (table) 
const char PC2_Table[48] = {
	14, 17, 11, 24,  1,  5,  3, 28, 15,  6, 21, 10,
		23, 19, 12,  4, 26,  8, 16,  7, 27, 20, 13,  2,
		41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48,
		44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32
};
// number left rotations of pc1 
const char LOOP_Table[16] = {
	1,1,2,2,2,2,2,2,1,2,2,2,2,2,2,1
};
// The (in)famous S-boxes 
const char S_Box[8][4][16] = {
	// S1 
	14,	 4,	13,	 1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
		0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
		4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
		15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13,
		// S2 
		15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
		3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
		0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
		13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9,
		// S3 
		10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
		13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
		13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
		1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12,
		// S4 
		7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
		13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
		10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
		3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14,
		// S5 
		2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
		14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
		4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
		11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3,
		// S6 
		12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
		10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
		9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
		4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13,
		// S7 
		4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
		13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
		1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
		6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12,
		// S8 
		13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
		1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0, 14,  9,  2,
		7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
		2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};


CDES::CDES()
{
}

CDES::~CDES()
{
}

/*******************************************************************/
/*
  ?? ?? ?? ??:	ByteToBit
  ?? ?? ?? ????	??BYTE??????Bit??
  ?? ?? ?? ????	Out:	??????Bit??[in][out]
				In:		??????BYTE??[in]
				bits:	Bit????????[in]

  ?????? ??????	void
/*******************************************************************/
static void ByteToBit(bool *Out, const char *In, int bits)
{
    for(int i=0; i<bits; ++i)
        Out[i] = (In[i>>3]>>(7 - i&7)) & 1;
}

/*******************************************************************/
/*
  ?? ?? ?? ??:	BitToByte
  ?? ?? ?? ????	??Bit??????Byte??
  ?? ?? ?? ????	Out:	??????BYTE??[in][out]
				In:		??????Bit??[in]
				bits:	Bit????????[in]

  ?????? ??????	void
/*******************************************************************/
static void BitToByte(char *Out, const bool *In, int bits)
{
    memset(Out, 0, bits>>3);
    for(int i=0; i<bits; ++i)
        Out[i>>3] |= In[i]<<(7 - i&7);
}



/*******************************************************************/
/*
  ?? ?? ?? ??:	RotateL
  ?? ?? ?? ????	??BIT??????????????
  ?? ?? ?? ????	In:		??????Bit??[in]
				len:	Bit????????[in]
				loop:	??????????????

  ?????? ??????	void
/*******************************************************************/
static void RotateL(bool *In, int len, int loop)
{
	bool Tmp[256];

    memcpy(Tmp, In, loop);
    memcpy(In, In+loop, len-loop);
    memcpy(In+len-loop, Tmp, loop);
}



/*******************************************************************/
/*
  ?? ?? ?? ??:	Xor
  ?? ?? ?? ????	??????Bit??????????
  ?? ?? ?? ????	InA:	??????Bit??[in][out]
				InB:	??????Bit??[in]
				loop:	Bit????????

  ?????? ??????	void
/*******************************************************************/
static void Xor(bool *InA, const bool *InB, int len)
{
    for(int i=0; i<len; ++i)
        InA[i] ^= InB[i];
}


/*******************************************************************/
/*
  ?? ?? ?? ??:	Transform
  ?? ?? ?? ????	??????Bit????????????????
  ?? ?? ?? ????	Out:	??????Bit??[out]
				In:		??????Bit??[in]
				Table:	????????????????
				len:	????????????

  ?????? ??????	void
/*******************************************************************/
static void Transform(bool *Out, bool *In, const char *Table, int len)
{
	bool Tmp[256];

    for(int i=0; i<len; ++i)
        Tmp[i] = In[ Table[i]-1 ];
    memcpy(Out, Tmp, len);
}



/*******************************************************************/
/*
  ?? ?? ?? ??:	S_func
  ?? ?? ?? ????	????????????S BOX????
  ?? ?? ?? ????	Out:	??????32Bit[out]
				In:		??????48Bit[in]

  ?????? ??????	void
/*******************************************************************/
static void S_func(bool Out[32], const bool In[48])
{
    for(char i=0,j,k; i<8; ++i,In+=6,Out+=4) 
	{
        j = (In[0]<<1) + In[5];
        k = (In[1]<<3) + (In[2]<<2) + (In[3]<<1) + In[4];	//????SID????
		
		for(int l=0; l<4; ++l)								//??????4bit????
			Out[l] = (S_Box[i][j][k]>>(3 - l)) & 1;
    }
}


/*******************************************************************/
/*
  ?? ?? ?? ??:	F_func
  ?? ?? ?? ????	??????????????????P
  ?? ?? ?? ????	Out:	??????32Bit[out]
				In:		??????48Bit[in]

  ?????? ??????	void
/*******************************************************************/
static void F_func(bool In[32], const bool Ki[48])
{
    bool MR[48];
    Transform(MR, In, E_Table, 48);
    Xor(MR, Ki, 48);
    S_func(In, MR);
    Transform(In, In, P_Table, 32);
}



/*************************************************************************************** 
 *******	??????????DES/3DES??????????????         								**** 
 *******	??????????																**** 
 *******			 bool bType:????????????ENCRYPT(0)????;DECRYPT(1)????			**** 
 *******			 char* In:??????DES/3DES??????????????????????8??????			**** 
 *******			 unsigned datalen:????????  									**** 
 *******			 const char* Key??????????????????[8??????DES??16??????3DES)    ****
 *******			 const unsigned char keylen????????????8??16 .					****
 *******			 char *Out??(????)??????????????????????????????????            ****
 *******	?? ?? ??????????????true(????);??????????false(??)						****
 ***************************************************************************************/
bool CDES::RunDes(bool bType,char* In,unsigned datalen,const char* Key,const unsigned char keylen,char* Out)
{
    bool bMode = 0;  // ??bMode ????????????????0????ECB????

	//??????????????
	if(!(In && Out && Key && datalen && keylen>=8))
	{
		return false;
	}
	
	//??????8??????????????????????????
	if(datalen & 0x00000007)
		return false;
	
	bool m_SubKey[3][16][48];		//????

	//??????????SubKeys
	unsigned char nKey	=	(keylen>>3)>3 ? 3: (keylen>>3);

	for(int i=0;i<nKey;i++)
	{
		//int n=i<<3;
		SetSubKey(&m_SubKey[i],&Key[i<<3]);
	}

	if(bMode == ECB)	//ECB????
	{
		if(nKey	==	1)	//??Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[0],bType);
			}
		}
		else
		if(nKey == 2)	//3DES 2Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[0],bType);
			}
		}
		else			//3DES 3Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				DES(Out,In,&m_SubKey[bType? 2 : 0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[bType? 0 : 2],bType);	
			}
		}
	}	
	else				//CBC????
	{
		char	cvec[8]	=	"";	//????????
		char	cvin[8]	=	""; //????????

		if(nKey == 1)	//??Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if(bType	==	CDES::ENCRYPT)
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						cvin[j]	=	In[j] ^ cvec[j];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}

				DES(Out,cvin,&m_SubKey[0],bType);

				if(bType	==	CDES::ENCRYPT)
				{
					memcpy(cvec,Out,8);			//????????????????????
				}
				else
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						Out[j]	=	Out[j] ^ cvec[j];
					}
					memcpy(cvec,cvin,8);			//????????????????????
				}
			}
		}
		else
		if(nKey == 2)	//3DES CBC 2Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if(bType	==	CDES::ENCRYPT)
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						cvin[j]	=	In[j] ^ cvec[j];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}
				
				DES(Out,cvin,&m_SubKey[0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[0],bType);
				
				if(bType	==	CDES::ENCRYPT)
				{
					memcpy(cvec,Out,8);			//????????????????????
				}
				else
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						Out[j]	=	Out[j] ^ cvec[j];
					}
					memcpy(cvec,cvin,8);			//????????????????????
				}
			}
		}
		else			//3DES CBC 3Key
		{
			for(int i=0,j=datalen>>3;i<j;++i,Out+=8,In+=8)
			{
				if(bType	==	CDES::ENCRYPT)
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						cvin[j]	=	In[j] ^ cvec[j];
					}
				}
				else
				{
					memcpy(cvin,In,8);
				}
				
				DES(Out,cvin,&m_SubKey[bType ? 2 : 0],bType);
				DES(Out,Out,&m_SubKey[1],!bType);
				DES(Out,Out,&m_SubKey[bType ? 0 : 2],bType);
				
				if(bType	==	CDES::ENCRYPT)
				{
					memcpy(cvec,Out,8);			//????????????????????
				}
				else
				{
					for(int j=0;j<8;++j)		//????????????????????
					{
						Out[j]	=	Out[j] ^ cvec[j];
					}
					memcpy(cvec,cvin,8);			//????????????????????
				}
			}
		}
	}
	
	return true;
}




/*******************************************************************/
/*
  ?? ?? ?? ??:	RunPad
  ?? ?? ?? ????	??????????????????????????????
  ?? ?? ?? ????	bType	:??????PAD????
				In		:??????????
				Out		:??????????????
				datalen	:??????????
				padlen	:(in,out)????buffer????????????????????

  ?????? ??????	bool	:????????????
/*******************************************************************/
bool	CDES::RunPad(int nType,const char* In,unsigned datalen,char* Out,unsigned& padlen)
{
	int res = (datalen & 0x00000007);
	
	
	if(padlen< (datalen+8-res))
	{
		return false;
	}
	else
	{
		padlen	=	(datalen+8-res);
		memcpy(Out,In,datalen);
	}
	
	
	if(nType	==	PAD_ISO_1)
	{
		memset(Out+datalen,0x00,8-res);
	}
	else
	if(nType	==	PAD_ISO_2)
	{
		memset(Out+datalen,0x80,1);
		memset(Out+datalen,0x00,7-res);
	}
	else
	if(nType	==	PAD_PKCS_7)
	{
		memset(Out+datalen,8-res,8-res);
	}
	else
	if(nType	==	PAD_PBOC)
	{
		memset(Out+datalen,0x20,8-res);
	}
	else
	{
		return false;
	}

	return true;
}




//??????????????????SubKey??????
void CDES::SetSubKey(PSubKey pSubKey, const char Key[8])
{
	//int k;
	bool K[64], *KL=&K[0], *KR=&K[28];
    ByteToBit(K, Key, 64);
	/*
	printf("K64=\n");
	for(k=0;k<64;k++)
	{
	  printf("%d\t",K[k]);
	  if(fmod((k+1), 8)==0)
		  printf("\n");
	}
	printf("\n");
	*/
    Transform(K, K, PC1_Table, 56);
/*
	printf("K56=\n");
	for(k=0;k<56;k++)
	{
	  printf("%d\t",K[k]);
	  if(fmod((k+1), 8)==0)
		  printf("\n");
	}
	printf("\n");
*/
    for(int i=0; i<16; ++i) {
        RotateL(KL, 28, LOOP_Table[i]);
        RotateL(KR, 28, LOOP_Table[i]);
        Transform((*pSubKey)[i], K, PC2_Table, 48);
    }
}



//DES????????
void CDES::DES(char Out[8], char In[8], const PSubKey pSubKey, bool Type)
{
    bool M[64], tmp[32], *Li=&M[0], *Ri=&M[32];
    ByteToBit(M, In, 64);
    Transform(M, M, IP_Table, 64);
    if( Type == ENCRYPT )
	{
        for(int i=0; i<16; ++i)
		{
            memcpy(tmp, Ri, 32);		//Ri[i-1] ????
            F_func(Ri, (*pSubKey)[i]);	//Ri[i-1]??????????SBox??????P
            Xor(Ri, Li, 32);			//Ri[i] = P XOR Li[i-1]
            memcpy(Li, tmp, 32);		//Li[i] = Ri[i-1]
        }
    }
	else
	{
        for(int i=15; i>=0; --i) 
		{
			memcpy(tmp, Ri, 32);		//Ri[i-1] ????
            F_func(Ri, (*pSubKey)[i]);	//Ri[i-1]??????????SBox??????P
            Xor(Ri, Li, 32);			//Ri[i] = P XOR Li[i-1]
            memcpy(Li, tmp, 32);		//Li[i] = Ri[i-1]
        }
	}
	RotateL(M,64,32);					//Ri??Li????????M
    Transform(M, M, IPR_Table, 64);		//????????????????
    BitToByte(Out, M, 64);				//??????????
}

void CDES::Not(char *Out, const char *In, int len)
{
	 for(int i=0; i<len; ++i)
        Out[i]= ~In[i];	
}
void CDES::XOR(char *Out, const char *In, int len)
{
	for(int i=0;i<len;i++)
		Out[i]^=In[i];
}
