package jetus.jvmport;
import java.lang.Object;

public class Pius{
	public int ordo;
	public Object indic;

	public static final int 	TEST_NOTITIA_FLAG = 0x0F000000;	/* 该4位为测试位, 表示其语言领域 */
	public static final int 	FRANK_NOTITIA_FLAG = 0x10000000;	/* 直通处理标志位 */
	public static final int 	CPP_NOTITIA_DOM = 0x00000000;	/* 该4位为0, 则为C++领域 */
	public static final int 	JAVA_NOTITIA_DOM =  0x01000000; /* JAVA语言 */
	
	public static final int		IGNITE_ALL_READY=0	;
	public static final int		MAIN_PARA	=1	;
	public static final int		WINMAIN_PARA	=2	;
	public static final int		CMD_MAIN_EXIT	=3	;
			
	public static final int		CLONE_ALL_READY	=5	;
	public static final int		CMD_GET_OWNER	=6	;
	public static final int		SET_SAME_PRIUS	=7	;
 		
	public static final int		LOG_EMERG	=10	;
	public static final int		LOG_ALERT	=11	;
	public static final int		LOG_CRIT	=12	;
	public static final int		LOG_ERR		=13	;
	public static final int		LOG_WARNING	=14	;
	public static final int		LOG_NOTICE	=15	;
	public static final int		LOG_INFO	=16	;
	public static final int		LOG_DEBUG	=17	;
 		
	public static final int		FAC_LOG_EMERG	=20	;
	public static final int		FAC_LOG_ALERT	=21	;
	public static final int		FAC_LOG_CRIT	=22	;
	public static final int		FAC_LOG_ERR	=23	;
	public static final int		FAC_LOG_WARNING	=24	;
	public static final int		FAC_LOG_NOTICE	=25	;
	public static final int		FAC_LOG_INFO	=26	;
	public static final int		FAC_LOG_DEBUG	=27	;
 		
	public static final int		CMD_GET_VERSION	=30	;
 		
	public static final int		CMD_GET_PIUS		=41	;
	public static final int		DMD_CONTINUE_SELF	=42	;
	public static final int		DMD_STOP_NEXT 		=43	;
	public static final int		DMD_CONTINUE_NEXT	=44	;
 		
	public static final int		CMD_ALLOC_IDLE		=45	;
	public static final int		CMD_FREE_IDLE		=46	;
	public static final int		DMD_CLONE_OBJ		=47	;
	public static final int		CMD_INCR_REFS		=48	;
	public static final int		CMD_DECR_REFS		=49	;
 		
	public static final int		JUST_START_THREAD	=51	;
	public static final int		FINAL_END_THREAD	=52	;
 		
	public static final int		NEW_SESSION 		=80	;
	public static final int		END_SERVICE 		=81	;
	public static final int		CMD_RELEASE_SESSION	=82	;
 		
	public static final int		CHANNEL_TIMEOUT		=88	;
	public static final int		CMD_CHANNEL_PAUSE	=89	;
	public static final int		CMD_CHANNEL_RESUME	=90	;
	public static final int		CHANNEL_NOT_ALIVE	=91	;
 		
	public static final int		CMD_NEW_SERVICE		=92	;
	public static final int		START_SERVICE 		=93	;
	public static final int		DMD_END_SERVICE		=94	;
	public static final int		DMD_BEGIN_SERVICE	=95	;
 		
	public static final int		END_SESSION 		=96	;
	public static final int		DMD_END_SESSION		=97	;
	public static final int		START_SESSION 		=98	;
	public static final int		DMD_START_SESSION	=99	;
 		
	public static final int		SET_TBUF 	=101	;
	public static final int		PRO_TBUF	=102	;
	public static final int		GET_TBUF 	=103	;
	public static final int		ERR_FRAME_LENGTH=104	;
	public static final int		ERR_FRAME_TIMEOUT =105	;
			
	public static final int		FD_SETRD 	=110	;
	public static final int		FD_SETWR 	=111	;
	public static final int		FD_SETEX 	=112	;
	public static final int		FD_CLRRD 	=113	;
	public static final int		FD_CLRWR 	=114	;
	public static final int		FD_CLREX 	=115	;
	public static final int		FD_PRORD 	=116	;
	public static final int		FD_PROWR 	=117	;
	public static final int		FD_PROEX 	=118	;
 		
	public static final int		TIMER		=120	;
	public static final int		DMD_SET_TIMER 	=121	;
	public static final int		DMD_CLR_TIMER 	=122	;
	public static final int		DMD_SET_ALARM 	=123	;
 		
	public static final int		PRO_HTTP_HEAD	=130	;
	public static final int		CMD_HTTP_GET 	=131	;
	public static final int		CMD_HTTP_SET 	=132	;
 		
	public static final int		CMD_GET_HTTP_HEADBUF	=133	;
	public static final int		CMD_GET_HTTP_HEADOBJ	=134	;
	public static final int		CMD_SET_HTTP_HEAD 	=135	;
	public static final int		PRO_HTTP_REQUEST  	=136	;
	public static final int		PRO_HTTP_RESPONSE 	=137	;
	public static final int		HTTP_Request_Complete 	=138	;
	public static final int		HTTP_Response_Complete 	=139	;
	public static final int		HTTP_Request_Cleaned 	=140	;
 		
	public static final int		GET_COOKIE 	=141	;
	public static final int		SET_COOKIE 	=142	;
	public static final int		GET_DOMAIN 	=143	;
 		
	public static final int		SET_TINY_XML 	=151	;
	public static final int		PRO_TINY_XML	=152	;
	public static final int		PRO_SOAP_HEAD	=153	;
	public static final int		PRO_SOAP_BODY	=154	;
	public static final int		ERR_SOAP_FAULT	=155	;
 		
	public static final int		CMD_GET_FD	=160	;
	public static final int		CMD_SET_PEER	=161	;
	public static final int		CMD_GET_PEER	=162	;
	public static final int		CMD_GET_SSL	=163	;
	public static final int		CMD_GET_CERT_NO	=164	;
 		
	public static final int		SET_WEIGHT_POINTER	=170	;
	public static final int		COMPLEX_PIUS		=171	;
	public static final int		REDIRECT_PIUS		=172	;
	public static final int		CMD_CANCEL_REQUEST	=173	;

	public static final int		CMD_FORK		=180	;
	public static final int		FORKED_PARENT		=181	;
	public static final int		FORKED_CHILD		=182	;
 		
	public static final int		NEW_HOLDING	=190	;
	public static final int		AUTH_HOLDING	=191	;
	public static final int		HAS_HOLDING	=192	;
	public static final int		CMD_SET_HOLDING	=193	;
	public static final int		CMD_CLR_HOLDING	=194	;
	public static final int		CLEARED_HOLDING	=195	;
 		
	public static final int		SET_UNIPAC	=200	;
	public static final int		PRO_UNIPAC	=201	;
	public static final int		ERR_UNIPAC_COMPOSE	=202	;
	public static final int		ERR_UNIPAC_RESOLVE	=203	;
	public static final int		ERR_UNIPAC_INFO		=204	;
	public static final int		MULTI_UNIPAC_END	=205	;
 		
	public static final int		CMD_SET_DBFACE	=300	;
	public static final int		CMD_SET_DBCONN	=301	;
 		
	public static final int		CMD_DBFETCH	=302	;
	public static final int		TEXTUS_RESERVED =-1	;
}

