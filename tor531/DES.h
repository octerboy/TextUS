#ifndef CDES_H_CAESAR__DEF
#define CDES_H_CAESAR__DEF

#include <windows.h>

class CDES  
{
public:
	CDES();
	virtual ~CDES();

	//加密解密
	enum	
	{
		ENCRYPT	=	0,	//加密(0)
		DECRYPT			//解密(1)
	};

	//DES算法的模式
	enum
	{
		ECB		=	0,	//ECB模式(0) 当前计算已直接设置成：ECB模式
		CBC				//CBC模式(1)
	};

	typedef bool    (*PSubKey)[16][48];

	//Pad填充的模式
	enum
	{
		PAD_ISO_1 =	0,	//ISO_1填充：数据长度不足8比特的倍数，以0x00补足，如果为8比特的倍数，补8个0x00
		PAD_ISO_2,		//ISO_2填充：数据长度不足8比特的倍数，以0x80,0x00..补足，如果为8比特的倍数，补0x80,0x00..0x00
		PAD_PKCS_7,		//PKCS7填充：数据长度除8余数为n,以(8-n)补足为8的倍数
		PAD_PBOC
	};



	static bool	RunPad(int nType,const char* In,unsigned datalen,char* Out,unsigned& padlen);

	static bool RunDes(bool bType,char* In,unsigned datalen,const char* Key,const unsigned char keylen,char* Out);

	// 计算SK,传人参数：16字节的KEY、输入数据8字节(4字节伪随机数 + 2字节交易序号 + 6字节终端) ，输出SK（8字节）
	static bool GenSK(char *Key,char *In,char *Out);

	//按位取反操作:(~)
	static void Not(char *Out, const char *In, int len);


	//按位异或操作(^)
	void XOR(char *Out, const char *In, int len);

protected:

	//计算并填充子密钥到SubKey数据中
	static void SetSubKey(PSubKey pSubKey, const char Key[8]);
	
	//DES单元运算
	static void DES(char Out[8], char In[8], const PSubKey pSubKey, bool Type);
	

};

#endif


