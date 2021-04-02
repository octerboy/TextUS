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
		public int para_pos; 	/* ��ѯ�����ʱ,	��������ʼλ�á� �����ڴ洢�������ܷ��ز�ѯ���������� */
		public int trace_field;/* ���ȡʱ�ĸ���field */
		public int chunk;	/* һ��ȡ������, ����Ϊ1, Ĭ��Ϊ1�� */
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
		public int pos;		/* ����λ��, posӦ�õ����������±�ֵ */
		public int fld;		/* ��Ӧ���, -1 ָδȷ��, ��������� */
		public String name;	/* ��������, pos���� */
		public String charset;	/* �ַ�������,�������UniPac���а��� */
		public int namelen;	/* �������Ƶĳ��� */
		public int data_type;	/* �������� */
		public int inout;	/* �����������, PARA_INΪ����, PARA_OUTΪ���, PARA_INOUT��������� */
		public long outlen;	/* ������� */
		public int scale;	/* ����Decimal�����͵�������� */
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

 	public int num;		/* �������� */
  	public Para[] paras;	/* ������������ */

 	public int outNum;	/* ���(PARA_OUT)�������� */
	public long outSize;	/* ���(PARA_OUT)��������ռ� */

	public int in;
	public int out;		/* ����/�����ָ��packet */
	public String sentence;	/* ����sql����, �洢�������� */
	public int pro;		/* �������� */
	public String id_name;	/* ����, Ҳ��ΪCMD_GET_DBFACE��������,����������ƻ���Ψһ�ĺá�������Դ��XML�ļ��� */
	public int offset;	/* ���ƫ���� */
	public RowSet rowset;
	
	public int ref_fldNo;	/* -1:��ʾ�޲ο� �ο���, ������packet���� */
	public String ref_content;	/* �ο����� */
	public int ref_len;	/* �ο����� */

	public int cRows_field;	/* ��¼������ŵ�PacketObj���, < 0 ��ʾ����Ҫ */
	public int cRowsObt_fld;	/* ȡ�˶�����, ���ĸ��� */
	public int errCode_field;	/* �����������ŵ�PacketObj���, < 0 ��ʾ����Ҫ */
	public int errStr_field;	/* ������Ϣ����ŵ�PacketObj���, < 0 ��ʾ����Ҫ */

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
		ref_fldNo = -1;	/* ��ʼ�޲ο��� */
		ref_content = "";

		errCode_field = -1;
		errStr_field = -1;
		cRows_field = -1;
		cRowsObt_fld = -1;
	};
}

