#ifndef _UniReaderApi_ARTC_H_
#define _UniReaderApi_ARTC_H_

#ifdef __cplusplus
extern "C"{
#endif


/*****************************************
函数名称: READER_open
函数描述: PC机与读写器之间建立通信连接
输入参数: Paras		连接方式及端口信息  ,具体格式如下：
					串口:"串口,波特率",   例如:Paras[] = "COM1,115200"
输出参数: 无
返回说明: 大于0打开设备成功,返回值为端口句柄号;否则失败
*****************************************/
int __stdcall  READER_open(char* Paras);

/*****************************************
函数名称: READER_close
函数描述: PC机与读写器之间断开通信连接
输入参数: DevHandle		端口句柄号
输出参数: 无
返回说明: 0:  关闭设备成功  其他错误：（错误代码及错误描述由厂家自定义）

*****************************************/
int __stdcall READER_close(long DevHandle);

/*****************************************
函数名称: SAM_reset
函数描述: 复位SAM卡
输入参数: DevHandle			端口句柄号
		  iSockID			SAM卡槽顺序号,1~4
输出参数: iReplylength		返回数据长度
		  sReply			返回数据内容
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall SAM_reset(long DevHandle,int iSockID,int* iReplylength,char* sReply);

/*****************************************
函数名称: SAM_command
函数描述: SAM卡指令的通用支持函数,可实现SAM卡的任意COS指令
输入参数: DevHandle			端口句柄号
		  iSockID			SAM卡槽顺序号,1~4
		  iCommandLength	命令长度(命令字符串长度)
		  sCommand			命令内容(十六进制字符串)
输出参数: iReplylength		返回数据长度
		  sReply			返回数据内容,包含SW1与SW2
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall SAM_command(long DevHandle,int iSockID,int iCommandLength, char* sCommand ,int* iReplylength,char* sReply);

/*****************************************
函数名称: CARD_open
函数描述: 打开IC卡
输入参数: DevHandle			端口句柄号
		  RequestMode		寻卡模式(0:自动寻卡;1:选择位置1;2:选择位置2;其他:选其他位置.)
		                    具体是代表接触式还是非接触式，1表示非接，2表示接触。
输出参数: PhysicsCardno		返回的物理卡号,字符串形式，低字节在前,非接触式返回物理卡号，接触式为空。
		  ResetInformation	片复位返回的信息，接触式返回复位信息，非接触式为空。
		  CardPlace			1:卡片放在1区;2:卡片放在2区;其他：选其他位置
		  CardType			01:逻辑加密卡;02:CPU卡;其他:预留
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall CARD_open(long DevHandle, int RequestMode,char* PhysicsCardno, char* ResetInformation, int* CardPlace, char* CardType);

/*****************************************
函数名称: CARD_close
函数描述: 关闭卡片,挂起非接触式卡
输入参数: DevHandle		端口句柄号
输出参数: 无
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall CARD_close(long DevHandle);

/*****************************************
函数名称: PRO_command
函数描述: CPU卡指令的通用支持函数，可实现CPU卡的任意COS指令
输入参数: DevHandle			端口句柄号
		  CardPlace			卡片位置区
		  iCommandLength	命令长度(命令字符串长度)
		  sCommand			命令内容(十六进制字符串)	
输出参数: iReplylength		返回数据长度
		  sReply			返回数据内容,包含SW1与SW2
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall PRO_command(long DevHandle,int CardPlace, int iCommandLength,char* sCommand, int* iReplylength, char* sReply);

/*****************************************
函数名称: ICC_authenticate
函数描述: 载入密钥并对扇区进行认证
输入参数: DevHandle		端口句柄号
		  CardPlace		卡片位置区号
		  sector		扇区号(0~15)
		  keytype		密钥类型，
		                取值
                        0,KEYA-- 类型为KEYA
                        1,KEYB-- 类型为KEYB
		  key	        密钥,16进制字串形式	
输出参数: 无
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall ICC_authenticate(long DevHandle, int CardPlace,int sector,int keytype, char* key);

/*****************************************
函数名称: ICC_readsector
函数描述: 读取通过扇区认证后的块中的数据
输入参数: DevHandle		端口句柄号
		  CardPlace		卡片位置区号
		  sector		扇区号(0~15)
		  start			起始字节位置(0~47)
		  len			所读取的数据字节数(1~48)
输出参数: data			读取的数据内容,字符串形式
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall ICC_readsector(long DevHandle, int CardPlace, int sector, int start, int len, char* data);

/*****************************************
函数名称: ICC_writesector
函数描述: 读取通过扇区认证后的块中的数据
输入参数: DevHandle		端口句柄号
		  CardPlace		卡片位置区号
		  sector		扇区号(0~15)
		  start			起始字节位置(取值仅限于0、16、32、48)
		  len			所写入的数据字节数(取值仅限于16、32、48)
		  data			写入的数据内容,字符串形式
输出参数: 无
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall ICC_writesector(long DevHandle, int CardPlace, int sector, int start, int len, char* data);

/*****************************************
函数名称: GetReaderVersion
函数描述: 获取读写器以及接口版本信息
输入参数: DevHandle			端口句柄号
          iRVerMaxLength	读写器版本信息字符串的最大字节长度
		  iAPIVerMaxlength	读写器接口函数库版本信息字符串的最大字节长度
输出参数: sReaderVersion	读写器版本信息
		  sAPIVersion		读写器接口函数库版本信息
返回说明: 等于0成功;否则失败
*****************************************/
int   __stdcall GetReaderVersion(long DevHandle,char* sReaderVersion, int iRVerMaxLength,char* sAPIVersion, int iAPIVerMaxlength);

/*****************************************
函数名称: Led_display
函数描述: 控制读写器发光二极管状态
输入参数: DevHandle			端口句柄号
          cRed				对应红色灯,0x01-亮,0x02-灭,0x03-闪烁一下,默认为灭
		  cGreen			对应绿色灯,0x01-亮,0x02-灭,0x03-闪烁一下,默认为灭
		  cBlue				对应蓝色灯,0x01-亮,0x02-灭,0x03-闪烁一下,默认为灭
输出参数: 无
返回说明: 等于0成功;否则失败
*****************************************/
int  __stdcall Led_display(long DevHandle,unsigned char cRed,unsigned char cGreen,unsigned char cBlue);

/*****************************************
函数名称: Led_display
函数描述: 控制读写器发光二极管状态
输入参数: DevHandle			端口句柄号
          cBeep				对应蜂鸣器,0x01-响,0x02-停,0x03-嘟的响一声,默认为停
输出参数: 无
返回说明: 等于0成功;否则失败
*****************************************/
int  __stdcall Audio_control(long DevHandle,unsigned  char cBeep);

/*****************************************
函数名称: GetOpInfo
函数描述: 获取执行状态信息
输入参数: retcode	执行状态代码,当前机具或者卡操作函数的返回值
输出参数: 无
返回说明: 所获取的执行结果中文描述
*****************************************/
char* __stdcall GetOpInfo(int retcode);


/*****************************************
函数名称: GetCardNo
函数描述: 获取433卡表面号
输入参数: 无
输出参数: CardNo	
返回说明: 
*****************************************/
bool  __stdcall GetCardNo_RFID(char* CardNo);


/*****************************************
函数名称: GetCPCID
函数描述: 获取433卡ID号
输入参数: 无
输出参数: CPCID	
返回说明: 
*****************************************/
bool  __stdcall GetCPCID_RFID(char* CPCID);


/*****************************************
函数名称: GetFlagStationInfo
函数描述: 获取433卡内标识站信息
输入参数: CPCID
输出参数: FlagStationCnt   FlagStationInfo	
返回说明: 
*****************************************/
bool  __stdcall GetFlagStationInfo_RFID(char* CPCID,int *FlagStationCnt,char* FlagStationInfo);


/*****************************************
函数名称: GetPowerInfo
函数描述: 获取433卡的电量信息
输入参数: CPCID
输出参数: PowerInfo	
返回说明: 
*****************************************/
bool  __stdcall GetPowerInfo_RFID(char* CPCID,int *PowerInfo);


/*****************************************
函数名称: Set433CardMode
函数描述:设置433卡的模式
输入参数: CPCID,iMode
输出参数: 	
返回说明: 
*****************************************/
bool  __stdcall Set433CardMode_RFID(char* CPCID,int iMode);


/*****************************************
函数名称: Get433CardMode
函数描述:  获取433卡的当前模式
输入参数: CPCID
输出参数: iMode	
返回说明: 
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
