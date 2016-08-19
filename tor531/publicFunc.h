

// ASC码转换成BCD码
char *  ftAtoh(char *ascstr, char *bcdstr,int bcdlen);

// BCD码转换成ASC码
int  ftHtoa(char *hexstr, char *ascstr,int  length);

// 16进制金额字符串转换成：10进制金额
long ftHexToLong(char *HexStr);

// 将long型的金额值转换成16进制的金额字符串
int ftLongToHex(long Val,char *HexStr,int Flag = 0);

// 将long型金额转换成4字节交易金额或余额[8个十六进制的ASCII字符串]
int ftLongToAmount(long Val,char *HexStr);

// 填充16进制的金额字符串到 4字节 BCD码的ASCII字符；
int  ftFillAmount(char *Str);

// MAC/TAC计算的字符串填充函数,填充80 或 80 00..到长度为8的倍数
int  ftFillString(char *Str);

// 填充字符串到指定长度
int  ftFillStringF(char *Str,int Len,char Val ='F');

// 将字符串转换成大写
void ftStringToUpper(char *Str);

// 将字符串转换成小写
void ftStringToLower(char *Str);

// 将字符值转换成二进制编码表示的字符串，字符串长度为8
int ftCharToBitString(unsigned char Val,char *OutBuf);

// 将字符串转换成二进制编码表示的字符串
int ftDataToBitString(char *InData,int Len,char *OutBuf);

// 将BIT字符串转换成整型值
int ftBitStringToVal(char *BitString,int Len,int *Val);

// 异或计算
int ftCalXOR(char *Param1,char *Param2,int Len,char *Out);


//字符串异或校验和计算,Flag  缺省值为0：计算输入数据为：ASCII码
int ftCalLRC(char *DataBuf,int Len,int Flag = 0 );
 
int ftCalLRC1(char *DataBuf,int Len,char *RetVal,int Flag = 0);

// 密码加密函数
int ftDataEnc(char *pIn,int pLen,char *pOut);

// 密码解密函数
int ftDataDec(char *pIn,int pLen,char *pOut);

// DES加密
int ftDesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// 3DES加密
int ft3DesEnc(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// DES解密
int ftDesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=0);

// 3DES解密
int ft3DesDec(char *pKey,int pKeyLen,char *pIn,char *pOut,int Len,int pFlag=1);

// 圈存、圈提、取现、消费 交易过程密钥计算函数
int ftCalSK(char *pKey,int pKeyLen,char *pIn,int pLen,char *pOut,int pFlag = 1 );

// 圈存、圈提、取现、消费 交易MAC计算函数
int ftCalMac(char *Key,char *Vector,char *Data,int DataLen,char *Out,int Flag =1);
int CalMac(char *Key,char *Vector,char *Data,char *AscMAC);


// 无特殊情况的MAC计算函数
int ftCalMacFor3Des(char *Key,char *InitData,char *Data,char *Out);

// 密钥分散函数
int ftDiversify(char *Data,char *Key,char *Out);


// 根据BCD码的ATR信息计算IC卡的通信协议,返回值为通信协议类型；0:通信协议为：T=0 ; 1:通信协议为：T=1
int ftGetProtocolForATR(unsigned char *pATR);

// 日志记录函数
void ftWriteLog(char *pFileName,char *format,...);


// 字符串拆分函数
int ftSplitStr(char *pStr,int pStrLen,char pSplitChar,char pData[][81]);

// 计算指定BCD数组的PCK值
int ftCalPCK(unsigned char *Str,int Len,unsigned int *PCK);

/*
warning C4819 -- 解决办法
解决方法(网上文档)：打开出现warning的文件，Ctrl+A全选，然后在文件菜单：file->advanced save options ，在弹出的选项中选择新的编码方式为：UNICODE codepage 1200 ，点击确定，问题就解决了
解决方法(测试通过)：单击文件，点击鼠标右键，在弹出菜单中选择：编辑 菜单，打开文件后，选择：文件 菜单 的 另存为 菜单项，
                    在打开的：另存为 对话框中，的编码 下拉列表中选择：Unicode 项保持即可解决此问题

*/