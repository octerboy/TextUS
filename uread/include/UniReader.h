#ifndef _UniReaderApi_ARTC_H_
#define _UniReaderApi_ARTC_H_

#ifdef __cplusplus
extern "C"{
#endif


/*****************************************
��������: READER_open
��������: PC�����д��֮�佨��ͨ������
�������: Paras		���ӷ�ʽ���˿���Ϣ  ,�����ʽ���£�
					����:"����,������",   ����:Paras[] = "COM1,115200"
�������: ��
����˵��: ����0���豸�ɹ�,����ֵΪ�˿ھ����;����ʧ��
*****************************************/
int __stdcall  READER_open(char* Paras);

/*****************************************
��������: READER_close
��������: PC�����д��֮��Ͽ�ͨ������
�������: DevHandle		�˿ھ����
�������: ��
����˵��: 0:  �ر��豸�ɹ�  �������󣺣�������뼰���������ɳ����Զ��壩

*****************************************/
int __stdcall READER_close(long DevHandle);

/*****************************************
��������: SAM_reset
��������: ��λSAM��
�������: DevHandle			�˿ھ����
		  iSockID			SAM����˳���,1~4
�������: iReplylength		�������ݳ���
		  sReply			������������
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall SAM_reset(long DevHandle,int iSockID,int* iReplylength,char* sReply);

/*****************************************
��������: SAM_command
��������: SAM��ָ���ͨ��֧�ֺ���,��ʵ��SAM��������COSָ��
�������: DevHandle			�˿ھ����
		  iSockID			SAM����˳���,1~4
		  iCommandLength	�����(�����ַ�������)
		  sCommand			��������(ʮ�������ַ���)
�������: iReplylength		�������ݳ���
		  sReply			������������,����SW1��SW2
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall SAM_command(long DevHandle,int iSockID,int iCommandLength, char* sCommand ,int* iReplylength,char* sReply);

/*****************************************
��������: CARD_open
��������: ��IC��
�������: DevHandle			�˿ھ����
		  RequestMode		Ѱ��ģʽ(0:�Զ�Ѱ��;1:ѡ��λ��1;2:ѡ��λ��2;����:ѡ����λ��.)
		                    �����Ǵ���Ӵ�ʽ���ǷǽӴ�ʽ��1��ʾ�ǽӣ�2��ʾ�Ӵ���
�������: PhysicsCardno		���ص�������,�ַ�����ʽ�����ֽ���ǰ,�ǽӴ�ʽ���������ţ��Ӵ�ʽΪ�ա�
		  ResetInformation	Ƭ��λ���ص���Ϣ���Ӵ�ʽ���ظ�λ��Ϣ���ǽӴ�ʽΪ�ա�
		  CardPlace			1:��Ƭ����1��;2:��Ƭ����2��;������ѡ����λ��
		  CardType			01:�߼����ܿ�;02:CPU��;����:Ԥ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall CARD_open(long DevHandle, int RequestMode,char* PhysicsCardno, char* ResetInformation, int* CardPlace, char* CardType);

/*****************************************
��������: CARD_close
��������: �رտ�Ƭ,����ǽӴ�ʽ��
�������: DevHandle		�˿ھ����
�������: ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall CARD_close(long DevHandle);

/*****************************************
��������: PRO_command
��������: CPU��ָ���ͨ��֧�ֺ�������ʵ��CPU��������COSָ��
�������: DevHandle			�˿ھ����
		  CardPlace			��Ƭλ����
		  iCommandLength	�����(�����ַ�������)
		  sCommand			��������(ʮ�������ַ���)	
�������: iReplylength		�������ݳ���
		  sReply			������������,����SW1��SW2
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall PRO_command(long DevHandle,int CardPlace, int iCommandLength,char* sCommand, int* iReplylength, char* sReply);

/*****************************************
��������: ICC_authenticate
��������: ������Կ��������������֤
�������: DevHandle		�˿ھ����
		  CardPlace		��Ƭλ������
		  sector		������(0~15)
		  keytype		��Կ���ͣ�
		                ȡֵ
                        0,KEYA-- ����ΪKEYA
                        1,KEYB-- ����ΪKEYB
		  key	        ��Կ,16�����ִ���ʽ	
�������: ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall ICC_authenticate(long DevHandle, int CardPlace,int sector,int keytype, char* key);

/*****************************************
��������: ICC_readsector
��������: ��ȡͨ��������֤��Ŀ��е�����
�������: DevHandle		�˿ھ����
		  CardPlace		��Ƭλ������
		  sector		������(0~15)
		  start			��ʼ�ֽ�λ��(0~47)
		  len			����ȡ�������ֽ���(1~48)
�������: data			��ȡ����������,�ַ�����ʽ
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall ICC_readsector(long DevHandle, int CardPlace, int sector, int start, int len, char* data);

/*****************************************
��������: ICC_writesector
��������: ��ȡͨ��������֤��Ŀ��е�����
�������: DevHandle		�˿ھ����
		  CardPlace		��Ƭλ������
		  sector		������(0~15)
		  start			��ʼ�ֽ�λ��(ȡֵ������0��16��32��48)
		  len			��д��������ֽ���(ȡֵ������16��32��48)
		  data			д�����������,�ַ�����ʽ
�������: ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall ICC_writesector(long DevHandle, int CardPlace, int sector, int start, int len, char* data);

/*****************************************
��������: GetReaderVersion
��������: ��ȡ��д���Լ��ӿڰ汾��Ϣ
�������: DevHandle			�˿ھ����
          iRVerMaxLength	��д���汾��Ϣ�ַ���������ֽڳ���
		  iAPIVerMaxlength	��д���ӿں�����汾��Ϣ�ַ���������ֽڳ���
�������: sReaderVersion	��д���汾��Ϣ
		  sAPIVersion		��д���ӿں�����汾��Ϣ
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int   __stdcall GetReaderVersion(long DevHandle,char* sReaderVersion, int iRVerMaxLength,char* sAPIVersion, int iAPIVerMaxlength);

/*****************************************
��������: Led_display
��������: ���ƶ�д�����������״̬
�������: DevHandle			�˿ھ����
          cRed				��Ӧ��ɫ��,0x01-��,0x02-��,0x03-��˸һ��,Ĭ��Ϊ��
		  cGreen			��Ӧ��ɫ��,0x01-��,0x02-��,0x03-��˸һ��,Ĭ��Ϊ��
		  cBlue				��Ӧ��ɫ��,0x01-��,0x02-��,0x03-��˸һ��,Ĭ��Ϊ��
�������: ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int  __stdcall Led_display(long DevHandle,unsigned char cRed,unsigned char cGreen,unsigned char cBlue);

/*****************************************
��������: Led_display
��������: ���ƶ�д�����������״̬
�������: DevHandle			�˿ھ����
          cBeep				��Ӧ������,0x01-��,0x02-ͣ,0x03-ཱུ���һ��,Ĭ��Ϊͣ
�������: ��
����˵��: ����0�ɹ�;����ʧ��
*****************************************/
int  __stdcall Audio_control(long DevHandle,unsigned  char cBeep);

/*****************************************
��������: GetOpInfo
��������: ��ȡִ��״̬��Ϣ
�������: retcode	ִ��״̬����,��ǰ���߻��߿����������ķ���ֵ
�������: ��
����˵��: ����ȡ��ִ�н����������
*****************************************/
char* __stdcall GetOpInfo(int retcode);


/*****************************************
��������: GetCardNo
��������: ��ȡ433�������
�������: ��
�������: CardNo	
����˵��: 
*****************************************/
bool  __stdcall GetCardNo_RFID(char* CardNo);


/*****************************************
��������: GetCPCID
��������: ��ȡ433��ID��
�������: ��
�������: CPCID	
����˵��: 
*****************************************/
bool  __stdcall GetCPCID_RFID(char* CPCID);


/*****************************************
��������: GetFlagStationInfo
��������: ��ȡ433���ڱ�ʶվ��Ϣ
�������: CPCID
�������: FlagStationCnt   FlagStationInfo	
����˵��: 
*****************************************/
bool  __stdcall GetFlagStationInfo_RFID(char* CPCID,int *FlagStationCnt,char* FlagStationInfo);


/*****************************************
��������: GetPowerInfo
��������: ��ȡ433���ĵ�����Ϣ
�������: CPCID
�������: PowerInfo	
����˵��: 
*****************************************/
bool  __stdcall GetPowerInfo_RFID(char* CPCID,int *PowerInfo);


/*****************************************
��������: Set433CardMode
��������:����433����ģʽ
�������: CPCID,iMode
�������: 	
����˵��: 
*****************************************/
bool  __stdcall Set433CardMode_RFID(char* CPCID,int iMode);


/*****************************************
��������: Get433CardMode
��������:  ��ȡ433���ĵ�ǰģʽ
�������: CPCID
�������: iMode	
����˵��: 
*****************************************/
bool  __stdcall Get433CardMode_RFID(char* CPCID,int* iMode);



int __stdcall ClearEtcPathInfo_ETC (int* iObuType,char* sObuId);
int __stdcall GetEtcPathInfo_ETC(int* iOBUType,char* sOBUID,int* iFlagStationCnt,char* sFlagStationInfo);
int __stdcall InitAnt_ETC(int iTxPower,int iChannelID,char *sTime);
int __stdcall OpenAnt_ETC(int iMode, int iMemoryArea);
int __stdcall CloseAnt_ETC();

int _stdcall PRO_detect(long DevHandle);


#ifdef __cplusplus
}
#endif

#endif
