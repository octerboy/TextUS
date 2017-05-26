/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Obu
 Build:created by octerboy 2011/06/06
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "casecmp.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "BTool.h"
#include <assert.h>
#include <openssl/des.h>
#define ObtainHex(s, X)   ( (s) > 9 ? (s)-10+X :(s)+'0')
#define Obtainx(s)   ObtainHex(s, 'a')
#define ObtainX(s)   ObtainHex(s, 'A')
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )
#define UNIGO_ERR_TITLE "UnitollGO Error"

static char* byte2hex(const unsigned char *byte, size_t blen, char *hex) 
{
	size_t i;
	for ( i = 0 ; i < blen ; i++ )
	{
		hex[2*i] =  ObtainX((byte[i] & 0xF0 ) >> 4 );
		hex[2*i+1] = ObtainX(byte[i] & 0x0F );
	}
//	hex[2*i] = '\0';
	return hex;
}

static unsigned char* hex2byte(unsigned char *byte, size_t blen, const char *hex)
{
	size_t i;
	const char *p ;	

	p = hex; i = 0;

	while ( i < blen )
	{
		byte[i] =  (0x0F & Obtainc( hex[2*i] ) ) << 4;
		byte[i] |=  Obtainc( hex[2*i+1] ) & 0x0f ;
		i++;
		p +=2;
	}
	return byte;
}
void diversify( DES_key_schedule *sub_keyL, DES_key_schedule *sub_keyR, 
			  DES_key_schedule *keyL, DES_key_schedule *keyR, const_DES_cblock *factor )
{
	DES_cblock un_factor;
	DES_cblock oL,oR;
	for ( int i = 0 ; i < 8; i++)
		un_factor[i] = ~(*factor)[i];
	
	DES_ecb2_encrypt(factor, &oL, keyL, keyR, DES_ENCRYPT);
	DES_ecb2_encrypt (&un_factor, &oR, keyL, keyR, DES_ENCRYPT);

	DES_set_odd_parity(&oL);
	DES_set_odd_parity(&oR);

	DES_set_key(&oL, sub_keyL);
	DES_set_key(&oR, sub_keyR);
}

void diversifyTAC( DES_key_schedule *tac_key, 
				  DES_key_schedule *keyL, DES_key_schedule *keyR, const_DES_cblock *factor )
{
	DES_cblock un_factor;
	DES_cblock oL,oR, oT;
	int i;
	for ( i = 0 ; i < 8; i++)
		un_factor[i] = ~(*factor)[i];

	DES_ecb2_encrypt(factor, &oL, keyL, keyR, DES_ENCRYPT);
	DES_ecb2_encrypt (&un_factor, &oR, keyL, keyR, DES_ENCRYPT);

	for ( i = 0 ; i < 8; i++)
		oT[i] = oL[i] ^ oR[i];

	DES_set_odd_parity(&oT);
	DES_set_key(&oT, tac_key);
}

void singleMAC(const unsigned char *input, size_t len, DES_cblock *mac, 
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

#define AUTH_KEY 1
#define CONSUME_KEY 2
#define TAC_KEY 6
#define TOLL_KEY_NUM 12

class Obu :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Obu();
	~Obu();

#include "PacData.h"
private:
	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;
	int obuid_fld_no;
	int s_fld_no, e_fld_no, gb_fld_no, data_fld_no, app_fld_no;
	int action_fld_no, mode_fld_no, mode_second_fld_no;
	int card_no_fld_no, trans_no_fld_no, mac1_fld_no;
	int mac2_fld_no, tac_fld_no, trans_date_fld_no;
	int offline_fld_no, p_random_fld_no, random_fld_no;
	int authcode_fld_no, amount_fld_no, term_fld_no;
	int qresp_fld_no;

	int obuid_len; unsigned char obuid_here[32];

	int bst_num, bst_counter;	//多少个BST后才反映
	Amor::Pius local_p;

	DES_key_schedule card_keyL, card_keyR, auth_keyL, auth_keyR;
	DES_key_schedule tac_key;

	DES_key_schedule key_left[TOLL_KEY_NUM],key_right[TOLL_KEY_NUM];

	DES_cblock random_code ;	//记录一下外部认证的随机数
	unsigned char p_rand_here[16], amount_here[16], termid_here[16], offline_here[16] ;
	int me_status;	//OBU交易状态: 0:未开始（已结束）, 1:开始中
	
	bool qback;
	void quick();
	void me_reset() {
		bst_counter = 0;
		me_status = 0;
	};
	
#include "wlog.h"
};

void Obu::ignite(TiXmlElement *cfg) 
{ 
	TiXmlElement *p_ele;
	DES_cblock lkey,rkey;
	const char *comm_str;
	//const char *comm_str;
	char *ckey[]={
		"3E074A92EAE661861F7AF4253E4A0E76"
	,	"7CD5387C6B7FE33480DA640D7919204F"
	,	"A15E9D57626D1A86E66134921C52A2CE"
	,	"348ABAA7F846799251D5AED3C2856838"
	,	"B5735EF1ABDF23A7A879BC52C4B31A26"
	,	"07295D8CE9405B3DB06875CECBF449D3"
	,	"6B760B85AE4F9E458A7010974C10A8F1"
	,	"DA136DCB678A7A9BF2C23779EA1625E5"
	,	"49FD5BFB1926A2EC4938F7F2C2CDC79B"
	,	"B8D2F5BFCF8FF2E2CE3092981C25AFFE"
	,0
	};

	for ( int i = 0; i < TOLL_KEY_NUM; i++)
	{
		if ( !ckey[i] ) break;
		hex2byte(lkey, 8, ckey[i]);
		hex2byte(rkey, 8, &ckey[i][16]);

		DES_set_odd_parity(&lkey);
		DES_set_odd_parity(&rkey);

		DES_set_key(&lkey, &key_left[i]);
		DES_set_key(&rkey, &key_right[i]);
	}

	me_status =0 ;
	bst_num = 2;	//第二个BST就回
	bst_counter = 0;
	qback = true;		//快速回应
	if ( cfg ) 
	{
		cfg->QueryIntAttribute("bst_many", &bst_num);
		if ( (comm_str = cfg->Attribute("quick_back")) )
		{
			if( strcasecmp (comm_str, "no") == 0 ) 
				qback = false;
		}

		cfg->QueryIntAttribute("bst_many", &bst_num);
	}

	p_ele = cfg->FirstChildElement("fields");
	if ( p_ele ) 
	{
		p_ele->QueryIntAttribute("start_at", &s_fld_no);
		p_ele->QueryIntAttribute("end_at", &e_fld_no);
		p_ele->QueryIntAttribute("standard", &gb_fld_no);
		p_ele->QueryIntAttribute("data", &data_fld_no);
		p_ele->QueryIntAttribute("application", &app_fld_no);
		p_ele->QueryIntAttribute("obuid", &obuid_fld_no);
		p_ele->QueryIntAttribute("action", &action_fld_no);
		p_ele->QueryIntAttribute("mode", &mode_fld_no);
		p_ele->QueryIntAttribute("mode_second", &mode_second_fld_no);
		p_ele->QueryIntAttribute("card_no", &card_no_fld_no);
		p_ele->QueryIntAttribute("trans_no", &trans_no_fld_no);
		p_ele->QueryIntAttribute("mac1", &mac1_fld_no);
		p_ele->QueryIntAttribute("mac2", &mac2_fld_no);
		p_ele->QueryIntAttribute("tac", &tac_fld_no);
		p_ele->QueryIntAttribute("trans_date", &trans_date_fld_no);
		p_ele->QueryIntAttribute("offline_no", &offline_fld_no);
		p_ele->QueryIntAttribute("p_random", &p_random_fld_no);
		p_ele->QueryIntAttribute("random", &random_fld_no);
		p_ele->QueryIntAttribute("auth_code", &authcode_fld_no);
		p_ele->QueryIntAttribute("amount", &amount_fld_no);
		p_ele->QueryIntAttribute("term_no", &term_fld_no);
		p_ele->QueryIntAttribute("qresp_no", &qresp_fld_no);
	}
	p_ele = cfg->FirstChildElement("obu_id");
	if ( p_ele ) 
	{
		obuid_len = BTool::unescape(p_ele->GetText(), obuid_here) ;
	}
}

void Obu::quick()
{
	if ( !qback) 
		return ;
	unsigned char resp=0x80;
	snd_pac->input(qresp_fld_no, &resp, 1);
	aptus->sponte(&local_p);	
	snd_pac->fld[qresp_fld_no].reset();	//把这个快速响应值去掉
}

bool Obu::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	unsigned char gb_type, *p;
	unsigned char *app_val, *act_val, *mod_val, *mod2_val, *obuid_val;
	unsigned short action, mode;
	unsigned char mode2;
	unsigned long len;
	char msg[2048];
	int pause;

	pause = 0;

	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::END_SESSION:
		me_reset();
		break;
	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		mode = 0xFFFF;
		app_val = rcv_pac->getfld(app_fld_no);
		act_val = rcv_pac->getfld(action_fld_no);
		mod_val = rcv_pac->getfld(mode_fld_no);
		mod2_val = rcv_pac->getfld(mode_second_fld_no);
		if (app_val) 
		{
			bool has_ic = false;
			bool has_rand = false;
			bool has_p_rand = false;
			p = rcv_pac->getfld(gb_fld_no);
			gb_type = (*p) |  0x01;	//对于RSU， 回去则为OBU
			snd_pac->input(gb_fld_no, &gb_type, 1);	
#ifndef NDEBUG
			memset(msg, 0, sizeof(msg));
			TEXTUS_SPRINTF(msg, "%s", "DB44_RSU ");
			p = rcv_pac->getfld(data_fld_no, &len);
			byte2hex(p, len, &msg[strlen(msg)]);
			/*
			for ( i = 0 ;i < len; i++)
			{
				char tmp[16];
				TEXTUS_SPRINTF(tmp, "%02X ", p[i]);
				TEXTUS_STRCAT(msg, tmp);
			}
			*/
			WBUG(msg);
#endif
			switch ( *app_val )  
			{
			case 0x00:		//BST
				WLOG(INFO, "BST")
				bst_counter++;
				if ( me_status == 0 && bst_counter >= bst_num ) 
				{
					snd_pac->input(obuid_fld_no, obuid_here, obuid_len);
					goto HAS_BACK;
				}  else {
					goto NO_BACK;
				}

				break;
			case 0x4B:
				obuid_val = rcv_pac->getfld(obuid_fld_no);
				if ( !obuid_val || memcmp(obuid_val, obuid_here, obuid_len) != 0 )
					goto NO_BACK;
				if ( act_val) 
				{
					me_status = 1;
					memcpy(&action, act_val, 2);
					switch (action ) 
					{
					case 0x0051:	//查询版本

						WLOG(INFO, "51 查询版本")
						break;
					case 0x0052:	//查询原始(OBU)数据车辆内容
						has_ic = true;
						WLOG(INFO, "52 查询原始(OBU)数据:车辆及IC卡")
						break;
					case 0x0053:	//取入口数据
						WLOG(INFO, "52 取入口数据")
						break;
					case 0x0000:	//	
						if ( mod_val) 
						{
							memcpy(&mode, mod_val, 2);
							switch (mode ) 
							{
							case 0x0002:	//车辆基本内容
								WLOG(INFO, "0000 0200 取车辆基本数据")
								pause=0;
								break;
							case 0x0004:	//通行的车辆内容,入口时间等
								WLOG(INFO, "0000 0400 取车辆通行数据")
								break;
							case 0x0102:	//IC卡基本内容
								has_ic = true;
								WLOG(INFO, "0000 0201 取IC卡基本数据")
								pause=0;
								break;
							case 0x0104:	//IC卡中入口站
								WLOG(INFO, "0000 0401 取入口站点数据")
								break;
							}
						}
						quick();
						if(pause > 0) Sleep(pause);
						break;

					case 0x0100:	//第一步：
						if ( mod_val) 
						{	//取随机数
							has_rand = true;
							WLOG(INFO, "01 取随机数")
						} else {	//消费初始化
							has_p_rand = true;
							WLOG(INFO, "01 消费初始化")
						}
						quick();
						//Sleep(5);
						break;

					case 0x0200:	//第二步：扣款，同时取随机数
						WLOG(INFO, "02 扣款")
						quick();
						{
						unsigned char buf[128];
						const_DES_cblock allzero;
						DES_cblock c_code;
						DES_cblock factor;
						unsigned char *datetime_val, *transno_val, *mac1_val;
						DES_key_schedule consume_skey_left, consume_skey_right;

						memcpy(factor, p_rand_here, 4);
						memcpy(&factor[4], offline_here, 2);
						transno_val = rcv_pac->getfld(trans_no_fld_no, &len);
						memcpy(&factor[6], &transno_val[2], 2);
						diversify  (&consume_skey_left, &consume_skey_right, &card_keyL, &card_keyR, &factor);
						/* 算MAC1 */
						memcpy(buf, amount_here, 4);
						buf[4] = 0x06; buf[5] = 0x00; buf[6] = 0x00;
						memcpy(&buf[7], termid_here, 4);
						buf[11] = 0x20; 	//都是2000年以后的事了
						datetime_val = rcv_pac->getfld(trans_date_fld_no, &len);
						memcpy(&buf[12], datetime_val, 6);
						buf[18] = 0x80;	
						memset(&buf[19], 0x00, 5);
							
						memset(allzero,	0, sizeof(const_DES_cblock));
						singleMAC (buf, 24, &c_code, &consume_skey_left, &allzero);
						mac1_val = rcv_pac->getfld(mac1_fld_no, &len);
						if (memcmp(mac1_val, c_code, 4) == 0 ) 
						{
							has_rand = true;

							//算MAC2
							memcpy(buf, amount_here, 4);
							buf[4] = 0x80;
							buf[5] = 0000;
							buf[6] = 0x00;
							buf[7] = 0x00;
							memset(allzero,	0, sizeof(const_DES_cblock));
							singleMAC (buf, 8, &c_code, &consume_skey_left, &allzero);
							snd_pac->input(mac2_fld_no, c_code, 4);
								
							//算TAC	
							memcpy(buf, amount_here, 4);
							buf[4] = 0x06;
							buf[5] = 0x00; buf[6] = 0x00;
							memcpy(&buf[7], termid_here, 4);
							memcpy(&buf[11], transno_val, 4);
							buf[15] = 0x20; 	//都是2000年以后的事了
							memcpy(&buf[16], datetime_val, 6);
							buf[22] = 0x80;
							buf[23] = 0x00;
							memset(allzero,	0, sizeof(const_DES_cblock));
							singleMAC (buf, 24, &c_code, &tac_key, &allzero);
							snd_pac->input(tac_fld_no, c_code, 4);
						} else {
							 WLOG(WARNING, "MAC1错误!")
						}

						}
						
						break;

					case 0x0300:	//第三步：写卡

						if (mod2_val )
						{
							mode2 = *mod2_val;
							switch (mode2 ) 
							{
							case 0x06:	//标识站
								WLOG(INFO, "03 06 写标识站")
								break;
							case 0x04:	//写出口
								WLOG(INFO, "03 04 写出口站")
								break;
							default:
								break;
							}
						} else { //写入口
							WLOG(INFO, "03 写入口站")
						}
						quick();
						{
							unsigned char *auth_val;
							DES_cblock factor;
							auth_val = rcv_pac->getfld(authcode_fld_no, &len);
							DES_ecb2_encrypt(&random_code, &factor, &auth_keyL, &auth_keyR, DES_ENCRYPT);
							if ( memcmp(auth_val, factor, len) != 0 )
							{
								WLOG(WARNING, "外部认证错误!")
							}
						}
						//Sleep(5);
						//me_reset();
						break;

					default:
						break;
					}
				}
				break;
			default:
				break;
			}
HAS_BACK:
			//if ( mode == 0x0102 ) 
			//exit(0);

			aptus->sponte(&local_p);	

			if ( has_ic )
			{
				DES_cblock factor;
				unsigned char *cardno_val;

				cardno_val = snd_pac->getfld(card_no_fld_no, &len);
				if (cardno_val)
				{
					/* 分散成卡片密钥 */
					memset(&factor, 0x00, 8);
					memcpy(&factor, cardno_val, len);
					diversify  (&card_keyL, &card_keyR, &key_left[CONSUME_KEY], &key_right[CONSUME_KEY], &factor);

					/* 分散成卡片中的TAC密钥 */
					memset(&factor, 0x00, 8);
					memcpy(&factor, cardno_val, len);
					diversifyTAC (&tac_key, &key_left[TAC_KEY], &key_right[TAC_KEY], &factor);

					/* 分散成卡片密钥, 外部认证 */
					memset(&factor, 0x00, 8);
					memcpy(&factor, cardno_val, len);
					diversify  (&auth_keyL, &auth_keyR, &key_left[AUTH_KEY], &key_right[AUTH_KEY], &factor);
				}
			}

			if ( has_rand )	//有随机数, 下一步要写卡
			{
				unsigned char *rand_val;

				rand_val = snd_pac->getfld(random_fld_no, &len);
				memset(random_code, 0x00, sizeof(random_code));
				memcpy(random_code, rand_val, len);
			}

			if ( has_p_rand )	//有伪随机数, 表明消费初始化
			{
				unsigned char *amount_val, *termid_val, *prand_val, *offline_val;

				amount_val = rcv_pac->getfld(amount_fld_no, &len);
				memset(amount_here, 0x00, sizeof(amount_here));
				memcpy(amount_here, amount_val, len);

				termid_val = rcv_pac->getfld(term_fld_no, &len);
				memset(termid_here, 0x00, sizeof(termid_here));
				memcpy(termid_here, termid_val, len);

				prand_val = snd_pac->getfld(p_random_fld_no, &len);
				memset(p_rand_here, 0x00, sizeof(p_rand_here));
				memcpy(p_rand_here, prand_val, len);

				offline_val = snd_pac->getfld(offline_fld_no, &len);
				memset(offline_here, 0x00, sizeof(offline_here));
				memcpy(offline_here, offline_val, len);
			}
		}
NO_BACK:
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	default:
		return false;
	}
	return true;
}

bool Obu::sponte( Amor::Pius *pius) { 
	assert(pius);
	WBUG("sponte Notitia::%d", pius->ordo);
	return false;
}

Amor* Obu::clone()
{
	Obu *child = new Obu();
	return  (Amor*)child;
}

Obu::Obu() 
{ 
	rcv_pac = 0;
	snd_pac = 0;
	local_p.ordo = Notitia::PRO_UNIPAC;
	local_p.indic = 0;
}

Obu::~Obu() { } 
#include "hook.c"

