/******************************************************************************************
 ** �ļ����ƣ�ATR.h						                                                 **
 ** �ļ�������IC���ϵ縴λ��Ϣ����ͷ�ļ�	                                             **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ����ܿ�ҵ��                                                               **
 ** �������ڣ�2012-07-09                                                                 **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ˣ�                                                                           **
 ** �޸����ڣ�                                                                           **
 **--------------------------------------------------------------------------------------**
 ** �� �� �ţ�V1.0                                                                       **
 **--------------------------------------------------------------------------------------**
 **                   Copyright (c) 2012  ftsafe                                         **
 ******************************************************************************************/

#ifndef _ATR_
#define _ATR_


// ����ֵ����
#define ATR_OK					0		// �ɹ�			 
#define ATR_NOT_FOUND			1		// ������		 
#define ATR_MALFORMED			2		// ATR��������	 
#define ATR_IO_ERROR			3		// I/O ����		 


// ��������
#define ATR_MAX_SIZE 			33		// ATR��󳤶�  
#define ATR_MAX_HISTORICAL		15		// ��ʷ�ֽ���󳤶�  
#define ATR_MAX_PROTOCOLS		7		// Э�������  
#define ATR_MAX_IB				4		// ÿ��Э��Ľӿ��ֽڵ������Ŀ  
#define ATR_CONVENTION_DIRECT	0		// ֱ��Լ��  
#define ATR_CONVENTION_INVERSE	1		// ����Լ  
#define ATR_PROTOCOL_TYPE_T0	0		// Э������ T=0  
#define ATR_PROTOCOL_TYPE_T1	1		// Э������ T=1  
#define ATR_PROTOCOL_TYPE_T2	2		// Э������ T=2  
#define ATR_PROTOCOL_TYPE_T3	3		// Э������ T=3  
#define ATR_PROTOCOL_TYPE_T14	14		// Э������ T=14  
#define ATR_INTERFACE_BYTE_TA	0		// �ӿ��ֽ� TAi  
#define ATR_INTERFACE_BYTE_TB	1		// �ӿ��ֽ� TBi  
#define ATR_INTERFACE_BYTE_TC	2		// �ӿ��ֽ� TCi  
#define ATR_INTERFACE_BYTE_TD	3		// �ӿ��ֽ� TDi  
#define ATR_PARAMETER_F			0		// ���� F  
#define ATR_PARAMETER_D			1		// ���� D  
#define ATR_PARAMETER_I			2		// ���� I  
#define ATR_PARAMETER_P			3		// ���� P  
#define ATR_PARAMETER_N			4		// ���� N  
#define ATR_INTEGER_VALUE_FI	0		// ����ֵ FI  
#define ATR_INTEGER_VALUE_DI	1		// ����ֵ DI  
#define ATR_INTEGER_VALUE_II	2		// ����ֵ II  
#define ATR_INTEGER_VALUE_PI1	3		// ����ֵ PI1  
#define ATR_INTEGER_VALUE_N		4		// ����ֵ N  
#define ATR_INTEGER_VALUE_PI2	5		// ����ֵ PI2  



// ����ȱʡֵ
#define ATR_DEFAULT_F			372
#define ATR_DEFAULT_D			1
#define ATR_DEFAULT_I 			50
#define ATR_DEFAULT_N			0
#define ATR_DEFAULT_P			5

// ���� bool ��������
//typedef int                bool;

// ����ATR�������ݽṹ
typedef struct
{
	unsigned length;							// ATR ����
	unsigned char  TS;							// ATR TS[��ʼ�ַ�]�ֽ�ֵ
	unsigned char  T0;							// ATR T0[��ʽ�ַ�]�ֽ�ֵ
	struct
	{
		BYTE value;								// TA~TD λֵ��
		int present;							// TA~TD λ���ڱ�־��TRUE(1):���ڣ�FALSE(0):������
	} ib[ATR_MAX_PROTOCOLS][ATR_MAX_IB], TCK;	// ���� TA~TDλ���ݽṹ,ATR_MAX_PROTOCOLS=7;ATR_MAX_IB = 4;
	unsigned pn;								// TA~TD ����ĸ������� tb ʵ�ʴ洢�����ݸ�����
	unsigned char  hb[ATR_MAX_HISTORICAL];		// ��ʷ�ֽ���,ATR_MAX_HISTORICAL=15
	unsigned hbn;								// ��ʷ�ֽڸ���
} ATRStruct;



// ��ATR�ֽ���������ATRStruct�ṹ
int ftATRInitFromArray (ATRStruct * atr, unsigned char  atr_buffer[ATR_MAX_SIZE], unsigned int length);


// ��ȡ ATR�ṹ��TD1��TD2�ĵͰ��ֽڼ�Э�����͵�ֵ,TD1ȱ��Ϊ��0�� TD2�����ڷ��أ�-1
int ftGetATRT(ATRStruct stATR, int *TD1T,int *TD2T);

// T !=0 ��ATR��TCKֵ���㺯��,
int ftATRTCKCal(unsigned char  atr_buffer[ATR_MAX_SIZE],int length ,unsigned int *TCK);


#endif 

/*  �ֽ����	�ֽ�����				ֵ��˵��
	====================================================================================
	0			TS[��ʼ�ַ�]			\x3B:����Լ����\x3F:����Լ��(�Ժ�汾����֧��)
	1			T0[��ʽ�ַ�]			�߰��ֽڣ�TA1~TD1�Ƿ���ڣ��ڰ��ֽڣ���ʱ���ַ���Ŀ(0~15) [�Ӹߵ��ͷֱ�:TD1~TA1]
	2			TA1						�߰��ֽڣ�FI������ȷ��F��ֵ��FΪʱ���ٶ�ת������,ȱʡFI=1,��ʶ F= 372;
	                                    �Ͱ��ֽڣ�DI������ȷ��D��ֵ��DΪλ���ʵ������ӣ�ȱʡֵΪ��DI=1,����D=1;
	3			TB1						b1~b5��PI1ֵ��ȷ��IC������ı�̵�ѹP��ֵ��=0 ��ʾIC����ʹ��VPP
										b6~b7��IIֵ,  ȷ��IC�����������̵���Iֵ��=0,PI1=0,��ʹ��
	4			TC1						����Nֵ��N���ڱ�ʾ���ӵ���С����ʱ��Ķ��Ᵽ��ʱ�䣻
	5			TD1						�߰��ֽڣ���ʾTA2~TD2�Ƿ����;[�Ӹߵ��ͷֱ�:TD2~TA2]
	                                    �Ͱ��ֽڣ�������Ϣ������ʹ�õ�Э������,����TD1λ��T=0��Ϊ�����������͵�ȱʡֵ��
	6			TA2[�ض�ģʽ/Э��ģʽ]	b8[],b7,b6[����],
	7			TB2[PI2]				ȷ��IC������ı�̵�ѹP��ֵ����PI2,��PI1��Ч��
	8			TC2[WI]					ר����T=0Э�飬���乤���ȴ�ʱ������(WI);
	9			TD2						�߰��ֽڣ���ʾTA3~TD3�Ƿ����;[�Ӹߵ��ͷֱ�:TD3~TA3],�Ͱ��ֽڣ�Э�����ͣ�
	10			TA3						���TD2��ָ��T=1)����IC������Ϣ���С����(IFSI)���ɽ��յĿ���Ϣ�����󳤶�(INF):01~FE
	11			TB3						���TD2��ָ��T=1)������������CWT��BWT��CWI��BWIֵ���߰��ֽ�Ϊ��BWI,�ڰ��ֽ�Ϊ��CWI
	12			TC3						���TD2��ָ��T=1)��������������У��(LRC)��Ϊ�������
	13			TCK[У���ַ�]			TCK��ֵӦʹ��T0��TCK���ڵ������ֽڽ����������Ľ��Ϊ�㣻							
*/

/*
 T=0 ATR =3B 6F 00 00 86 05 47 44 11 86 50 32 02 00 01 74 60 47 44             [T0=6F,TC1,TB1����]
 T=1 ATR =3B 9F 95 81 31 F0 9F 00 66 46 53 00 10 01 21 71 DF 00 00 81 90 00 29 [T0=9F,TD1,TA1����]

 TA1��ȱʡ��F=372,D=1,TA1= 95,[F=512,D=16]
 TB1��[00]��b1~b5=0��IC��������VPP(��̵�ѹ);
 TC1��[00]������Nֵ��N���ڱ�ʾ���ӵ���С����ʱ��Ķ��Ᵽ��ʱ�䣻
 TD1���ޣ��޺����ӿ��ֽڣ�[81]��TD2:���ڡ�Э������=T=1
 TD2��[31]��TB3,TA3���ڣ�Э������Ϊ��T=1
 TA3��[F0]��
 TB3��[9F]��
 */