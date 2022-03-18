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
		unsigned int pos;	/* 参数位置, pos应用等于其数组下标值 */
		int fld;		/* 相应域号, -1 指未确定, 对输出更是 */
		const char *name;	/* 参数名称, pos优先 */
		const char *charset;	/* 字符集名 */
		int namelen;		/* 参数名称的长度 */
		DataType type;		/* 数据类型 */
		DIRECTION inout;	/* 定义参数方向, PARA_IN为输入, PARA_OUT为输出, PARA_INOUT输入与输出 */
		unsigned TEXTUS_LONG outlen;		/* 输出长度 */
		unsigned char scale;		/* 对于Decimal等类型的输出定义 */
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
		unsigned int para_pos; 	/* 查询结果集时, 参数的起始位置。 
				这用于存储过程中能返回查询结果集的情况 */
		int  trace_field;	/* 多次取时的跟踪field */
		unsigned int chunk;	/* 一次取多少行, 至少为1, 默认为1行 */
		bool useEnd;
		inline Rowset ()
		{
			para_pos = 0;
			trace_field = -1;
			chunk = 1;
			useEnd = false;
		};
	};

 	unsigned int num;		/* 参数个数 */
  	struct Para  *paras;		/* 参数定义数组 */

 	unsigned int outNum;		/* 输出(PARA_OUT)参数个数 */
	unsigned TEXTUS_LONG outSize;		/* 输出(PARA_OUT)参数所需空间 */

	WHAT in, out;		/* 输入/输出所指的packet */
	const char *sentence;	/* 描述sql语名, 存储过程名等 */
	size_t sentence_len;
	PROCTYPE pro;		/* 处理类型 */
	const char *id_name;	/* 名称, 也作为CMD_GET_DBFACE的索引名,所以这个名称还是唯一的好。这是来源于XML文件的 */
	int offset;		/* 域号偏移量 */
	struct Rowset *rowset;
	
	struct {
		int fldNo;	/* -1:表示无参考 */
		unsigned char *content;	/* 参考内容 */
		unsigned int len;	/* 参考长度 */
	} ref;			/* 参考域, 对输入packet而言 */

	int	cRows_field;	/* 记录数所存放的PacketObj域号, < 0 表示不需要 */
	int 	cRowsObt_fld;	/* 取了多少行, 在哪个域 */

	int	errCode_field;	/* 错误代码所存放的PacketObj域号, < 0 表示不需要 */
	int	errStr_field;	/* 错误信息所存放的PacketObj域号, < 0 表示不需要 */


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
		ref.fldNo = -1;	/* 初始无参考域 */
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
