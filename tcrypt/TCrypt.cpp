/* Copyright (c) 2016-2018 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title:TCrypt
 Build: created by octerboy, 2017/05/03 Guangzhou
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "PacData.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#if defined(__APPLE__)
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#include <CommonCrypto/CommonCryptor.h>
#else
#include <openssl/md5.h>
#include <openssl/des.h>
#endif

unsigned char Sbox[256] = { 
 	0xD6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05, 
 	0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99, 
 	0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62, 
 	0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6, 
 	0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8, 
 	0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35, 
 	0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87, 
 	0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e, 
 	0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1, 
 	0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3, 
 	0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f, 
 	0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51, 
 	0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8, 
 	0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0, 
 	0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84, 
 	0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48 
 }; 

typedef unsigned char SM4_cblock[16];
typedef struct _SM4_Key {
	SM4_cblock key;
	unsigned int rk[32];	//ROT key
} SM4_Key;
#define ROT(x,i) ((unsigned int)((((unsigned TEXTUS_LONG)x << i) | ((unsigned TEXTUS_LONG)x >> (32-i))) & 0xFFFFFFFF))
#define Linear(b) (b^ROT(b,2)^ROT(b,10)^ROT(b,18)^ ROT(b,24))
#define Linear_Alt(b) (b^ROT(b,13)^ROT(b,23))
#define Tao(b) ((Sbox[(b>>24 & 0xFF)] << 24 ) | (Sbox[(b>>16 & 0xFF)] << 16 ) | (Sbox[(b>>8 & 0xFF)] << 8 ) | (Sbox[(b & 0xFF)]))
int set_sm4_key(SM4_cblock kk, SM4_Key *key)
{
	static unsigned int fk[4] = { 0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC }; 
 
	static unsigned int ck[32] = { 0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269, 0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
        	0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249, 0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
        	0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229, 0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
        	0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209, 0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279 };

	unsigned int alt_k[36];
	unsigned int tmp, tmp2, tmp3;
	int i;
	for ( i = 0 ; i < 4; i ++  ) 
	{
		alt_k[i] = ((kk[i*4] << 24) | (kk[i*4+1] << 16) | (kk[i*4+2] << 8) | kk[i*4+3]) ^ fk[i];
	}
	for ( i = 0 ; i < 32; i ++  ) 
	{
		tmp = alt_k[i+1]^alt_k[i+2]^alt_k[i+3]^ck[i];
		tmp2 = Tao(tmp); tmp3 =Linear_Alt(tmp2);
		key->rk[i] = alt_k[i+4] = alt_k[i] ^ tmp3;
	}
	return 0;
}

#define SM4_ENC 0
#define SM4_DEC 0xFF 
void sm4_enc(SM4_cblock in,  SM4_cblock out, SM4_Key *key, int alog)
{
	int i;
	unsigned int x[36];
	unsigned int tmp, tmp2, tmp3;
	for ( i = 0 ; i < 4; i ++  ) 
	{
		x[i] = (in[i*4] << 24) | (in[i*4+1] << 16) | (in[i*4+2] << 8) | in[i*4+3];
	}
	if ( alog == SM4_ENC )
	for ( i = 0; i < 32 ; i++ )
	{
		tmp = x[i+1] ^ x[i+2] ^ x[i+3] ^ key->rk[i];
		tmp2 = Tao(tmp); tmp3 = Linear(tmp2);
		x[i+4] = x[i] ^ tmp3;
	} 
	if ( alog == SM4_DEC )
	for ( i = 0; i < 32 ; i++ )
	{
		tmp = x[i+1] ^ x[i+2] ^ x[i+3] ^ key->rk[31-i];
		tmp2 = Tao(tmp); tmp3 = Linear(tmp2);
		x[i+4] = x[i] ^ tmp3;
	}
	for ( i = 0; i < 4; i++ )
	{
		out[i*4] = (x[35-i] >> 24) & 0xFF ;
		out[i*4+1] = (x[35-i] >> 16) & 0xFF ;
		out[i*4+2] = (x[35-i] >> 8) & 0xFF ;
		out[i*4+3] = x[35-i]  & 0xFF ;
	}
}

static void sm4_enc_hex(char in[], unsigned TEXTUS_LONG len, char k_buf[], char out[] )
{
	unsigned TEXTUS_LONG i;
	unsigned TEXTUS_LONG chunk = len/32;
	SM4_Key sm_key;
	SM4_cblock key_blk, in_blk, out_blk;
	hex2byte(key_blk, 16, k_buf);
	set_sm4_key(key_blk, &sm_key);
	for ( i = 0 ; i < chunk; i++)
	{
		hex2byte(in_blk, 16, &in[i*32]);
		sm4_enc(in_blk, out_blk, &sm_key, SM4_ENC);
		byte2hex(out_blk, 16, &out[i*32]);	
	}
}

static void sm4_mac(const unsigned char *input, size_t len, SM4_cblock mac, 
			   SM4_Key *key, SM4_cblock ivec)
{
	SM4_cblock blk;
	size_t offset=0,i,remain;
	unsigned char patch[16] ={0x80,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	memcpy(mac, ivec, 16);
	remain = len%16;
	len = len - remain;	//只取16的整数倍

	while ( offset < len ) 
	{
		for ( i =0 ; i < 16 ; i++ ) 
		{ 
			blk[i] =  mac[i] ^ input[offset]; 
			offset++;
		}
		sm4_enc(blk, mac, key, SM4_ENC);
	}
	for ( i = 0; i < remain; i++ )
	{
		blk[i] =  mac[i] ^ input[offset]; 
		offset++;
	}
	for ( i = remain; i < 16; i++ )
		blk[i] =  mac[i] ^ patch[i-remain]; 
	sm4_enc(blk, mac, key, SM4_ENC);
}

static void sm4_mac_hex(char data[], TEXTUS_LONG d_len,  char k_buf[], char vec[], char mac[])
{
	SM4_cblock vector;
	unsigned char *buf;
	SM4_cblock c_code;

	SM4_Key skey;
	SM4_cblock key_blk;
	hex2byte(key_blk, 16, k_buf);
	set_sm4_key(key_blk, &skey);

	hex2byte(vector, 16, vec);	//准备好向量

	d_len = d_len/2;	//先算成字节数, 准备好数据
	buf = new unsigned char[d_len];
	hex2byte(buf, d_len, data);

	sm4_mac(buf, d_len, c_code, &skey, vector);

	byte2hex(c_code, 16, mac);
	delete[] buf;
}

static void encrypt(char in[], TEXTUS_LONG len, char k_buf[], char out[] )
{
	TEXTUS_LONG i;
	TEXTUS_LONG chunk = len/16;

#if defined(__APPLE__)
	unsigned char key[24], in_blk[8], out_blk[8];
	CCCryptorRef ref;
	CCCryptorStatus st;
	size_t o_len = 0;

	hex2byte(key, 8, k_buf);
	hex2byte(&key[8], 8, &k_buf[16]);
	memcpy(&key[16], key, 8);
	st = CCCryptorCreateWithMode(kCCEncrypt, kCCModeECB, kCCAlgorithm3DES, ccNoPadding, 0, (const void *)key, kCCKeySize3DES, 0,0,0,0, &ref);
	if ( st != kCCSuccess )  { printf("CCCryptorCreateWithMode failed!\n"); return ; }
	for ( i = 0 ; i < chunk; i++)
	{
		hex2byte(in_blk, 8, &in[i*16]);
		st = CCCryptorUpdate(ref, (const void *)in_blk, 8, out_blk, 8, &o_len);
		if ( st != kCCSuccess ) printf("CCCryptorUpdate ok out %lu bytes\n", o_len);
		byte2hex(out_blk, 8, &out[i*16]);	
	}
	st =CCCryptorRelease(ref);	
#else
	DES_key_schedule kLeft, kRight;
	DES_cblock lkey,rkey, in_blk, out_blk;
	hex2byte(lkey, 8, k_buf);
	hex2byte(rkey, 8, &k_buf[16]);

	DES_set_odd_parity(&lkey);
	DES_set_odd_parity(&rkey);

	DES_set_key(&lkey, &kLeft);
	DES_set_key(&rkey, &kRight);
	for ( i = 0 ; i < chunk; i++)
	{
		hex2byte(in_blk, 8, &in[i*16]);
		DES_ecb2_encrypt(&in_blk, &out_blk, &kLeft, &kRight, DES_ENCRYPT);
		byte2hex(out_blk, 8, &out[i*16]);	
	}
#endif
}

static void encrypt_cbc(char in[], TEXTUS_LONG len, char k_buf[], char out[] )
{
	unsigned char *indata, in_buf[256], *outdata, out_buf[256];

#if defined(__APPLE__)
	unsigned char key[24], in_blk[8], out_blk[8], iv[8];
	CCCryptorRef ref;
	CCCryptorStatus st;
	size_t o_len = 0;

	hex2byte(key, 8, k_buf);
	hex2byte(&key[8], 8, &k_buf[16]);
	memcpy(&key[16], key, 8);
	memset(iv, 0, 8);
	st = CCCryptorCreateWithMode(kCCEncrypt, kCCModeCBC, kCCAlgorithm3DES, ccNoPadding, iv, (const void *)key, kCCKeySize3DES, 0,0,0,0, &ref);
	if ( st != kCCSuccess )  { printf("CCCryptorCreateWithMode failed!\n"); return ; }
	if (  len > 500 ) 
	{
		indata = (unsigned char*)malloc(len);
		outdata = (unsigned char*)malloc(len);
	} else {
		indata = &in_buf[0];
		outdata = &out_buf[0];
	}
	hex2byte(indata, len/2, &in[0]);
	st = CCCryptorUpdate(ref, indata, len/2, outdata, len/2, &o_len);
	byte2hex(outdata, len/2, &out[0]);	
	st =CCCryptorRelease(ref);	
#else
	DES_key_schedule kLeft, kRight;
	DES_cblock lkey,rkey ,ivec;

	hex2byte(lkey, 8, k_buf);
	hex2byte(rkey, 8, &k_buf[16]);

	DES_set_odd_parity(&lkey);
	DES_set_odd_parity(&rkey);

	DES_set_key(&lkey, &kLeft);
	DES_set_key(&rkey, &kRight);
	if (  len > 500 ) 
	{
		indata = (unsigned char*)malloc(len);
		outdata = (unsigned char*)malloc(len);
	} else {
		indata = &in_buf[0];
		outdata = &out_buf[0];
	}

	hex2byte(indata, len/2, &in[0]);
	memset(&ivec, 0, sizeof(ivec));
	DES_ede2_cbc_encrypt(indata, outdata, len/2, &kLeft, &kRight, &ivec, DES_ENCRYPT);
	byte2hex(outdata, len/2, &out[0]);	
#endif

	if (  len > 500 ) 
	{
		free(indata);
		free(outdata);
	}
}

#if defined(__APPLE__)
static void singleMAC(const unsigned char *input, size_t len, unsigned char *mac, 
			   const unsigned char *key, const unsigned char *ivec)
{
	unsigned char blk[8];
	size_t offset=0,i;
	CCCryptorRef ref;
	CCCryptorStatus st;
	size_t o_len = 0;

	memcpy(mac, ivec, 8);

	st = CCCryptorCreateWithMode(kCCEncrypt, kCCModeECB, kCCAlgorithmDES, ccNoPadding, 0, key, kCCKeySizeDES, 0,0,0,0, &ref);
	while ( offset < len ) 
	{
		for ( i =0 ; i < 8 ; i++ ) 
		{ 
			blk[i] =  mac[i] ^ input[offset]; 
			offset++;
		}
		st = CCCryptorUpdate(ref, blk, 8, mac, 8, &o_len);
	}
	st =CCCryptorRelease(ref);	
}

static void doubleMAC(const unsigned char *input, size_t len, unsigned char *mac, const unsigned char*key, unsigned char *ivec)
{
	unsigned char blk[8];
	CCCryptorRef ref;
	CCCryptorStatus st;
	size_t o_len;

	singleMAC (input, len, mac, key, ivec);

	st = CCCryptorCreateWithMode(kCCDecrypt, kCCModeECB, kCCAlgorithmDES, ccNoPadding, 0, &key[8], kCCKeySizeDES, 0,0,0,0, &ref);
	st = CCCryptorUpdate(ref, mac, 8, blk, 8, &o_len);
	st =CCCryptorRelease(ref);	
	//DES_ecb_encrypt (mac, &blk, keyR, DES_DECRYPT);
	st = CCCryptorCreateWithMode(kCCEncrypt, kCCModeECB, kCCAlgorithmDES, ccNoPadding, 0, key, kCCKeySizeDES, 0,0,0,0, &ref);
	st = CCCryptorUpdate(ref, blk, 8, mac, 8, &o_len);
	st =CCCryptorRelease(ref);	
	//DES_ecb_encrypt (&blk, mac,  keyL, DES_ENCRYPT);
}

static void TDesMac(char data[], size_t d_len,  char k_buf[], char vec[], char mac[])
{
	unsigned char key[24], in_blk[8], out_blk[8], vector[8], c_code[8];
	unsigned char *buf;

	hex2byte(key, 8, k_buf);
	hex2byte(&key[8], 8, &k_buf[16]);
	memcpy(&key[16], key, 8);
	hex2byte(vector, 8, vec);	//准备好向量

	d_len = d_len/2;	//先算成字节数, 准备好数据
	buf = new unsigned char[d_len + 32];
	hex2byte(buf, d_len, data);

	buf[d_len] = 0x80;
	memset(&buf[d_len+1], 0x00, 7-d_len%8);	//补齐80 00
	d_len += (8-d_len%8);
	doubleMAC (buf, d_len, c_code, key, vector);

	byte2hex(c_code, 8, mac);
	delete[] buf;
}

static void SDesMac(char data[], size_t d_len,  char k_buf[], char vec[], char mac[])
{
	unsigned char vector[8], c_code[8], lkey[16];
	unsigned char *buf;

	hex2byte(lkey, 8, k_buf);
	hex2byte(vector, 8, vec);	//准备好向量

	d_len = d_len/2;	//先算成字节数, 准备好数据
	buf = new unsigned char[d_len+24];
	hex2byte(buf, d_len, data);

	buf[d_len] = 0x80;
	memset(&buf[d_len+1], 0x00, 7-d_len%8);	//补齐80 00
	d_len += (8-d_len%8);
	singleMAC (buf, d_len, c_code, lkey, vector);
	delete [] buf;
	byte2hex(c_code, 8, mac);
}
#else
static void singleMAC(const unsigned char *input, size_t len, DES_cblock *mac, 
			   DES_key_schedule *key, DES_cblock *ivec)
{
	DES_cblock blk;
	size_t offset=0,i;

	memcpy(*mac, *ivec,8);

	while ( offset < len ) 
	{
		for ( i =0 ; i < 8 ; i++ ) 
		{ 
			blk[i] =  (*mac)[i] ^ input[offset]; 
			offset++;
		}
		DES_ecb_encrypt (&blk, mac, key, DES_ENCRYPT);
	}
}

static void doubleMAC(const unsigned char *input, size_t len, DES_cblock *mac,  
					  DES_key_schedule *keyL, DES_key_schedule *keyR, DES_cblock *ivec)
{
        DES_cblock blk;

	singleMAC (input, len, mac, keyL, ivec);

	DES_ecb_encrypt (mac, &blk, keyR, DES_DECRYPT);
	DES_ecb_encrypt (&blk, mac,  keyL, DES_ENCRYPT);
}

static void TDesMac(char data[], size_t d_len,  char k_buf[], char vec[], char mac[])
{
	DES_cblock vector;
	unsigned char buf[1024];
	DES_cblock c_code;

	DES_key_schedule kLeft, kRight;
	DES_cblock lkey,rkey;
	hex2byte(lkey, 8, k_buf);
	hex2byte(rkey, 8, &k_buf[16]);

	DES_set_odd_parity(&lkey);
	DES_set_odd_parity(&rkey);

	DES_set_key(&lkey, &kLeft);
	DES_set_key(&rkey, &kRight);	/* 准备好密钥 */

	hex2byte(vector, 8, vec);	//准备好向量

	d_len = d_len/2;	//先算成字节数, 准备好数据
	hex2byte(buf, d_len, data);

	buf[d_len] = 0x80;
	memset(&buf[d_len+1], 0x00, 7-d_len%8);	//补齐80 00
	d_len += (8-d_len%8);
	doubleMAC (buf, d_len, &c_code, &kLeft, &kRight, &vector);

	byte2hex(c_code, 8, mac);
}

static void SDesMac(char data[], size_t d_len,  char k_buf[], char vec[], char mac[])
{
	DES_cblock vector;
	unsigned char buf[1024];
	DES_cblock c_code;

	DES_key_schedule kLeft;
	DES_cblock lkey;
	hex2byte(lkey, 8, k_buf);

	DES_set_odd_parity(&lkey);

	DES_set_key(&lkey, &kLeft);

	hex2byte(vector, 8, vec);	//准备好向量

	d_len = d_len/2;	//先算成字节数, 准备好数据
	hex2byte(buf, d_len, data);

	buf[d_len] = 0x80;
	memset(&buf[d_len+1], 0x00, 7-d_len%8);	//补齐80 00
	d_len += (8-d_len%8);
	singleMAC (buf, d_len, &c_code, &kLeft, &vector);

	byte2hex(c_code, 8, mac);
}
#endif

struct List {
	const char *a_str; 
	List *prev;
	List *next;
	inline List ()
	{
		a_str = 0;
		prev = 0;
		next = 0;
	};
	inline void put ( struct List *neo ) 
	{
		if( !neo ) return;
		neo->next = next;
		neo->prev = this;
		if ( next != 0 )
			next->prev = neo;
		next = neo;
	};

	inline struct List *fetch() 
	{
		struct List *obj = 0;

		obj = next;

		if ( !obj ) return 0;	/* 没有一个有这样的 */
		/* 至此, 一个obj, 该符合条件, 这个obj要去掉  */
		obj->prev->next = obj->next; 
		if ( obj->next )
			obj->next->prev  =  obj->prev;
		return obj;
	};
};

class TCrypt: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	void handle_pac();	//左边状态处理
	bool sponte( Pius *);
	Amor *clone();

	TCrypt();
	~TCrypt();
private:
	bool back_i();
	bool fetch_i();
	bool gm_cipher();
	bool jt_gm_auth();
	bool gm_cipher_mac();
	bool gm_mac();
	bool tdes_mac();
	bool sdes_mac();
	bool tdes_cipher_mac();
	bool tdes_cipher();
	bool tdes_cbc();
	bool diversify();
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	struct G_CFG
	{
		List pool;
		inline ~G_CFG() { 
		};

		inline G_CFG(TiXmlElement *cfg) 
		{
			List *a_l;
			TiXmlElement *var_ele;
			const char *vn="index";
			for (var_ele = cfg->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
			{
				a_l = new List;
				a_l->a_str = var_ele->GetText();
				pool.put(a_l);
			}
		};
	};

	List *me_l;
	struct G_CFG *gcfg;  
	bool has_config;
#include "wlog.h"
	
};

void TCrypt::ignite(TiXmlElement *cfg)
{
	if (!cfg) return;

	if ( !gcfg ) 
	{
		gcfg = new struct G_CFG(cfg);
		has_config = true;
	}
}

TCrypt::TCrypt()
{
	gcfg = 0;
	has_config = false;
	me_l = 0;
}

TCrypt::~TCrypt()
{
	if ( has_config  )
	{	
		if(gcfg) delete gcfg;
	}
}

Amor* TCrypt::clone()
{
	TCrypt *child = new TCrypt();
	child->gcfg = gcfg;
	return (Amor*) child;
}

bool TCrypt::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	unsigned char *actp;
	unsigned TEXTUS_LONG alen;

	switch ( pius->ordo )
	{
	case Notitia::SET_UNIPAC:
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("facio SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("facio SET_UNIPAC snd_pac null");
			}

			WBUG("facio SET_UNIPAC rcv(%p) snd(%p)", rcv_pac, snd_pac);
		} else {
			WBUG("facio SET_TBUF null");
		}
		break;

	case Notitia::PRO_UNIPAC:    
		WBUG("facio PRO_UNIPAC");

		actp=rcv_pac->getfld(1, &alen);		//功能代码
		if ( !actp)
		{
			WLOG(ERR, "function  code field(1) is null");
			break;
		}
		snd_pac->input(1,'0');
		switch (actp[0])
		{
		case '4':
			if ( gm_cipher())
				snd_pac->input(1,'d');
			break;
		case '5':
			if (gm_cipher_mac())
				snd_pac->input(1,'e');
			break;
		case '6':
			if (tdes_cipher())
				snd_pac->input(1,'f');
			break;
		case '7':
			if (tdes_cipher_mac())
				snd_pac->input(1,'g');
			break;
		case '8':
			if (tdes_cbc())
				snd_pac->input(1,'h');
			break;
		case '9':
			if (diversify())
				snd_pac->input(1,'i');
			break;

		case 'A':
			if (jt_gm_auth())
				snd_pac->input(1,'j');
			break;
		case 'B':
			if (gm_mac())
				snd_pac->input(1,'k');
			break;
		case 'C':
			if (tdes_mac())
				snd_pac->input(1,'l');
			break;
		case 'E':
			if ( fetch_i() )
				snd_pac->input(1,'o');
			break;
		case 'F':
			if ( back_i() )
				snd_pac->input(1,'p');
			break;
		case 'G':
			if (sdes_mac())
				snd_pac->input(1,'q');
			break;
		default:
			return false;
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION" );
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION" );
		break;

	default:
		return false;
	}
	return true;
}

bool TCrypt::sponte( Amor::Pius *pius)
{
	return true;
}

bool TCrypt::gm_cipher()
{
	char k_buf[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(3, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	
	k_buf[h_len] = 0;

	h_val=rcv_pac->getfld(2, &h_len);
	snd_pac->grant(h_len);

	sm4_enc_hex((char*)h_val, h_len, (char*)k_buf, (char*)snd_pac->buf.point);
	snd_pac->commit(2, h_len);
	return true;
}

bool TCrypt::jt_gm_auth()
{
	char k_buf[64]={0}, rnd[64]={0}, cipher[64]={0}, auth[64]={0};
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;
	int i;

	h_val=rcv_pac->getfld(3, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	
	//printf("key len %d\n", h_len);
	if ( h_len != 32 ) return false;
	k_buf[h_len] = 0;

	h_val=rcv_pac->getfld(2, &h_len);
	//printf("rnd len %d\n", h_len);
	if ( h_len != 16 ) return false;
	memcpy(rnd, h_val, 16);	
	memset(&rnd[16], '0', 16);

	snd_pac->grant(17);

	//printf("---rnd %s key %s\n", rnd, k_buf);
	sm4_enc_hex(rnd, 32, k_buf, cipher);
	//printf("---cipher %s\n", cipher);
	for ( i = 0 ; i < 16; i++)
	{
		auth[i] = ObtainX(((Obtainc(cipher[i])) ^ Obtainc(cipher[i+16])));  
	}
	snd_pac->input(2, auth, 16);
	return true;
}

bool TCrypt::gm_cipher_mac()
{
	const char *patch ="80000000000000000000000000000000";
	const char *zero ="00000000000000000000000000000000";
	unsigned TEXTUS_LONG t_len, head_len;
	char cmd_buf[1024], body[1024], k_buf[64], rnd[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(2, &h_len);	//head
	memcpy(cmd_buf, h_val, h_len);	//指令头
	head_len = h_len;
	h_val=rcv_pac->getfld(3, &h_len);	//body
	memcpy(body, h_val, h_len);

	t_len = (32  - h_len%32);	
	memcpy( &(body[h_len]), patch, t_len);	//补齐0x80 00
	t_len += h_len;
	//TEXTUS_SNPRINTF( &cmd_buf[head_len-2], sizeof(cmd_buf)-head_len+2,"%02X",  t_len/2 + 4);
			
	h_val=rcv_pac->getfld(4, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	

	h_val=rcv_pac->getfld(5, &h_len);	//rnd
	memcpy(rnd, h_val, h_len);
	memcpy(&rnd[h_len], zero, 32-h_len);

	sm4_enc_hex(body, t_len, (char*)k_buf, &cmd_buf[head_len]);	//body加密内容就在cmd_head后
	t_len += head_len;	
	sm4_mac_hex(cmd_buf, t_len,  (char*)k_buf, rnd, &cmd_buf[t_len]);
	t_len +=8;
	cmd_buf[t_len] = '\0';
	//snd_pac->input(2, cmd_buf, strlen(cmd_buf));
	snd_pac->input(2, (unsigned char*)&cmd_buf[head_len], (unsigned TEXTUS_LONG)strlen(&cmd_buf[head_len]));
	//{int *a= 0 ; *a=0;}
	return true;
}

bool TCrypt::gm_mac()
{
	const char *patch ="80000000000000000000000000000000";
	const char *zero ="00000000000000000000000000000000";
	char mac[64], k_buf[64], rnd[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(4, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	

	h_val=rcv_pac->getfld(5, &h_len);	//rnd
	memcpy(rnd, h_val, h_len);
	memcpy(&rnd[h_len], zero, 32-h_len);

	h_val=rcv_pac->getfld(2, &h_len);	//head

	sm4_mac_hex((char*)h_val, h_len,  (char*)k_buf, rnd, mac);
	mac[8] = '\0';
	snd_pac->input(2, mac, 8);
	//{int *a= 0 ; *a=0;}
	return true;
}

bool TCrypt::tdes_mac()
{
	const char *patch ="80000000000000000000000000000000";
	const char *zero ="00000000000000000000000000000000";
	char mac[64], k_buf[64], rnd[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(4, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	

	h_val=rcv_pac->getfld(5, &h_len);	//rnd
	memcpy(rnd, h_val, h_len);
	memcpy(&rnd[h_len], zero, 32-h_len);

	h_val=rcv_pac->getfld(2, &h_len);	//head

	TDesMac((char*)h_val, h_len,  (char*)k_buf, rnd, mac);
	mac[16] = '\0';
	snd_pac->input(2, mac, 16);
	//{int *a= 0 ; *a=0;}
	return true;
}

bool TCrypt::sdes_mac()
{
	const char *patch ="80000000000000000000000000000000";
	const char *zero ="00000000000000000000000000000000";
	char mac[64], k_buf[64], rnd[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(4, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	

	h_val=rcv_pac->getfld(5, &h_len);	//rnd
	memcpy(rnd, h_val, h_len);
	memcpy(&rnd[h_len], zero, 32-h_len);

	h_val=rcv_pac->getfld(2, &h_len);	//head

	SDesMac((char*)h_val, h_len,  (char*)k_buf, rnd, mac);
	mac[16] = '\0';
	snd_pac->input(2, mac, 16);
	//{int *a= 0 ; *a=0;}
	return true;
}

bool TCrypt::tdes_cipher_mac()
{
	const char *patch ="80000000000000000000000000000000";
	const char *zero ="00000000000000000000000000000000";
	unsigned TEXTUS_LONG t_len, head_len;
	char cmd_buf[1024], body[1024], k_buf[64], rnd[64];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	h_val=rcv_pac->getfld(2, &h_len);	//head
	memcpy(cmd_buf, h_val, h_len);	//指令头
	head_len = h_len;

	h_val=rcv_pac->getfld(4, &h_len);	//protect key
	memcpy(k_buf, h_val, h_len);	
	k_buf[h_len]=0;
	//printf("k_buf %s\n", k_buf);

	h_val=rcv_pac->getfld(3, &h_len);	//body
	memcpy(body, h_val, h_len);

	t_len = (16  - h_len%16);	
	memcpy( &(body[h_len]), patch, t_len);	//补齐0x80 00
	t_len += h_len;
	body[t_len] = 0;
			
	//printf("--body %s\n", body);
	encrypt(body, t_len, (char*)k_buf, &cmd_buf[head_len]);	//body加密内容就在cmd_head后
	t_len += head_len;	

	h_val=rcv_pac->getfld(5, &h_len);	//rnd
	memcpy(rnd, h_val, h_len);
	if ( h_len < 16 ) memcpy(&rnd[h_len], zero, 16-h_len);
	rnd[16] =0;
	//printf("rnd %s\n", rnd);

	TDesMac(cmd_buf, t_len,  (char*)k_buf, rnd, &cmd_buf[t_len]);
	t_len +=8;
	cmd_buf[t_len] = '\0';
	snd_pac->input(2, (unsigned char*)&cmd_buf[head_len], (unsigned TEXTUS_LONG)strlen(&cmd_buf[head_len]));
	return true;
}

bool TCrypt::tdes_cipher()
{
	char *key;
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	key=(char*)rcv_pac->getfld(3, &h_len);	//protect key
	if ( h_len != 32) return false;

	h_val=rcv_pac->getfld(2, &h_len);
	snd_pac->grant(h_len);

	encrypt((char*)h_val, h_len, key, (char*)snd_pac->buf.point);
	snd_pac->commit(2, h_len);
	return true;
}

bool TCrypt::tdes_cbc()
{
	char *key;
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len;

	key=(char*)rcv_pac->getfld(3, &h_len);	//protect key
	if ( h_len != 32) return false;

	h_val=rcv_pac->getfld(2, &h_len);
	snd_pac->grant(h_len);

	encrypt_cbc((char*)h_val, h_len, key, (char*)snd_pac->buf.point);
	snd_pac->commit(2, h_len);
	return true;
}

bool TCrypt::diversify()
{
	char *key, *alog, div[33], nkey[33], skey[33];
	unsigned char *h_val;
	unsigned TEXTUS_LONG h_len, div_num;
	int i,j;

	alog=(char*)rcv_pac->getfld(4, &h_len);	//分散算法
	if ( h_len != 1) 
	{
		WBUG("alog len !=1");
		return false;
	}

	key=(char*)rcv_pac->getfld(3, &h_len);	//protect key
	if ( h_len != 32) 
	{
		WBUG("key len !=32");
		return false;
	}
	memcpy(skey, key, 32);
	memcpy(nkey, key, 32);

	h_len = 0;
	h_val=rcv_pac->getfld(2, &h_len);	//分散因子
	if ( h_len %16 !=0) 
	{
		WBUG("factor len %%16 !=0");
		return false;
	}
	div_num = h_len/16;
	for ( j = 0; j < div_num; j++ )
	{
		memcpy(div, &h_val[j*16], 16);
		for ( i = 0 ; i < 16; i++)
		{
			div[i+16] = ObtainX((~(Obtainc(h_val[i])) & 0x0F));  	//取反
		}
		
		switch ( alog[0] ) 
		{
		case 't':	//PBOC tdes
			encrypt(div, 32, skey, nkey);
			break;

		case 's':	//PBOC sm4
			sm4_enc_hex(div, 32, skey, nkey);
			break;
		default:
			break;
		}
		memcpy(skey,nkey,32);
	}

	snd_pac->grant(33);
	snd_pac->input(2, nkey, 32);
	snd_pac->buf.point[0] = 0;
	nkey[32]=0;
	//printf("-----key %s\n", (char*)nkey);
	return true;
}
		
bool TCrypt::fetch_i()
{
	me_l = gcfg->pool.fetch();
	if ( !me_l ) 
		return false;
	snd_pac->input(2, (unsigned char*)me_l->a_str, (unsigned TEXTUS_LONG)strlen(me_l->a_str));
	return true;
}

bool TCrypt::back_i()
{
	if ( me_l ) 
	{
		gcfg->pool.put(me_l);
		me_l = 0;
	} 
	return true;
}

#include "hook.c"
