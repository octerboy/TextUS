   struct DBFace{
	enum DIRECTION {
		PARA_IN =1, 
		PARA_OUT =2, 
		PARA_INOUT =3, 
		UNKNOWN = 0
	};
	enum PROCTYPE {DBPROC = 0 , QUERY=1, FUNC = 2, FETCH = 3, DML=4, CURSOR=5} ;
	enum DataType {
		UNKNOWN_TYPE = 0,
		BigInt	= 1,
		SmallInt= 2,
		TinyInt	= 3,
		Binary	= 4,
		Boolean	= 5,
		Byte	= 6,
		Char	= 7,
		String	= 8,
		Currency= 9,
		Date	= 10,
		Decimal	= 11,
		Double	= 12,
		Float	= 13,
		GUID	= 14,
		Integer	= 15,
		Long	= 16,
		LongBinary =17,
		Memo	= 18,
		Numeric	= 19,
		Single	= 20,
		Text	= 21,
		Time	= 22,
		TimeStamp =23,
		VarBinary =24,
		DateTime =25,
		Bit =26,
		Year =27,
		LongText =28
	};

	enum WHAT {FIRST, SECOND} ;
	struct Para {
		unsigned int pos;	/* ����λ��, posӦ�õ����������±�ֵ */
		int fld;		/* ��Ӧ���, -1 ָδȷ��, ��������� */
		const char *name;	/* ��������, pos���� */
		const char *charset;	/* �ַ����� */
		int namelen;		/* �������Ƶĳ��� */
		DataType type;		/* �������� */
		DIRECTION inout;	/* �����������, PARA_INΪ����, PARA_OUTΪ���, PARA_INOUT��������� */
		unsigned TEXTUS_LONG outlen;		/* ������� */
		unsigned char scale;		/* ����Decimal�����͵�������� */
		unsigned char precision;
		inline Para () {
			pos = 0;
			fld = 0;
			name = (const char*)0;
			charset = 0;
			namelen = 0;
			type = DBFace::UNKNOWN_TYPE;
			inout = DBFace::UNKNOWN;
			outlen = 0;
			scale = 0;
			precision = 0;
		};
	};
	
	struct Rowset {
		unsigned int para_pos; 	/* ��ѯ�����ʱ, ��������ʼλ�á� 
				�����ڴ洢�������ܷ��ز�ѯ���������� */
		int  trace_field;	/* ���ȡʱ�ĸ���field */
		unsigned int chunk;	/* һ��ȡ������, ����Ϊ1, Ĭ��Ϊ1�� */
		bool useEnd;
		inline Rowset ()
		{
			para_pos = 0;
			trace_field = -1;
			chunk = 1;
			useEnd = false;
		};
	};

 	unsigned int num;		/* �������� */
  	struct Para  *paras;		/* ������������ */

 	unsigned int outNum;		/* ���(PARA_OUT)�������� */
	unsigned TEXTUS_LONG outSize;		/* ���(PARA_OUT)��������ռ� */

	WHAT in, out;		/* ����/�����ָ��packet */
	const char *sentence;	/* ����sql����, �洢�������� */
	size_t sentence_len;
	PROCTYPE pro;		/* �������� */
	const char *id_name;	/* ����, Ҳ��ΪCMD_GET_DBFACE��������,����������ƻ���Ψһ�ĺá�������Դ��XML�ļ��� */
	int offset;		/* ���ƫ���� */
	struct Rowset *rowset;
	
	struct {
		int fldNo;	/* -1:��ʾ�޲ο� */
		unsigned char *content;	/* �ο����� */
		unsigned int len;	/* �ο����� */
	} ref;			/* �ο���, ������packet���� */

	int	cRows_field;	/* ��¼������ŵ�PacketObj���, < 0 ��ʾ����Ҫ */
	int 	cRowsObt_fld;	/* ȡ�˶�����, ���ĸ��� */

	int	errCode_field;	/* �����������ŵ�PacketObj���, < 0 ��ʾ����Ҫ */
	int	errStr_field;	/* ������Ϣ����ŵ�PacketObj���, < 0 ��ʾ����Ҫ */


	inline DBFace() {
		num =0;
		paras = 0;
		outNum = 0;
		outSize = 0;

		sentence = 0;
		offset = 0;
		in = FIRST;
		out = SECOND;
		pro = DBPROC;
		ref.fldNo = -1;	/* ��ʼ�޲ο��� */
		ref.content = 0;
		rowset = 0;

		errCode_field = -1;
		errStr_field = -1;
		cRows_field = -1;
		cRowsObt_fld = -1;
	};

	inline ~DBFace() {
		if (paras )
			delete []paras;

		if ( ref.content)
			delete []ref.content;
	
		if ( rowset )
			delete rowset;
		rowset = 0;
	};

   };
