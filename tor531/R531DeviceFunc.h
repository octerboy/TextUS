/******************************************************************************************
 ** �ļ����ƣ�R531DeviceFunc.h	                                                         **
 ** �ļ�������ROCKEY531�豸�ӿں���					                                     **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ����ܿ�ҵ��                                                               **
 ** �������ڣ�2012-07-05                                                                 **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ�                                                                           **
 ** �޸����ڣ�                                                                           **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ţ� V1.0                                                                      **
 **--------------------------------------------------------------------------------------**
 **                      Copyright (c) 2012  ftsafe                                      **
 ******************************************************************************************/



// ��һ���������豸��ʼ�������Ӽ��رպ���
//1 R531�豸������ʼ�����豸����ĵ�һ������ֻ�����һ�Σ�
int R531DeviceFind();

//2 ����ָ���������豸
int R531ConnDev(char *pDevName);

//3 �ر�ָ���������豸����
int R531CloseDev(int pDevHandle);



// �ڶ����������豸�ӿ�ָ�����
// -------------------------------------------------------------------------------------------------
// �豸�ຯ��
// -------------------------------------------------------------------------------------------------

//4 ��ȡ��Ʒ���
int R531DevSeqNo(int hDev,char *OutBuf,int *Status=NULL);

//5 ����
int R531DevBeep(int hDev,unsigned char CycleNum,unsigned char  Times,unsigned char Interval,int *Status=NULL );

//6 ��Ƶģ��Ӳ��λ
int R531DevResetHW(int hDev,unsigned char Msec,int *Status =NULL);

//7 ��Ƶ�����غ���
int R531DevRField(int hDev,unsigned char Mode,unsigned char Inteval, int *Status =NULL);

//8 ���÷ǽӹ���ģʽ����
int R531DevSetRfMode(int hDev,unsigned char Mode,unsigned char Speed ,int *Status =NULL);

//9 ��ȡ�ǽӹ���ģʽ����
int R531DevGetRfMode(int hDev, int *Mode,int *Status =NULL);

//10 ����û��������Ƿ��п������Ƿ��ϵ�
int R531DevCheckUserCardSlot(int hDev,int *SlotStatus,int *Status =NULL);

//11 ROCKEY501��������ƿ���
int R531DevRedLightCtl(int hDev,unsigned char CycleNum,unsigned char LightTime,unsigned char OffTime,int *Status =NULL);

//12 �Զ�����Ƶ����ʱʱ��
int R531DevSetRFTimeOut(int hDev,unsigned char TimeOut,int *Status =NULL);

//13 ǿ���˳�
int R531DevForceQuit(int hDev,int *Status =NULL);

//14 �����û�UID
int R531DevGenUID(int nDev,int DataLen,unsigned char *Data ,int *Status = NULL);

//15 ��ȡ�û�UID 
int R531DevGetUID(int hDev,int *DataLen,unsigned char *Data ,int *Status = NULL);

//16 �޸��û�UID
int R531DevModUID(int hDev,int DataLen,unsigned char *Data ,int *Status = NULL);

//17 �޸Ķ���������
int R531DevModPara(int hDev,int ParaLen, char *Para,int *Status = NULL);

//18 �ָ�����������
int R531DevRecoverPara(int hDev,int *Status = NULL);

// -------------------------------------------------------------------------------------------------
// �Ӵ������������
// -------------------------------------------------------------------------------------------------

//19 ѡ�񿨲ۺ�
int R531CpuSetSlot(int hDev,int Slot,int *Status =NULL);

//20 �ϵ縴λ  
int R531CpuReset(int hDev,int *AtrLen,char *Atr,int *Status = NULL );

//21 �µ�
int R531CpuPowerOff(int hDev,int *Status = NULL);

//22 PPS
int R531CpuPPS(int hDev,int PPSLen,char *PPS,char *Response,int *Status = NULL);

//23 ִ��APDUָ��
int R531CpuAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status=NULL);
int R531CpuAPDU(int hDev,char *Send,char *Recv,int *Sttus=NULL);


// -------------------------------------------------------------------------------------------------
// �ǽ�TypeA����ģʽ�����
// -------------------------------------------------------------------------------------------------

//24 Ѱ��
int R531TypeARequest(int hDev,unsigned char ReqMode,char *Atqa,int *Status =NULL);

//25 ����ͻ
int R531TypeAAntiCollision(int hDev,int CLVL,char *UID,int *Status = NULL);

//26 ѡ��Ƭ
int R531TypeASelect(int hDev,int CLVL,char *UID,char *SAK,int *Status =NULL) ;

//27 �ж�
int R531TypeAHalt(int hDev,int *Status =NULL);

//28 ѡ��Ӧ��
int R531TypeARats(int hDev,int CID,char *ATS,int *Status=NULL);


// -------------------------------------------------------------------------------------------------
// �ǽ�TypeB����ģʽ�����
// -------------------------------------------------------------------------------------------------

// 29 TypeB����ģʽ�ǽ�IC��Ѱ��
int R531TypeBRequest(int hDev,unsigned char ReqMode,unsigned char AFI,unsigned char TimeN,char *Atqb ,int *Status =NULL);


// 30 TypeB����ģʽ�ǽ�IC������ͻ
int R531TypeBSlotMarker(int hDev,int SlotNum,char *ATQB,int *Status = NULL);

// 31 TypeB����ģʽ�ǽ�IC����Ƭѡ��
int R531TypeBAttrib(int hDev,int SlotNum,char *ATQB,int *Status = NULL);

// 32.TypeB����ģʽ�ǽ�IC���ж�
int R531TypeBHalt(int hDev,char *PUPI,int *Status = NULL);


// -------------------------------------------------------------------------------------------------
// 14443-4���������
// -------------------------------------------------------------------------------------------------

//33 ȡ����Ƭѡ��
int R531TypeDeselect(int hDev,int CID,int *Status =NULL);

//34 �ǽ�TypeA��TypeB APDUָ��ִ��
int R531TypeAPDU(int hDev,int SendLen,char *Send,int *RecvLen,char *Recv,int *Status=NULL,int CardType = 4 );
int R531TypeAPDU(int hDev,char *Send,char *Recv,int *Status=NULL,int CardType=4);


// -------------------------------------------------------------------------------------------------
// M1�������
// -------------------------------------------------------------------------------------------------

// 35 M1-��֤
int R531M1Auth(int hDev,unsigned char ABLK,unsigned char KT, char *KData, char *SN,int *Status  =NULL);


// 36 M1-����
int R531M1ReadBlock(int hDev,unsigned char ABLK,int *RecvLen,char *Recv,int *Status  = NULL,unsigned char Lc = 0x10);

// 37 M1-д��
int R531M1WriteBlock(int hDev,unsigned char ABLK,unsigned char Lc, char *WriteData,int *Status = NULL);

// 38 M1-��ֵ
int R531M1AddValue(int hDev,unsigned char ABLK,unsigned char Lc,char *Data,int *Status =NULL);

// 39 M1-��ֵ
int R531M1SubValue(int hDev,unsigned char ABLK,unsigned char Lc, char *Data,int *Status =NULL);

// 40 M1-��ֵ����ֵ��ֵ�洢(������ʱ�Ĵ���������д��ֵ�洢��)
int R531M1Transfer(int hDev,unsigned char ABLK,int *Status = NULL);

// 41 M1-��ֵ�洢�ε����������ڲ���ʱ�Ĵ���
int R531M1Restore(int hDev,unsigned char ABLK,int *Status = NULL);




// Add-2013-11-06 -- ����״̬Ѱ��(�ǽ�ģ��Ѱ��)
//42.δ����14443-4���Ѱ��
int R531TypeFindCard(int hDev,int *Status);

//43.�ѽ���14443-4���Ѱ��
int R531TypeFindCard2(int hDev,int *Status);


//44.��ȡ������������Ϣ
int R531GetErrMsg(int ErrCode,char *ErrMsg);


// ϵͳ��̬ȫ�ֱ������Ժ���
int R531Test();




// ��������IC��Ӧ���ຯ��(�Եײ��豸��������Ӧ�÷�װ)



// ϵͳ�ڲ������R531�豸������
#define R531DEVMAX           9				// ϵͳ�ڲ�֧�ֵ�R531������������


// M1���ǽ�IC������,M1������TypeA����ģʽ
#define R531MODES50			 4				// M1-S40 �� 
#define R531MODES70			 2				// M1-S70 ��
#define R531MODEML			 8				// ML ��

// M1����֤����Կ����ֵ����
#define R531KTUSERKEYA		 1				// �û�������Կ��֤KEYA
#define R531KTUSERKEYB		 2				// �û�������Կ��֤KEYB
#define R531KTROMKEYA		 3				// ʹ����Ƶģ���д洢����Կ��֤KEYA
#define R531KTROMKEYB		 4				// ʹ����Ƶģ���д洢����Կ��֤KEYB



// �ǽ�TypeB����ģʽѰ����ʽ
#define R531REQBIDLE		 0				//  ������Ƶ���ڴ���IDLE״̬�Ŀ�
#define R531REQBALL			 8				// ������Ƶ���ڴ���IDLE��HALT״̬�����п�


// �������Ӵ���ͨ��Э��
#define R531PROTOCOLNO		-1				// �Ӵ���ͨ��Э��δ֪
#define R531PROTOCOLT0		 0				// �Ӵ���ͨ��Э��Ϊ��T=0
#define R531PROTOCOLT1		 1				// �Ӵ���ͨ��Э��Ϊ��T=1


// ����û�������λ��״̬����
#define R531CARDSTATNO		 0				// ���û�����
#define R531CARDSTATNOPOWER  1				// �û���δ�ϵ�
#define R531CARDSTATNORMAL   2				// �û������ϵ�



// �������豸��ʼ����־����
#define R531INITFLAGNO		 0				// R531�豸����δ��ʼ��
#define R531INITFLAGOK		 1				// R531�豸�����ѳ�ʼ��

// �������豸����״̬����
#define CONNSTATNODEV		-1				// �޴˶�����
#define CONNSTATNOCONN		 0				// δ����
#define CONNSTATCONN		 1				// ������
#define CONNSTATCLOSE		 2				// �ѹر�

// IC����λ��־����
#define RESETSTATNOT		 0				// δ�ϵ縴λ
#define RESETSTATON			 1				// ���ϵ縴λ
#define RESETSTATOFF		 2				// ���µ�

// �豸���۳���
#define R531SLOTNOTSET		-1				// δѡ�񿨲�
#define R531SLOTUSER		 0				// �û�����
#define R531SLOTSAM1		 1				// SAM1����
#define R531SLOTSAM2		 2				// SAM2����
#define R531SLOTSAM3		 3				// SAM3����


// ��������ֵ����
#define R531OK				 0				// �ɹ����
#define R531SYSERR			-1				// ϵͳ����(��ȡϵͳR531�豸ʱ����ϵͳ��������)
#define R531NODEV			-2				// ϵͳ��R531�豸��
#define R531NOEXIST			-3				// ��ǰ�豸������
#define R531CONNERR			-4				// �豸���Ӵ���
#define R531UNCONN			-5				// ������δ����
#define R531PARAMERR		-6				// ������������
#define R531NOCARD			-7				// �޿�
#define R531NORESPONSE		-8				// ��Ƭ��Ӧ��
#define R531NOINIT			-9				// δ��ʼ���豸
#define R531RESPDATALENERR	-10				// IC����Ӧ���Ӧ�����ݳ��ȴ���
#define R531EXECUTEERR		-11				// �ײ��豸ָ��ִ��ʧ��
#define R531SLOTERR			-12				// ���۲�������
#define R531NOSETSLOT		-13				// δ���ÿ���
#define R531DEVERR			-14				// ����������Ƿ�
#define R531RESETERR		-15				// �ϵ縴λʧ��
#define R531RESETNOT		-16				// δ�ϵ縴λ
#define R531NOUSE           -17				// �豸δ��Ȩ����ʹ��

// ��ͨ��ײ�������
#define R531SAMNOERR		-100			// PSAM�͹�����һ��(���������Ŀ��Ų���ͬ)
#define R531VERMACERR		-101			// ��֤���ĵ�MACִ�е�ָ�����
#define R531MACERR          -107			// MAC��֤����
#define R531RESPLENERR		-108			// Ӧ�����ݳ��ȴ���
#define R531RESPERR			-109			// APDUִ�д��󷵻ص�SWֵ��Ϊ��9000

