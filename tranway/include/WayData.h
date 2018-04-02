/* ����ּ��֣�INS_Normal����׼��INS_Abort����ֹ */
struct DyVarBase { /* ��̬������ �������Ա��ĵ� */
	int index;	//����, Ҳ�����±�ֵ
	unsigned char *val_p;	//p��ʱָ�������val�� ��Ҳ����ָ�����롢�������.c_len���ǳ��ȡ�
	size_t c_len;
	TBuffer *con;		//�ⲿ�ɷ���, ͨ��ָ��nal, Globalʱָ��G_CFG�еı���
	TBuffer nal;		//�ڲ�����, 
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
	unsigned char *con;  /* ָ�� cmd_buf�е�ĳ���� */
	size_t len;
	int dy_pos;
	DyList () {
		con  =0;
		len = 0;
		dy_pos = -1;
	};
};

struct CmdSnd {
	int fld_no;	//���͵����
	void *ext_fld;	//��չ��, �������XML��JSON��
	struct DyList *dy_list;
	int dy_num;
	unsigned char *cmd_buf;
	size_t cmd_len;
	const char *tag;	/*  ����"component"���������ݣ�ָ���������е�Ԫ�� */

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
	int fld_no;	//���յ����, �ж�����ն��壬ÿ��ֻ����һ������.���ԣ��ж�����壬ָ��ͬһ����
	void *ext_fld;	//��չ��, �������XML��JSON��
	int dyna_pos;	//��̬����λ��, -1��ʾ��̬
	size_t start;
	size_t length;
	unsigned char *src_con;	//��ֵ�������Զ����ļ��е�����
	size_t src_len;
	unsigned char *must_con;
	size_t must_len;
	const char *err_code; //pac_ele�����, �������滻, def_ele����ģ������κδ��������򲻷���Ҫ�����ô˴����롣
	bool err_disp_hex;

	const char *tag;//���磺 reply, sw
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
	const char *ins_tag;//���磺 CMac��
	int up_subor;	//ָʾָ����͸���һ���¼�ģ��

	struct CmdSnd *snd_lst;
	int snd_num;

	struct CmdRcv *rcv_lst;
	int rcv_num;

	const char *err_code; //�ȴ�map�ļ��еõ�, ���Ǳ���, ��תΪʵ��ֵ, ��pac_def�еõ���, �����κδ���
	bool counted;		//�Ƿ����
	bool isFunction;	//�Ƿ�Ϊ������ʽ
	const char *log_str;	//����ֱ�Ӵ��ⲿ�����ļ��õ������ݣ������κδ���

	void *ext_ins;	//�������Ķ��壬��һ������
	InsData() {
		log_str = 0;
		up_subor = -1;
		snd_lst = 0;
		snd_num = 0;
		rcv_lst = 0;
		rcv_num = 0;

		err_code = 0;
		counted = false;	/* ������ָ�������� */
		isFunction = false;
		ext_ins = 0;
	};
};

struct InsReply {	//Ans_InsWay��indic
	char *err_str;
	const char *err_code;
};

struct InsWay {		//Pro_InsWay��indic
	struct InsData *dat;
	struct DyVarBase *snap;     //��ʱ�ı���, ����
	int snap_num;
	struct InsReply *reply;
};

struct FlowStr {		//Pro_TranWay��indic
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
