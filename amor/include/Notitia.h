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
#define TEST_NOTITIA_FLAG 0xF0000000	/* ���ø�4λΪ����λ, ��ʾ��ͬ���������� */
#define CPP_NOTITIA_DOM   0x00000000	/* ��4λΪ0, ��ΪC++���� */
#define JAVA_NOTITIA_DOM  0x10000000	/* ��4λΪ0x1, ��ΪJAVA���� */

namespace Notitia
{
enum HERE_ORDO { 
	MAIN_PARA		=1,	/* the parameter of main(), *indic[0] = argc, *indic[1] = argv  */
	WINMAIN_PARA	=2,	/* the parameter of WinMain()  */
	CMD_MAIN_EXIT	=3,	/* �˳������� */
	
	CLONE_ALL_READY	=5,	/* �����ӽڵ���clone��� */
	CMD_GET_OWNER	=6,	/* ȡ��aptus��owner����ָ�� */
	SET_SAME_PRIUS	=7,	/* ʹindic��ָ��amor�����뱾��������ͬ��prius */
	WHO_AM_I		=8,	/* ��sponte��ʽ����, ָʾ�����thisָ�� */
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

	CMD_GET_VERSION	=30,	//ȡ�ð汾��Ϣ		
    CMD_ZERO_FILE	=31,
    CMD_CLOSE_FILE	=32,
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
	WebSock_Start	=145,	/* websocket������� */
	WebSock_End	=146,	/* websocket���� */

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
	ICC_CARD_open		=331,	/* ��IC_OPEN_PRO ��ͬ��������ȫ����unireader�Ĳ���*/
	URead_ReLoad_Dll	=332,	/* ָʾ����unireader.dll */
	URead_UnLoad_Dll	=333,	/* ָʾж��unireader.dll */
	URead_Load_Dll		=334,	/* ָʾ����unireader.dll */

	TEXTUS_RESERVED =0	/* reserved */
};
	TEXTUS_AMOR_STORAGE unsigned long get_ordo(const char *comm_str);
	void env_sub(const char *ps, char *pt);
};
#endif