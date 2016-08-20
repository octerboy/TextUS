/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 Title: enumerate the ordo of Pius 
 Build: created by octerboy, 2005/06/10
 $Date: 14-03-24 10:15 $
 $Revision: 57 $
*/

/* $NoKeywords: $ */

#ifndef NOTITIA__H
#define NOTITIA__H
#define TEST_NOTITIA_FLAG 0xF0000000	/* ���ø�4λΪ����λ, ��ʾ��ͬ���������� */
#define CPP_NOTITIA_DOM   0x00000000	/* ��4λΪ0, ��ΪC++���� */
#define JAVA_NOTITIA_DOM  0x10000000	/* ��4λΪ0x1, ��ΪJAVA���� */

#define NOTITIA_SUB_OFFSET 20 		/* ��һ��ordo������ô��λ, �͵õ�ƫ��ֵ*/
#define NOTITIA_SUB_FLAG 0x0FF00000	/* ��8λΪƫ����ֵ.
					����ͬΪPRO_UNIPAC, �ӽڵ�Module�м���dbport, ����ͬ���ݿ��, ������unipac�ӿڵ�. 
					�������ֲ�ͬ��Module. ���ڵ��Module�������ò�ֵͬ, �Ա㲻ͬ��PacketObj������ͬ����Module�� */
namespace Notitia
{
enum HERE_ORDO { 
	IGNITE_ALL_READY=0,	/* info, all children node have been ignited */
	MAIN_PARA	=1,	/* the parameter of main(), *indic[0] = argc, *indic[1] = argv  */
	WINMAIN_PARA	=2,	/* the parameter of WinMain()  */
	CMD_MAIN_EXIT	=3,	/* �˳������� */
	
	CLONE_ALL_READY	=5,	/* �����ӽڵ���clone��� */
	CMD_GET_OWNER	=6,	/* ȡ��aptus��owner����ָ�� */
	SET_SAME_PRIUS	=7,	/* ʹindic��ָ��amor�����뱾��������ͬ��prius */
	WHO_AM_I	=8,	/* ��sponte��ʽ����, ָʾ�����thisָ�� */

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

	CMD_GET_VERSION	=30,	//ȡ�ð汾��Ϣ		
	SET_DEXTRA_SKIP	=31,	//���ú���dextraʱ���������ordo, indicָ��һ������
	SET_LAEVE_SKIP	=32,	//���ú���laeveʱ���������ordo, indicָ��һ������
	SET_FACIO_SKIP	=33,	//���ú���facioʱ���������ordo, indicָ��һ������
	SET_SPONTE_SKIP	=34,	//���ú���sponteʱ���������ordo, indicָ��һ������

	CMD_GET_PIUS		=41,	/* get pius from a keeper */
	DMD_CONTINUE_SELF	=42,	//�����±�������
	DMD_STOP_NEXT 		=43,	//ֹͣ��һ��������
	DMD_CONTINUE_NEXT	=44,	//������һ��������

	CMD_ALLOC_IDLE		=45,	//ץһ��������
	CMD_FREE_IDLE		=46,	//�ͷ�һ��������
	DMD_CLONE_OBJ		=47,	//��¡һ������
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
	
	FD_SETRD 	=110, 	//�ÿɶ�������, 
	FD_SETWR 	=111,	//�ÿ�д������, 
	FD_SETEX 	=112,	//���쳣������, 
	FD_CLRRD 	=113,	//��ɶ�������, 
	FD_CLRWR 	=114,	//���д������,
	FD_CLREX 	=115,	//���쳣������,
	FD_PRORD 	=116,	//����ɶ�������,
	FD_PROWR 	=117,	//�����д������
	FD_PROEX 	=118,	//�����쳣������

	TIMER		=120,	/* һ��ʱ��Ƭ�ѵ�, �����ǳ�ʱ */
	DMD_SET_TIMER 	=121,	/* Ҫ��ʱ֪ͨʱ��Ƭ */
	DMD_CLR_TIMER 	=122,	/* ���ʱ��֪ͨ */
	DMD_SET_ALARM 	=123,	/* ����һ����ʱ */

	PRO_HTTP_HEAD	=130,	/* ����http����ͷ */
	CMD_HTTP_GET 	=131,	/* ȡhttp�����е����� */
	CMD_HTTP_SET 	=132,	/* ��http��Ӧ�е����� */

	CMD_GET_HTTP_HEADBUF	=133,	/* ȡ��http����ͷ��ԭʼ���� */
	CMD_GET_HTTP_HEADOBJ	=134,	/* ȡ��http����ͷ�Ķ���ָ�� */
	CMD_SET_HTTP_HEAD 	=135,	/* ����http����ͷ��ԭʼ���� */
	PRO_HTTP_REQUEST  	=136,	/* ����http���� */
	PRO_HTTP_RESPONSE 	=137,	/* ����http��Ӧ */
	HTTP_Request_Complete 	=138,	/* HTTP������������ */
	HTTP_Response_Complete 	=139,	/* HTTP��Ӧ�����ѽ��� */
	HTTP_Request_Cleaned 	=140,	/* HTTP������������� */

	GET_COOKIE 	=141,	/* ��ȡĳ��cookie */
	SET_COOKIE 	=142,	/* ����ĳ��cookie */ 
	GET_DOMAIN 	=143,	/* ��ȡdomain */ 
	HTTP_ASKING 	=144,	/* HTTP������, head��OK, ����δ���� */
	PRO_WEBSock_HEAD=145,	/* ����http��websocket����ͷ */

	SET_TINY_XML 	=151,
	PRO_TINY_XML	=152, 
	PRO_SOAP_HEAD	=153, 	/* indic��һ��TiXmlElementָ�� */
	PRO_SOAP_BODY	=154, 	/* indic��һ��TiXmlElementָ�� */
	ERR_SOAP_FAULT	=155, 	/* indic��һ��TiXmlElementָ�� */

	CMD_GET_FD	=160,	//ȡ��������
	CMD_SET_PEER	=161,	//���öԷ�(��IP��ַ), indic��һ��TiXmlElmentָ��
	CMD_GET_PEER	=162,	//ȡ��˫��(��˫����IP��ַ, ˫����PORT), indic��һ��TiXmlElmentָ��
	CMD_GET_SSL	=163,	/* ȡ��SSLͨѶ��� */
	CMD_GET_CERT_NO	=164,	/* ȡ��֤��� */

	SET_WEIGHT_POINTER	=170,	/* ���ø���������ָ�� */
	COMPLEX_PIUS		=171,	/* ����Pius, ��indicָ��һ����:��һ��ΪԭAmor*,�ڶ���ΪPius */
	TRANS_TO_SEND		=172,	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ����󷢳����� */
	TRANS_TO_RECV		=173,	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ������������ */
	TRANS_TO_HANDLE		=174,	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ����������� */
	CMD_BEGIN_TRANS		=175,	/* indicָ��Amor, ��Amor����ʼ���� */
	CMD_CANCEL_TRANS	=176,	/* indicָ��Amor, ��Amor����ֹ���� */
	CMD_FAIL_TRANS		=177,	/* indicָ��Amor, ��Amor����ʧ�� */
	CMD_RETAIN_TRANS	=178,	/* indicָ��Amor, ��Amor�ͷŽ��� */
	CMD_END_TRANS		=179,	/* indicָ��Amor, ��Amor���������� */

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
	ERR_UNIPAC_INFO		=204,	/* ��������packetobjҪ��Ӧ��SOAP��Ӧ��fault��ȥ */
	MULTI_UNIPAC_END	=205,	/* ���PACKETOBJ���� */


	CMD_SET_DBFACE	=300, 	/* set the DB data interface definition */
	CMD_SET_DBCONN	=301, 	/* indic[0] points type (an integer): 0: SQL, 1: RPC, 2:FUNCTION
				   indic[1] is a char *: SQL statement, procedure name etc */
	CMD_DBFETCH	=302, 	/* to fetch query result */
	CMD_GET_DBFACE	=303, 	/* get the DB data interface definition, indic[0]: input name; indic[1]: output face */
	CMD_DB_CANCEL	=304, 	/* cancel DB command */
	PRO_DBFACE	=305, 	/* pro a dbface, indic=face. ����, ����DBPort, ���Բ���PRO_UNIPAC, ��Ҫreference field */

	IC_DEV_INIT_BACK=309, 	/* IC�豸����Ӧ, indicָ��bool����, true�ɹ�, falseʧ�� */
	IC_DEV_INIT	=310, 	/* IC�豸��, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊ�������(��Ϊ��ָ��) */
	IC_DEV_QUIT	=311, 	/* IC�豸�ر�, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ�� */
	IC_OPEN_PRO	=312, 	/* �򿪿�Ƭ, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *uid(���) 
				���ĸ�Ϊ��������ָ�� */
	IC_CLOSE_PRO	=313, 	/* */
	IC_PRO_COMMAND	=314, 	/* �û���ָ��, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *req(ָ��), 
				��5��ָ��char *res(���), ��6��ָ��int *sw(���) */
	IC_SAM_COMMAND	=315, 	/* SAM��ָ��, ͬ��*/
	IC_RESET_SAM	=316, 	/* ��λPSAM, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *ATR(���) */
	IC_PRO_PRESENT	=317, 	/* IC���Ƿ���(�����ǽ�), ��1��int*����(���п����1, ���򲻼�), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��) */

	TEXTUS_RESERVED =0xFFFFFFFF	/* reserved */
};
unsigned long get_ordo(const char *comm_str)
{
	unsigned long ret_ordo;
	if ( !comm_str  )
		return TEXTUS_RESERVED;
	
#define WHAT_ORDO(X,Y) if ( comm_str && strcasecmp(comm_str, #X) == 0 ) Y = X 
#define GET_ORDO(Y) 	\
	Y = Notitia::TEXTUS_RESERVED;	\
	WHAT_ORDO(WINMAIN_PARA , Y); \
	WHAT_ORDO(CMD_MAIN_EXIT , Y); \
	WHAT_ORDO(CLONE_ALL_READY , Y); \
	WHAT_ORDO(CMD_GET_OWNER , Y); \
	WHAT_ORDO(WHO_AM_I, Y); \
	WHAT_ORDO(LOG_EMERG , Y); \
	WHAT_ORDO(LOG_ALERT , Y); \
	WHAT_ORDO(LOG_CRIT , Y); \
	WHAT_ORDO(LOG_ERR , Y); \
	WHAT_ORDO(LOG_WARNING , Y); \
	WHAT_ORDO(LOG_NOTICE , Y); \
	WHAT_ORDO(LOG_INFO , Y); \
	WHAT_ORDO(LOG_DEBUG , Y); \
	WHAT_ORDO(FAC_LOG_EMERG , Y); \
	WHAT_ORDO(FAC_LOG_ALERT , Y); \
	WHAT_ORDO(FAC_LOG_CRIT , Y); \
	WHAT_ORDO(FAC_LOG_ERR , Y); \
	WHAT_ORDO(FAC_LOG_WARNING , Y); \
	WHAT_ORDO(FAC_LOG_NOTICE , Y); \
	WHAT_ORDO(FAC_LOG_INFO , Y); \
	WHAT_ORDO(FAC_LOG_DEBUG , Y); \
	WHAT_ORDO(CMD_GET_VERSION , Y); \
	WHAT_ORDO(CMD_GET_PIUS , Y); \
	WHAT_ORDO(DMD_CONTINUE_SELF , Y); \
	WHAT_ORDO(DMD_STOP_NEXT , Y); \
	WHAT_ORDO(DMD_CONTINUE_NEXT , Y); \
	WHAT_ORDO(CMD_ALLOC_IDLE , Y); \
	WHAT_ORDO(CMD_FREE_IDLE , Y); \
	WHAT_ORDO(DMD_CLONE_OBJ , Y); \
	WHAT_ORDO(CMD_INCR_REFS , Y); \
	WHAT_ORDO(CMD_DECR_REFS , Y); \
	WHAT_ORDO(JUST_START_THREAD , Y); \
	WHAT_ORDO(FINAL_END_THREAD , Y); \
	WHAT_ORDO(NEW_SESSION , Y); \
	WHAT_ORDO(END_SERVICE , Y); \
	WHAT_ORDO(CMD_RELEASE_SESSION , Y); \
	WHAT_ORDO(CHANNEL_TIMEOUT , Y); \
	WHAT_ORDO(CMD_NEW_SERVICE , Y); \
	WHAT_ORDO(START_SERVICE , Y); \
	WHAT_ORDO(DMD_END_SERVICE , Y); \
	WHAT_ORDO(DMD_BEGIN_SERVICE , Y); \
	WHAT_ORDO(END_SESSION , Y); \
	WHAT_ORDO(DMD_END_SESSION , Y); \
	WHAT_ORDO(START_SESSION , Y); \
	WHAT_ORDO(DMD_START_SESSION , Y); \
	WHAT_ORDO(SET_TBUF , Y); \
	WHAT_ORDO(PRO_TBUF , Y); \
	WHAT_ORDO(GET_TBUF , Y); \
	WHAT_ORDO(ERR_FRAME_LENGTH , Y); \
	WHAT_ORDO(ERR_FRAME_TIMEOUT , Y); \
	WHAT_ORDO(FD_SETRD , Y); \
	WHAT_ORDO(FD_SETWR , Y); \
	WHAT_ORDO(FD_SETEX , Y); \
	WHAT_ORDO(FD_CLRRD , Y); \
	WHAT_ORDO(FD_CLRWR , Y); \
	WHAT_ORDO(FD_CLREX , Y); \
	WHAT_ORDO(FD_PRORD , Y); \
	WHAT_ORDO(FD_PROWR , Y); \
	WHAT_ORDO(FD_PROEX , Y); \
	WHAT_ORDO(TIMER , Y); \
	WHAT_ORDO(DMD_SET_TIMER , Y); \
	WHAT_ORDO(DMD_CLR_TIMER , Y); \
	WHAT_ORDO(DMD_SET_ALARM , Y); \
	WHAT_ORDO(PRO_HTTP_HEAD , Y); \
	WHAT_ORDO(CMD_HTTP_GET , Y); \
	WHAT_ORDO(CMD_HTTP_SET , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADBUF , Y); \
	WHAT_ORDO(CMD_GET_HTTP_HEADOBJ , Y); \
	WHAT_ORDO(CMD_SET_HTTP_HEAD , Y); \
	WHAT_ORDO(PRO_HTTP_REQUEST , Y); \
	WHAT_ORDO(PRO_HTTP_RESPONSE , Y); \
	WHAT_ORDO(HTTP_Request_Complete , Y); \
	WHAT_ORDO(HTTP_Response_Complete , Y); \
	WHAT_ORDO(HTTP_Request_Cleaned , Y); \
	WHAT_ORDO(GET_COOKIE , Y); \
	WHAT_ORDO(SET_COOKIE , Y); \
	WHAT_ORDO(SET_TINY_XML , Y); \
	WHAT_ORDO(PRO_TINY_XML , Y); \
	WHAT_ORDO(PRO_SOAP_HEAD , Y); \
	WHAT_ORDO(PRO_SOAP_BODY , Y); \
	WHAT_ORDO(ERR_SOAP_FAULT , Y); \
	WHAT_ORDO(CMD_GET_FD , Y); \
	WHAT_ORDO(CMD_SET_PEER , Y); \
	WHAT_ORDO(CMD_GET_PEER , Y); \
	WHAT_ORDO(CMD_GET_SSL , Y); \
	WHAT_ORDO(CMD_GET_CERT_NO , Y); \
	WHAT_ORDO(SET_WEIGHT_POINTER , Y); \
	WHAT_ORDO(COMPLEX_PIUS , Y); \
    WHAT_ORDO(TRANS_TO_SEND, Y); \
    WHAT_ORDO(TRANS_TO_RECV, Y); \
    WHAT_ORDO(TRANS_TO_HANDLE, Y); \
    WHAT_ORDO(CMD_BEGIN_TRANS, Y); \
    WHAT_ORDO(CMD_CANCEL_TRANS, Y); \
    WHAT_ORDO(CMD_FAIL_TRANS, Y); \
    WHAT_ORDO(CMD_RETAIN_TRANS, Y); \
    WHAT_ORDO(CMD_END_TRANS, Y); \
	WHAT_ORDO(CMD_FORK , Y); \
	WHAT_ORDO(FORKED_PARENT , Y); \
	WHAT_ORDO(FORKED_CHILD , Y); \
	WHAT_ORDO(NEW_HOLDING , Y); \
	WHAT_ORDO(AUTH_HOLDING , Y); \
	WHAT_ORDO(HAS_HOLDING , Y); \
	WHAT_ORDO(CMD_SET_HOLDING , Y); \
	WHAT_ORDO(CMD_CLR_HOLDING , Y); \
	WHAT_ORDO(CLEARED_HOLDING , Y); \
	WHAT_ORDO(SET_UNIPAC , Y); \
	WHAT_ORDO(PRO_UNIPAC , Y); \
	WHAT_ORDO(ERR_UNIPAC_COMPOSE , Y); \
	WHAT_ORDO(ERR_UNIPAC_RESOLVE , Y); \
	WHAT_ORDO(ERR_UNIPAC_INFO, Y); \
	WHAT_ORDO(MULTI_UNIPAC_END,Y); \
	WHAT_ORDO(CMD_SET_DBFACE , Y); \
	WHAT_ORDO(CMD_SET_DBCONN , Y); \
	WHAT_ORDO(CMD_DBFETCH , Y); \
	WHAT_ORDO(CMD_GET_DBFACE , Y); \
	WHAT_ORDO(CMD_DB_CANCEL , Y); \
	WHAT_ORDO(PRO_DBFACE , Y); \
    WHAT_ORDO(IC_DEV_INIT_BACK, Y); \
    WHAT_ORDO(IC_DEV_INIT, Y); \
    WHAT_ORDO(IC_DEV_QUIT, Y); \
    WHAT_ORDO(IC_OPEN_PRO, Y); \
    WHAT_ORDO(IC_CLOSE_PRO, Y); \
    WHAT_ORDO(IC_PRO_COMMAND, Y); \
    WHAT_ORDO(IC_SAM_COMMAND, Y); \
    WHAT_ORDO(IC_RESET_SAM, Y); \
    WHAT_ORDO(IC_PRO_PRESENT, Y); \
	if ( Y == Notitia::TEXTUS_RESERVED && comm_str && atoi(comm_str) >= 0) 	\
		Y = atoi(comm_str);

	GET_ORDO(ret_ordo);
	return ret_ordo;
#undef GET_ORDO
#undef WHAT_ORDO
}
};
#endif
