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

	public static final int 	TEST_NOTITIA_FLAG = 0x0F000000;	/* 该4位为测试位, 表示其语言领域 */
	public static final int 	FRANK_NOTITIA_FLAG = 0x10000000;	/* 直通处理标志位 */
	public static final int 	CPP_NOTITIA_DOM = 0x00000000;	/* 该4位为0, 则为C++领域 */
	public static final int 	JAVA_NOTITIA_DOM =  0x01000000; /* JAVA语言 */
	
	public static final long MAIN_PARA	=1;	/* the parameter of main(), *indic[0] = argc, *indic[1] = argv  */
	public static final long WINMAIN_PARA	=2;	/* the parameter of WinMain()  */
	public static final long CMD_MAIN_EXIT	=3;	/* 退出主程序 */
	
	public static final long CLONE_ALL_READY=5;	/* 所有子节点已clone完毕 */
	public static final long CMD_GET_OWNER	=6;	/* 取得aptus的owner对象指针 */
	public static final long SET_SAME_PRIUS	=7;	/* 使indic所指的amor对象与本对象有相同的prius */
	public static final long WHO_AM_I	=8;	/* 以sponte方式给出, 指示自身的this指针 */
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

	public static final long CMD_GET_VERSION=30;	//取得版本信息		
	public static final long SET_DEXTRA_SKIP=31;	//设置忽略dextra时辅助处理的ordo, indic指向一个整数
	public static final long SET_LAEVE_SKIP	=32;	//设置忽略laeve时辅助处理的ordo, indic指向一个整数
	public static final long SET_FACIO_SKIP	=33;	//设置忽略facio时辅助处理的ordo, indic指向一个整数
	public static final long SET_SPONTE_SKIP=34;	//设置忽略sponte时辅助处理的ordo, indic指向一个整数

	public static final long CMD_GET_PIUS	=41;	/* get pius from a keeper */
	public static final long DMD_CONTINUE_SELF	=42;	//继续下本接力者
	public static final long DMD_STOP_NEXT 		=43;	//停止下一个接力者
	public static final long DMD_CONTINUE_NEXT	=44;	//继续下一个接力者

	public static final long CMD_ALLOC_IDLE		=45;	//抓一个空闲者
	public static final long CMD_FREE_IDLE		=46;	//释放一个空闲者
	public static final long DMD_CLONE_OBJ		=47;	//克隆一个对象
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
	
	public static final long FD_SETRD 	=110;	//置可读描述符, 
	public static final long FD_SETWR 	=111;	//置可写描述符, 
	public static final long FD_SETEX 	=112;	//置异常描述符, 
	public static final long FD_CLRRD 	=113;	//清可读描述符, 
	public static final long FD_CLRWR 	=114;	//清可写描述符,
	public static final long FD_CLREX 	=115;	//清异常描述符,
	public static final long FD_PRORD 	=116;	//处理可读描述符,
	public static final long FD_PROWR 	=117;	//处理可写描述符
	public static final long FD_PROEX 	=118;	//处理异常描述符

	public static final long TIMER		=120;	/* 一个时间片已到, 或者是超时 */
	public static final long DMD_SET_TIMER 	=121;	/* 要求定时通知时间片 */
	public static final long DMD_CLR_TIMER 	=122;	/* 清除时间通知 */
	public static final long DMD_SET_ALARM 	=123;	/* 设置一个定时 */
	public static final long TIMER_HANDLE 	=124;	/* 设置一个定时句柄 */

	public static final long PRO_HTTP_HEAD	=130;	/* 处理http报文头 */
	public static final long CMD_HTTP_GET 	=131;	/* 取http请求中的数据 */
	public static final long CMD_HTTP_SET 	=132;	/* 置http响应中的数据 */

	public static final long CMD_GET_HTTP_HEADBUF	=133;	/* 取得http报文头的原始数据 */
	public static final long CMD_GET_HTTP_HEADOBJ	=134;	/* 取得http报文头的对象指针 */
	public static final long CMD_SET_HTTP_HEAD 	=135;	/* 设置http报文头的原始数据 */
	public static final long PRO_HTTP_REQUEST  	=136;	/* 处理http请求 */
	public static final long PRO_HTTP_RESPONSE 	=137;	/* 处理http响应 */
	public static final long HTTP_Request_Complete 	=138;	/* HTTP请求报文已完整 */
	public static final long HTTP_Response_Complete =139;	/* HTTP响应报文已结束 */
	public static final long HTTP_Request_Cleaned 	=140;	/* HTTP请求报文体已清空 */

	public static final long GET_COOKIE 	=141;	/* 获取某个cookie */
	public static final long SET_COOKIE 	=142;	/* 设置某个cookie */ 
	public static final long GET_DOMAIN 	=143;	/* 获取domain */ 
	public static final long HTTP_ASKING 	=144;	/* HTTP请求中, head已OK, 报文未完整 */
	public static final long WebSock_Start	=145;	/* 处理http的websocket报文头 */

	public static final long SET_TINY_XML 	=151;
	public static final long PRO_TINY_XML	=152; 
	public static final long PRO_SOAP_HEAD	=153; 	/* indic是一个TiXmlElement指针 */
	public static final long PRO_SOAP_BODY	=154; 	/* indic是一个TiXmlElement指针 */
	public static final long ERR_SOAP_FAULT	=155; 	/* indic是一个TiXmlElement指针 */

	public static final long CMD_GET_FD	=160;	//取得描述符
	public static final long CMD_SET_PEER	=161;	//设置对方(如IP地址), indic是一个TiXmlElment指针
	public static final long CMD_GET_PEER	=162;	//取得双方(如双方的IP地址, 双方的PORT), indic是一个TiXmlElment指针
	public static final long CMD_GET_SSL	=163;	/* 取得SSL通讯句柄 */
	public static final long CMD_GET_CERT_NO=164;	/* 取得证书号 */

	public static final long SET_WEIGHT_POINTER	=170;	/* 设置负载重量的指针 */
	public static final long TRANS_TO_SEND		=172;	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象发出数据 */
	public static final long TRANS_TO_RECV		=173;	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象接收数据 */
	public static final long TRANS_TO_HANDLE	=174;	/* indic指向发出点Amor, 此Amor向请求源通知, 是本对象处理数据 */
	public static final long CMD_BEGIN_TRANS	=175;	/* indic指向Amor, 此Amor将开始交易 */
	public static final long CMD_CANCEL_TRANS	=176;	/* indic指向Amor, 此Amor将中止交易 */
	public static final long CMD_FAIL_TRANS		=177;	/* indic指向Amor, 此Amor交易失败 */
	public static final long CMD_RETAIN_TRANS	=178;	/* indic指向Amor, 此Amor释放交易 */
	public static final long CMD_END_TRANS		=179;	/* indic指向Amor, 此Amor将结束交易 */

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
	public static final long ERR_UNIPAC_INFO	=204;	/* 这里来的packetobj要对应到SOAP响应的fault中去 */
	public static final long MULTI_UNIPAC_END	=205;	/* 多个PACKETOBJ结束 */

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
	public static final long PRO_DBFACE	=305; 	/* pro a dbface, indic=face. 这样, 对于DBPort, 可以不用PRO_UNIPAC, 不要reference field */

	public static final long IC_DEV_INIT_BACK=309; 	/* IC设备打开响应, indic指向bool类型, true成功, false失败 */
	public static final long IC_DEV_INIT	=310; 	/* IC设备打开, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为输入参数(可为空指针) */
	public static final long IC_DEV_QUIT	=311; 	/* IC设备关闭, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针 */
	public static final long IC_OPEN_PRO	=312; 	/* 打开卡片, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *uid(输出) 
				第四个为错误描述指针 */
	public static final long IC_CLOSE_PRO	=313; 	/* */
	public static final long IC_PRO_COMMAND	=314;	/* 用户卡指令, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *req(指令), 
				第5个指向char *res(输出), 第6个指向int *sw(输出) */
	public static final long IC_SAM_COMMAND	=315; 	/* SAM卡指令, 同上*/
	public static final long IC_RESET_SAM	=316; 	/* 复位PSAM, indic指向数组, 第1个int*返回(0表示OK), 第2个错误描述指针, 第3个为int *slot(空指针为默认), 第4个指向char *ATR(输出) */
	public static final long IC_PRO_PRESENT	=317; 	/* IC卡是否在(包括非接), 第1个int*返回(如有卡则加1, 否则不加), 第2个错误描述指针, 第3个为int *slot(空指针为默认) */
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
	public static final long ICC_CARD_open		=331;	/* 与IC_OPEN_PRO 不同，这里完全保留unireader的参数*/
	public static final long URead_ReLoad_Dll	=332;	/* 指示重载unireader.dll */
	public static final long URead_UnLoad_Dll	=333;	/* 指示卸载unireader.dll */
	public static final long URead_Load_Dll		=334;	/* 指示加载unireader.dll */

	public static final long TEXTUS_RESERVED =0;	/* reserved */
}

