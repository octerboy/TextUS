/* 命令分几种，INS_Normal：标准，INS_Abort：终止 */
struct DyVarBase { /* 动态变量， 包括来自报文的 */
	int index;	//索引, 也就是下标值
	unsigned char *val_p;	//p有时指向这里的val， 但也可能指向输入、输出的域.c_len就是长度。
	size_t c_len;
	TBuffer *con;		//外部可访问, 通常指向nal, Global时指向G_CFG中的变量
	TBuffer nal;		//内部数据, 
	bool def_link;
	void input(unsigned char *p, size_t len, bool link=false)
	{
		if ( def_link || link)
		{
			val_p = p;
			c_len = len;
		} else {
			con->reset();
			con->grant(len+1);
			con->input(p,len);
			con->point[0] = 0;
			val_p = con->base;
			c_len = len;
		}
	};

};

struct DyList {
	unsigned char *con;  /* 指向 cmd_buf中的某个点 */
	size_t len;
	int dy_pos;
	DyList () {
		con  =0;
		len = 0;
		dy_pos = -1;
	};
};

struct CmdSnd {
	int fld_no;	//发送的域号
	void *ext_fld;	//扩展域, 比如针对XML、JSON等
	struct DyList *dy_list;
	int dy_num;
	unsigned char *cmd_buf;
	size_t cmd_len;
	const char *tag;	/*  比如"component"这样的内容，指明子序列中的元素 */

	CmdSnd () {
		cmd_buf = 0;
		cmd_len = 0;
		fld_no = -1;
		ext_fld = 0;
		dy_num = 0;
		dy_list = 0;
	};
};

struct CmdRcv {
	int fld_no;	//接收的域号, 有多个接收定义，每个只定义一个变量.所以，有多个定义，指向同一个域。
	void *ext_fld;	//扩展域, 比如针对XML、JSON等
	int dyna_pos;	//动态变量位置, -1表示静态
	size_t start;
	size_t length;
	unsigned char *src_con;	//赋值可能来自定义文件中的数据
	size_t src_len;
	unsigned char *must_con;
	size_t must_len;
	const char *err_code; //pac_ele定义的, 作变量替换, def_ele定义的，不作任何处理。当本域不符合要求，设置此错误码。
	bool err_disp_hex;

	const char *tag;//比如： reply, sw
	CmdRcv() {
		fld_no = -1;
		ext_fld = 0;
		dyna_pos = -1;
		start =1;
		length = 0;
		must_con = 0;
		must_len = 0;
		src_con = 0;
		src_len = 0;
		err_code = 0;
		err_disp_hex = false;
	};
};

struct InsData {
	const char *ins_tag;//比如： CMac等
	int up_subor;	//指示指令报文送给哪一个下级模块

	struct CmdSnd *snd_lst;
	int snd_num;

	struct CmdRcv *rcv_lst;
	int rcv_num;

	const char *err_code; //先从map文件中得到, 若是变量, 则转为实际值, 从pac_def中得到的, 不作任何处理
	bool counted;		//是否计数
	bool isFunction;	//是否为函数方式
	const char *log_str;	//这是直接从外部定义文件得到的内容，不作任何处理。

	void *ext_ins;	//基础报文定义，下一级处理
	InsData() {
		log_str = 0;
		up_subor = -1;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		counted = false;	/* 不计入指令数计算 */
		isFunction = false;
		ext_ins = 0;
	};
};

struct InsReply {	//Ans_InsWay的indic
	char *err_str;
	const char *err_code;
};

struct InsWay {		//Pro_InsWay的indic
	struct InsData *dat;
	struct DyVarBase *snap;     //随时的变量, 包括
	int snap_num;
	struct InsReply *reply;
};

struct FlowStr {		//Pro_TranWay的indic
	unsigned char *flow_str;
	size_t len;
};

int load_xml(const char *f_name, TiXmlDocument &doc,  TiXmlElement *&root, const char *md5_content, char err_str[])
{
	if ( !f_name || strlen(f_name) ==0 ) return 0;
	doc.SetTabSize( 8 );
	if ( !doc.LoadFile (f_name) || doc.Error()) 
	{
		TEXTUS_SPRINTF(err_str, "Loading %s file failed in row %d and column %d, %s", f_name, doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
		return -1;
	} 
	if ( md5_content) {
		FILE *inFile;
		MD5_CTX mdContext;
		int bytes;
		unsigned char data[1024];
		char md_str[64];
		unsigned char md[16];
		
		if ( strlen(md5_content) < 10 )
			return -3;

		TEXTUS_FOPEN(inFile, f_name, "rb");
		if ( !inFile ) 
			return -2;

		MD5_Init (&mdContext);
		while ((bytes = fread (data, 1, 1024, inFile)) != 0)
		MD5_Update (&mdContext, data, bytes);

		MD5_Final (&md[0], &mdContext);
		byte2hex(md, 16, md_str);
		fclose (inFile);
		md_str[32] = 0;
		if ( strncasecmp(md_str, md5_content, 10) != 0) 
		{
			TEXTUS_SPRINTF(err_str, "Loading %s file failed in md5 error", f_name);
			return -3;
		}
		}
	root = doc.RootElement();
	return 0;
};
