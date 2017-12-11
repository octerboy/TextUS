/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: enumerate the ordo of Pius 
 Build: created by octerboy, 2005/06/10
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef NOTITIA__H
#define NOTITIA__H
#define TEST_NOTITIA_FLAG 0xF0000000	/* 设置该4位为测试位, 表示不同的语言领域 */
#define CPP_NOTITIA_DOM   0x00000000	/* 该4位为0, 则为C++领域 */
#define JAVA_NOTITIA_DOM  0x10000000	/* 该4位为0x1, 则为JAVA领域 */

namespace Notitia
{
enum HERE_ORDO { 
	MAIN_PARA		=1,	/* the parameter of main(), *indic[0] = argc, *indic[1] = argv  */
	WINMAIN_PARA	=2,	/* the parameter of WinMain()  */
	CMD_MAIN_EXIT	=3,	/* 退出主程序 */
	
	CLONE_ALL_READY	=5,	/* 所有子节点已clone完毕 */
	CMD_GET_OWNER	=6,	/* 取得aptus的owner对象指针 */
	SET_SAME_PRIUS	=7,	/* 使indic所指的amor对象与本对象有相同的prius */
	WHO_AM_I		=8,	/* 以sponte方式给出, 指示自身的this指针 */
	IGNITE_ALL_READY=9,	/* info, all children node have been ignited */

	LOG_EMERG	=10,
	LOG_ALERT	=11,
	LOG_CRIT	=12,
	LOG_ERR		=13,
	LOG_WARNING	=14,
	LOG_NOTICE	=15,
	LOG_INFO	=16,
	LOG_DEBUG	=17,

	FAC_LOG_EMERG	=20,
	FAC_LOG_ALERT	=21,
	FAC_LOG_CRIT	=22,
	FAC_LOG_ERR	=23,
	FAC_LOG_WARNING	=24,
	FAC_LOG_NOTICE	=25,
	FAC_LOG_INFO	=26,
	FAC_LOG_DEBUG	=27,

	CMD_GET_VERSION	=30,	//取得版本信息		
    CMD_ZERO_FILE	=31,
    CMD_CLOSE_FILE	=32,
	CMD_GET_PIUS		=41,	/* get pius from a keeper */
	DMD_CONTINUE_SELF	=42,	//继续下本接力者
	DMD_STOP_NEXT 		=43,	//停止下一个接力者
	DMD_CONTINUE_NEXT	=44,	//继续下一个接力者

	CMD_ALLOC_IDLE		=45,	//抓一个空闲者
	CMD_FREE_IDLE		=46,	//释放一个空闲者
	DMD_CLONE_OBJ		=47,	//克隆一个对象
	CMD_INCR_REFS		=48,	/* increase a reference */
	CMD_DECR_REFS		=49,	/* decrease a reference */

	JUST_START_THREAD	=51,	/* start a new thread or resume one suspended JUST NOW */
	FINAL_END_THREAD	=52,	/* the thread will end finally */

	LOG_VAR_EMERG	=60,
	LOG_VAR_ALERT	=61,
	LOG_VAR_CRIT	=62,
	LOG_VAR_ERR	=63,
	LOG_VAR_WARNING	=64,
	LOG_VAR_NOTICE	=65,
	LOG_VAR_INFO	=66,
	LOG_VAR_DEBUG	=67,

	NEW_SESSION 		=80,	/* listener info when a new session */
	END_SERVICE 		=81,	/* */
	CMD_RELEASE_SESSION	=82,	/* release a sesion which may not to be end */

	CHANNEL_TIMEOUT		=88,	/* channel timeout when accept DMD_START_SESSION */
	CMD_CHANNEL_PAUSE	=89,	/* pause a channel */
	CMD_CHANNEL_RESUME	=90,	/* resume a channel */
	CHANNEL_NOT_ALIVE	=91,	/* channel not alive yet  */

	CMD_NEW_SERVICE		=92,	/* set new service, for tcp server etc. */
	START_SERVICE 		=93,
	DMD_END_SERVICE		=94,
	DMD_BEGIN_SERVICE	=95,

	END_SESSION 		=96,
	DMD_END_SESSION		=97,
	START_SESSION 		=98,
	DMD_START_SESSION	=99,

	SET_TBUF 		=101,
	PRO_TBUF		=102, 
	GET_TBUF 		=103,
	ERR_FRAME_LENGTH	=104, 
	ERR_FRAME_TIMEOUT 	=105,
	
	FD_SETRD 	=110, 	//置可读描述符, 
	FD_SETWR 	=111,	//置可写描述符, 
	FD_SETEX 	=112,	//置异常描述符, 
	FD_CLRRD 	=113,	//清可读描述符, 
	FD_CLRWR 	=114,	//清可写描述符,
	FD_CLREX 	=115,	//清异常描述符,
	FD_PRORD 	=116,	//处理可读描述符,
	FD_PROWR 	=117,	//处理可写描述符
	FD_PROEX 	=118,	//处理异常描述符

	TIMER		=120,	/* 一个时间片已到, 或者是超时 */
	DMD_SET_TIMER 	=121,	/* 要求定时通知时间片 */
	DMD_CLR_TIMER 	=122,	/* 清除时间通知 */
	DMD_SET_ALARM 	=123,	/* 设置一个定时 */

	PRO_HTTP_HEAD	=130,	/* 处理http报文头 */
	CMD_HTTP_GET 	=131,	/* 取http请求中的数据 */
	CMD_HTTP_SET 	=132,	/* 置http响应中的数据 */

	CMD_GET_HTTP_HEADBUF	=133,	/* 取得http报文头的原始数据 */
	CMD_GET_HTTP_HEADOBJ	=134,	/* 取得http报文头的对象指针 */
	CMD_SET_HTTP_HEAD 	=135,	/* 设置http报文头的原始数据 */
	PRO_HTTP_REQUEST  	=136,	/* 处理http请求 */
	PRO_HTTP_RESPONSE 	=137,	/* 处理http响应 */
	HTTP_Request_Complete 	=138,	/* HTTP请求报文已完整 */
	HTTP_Response_Complete 	=139,	/* HTTP响应报文已结束 */
	HTTP_Request_Cleaned 	=140,	/* HTTP请求报文体已清空 */

	GET_COOKIE 	=141,	/* 获取某个cookie */
	SET_COOKIE 	=142,	/* 设置某个cookie */ 
	GET_DOMAIN 	=143,	/* 获取domain */ 
	HTTP_ASKING 	=144,	/* HTTP请求中, head已OK, 报文未完整 */
	WebSock_Start	=145,	/* websocket握手完成 */
	WebSock_End	=146,	/* websocket结束 */

	SET_TINY_XML 	=151,
	PRO_TINY_XML	=152, 
	PRO_SOAP_HEAD	=153, 	/* indic是一个TiXmlElement指针 */
	PRO_SOAP_BODY	=154, 	/* indic是一个TiXmlElement指针 */
	ERR_SOAP_FAULT	=155, 	/* indic是一个TiXmlElement指针 */

	CMD_GET_FD	=160,	//取得描述符
	CMD_SET_PEER	=161,	//设置对方(如IP地址), indic是一个TiXmlElment指针
	CMD_GET_PEER	=162,	//取得双方(如双方的IP地址, 双方的PORT), indic是一个TiXmlElment指针
	CMD_GET_SSL	=163,	/* 取得SSL通讯句柄 */
	CMD_GET_CERT_NO	=164,	/* 取得证书号 */

	SET_WEIGHT_POINTER	=170,	/* 设置负载重量的指针 */
	TRANS_TO_SEND		=172,	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象发出数据 */
	TRANS_TO_RECV		=173,	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象接收数据 */
	TRANS_TO_HANDLE		=174,	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象处理数据 */
	CMD_BEGIN_TRANS		=175,	/* indic指向Amor, 此Amor将开始交易 */
	CMD_CANCEL_TRANS	=176,	/* indic指向Amor, 此Amor将中止交易 */
	CMD_FAIL_TRANS		=177,	/* indic指向Amor, 此Amor交易失败 */
	CMD_RETAIN_TRANS	=178,	/* indic指向Amor, 此Amor释放交易 */
	CMD_END_TRANS		=179,	/* indic指向Amor, 此Amor将结束交易 */

	CMD_FORK	=180,	/* cmd to fork a new process */
	FORKED_PARENT	=181,	/* in  the  parent's thread  of execution, indic point to pid of child */
	FORKED_CHILD	=182,	/* in the child's thread of execution, indic = 0 */

	NEW_HOLDING	=190,	/* new session */
	AUTH_HOLDING	=191,	/* auth this session */
	HAS_HOLDING	=192,	/* has a valid session, indic points to the id */
	CMD_SET_HOLDING	=193,	/* set a session, indic will return an id  */
	CMD_CLR_HOLDING	=194,	/* to clear a session, indic should point the id */
	CLEARED_HOLDING	=195,	/* has cleared a session, indic points to the id */

	SET_UNIPAC		=200, 
	PRO_UNIPAC		=201, 
	ERR_UNIPAC_COMPOSE	=202,
	ERR_UNIPAC_RESOLVE	=203,
	ERR_UNIPAC_INFO		=204,	/* 这里来的packetobj要对应到SOAP响应的fault中去 */
	MULTI_UNIPAC_END	=205,	/* 多个PACKETOBJ结束 */

	CMD_SET_DBFACE	=300, 	/* set the DB data interface definition */
	CMD_SET_DBCONN	=301, 	/* indic[0] points type (an integer): 0: SQL, 1: RPC, 2:FUNCTION
				   indic[1] is a char *: SQL statement, procedure name etc */
	CMD_DBFETCH	=302, 	/* to fetch query result */
	CMD_GET_DBFACE	=303, 	/* get the DB data interface definition, indic[0]: input name; indic[1]: output face */
	CMD_DB_CANCEL	=304, 	/* cancel DB command */
	PRO_DBFACE	=305, 	/* pro a dbface, indic=face. 这样, 对于DBPort, 可以不用PRO_UNIPAC, 不要reference field */

	IC_DEV_INIT_BACK=309, 	/* IC设备打开响应, indic指向bool类型, true成功, false失败 */
	IC_DEV_INIT	=310, 	/* IC设备打开, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为输入参数(可为空指针) */
	IC_DEV_QUIT	=311, 	/* IC设备关闭, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针 */
	IC_OPEN_PRO	=312, 	/* 打开卡片, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *uid(输出) 
				第四个为错误描述指针 */
	IC_CLOSE_PRO	=313, 	/* */
	IC_PRO_COMMAND	=314, 	/* 用户卡指令, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *req(指令), 
				第5个指向char *res(输出), 第6个指向int *sw(输出) */
	IC_SAM_COMMAND	=315, 	/* SAM卡指令, 同上*/
	IC_RESET_SAM	=316, 	/* 复位PSAM, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *ATR(输出) */
	IC_PRO_PRESENT	=317, 	/* IC卡是否在(包括非接), 第1个int*返回(如有卡则加1, 否则不加), 第2个错误描述指针, 第3个为int *slot(空指针为默认) */
	ICC_Authenticate=318,
	ICC_Read_Sector	=319,
	ICC_Write_Sector	=320, 
	ICC_Reader_Version	=321,
	ICC_Led_Display		=322,
	ICC_Audio_Control	=323,
	ICC_GetOpInfo		=324,
	ICC_Get_Card_RFID	=325,
	ICC_Get_CPC_RFID	=326,
	ICC_Get_Flag_RFID	=327,
	ICC_Get_Power_RFID	=328,
	ICC_Set433_Mode_RFID=329,
	ICC_Get433_Mode_RFID=330,
	ICC_CARD_open		=331,	/* 与IC_OPEN_PRO 不同，这里完全保留unireader的参数*/
	URead_ReLoad_Dll	=332,	/* 指示重载unireader.dll */
	URead_UnLoad_Dll	=333,	/* 指示卸载unireader.dll */
	URead_Load_Dll		=334,	/* 指示加载unireader.dll */

	TEXTUS_RESERVED =0	/* reserved */
};
	TEXTUS_AMOR_STORAGE unsigned long get_ordo(const char *comm_str);
	void env_sub(const char *ps, char *pt);
};
#endif