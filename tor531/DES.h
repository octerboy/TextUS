#ifndef CDES_H_CAESAR__DEF
#define CDES_H_CAESAR__DEF

#include <windows.h>

class CDES  
{
public:
	CDES();
	virtual ~CDES();

	//���ܽ���
	enum	
	{
		ENCRYPT	=	0,	//����(0)
		DECRYPT			//����(1)
	};

	//DES�㷨��ģʽ
	enum
	{
		ECB		=	0,	//ECBģʽ(0) ��ǰ������ֱ�����óɣ�ECBģʽ
		CBC				//CBCģʽ(1)
	};

	typedef bool    (*PSubKey)[16][48];

	//Pad����ģʽ
	enum
	{
		PAD_ISO_1 =	0,	//ISO_1��䣺���ݳ��Ȳ���8���صı�������0x00���㣬���Ϊ8���صı�������8��0x00
		PAD_ISO_2,		//ISO_2��䣺���ݳ��Ȳ���8���صı�������0x80,0x00..���㣬���Ϊ8���صı�������0x80,0x00..0x00
		PAD_PKCS_7,		//PKCS7��䣺���ݳ��ȳ�8����Ϊn,��(8-n)����Ϊ8�ı���
		PAD_PBOC
	};



	static bool	RunPad(int nType,const char* In,unsigned datalen,char* Out,unsigned& padlen);

	static bool RunDes(bool bType,char* In,unsigned datalen,const char* Key,const unsigned char keylen,char* Out);

	// ����SK,���˲�����16�ֽڵ�KEY����������8�ֽ�(4�ֽ�α����� + 2�ֽڽ������ + 6�ֽ��ն�) �����SK��8�ֽڣ�
	static bool GenSK(char *Key,char *In,char *Out);

	//��λȡ������:(~)
	static void Not(char *Out, const char *In, int len);


	//��λ������(^)
	void XOR(char *Out, const char *In, int len);

protected:

	//���㲢�������Կ��SubKey������
	static void SetSubKey(PSubKey pSubKey, const char Key[8]);
	
	//DES��Ԫ����
	static void DES(char Out[8], char In[8], const PSubKey pSubKey, bool Type);
	

};

#endif


