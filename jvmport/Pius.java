/**
 Title: pius class for Java
 Build:created by octerboy 2007/10/27, Panyu
 $Id: Pius.java 244 2017-12-23 03:38:31Z octerboy $
*/
package textor.jvmport;
import java.lang.Object;

public class Pius {
	public long ordo;
	public int subor;
	public Object indic;
	public Pius() {
		subor = -1;
		indic = null;
	}

	public static final int 	TEST_NOTITIA_FLAG = 0x0F000000;	/* ��4λΪ����λ, ��ʾ���������� */
	public static final int 	FRANK_NOTITIA_FLAG = 0x10000000;	/* ֱͨ�����־λ */
	public static final int 	CPP_NOTITIA_DOM = 0x00000000;	/* ��4λΪ0, ��ΪC++���� */
	public static final int 	JAVA_NOTITIA_DOM =  0x01000000; /* JAVA���� */
	
	public static final long MAIN_PARA	=1;	/* the parameter of main(), *indic[0] = argc, *indic[1] = argv  */
	public static final long WINMAIN_PARA	=2;	/* the parameter of WinMain()  */
	public static final long CMD_MAIN_EXIT	=3;	/* �˳������� */
	
	public static final long CLONE_ALL_READY=5;	/* �����ӽڵ���clone��� */
	public static final long CMD_GET_OWNER	=6;	/* ȡ��aptus��owner����ָ�� */
	public static final long SET_SAME_PRIUS	=7;	/* ʹindic��ָ��amor�����뱾��������ͬ��prius */
	public static final long WHO_AM_I	=8;	/* ��sponte��ʽ����, ָʾ�����thisָ�� */
	public static final long IGNITE_ALL_READY	=9;	/* info, all children node have been ignited */

	public static final long LOG_EMERG	=10;
	public static final long LOG_ALERT	=11;
	public static final long LOG_CRIT	=12;
	public static final long LOG_ERR	=13;
	public static final long LOG_WARNING	=14;
	public static final long LOG_NOTICE	=15;
	public static final long LOG_INFO	=16;
	public static final long LOG_DEBUG	=17;

	public static final long FAC_LOG_EMERG	=20;
	public static final long FAC_LOG_ALERT	=21;
	public static final long FAC_LOG_CRIT	=22;
	public static final long FAC_LOG_ERR	=23;
	public static final long FAC_LOG_WARNING=24;
	public static final long FAC_LOG_NOTICE	=25;
	public static final long FAC_LOG_INFO	=26;
	public static final long FAC_LOG_DEBUG	=27;

	public static final long CMD_GET_VERSION=30;	//ȡ�ð汾��Ϣ		
	public static final long SET_DEXTRA_SKIP=31;	//���ú���dextraʱ���������ordo, indicָ��һ������
	public static final long SET_LAEVE_SKIP	=32;	//���ú���laeveʱ���������ordo, indicָ��һ������
	public static final long SET_FACIO_SKIP	=33;	//���ú���facioʱ���������ordo, indicָ��һ������
	public static final long SET_SPONTE_SKIP=34;	//���ú���sponteʱ���������ordo, indicָ��һ������

	public static final long CMD_GET_PIUS	=41;	/* get pius from a keeper */
	public static final long DMD_CONTINUE_SELF	=42;	//�����±�������
	public static final long DMD_STOP_NEXT 		=43;	//ֹͣ��һ��������
	public static final long DMD_CONTINUE_NEXT	=44;	//������һ��������

	public static final long CMD_ALLOC_IDLE		=45;	//ץһ��������
	public static final long CMD_FREE_IDLE		=46;	//�ͷ�һ��������
	public static final long DMD_CLONE_OBJ		=47;	//��¡һ������
	public static final long CMD_INCR_REFS		=48;	/* increase a reference */
	public static final long CMD_DECR_REFS		=49;	/* decrease a reference */

	public static final long JUST_START_THREAD	=51;	/* start a new thread or resume one suspended JUST NOW */
	public static final long FINAL_END_THREAD	=52;	/* the thread will end finally */

	public static final long SET_EVENT_HD	=53;
	public static final long CLR_EVENT_HD	=54;
	public static final long PRO_EVENT_HD	=55;
	public static final long ERR_EVENT_HD	=56;

	public static final long LOG_VAR_EMERG	=60;
	public static final long LOG_VAR_ALERT	=61;
	public static final long LOG_VAR_CRIT	=62;
	public static final long LOG_VAR_ERR	=63;
	public static final long LOG_VAR_WARNING=64;
	public static final long LOG_VAR_NOTICE	=65;
	public static final long LOG_VAR_INFO	=66;
	public static final long LOG_VAR_DEBUG	=67;

	public static final long MORE_DATA_EPOLL=68;
	public static final long POST_EPOLL	=69;
	public static final long SIGNAL_EPOLL	=70;
	public static final long AIO_EPOLL	=71;
	public static final long ACCEPT_EPOLL	=72;
	public static final long SET_EPOLL 	=73;
	public static final long CLR_EPOLL 	=74;
	public static final long PRO_EPOLL 	=75;
	public static final long WR_EPOLL 	=76;
	public static final long RD_EPOLL 	=77;
	public static final long EOF_EPOLL 	=78;
	public static final long ERR_EPOLL 	=79;

	public static final long NEW_SESSION 	=80;	/* listener info when a new session */
	public static final long END_SERVICE 	=81;	/* */
	public static final long CMD_RELEASE_SESSION	=82;	/* release a sesion which may not to be end */

	public static final long NT_SERVICE_PAUSE	=86;
	public static final long NT_SERVICE_RESUME	=87;
	public static final long CHANNEL_TIMEOUT	=88;	/* channel timeout when accept DMD_START_SESSION */
	public static final long CMD_CHANNEL_PAUSE	=89;	/* pause a channel */
	public static final long CMD_CHANNEL_RESUME	=90;	/* resume a channel */
	public static final long CHANNEL_NOT_ALIVE	=91;	/* channel not alive yet  */

	public static final long CMD_NEW_SERVICE	=92;	/* set new service, for tcp server etc. */
	public static final long START_SERVICE 		=93;
	public static final long DMD_END_SERVICE	=94;
	public static final long DMD_BEGIN_SERVICE	=95;

	public static final long END_SESSION 		=96;
	public static final long DMD_END_SESSION	=97;
	public static final long START_SESSION 		=98;
	public static final long DMD_START_SESSION	=99;

	public static final long SET_TBUF 		=101;
	public static final long PRO_TBUF		=102; 
	public static final long GET_TBUF 		=103;
	public static final long ERR_FRAME_LENGTH	=104; 
	public static final long ERR_FRAME_TIMEOUT 	=105;
	
	public static final long FD_SETRD 	=110;	//�ÿɶ�������, 
	public static final long FD_SETWR 	=111;	//�ÿ�д������, 
	public static final long FD_SETEX 	=112;	//���쳣������, 
	public static final long FD_CLRRD 	=113;	//��ɶ�������, 
	public static final long FD_CLRWR 	=114;	//���д������,
	public static final long FD_CLREX 	=115;	//���쳣������,
	public static final long FD_PRORD 	=116;	//����ɶ�������,
	public static final long FD_PROWR 	=117;	//�����д������
	public static final long FD_PROEX 	=118;	//�����쳣������

	public static final long TIMER		=120;	/* һ��ʱ��Ƭ�ѵ�, �����ǳ�ʱ */
	public static final long DMD_SET_TIMER 	=121;	/* Ҫ��ʱ֪ͨʱ��Ƭ */
	public static final long DMD_CLR_TIMER 	=122;	/* ���ʱ��֪ͨ */
	public static final long DMD_SET_ALARM 	=123;	/* ����һ����ʱ */
	public static final long TIMER_HANDLE 	=124;	/* ����һ����ʱ��� */

	public static final long PRO_HTTP_HEAD	=130;	/* ����http����ͷ */
	public static final long CMD_HTTP_GET 	=131;	/* ȡhttp�����е����� */
	public static final long CMD_HTTP_SET 	=132;	/* ��http��Ӧ�е����� */

	public static final long CMD_GET_HTTP_HEADBUF	=133;	/* ȡ��http����ͷ��ԭʼ���� */
	public static final long CMD_GET_HTTP_HEADOBJ	=134;	/* ȡ��http����ͷ�Ķ���ָ�� */
	public static final long CMD_SET_HTTP_HEAD 	=135;	/* ����http����ͷ��ԭʼ���� */
	public static final long PRO_HTTP_REQUEST  	=136;	/* ����http���� */
	public static final long PRO_HTTP_RESPONSE 	=137;	/* ����http��Ӧ */
	public static final long HTTP_Request_Complete 	=138;	/* HTTP������������ */
	public static final long HTTP_Response_Complete =139;	/* HTTP��Ӧ�����ѽ��� */
	public static final long HTTP_Request_Cleaned 	=140;	/* HTTP������������� */

	public static final long GET_COOKIE 	=141;	/* ��ȡĳ��cookie */
	public static final long SET_COOKIE 	=142;	/* ����ĳ��cookie */ 
	public static final long GET_DOMAIN 	=143;	/* ��ȡdomain */ 
	public static final long HTTP_ASKING 	=144;	/* HTTP������, head��OK, ����δ���� */
	public static final long WebSock_Start	=145;	/* ����http��websocket����ͷ */

	public static final long SET_TINY_XML 	=151;
	public static final long PRO_TINY_XML	=152; 
	public static final long PRO_SOAP_HEAD	=153; 	/* indic��һ��TiXmlElementָ�� */
	public static final long PRO_SOAP_BODY	=154; 	/* indic��һ��TiXmlElementָ�� */
	public static final long ERR_SOAP_FAULT	=155; 	/* indic��һ��TiXmlElementָ�� */

	public static final long CMD_GET_FD	=160;	//ȡ��������
	public static final long CMD_SET_PEER	=161;	//���öԷ�(��IP��ַ), indic��һ��TiXmlElmentָ��
	public static final long CMD_GET_PEER	=162;	//ȡ��˫��(��˫����IP��ַ, ˫����PORT), indic��һ��TiXmlElmentָ��
	public static final long CMD_GET_SSL	=163;	/* ȡ��SSLͨѶ��� */
	public static final long CMD_GET_CERT_NO=164;	/* ȡ��֤��� */

	public static final long SET_WEIGHT_POINTER	=170;	/* ���ø���������ָ�� */
	public static final long TRANS_TO_SEND		=172;	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ����󷢳����� */
	public static final long TRANS_TO_RECV		=173;	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ������������ */
	public static final long TRANS_TO_HANDLE	=174;	/* indicָ�򷢳���Amor, ��Amor������Դ֪ͨ, �Ǳ����������� */
	public static final long CMD_BEGIN_TRANS	=175;	/* indicָ��Amor, ��Amor����ʼ���� */
	public static final long CMD_CANCEL_TRANS	=176;	/* indicָ��Amor, ��Amor����ֹ���� */
	public static final long CMD_FAIL_TRANS		=177;	/* indicָ��Amor, ��Amor����ʧ�� */
	public static final long CMD_RETAIN_TRANS	=178;	/* indicָ��Amor, ��Amor�ͷŽ��� */
	public static final long CMD_END_TRANS		=179;	/* indicָ��Amor, ��Amor���������� */

	public static final long CMD_FORK	=180;	/* cmd to fork a new process */
	public static final long FORKED_PARENT	=181;	/* in  the  parent's thread  of execution, indic point to pid of child */
	public static final long FORKED_CHILD	=182;	/* in the child's thread of execution, indic = 0 */
	public static final long Pro_File_Err_Op=183;	
	public static final long Pro_File_Open	=184;	
	public static final long Pro_File_Err	=185;	
	public static final long Pro_File_End 	=186;	
	public static final long Move_File_From_Current	=187;	
	public static final long Move_File_From_Begin 	=188;	
	public static final long Move_File_From_End	=189;	

	public static final long NEW_HOLDING	=190;	/* new session */
	public static final long AUTH_HOLDING	=191;	/* auth this session */
	public static final long HAS_HOLDING	=192;	/* has a valid session, indic points to the id */
	public static final long CMD_SET_HOLDING=193;	/* set a session, indic will return an id  */
	public static final long CMD_CLR_HOLDING=194;	/* to clear a session, indic should point the id */
	public static final long CLEARED_HOLDING=195;	/* has cleared a session, indic points to the id */

	public static final long PRO_FILE	=196;	
	public static final long GET_FILE	=197;	
	public static final long PRO_FILE_Pac	=199;	

	public static final long SET_UNIPAC	=200; 
	public static final long PRO_UNIPAC	=201; 
	public static final long ERR_UNIPAC_COMPOSE	=202;
	public static final long ERR_UNIPAC_RESOLVE	=203;
	public static final long ERR_UNIPAC_INFO	=204;	/* ��������packetobjҪ��Ӧ��SOAP��Ӧ��fault��ȥ */
	public static final long MULTI_UNIPAC_END	=205;	/* ���PACKETOBJ���� */

	public static final long Set_InsWay=210;	
	public static final long Pro_InsWay=211;	
	public static final long Ans_InsWay=212;	
	public static final long Pro_TranWay=213;	
	public static final long Ans_TranWay=214;	
	public static final long Log_InsWay=215;	

	public static final long Comm_Recv_Timeout=280;	
	public static final long Comm_Event_Break=281;	
	public static final long Comm_Event_CTS=282;	
	public static final long Comm_Event_DSR=283;	
	public static final long Comm_Event_Err=284;	
	public static final long Comm_Event_Ring=285;	
	public static final long Comm_Event_RLSD=286;	
	public static final long Comm_Event_RxChar=287;	
	public static final long Comm_Event_RxFlag=288;	
	public static final long Comm_Event_TxEmpty=289;	

	public static final long Comm_Err_Break=295;	
	public static final long Comm_Err_Frame=296;	
	public static final long Comm_Err_OverRun=297;	
	public static final long Comm_Err_RxOver=298;	
	public static final long Comm_Err_RxParity=299;	

	public static final long CMD_SET_DBFACE	=300; 	/* set the DB data interface definition */
	public static final long CMD_SET_DBCONN	=301; 	/* indic[0] points type (an integer): 0: SQL, 1: RPC, 2:FUNCTION
				   indic[1] is a char *: SQL statement, procedure name etc */
	public static final long CMD_DBFETCH	=302; 	/* to fetch query result */
	public static final long CMD_GET_DBFACE	=303; 	/* get the DB data interface definition, indic[0]: input name; indic[1]: output face */
	public static final long CMD_DB_CANCEL	=304; 	/* cancel DB command */
	public static final long PRO_DBFACE	=305; 	/* pro a dbface, indic=face. ����, ����DBPort, ���Բ���PRO_UNIPAC, ��Ҫreference field */

	public static final long IC_DEV_INIT_BACK=309; 	/* IC�豸����Ӧ, indicָ��bool����, true�ɹ�, falseʧ�� */
	public static final long IC_DEV_INIT	=310; 	/* IC�豸��, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊ�������(��Ϊ��ָ��) */
	public static final long IC_DEV_QUIT	=311; 	/* IC�豸�ر�, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ�� */
	public static final long IC_OPEN_PRO	=312; 	/* �򿪿�Ƭ, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *uid(���) 
				���ĸ�Ϊ��������ָ�� */
	public static final long IC_CLOSE_PRO	=313; 	/* */
	public static final long IC_PRO_COMMAND	=314;	/* �û���ָ��, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *req(ָ��), 
				��5��ָ��char *res(���), ��6��ָ��int *sw(���) */
	public static final long IC_SAM_COMMAND	=315; 	/* SAM��ָ��, ͬ��*/
	public static final long IC_RESET_SAM	=316; 	/* ��λPSAM, indicָ������, ��1��int*����(0��ʾOK), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��), ��4��ָ��char *ATR(���) */
	public static final long IC_PRO_PRESENT	=317; 	/* IC���Ƿ���(�����ǽ�), ��1��int*����(���п����1, ���򲻼�), ��2����������ָ��, ��3��Ϊint *slot(��ָ��ΪĬ��) */
	public static final long ICC_Authenticate	=318;
	public static final long ICC_Read_Sector	=319;
	public static final long ICC_Write_Sector	=320;
	public static final long ICC_Reader_Version	=321;
	public static final long ICC_Led_Display	=322;
	public static final long ICC_Audio_Control	=323;
	public static final long ICC_GetOpInfo		=324;
	public static final long ICC_Get_Card_RFID	=325;
	public static final long ICC_Get_CPC_RFID	=326;
	public static final long ICC_Get_Flag_RFID	=327;
	public static final long ICC_Get_Power_RFID	=328;
	public static final long ICC_Set433_Mode_RFID	=329;
	public static final long ICC_Get433_Mode_RFID	=330;
	public static final long ICC_CARD_open		=331;	/* ��IC_OPEN_PRO ��ͬ��������ȫ����unireader�Ĳ���*/
	public static final long URead_ReLoad_Dll	=332;	/* ָʾ����unireader.dll */
	public static final long URead_UnLoad_Dll	=333;	/* ָʾж��unireader.dll */
	public static final long URead_Load_Dll		=334;	/* ָʾ����unireader.dll */

	public static final long TEXTUS_RESERVED =0;	/* reserved */
}

