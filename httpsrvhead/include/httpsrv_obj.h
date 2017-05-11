/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/* Amor��, Pius->indic ָ������Ľṹ */
struct GetRequestCmd {
	enum {
		GetHead=0,		/*0: getHead(char* name)  */
		GetHeadInt=1,		/*1: getHeadInt(char* name) */
		GetPara=2,		/*2: getPara(char* name);   */
		GetParaLen=3,		/*3: getPara(char* name,int *len); */
		GetLenOfContent=4,	/*4: getContentSize(); */
		GetFile=5,              /*5: getFile(char* name);          */
		GetFileLen=6,           /*6: getFile(char* name, int *len);*/
		GetFileLenType=7,       /*7: getFile(char* name, int *len, char **filename, char** type); */
		GetQuery=8,		/*8: getQuery() */
		GetHeadArr=9		/*9: getHeadArr(char* name)  */
	} fun;	/*  �������,ָ������*/
	
	const char *name; 	/*  �������
			����fun=0,1, ָ��HTTPͷ������;
			����fun=2~6, ָ����еĲ�������;
			����fun=7~12, �˲����޶���,��������
			*/
	const char *valStr;	/* ���������fun=0,2~6,9~12��ָ��HTTPͷ���ݡ���������Ӧ�����ݡ��ļ�����(char *) */
	const char **valStrArr;	/* ���������fun=0,2~6,9~12��ָ��HTTPͷ���ݡ���������Ӧ�����ݡ��ļ�����(char *) */
	long valInt;	/* ���������fun=1��ָ��HTTPͷ����  */	
	long len;	/* ���������fun=3,4,6,7��ָ��������ݣ�����Ӧ���������ݡ��ļ����ݣ��ĳ��ȣ����ֽ��� */
	char *filename;	/* ���������fun=7��ָ���ļ��������������ģ� */ 
	char *type;	/* ���������fun=7��ָ���ļ����ͣ����������ģ�*/ 
	void *content;	/* ���������fun=8,9��ָ��TiXmlDocument*��TiXmlElement*   */
	void *extend;	/* ������չ */
};

/* Amor��, Pius->indic ָ������Ľṹ */
struct SetResponseCmd {
	enum {
		GetHead = 0,		/*0: setHead(char* name, char* value); 	*/
		GetHeadInt = 1,		/*1: setHead(char* name, int value); 	*/		
		SetHead = 2,		/*2: setHead(char* name, char* value); 	*/
		SetHeadInt = 3,		/*3: setHead(char* name, int value); 	*/		
		SetHeadTime = 4,	/*4: setHead(char* name, int value); 	*/		
		AddHead = 5,		/*5: addHead(char* name, char* value)  	*/
		AddHeadInt = 6,		/*6: addHead(char* name, int value)    	*/
		SetStatus = 7,		/*7: setStatus(int sc)                 	*/
		SendError = 8,		/*8: sendError(int sc)                 	*/
		SetLenOfContent = 9,	/*9: setContentSize			*/		

		OutPut = 10,		/*10:output(char* str)			*/
		OutPutLen = 11,		/*11:output(char* str, int len)         	*/
		OutPutJS = 12		/*12:output(char *in, int inlen, char *js) */
	} fun;	/* �������,ָ������ */
	
	const char *name;	/* ���룬����fun=setHead, ָ��HTTP����ͷ������name */
	const char *valStr;	/* ���룬����fun=setHead, sendError,output,setXMLComment,setContentType��
				 ָ��value, err_title, value,type, str */
	long valInt;	/* ���룬����fun=setHead��ָint value */
	time_t valTime;	/* ���룬����fun=setHead��ָint value */
	int sc;		/* ���룬����fun=setStatus,sendError��ָsc */
	long len;	/* ���룬����fun=OutPut,SetLenOfContent, ָlen */
	char type;	/* ���룬����fun=setDataType��ָdataType */
	void *extend;	/* ������չ */
};

/*----------- ȡ��HTTP�������� -----------*/
#define SponteGetCmd	cmd_pius.ordo = Notitia::CMD_HTTP_GET; \
			cmd_pius.indic = &request_cmd; \
			aptus->sponte(&cmd_pius);

#define REQ_CMD			\
struct GetRequestCmd  request_cmd;	\
Amor::Pius cmd_pius;  /* ��������httpsrvȡ��HTTP�������ݻ�����HTTP��Ӧ���� */

	inline const char* getHead(const char* name) 
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetHead ;
		request_cmd.name = name;
		SponteGetCmd;
		return( request_cmd.valStr);
	}; 
	
	inline const char** getHeadArr(const char* name) 
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetHeadArr ;
		request_cmd.name = name;
		SponteGetCmd;
		return( request_cmd.valStrArr);
	}; 
	
	inline bool headArrContain(const char* name, char *val) 
	{
		const char **p;
		p = getHeadArr(name);
		if ( !p ) return false;
		while ( *p )
		{
			if (strcasecmp(*p, val) == 0 )
				return true;
			p++;
		}
		return false;
	};

	inline bool headArrContain(const char** arr, const char *val) 
	{
		if ( !arr ) return false;
		while ( *arr )
		{
			if (strcasecmp(*arr, val) == 0 )
				return true;
			arr++;
		}
		return false;
	};
	inline long getHeadInt(const char* name)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetHeadInt ;
		request_cmd.name = name;
		SponteGetCmd;
		return( request_cmd.valInt);
	}; 
	
	/* ȡ������ */
	inline const char* getPara(const char* name)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetPara ;
		request_cmd.name = name;
		SponteGetCmd;
		return( request_cmd.valStr);		
	};
	
	inline const char* getPara(const char* name, long *len)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetParaLen ;
		request_cmd.name = name;
		SponteGetCmd;
		*len = request_cmd.len;
		return( request_cmd.valStr);		
	};
	
	inline long getContentSize()
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetLenOfContent ;
		SponteGetCmd;
		return(request_cmd.len);		
	};

	inline const char* getFile(const char* name)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetFile ;
		request_cmd.name = name;
		SponteGetCmd;
		return( request_cmd.valStr);				
	};

	inline const char* getFile(const char* name, long *len)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetFileLen ;
		request_cmd.name = name;
		SponteGetCmd;
		*len = request_cmd.len;
		return( request_cmd.valStr);						
	};

	inline const char* getFile(const char* name, long *len, char **filename, char** type)
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetFileLenType ;
		request_cmd.name = name;
		SponteGetCmd;
		*len = request_cmd.len;
		*filename = request_cmd.filename;
		*type = request_cmd.type;
		return( request_cmd.valStr);						
	};

	inline const char* getQuery()
	{
		REQ_CMD
		request_cmd.fun = GetRequestCmd::GetQuery;
		SponteGetCmd;
		return( request_cmd.valStr);						
	};
	
/*----------- ����HTTP��Ӧ���� -----------*/
#define SponteSetCmd  	cmd_pius.ordo = Notitia::CMD_HTTP_SET; \
			cmd_pius.indic = &response_cmd; \
			aptus->sponte(&cmd_pius);

#define RES_CMD			\
struct SetResponseCmd  response_cmd;	\
Amor::Pius cmd_pius;  /* ��������httpsrvȡ��HTTP�������ݻ�����HTTP��Ӧ���� */

	inline const char* getResHead(const char* name) 
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::GetHead ;
		response_cmd.name = name;
		SponteSetCmd;
		return( response_cmd.valStr);
	}; 
	
	inline long getResHeadInt(const char* name)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::GetHeadInt ;
		response_cmd.name = name;
		SponteSetCmd;
		return( response_cmd.valInt);
	}; 

	inline void setHead(const char* name, const char* value)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SetHead ;
		response_cmd.name = name;
		response_cmd.valStr = value;
		SponteSetCmd;
	};

	inline void setHead(const char* name, long value)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SetHeadInt;
		response_cmd.name = name;
		response_cmd.valInt = value;
		SponteSetCmd;
	};

	inline void setHeadTime(const char* name, time_t value)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SetHeadTime;
		response_cmd.name = name;
		response_cmd.valTime = value;
		SponteSetCmd;
	};

	inline void addHead(const char* name, const char* value)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::AddHead ;
		response_cmd.name = name;
		response_cmd.valStr = value;
		SponteSetCmd;
	};

	inline void addHead(const char* name, long value)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::AddHeadInt;
		response_cmd.name = name;
		response_cmd.valInt = value;
		SponteSetCmd;		
	};
	
	inline void setStatus(int sc)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SetStatus;
		response_cmd.sc = sc;
		SponteSetCmd;				
	};	

	inline void setContentSize(long len)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SetLenOfContent;
		response_cmd.len = len;
		SponteSetCmd;				
	};

	inline void sendError(int sc) 
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::SendError;
		response_cmd.sc = sc;
		SponteSetCmd;		
	};
	
	inline void output(const char* str)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::OutPut;
		response_cmd.valStr = str;
		SponteSetCmd;				
	};

	inline void output(const char* str, long len)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::OutPutLen;
		response_cmd.valStr = str;
		response_cmd.len = len;
		SponteSetCmd;						
	};
	
	inline void outputJS(const char *in)
	{
		RES_CMD
		response_cmd.fun = SetResponseCmd::OutPutJS;
		response_cmd.valStr = in;
		SponteSetCmd;
	};

	inline long formatOutput(const char* format,...)
	{
		RES_CMD
	 	va_list va;
 		char buf[1024];
 
		response_cmd.fun = SetResponseCmd::OutPutLen;
 		va_start(va, format);
		response_cmd.len = TEXTUS_VSNPRINTF(buf, sizeof(buf)-1, format, va);
		va_end(va);
		response_cmd.valStr = buf;
		SponteSetCmd;
		return response_cmd.len;
	};	
