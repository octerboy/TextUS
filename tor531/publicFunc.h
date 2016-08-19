

// ASC��ת����BCD��
char *  ftAtoh(char *ascstr, char *bcdstr,int bcdlen);

// BCD��ת����ASC��
int  ftHtoa(char *hexstr, char *ascstr,int  length);

// 16���ƽ���ַ���ת���ɣ�10���ƽ��
long ftHexToLong(char *HexStr);

// ��long�͵Ľ��ֵת����16���ƵĽ���ַ���
int ftLongToHex(long Val,char *HexStr,int Flag = 0);

// ��long�ͽ��ת����4�ֽڽ��׽������[8��ʮ�����Ƶ�ASCII�ַ���]
int ftLongToAmount(long Val,char *HexStr);

// ���16���ƵĽ���ַ����� 4�ֽ� BCD���ASCII�ַ���
int  ftFillAmount(char *Str);

// MAC/TAC������ַ�����亯��,���80 �� 80 00..������Ϊ8�ı���
int  ftFillString(char *Str);

// ����ַ�����ָ������
int  ftFillStringF(char *Str,int Len,char Val ='F');

// ���ַ���ת���ɴ�д
void ftStringToUpper(char *Str);

// ���ַ���ת����Сд
void ftStringToLower(char *Str);

// ���ַ�ֵת���ɶ����Ʊ����ʾ���ַ������ַ�������Ϊ8
int ftCharToBitString(unsigned char Val,char *OutBuf);

// ���ַ���ת���ɶ����Ʊ����ʾ���ַ���
int ftDataToBitString(char *InData,int Len,char *OutBuf);

// ��BIT�ַ���ת��������ֵ
int ftBitStringToVal(char *BitString,int Len,int *Val);

// ������
int ftCalXOR(char *Param1,char *Param2,int Len,char *Out);


//�ַ������У��ͼ���,Flag  ȱʡֵΪ0��������������Ϊ��ASCII��
int ftCalLRC(char *DataBuf,int Len,int Flag = 0 );
 
int ftCalLRC1(char *DataBuf,int Len,char *RetVal,int Flag = 0);

// ������ܺ���
int ftDataEnc(char *pIn,int pLen,char *pOut);

// ������ܺ���
int ftDataDec(char *pIn,int pLen,char *pOut);

// DES����
int ftDesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// 3DES����
int ft3DesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// DES����
int ftDesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// 3DES����
int ft3DesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=1);

// Ȧ�桢Ȧ�ᡢȡ�֡����� ���׹�����Կ���㺯��
int ftCalSK(char *pKey,int pKeyLen,char *pIn,int pLen,char *pOut,int pFlag = 1 );

// Ȧ�桢Ȧ�ᡢȡ�֡����� ����MAC���㺯��
int ftCalMac(char *Key,char *Vector,char *Data,int DataLen,char *Out,int Flag =1);
int CalMac(char *Key,char *Vector,char *Data,char *AscMAC);


// �����������MAC���㺯��
int ftCalMacFor3Des(char *Key,char *InitData,char *Data,char *Out);

// ��Կ��ɢ����
int ftDiversify(char *Data,char *Key,char *Out);


// ����BCD���ATR��Ϣ����IC����ͨ��Э��,����ֵΪͨ��Э�����ͣ�0:ͨ��Э��Ϊ��T=0 ; 1:ͨ��Э��Ϊ��T=1
int ftGetProtocolForATR(unsigned char *pATR);

// ��־��¼����
void ftWriteLog(char *pFileName,char *format,...);


// �ַ�����ֺ���
int ftSplitStr(char *pStr,int pStrLen,char pSplitChar,char pData[][81]);

// ����ָ��BCD�����PCKֵ
int ftCalPCK(unsigned char *Str,int Len,unsigned int *PCK);

/*
warning C4819 -- ����취
�������(�����ĵ�)���򿪳���warning���ļ���Ctrl+Aȫѡ��Ȼ�����ļ��˵���file->advanced save options ���ڵ�����ѡ����ѡ���µı��뷽ʽΪ��UNICODE codepage 1200 �����ȷ��������ͽ����
�������(����ͨ��)�������ļ����������Ҽ����ڵ����˵���ѡ�񣺱༭ �˵������ļ���ѡ���ļ� �˵� �� ���Ϊ �˵��
                    �ڴ򿪵ģ����Ϊ �Ի����У��ı��� �����б���ѡ��Unicode ��ּ��ɽ��������

*/