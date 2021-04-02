/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: DBFace Class for Java
 Build: Created by octerboy 2017/06/03 Panyu
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

package textor.jvmport;
import java.lang.Object;

public class DBFace {
	public class RowSet {
		public int para_pos; 	/* 查询结果集时,	参数的起始位置。 这用于存储过程中能返回查询结果集的情况 */
		public int trace_field;/* 多次取时的跟踪field */
		public int chunk;	/* 一次取多少行, 至少为1, 默认为1行 */
		public boolean useEnd;
		public RowSet ()
		{
			para_pos = 0;
			trace_field = -1;
			chunk = 1;
			useEnd = false;
		};
	}
	
	public class Para {
		public int pos;		/* 参数位置, pos应用等于其数组下标值 */
		public int fld;		/* 相应域号, -1 指未确定, 对输出更是 */
		public String name;	/* 参数名称, pos优先 */
		public String charset;	/* 字符集名称,这对来自UniPac的有帮助 */
		public int namelen;	/* 参数名称的长度 */
		public int data_type;	/* 数据类型 */
		public int inout;	/* 定义参数方向, PARA_IN为输入, PARA_OUT为输出, PARA_INOUT输入与输出 */
		public long outlen;	/* 输出长度 */
		public int scale;	/* 对于Decimal等类型的输出定义 */
		public int precision;
		public Para () {
			pos = 0;
			fld = 0;
			name = "";
			charset= "";
			namelen = 0;
			data_type = 0;
			inout = 0;
			outlen = 0;
			scale = 0;
			precision = 0;
		};
	}

	public static final int PARA_IN =31;
	public static final int PARA_OUT =32;
	public static final int PARA_INOUT =33;
	public static final int PARA_UNKNOWN =34;
	public static final int DBPROC = 40;
	public static final int QUERY=41 ;
	public static final int FUNC = 42;
	public static final int FETCH = 43;
	public static final int DML = 44;	
	public static final int CURSOR= 45;
	public static final int BigInt	= 1;
	public static final int SmallInt= 2;
	public static final int TinyInt	= 3;
	public static final int Binary	= 4;
	public static final int Boolean	= 5;
	public static final int Byte	= 6;
	public static final int Char	= 7;
	public static final int String	= 8;
	public static final int	Currency= 9;
	public static final int	Date	= 10;
	public static final int	Decimal	= 11;
	public static final int	Double	= 12;
	public static final int	Float	= 13;
	public static final int	GUID	= 14;
	public static final int	Integer	= 15;
	public static final int	Long	= 16;
	public static final int	LongBinary =17;
	public static final int	Memo	= 18;
	public static final int	Numeric	= 19;
	public static final int	Single	= 20;
	public static final int	Text	= 21;
	public static final int	Time	= 22;
	public static final int	TimeStamp =23;
	public static final int	VarBinary =24;
	public static final int	UNKNOWN_TYPE =25;

	public static final int	FIRST =101;
	public static final int	SECOND =102;	

 	public int num;		/* 参数个数 */
  	public Para[] paras;	/* 参数定义数组 */

 	public int outNum;	/* 输出(PARA_OUT)参数个数 */
	public long outSize;	/* 输出(PARA_OUT)参数所需空间 */

	public int in;
	public int out;		/* 输入/输出所指的packet */
	public String sentence;	/* 描述sql语名, 存储过程名等 */
	public int pro;		/* 处理类型 */
	public String id_name;	/* 名称, 也作为CMD_GET_DBFACE的索引名,所以这个名称还是唯一的好。这是来源于XML文件的 */
	public int offset;	/* 域号偏移量 */
	public RowSet rowset;
	
	public int ref_fldNo;	/* -1:表示无参考 参考域, 对输入packet而言 */
	public String ref_content;	/* 参考内容 */
	public int ref_len;	/* 参考长度 */

	public int cRows_field;	/* 记录数所存放的PacketObj域号, < 0 表示不需要 */
	public int cRowsObt_fld;	/* 取了多少行, 在哪个域 */
	public int errCode_field;	/* 错误代码所存放的PacketObj域号, < 0 表示不需要 */
	public int errStr_field;	/* 错误信息所存放的PacketObj域号, < 0 表示不需要 */

	public DBFace() 
	{
		num =0;
		outNum = 0;
		outSize = 0;

		sentence = "";
		id_name = "";
		offset = 0;
		in = FIRST;
		out = SECOND;
		pro = DBPROC;
		ref_fldNo = -1;	/* 初始无参考域 */
		ref_content = "";

		errCode_field = -1;
		errStr_field = -1;
		cRows_field = -1;
		cRowsObt_fld = -1;
	};
}

