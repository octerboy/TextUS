/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: JDBC Client
 Build: created by octerboy,  2017/05/30, Panyu
 $Id$
*/

import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.Date;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.*;
import java.math.*;
import textor.jvmport.Amor;
import textor.jvmport.Pius;
import textor.jvmport.TBuffer;
import textor.jvmport.PacketData;
import textor.jvmport.DBFace;
import textor.jvmport.DBFace.Para;
//import java.io.ByteArrayInputStream;
//import java.io.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
//import org.xml.sax.*;

public class JdbCli 
{
	public Amor aptus;
  
	public static final String JETUS_MODTIME = "$Date$";
	public static final String JETUS_BUILDNO =  "$Revision$";

	String connect_url, username, password ;   
	DBFace face;


	PacketData rcv_pac, snd_pac;
	boolean isTalking;

    	Connection connection;
	ResultSet  rSet;
	Pius dopac_ps, end_ps;

	public JdbCli () { 
		isTalking = false;
		face = null;
		connect_url = null;
		password = null;
		username = null;
		rcv_pac = null;
		snd_pac = null;
		rSet = null;
		dopac_ps = new Pius();
		dopac_ps.ordo = Pius.PRO_UNIPAC;
		dopac_ps.subor = 0;
		dopac_ps.indic = null;

		end_ps = new Pius();
		end_ps.ordo = Pius.MULTI_UNIPAC_END;
		end_ps.subor = 0;
		end_ps.indic = null;
	}

	public void ignite (Document doc) 
	{
		Element db_cfg;	
		String drv_str=null;
		db_cfg = doc.getDocumentElement();
		drv_str = db_cfg.getAttribute("driver");
		connect_url = db_cfg.getAttribute("connect");
		username = db_cfg.getAttribute("user");
		password = db_cfg.getAttribute("password");
		if ( drv_str != null && drv_str.length() > 0)
		{
			try {   //加载数据库的驱动类   
    				//Class.forName("com.mysql.jdbc.Driver") ;   
    				Class.forName(drv_str);   
				aptus.log_bug("Class.forName "+ drv_str + " succeeded!");
    			} catch(ClassNotFoundException e) {   
				aptus.log_err("Class.forName " + e.getMessage());
    				e.printStackTrace() ;   
			} 
		} else {
			aptus.log_err("no database driver class");
		}
	}

	public boolean facio(Pius pius ) {
		Pius nps = new Pius();
		if  (pius.ordo == Pius.MAIN_PARA ) {
			aptus.log_bug("facio MAIN_PARA");
		} else if  (pius.ordo == Pius.PRO_UNIPAC ) {
			aptus.log_bug("facio PRO_UNIPAC");
			if ( !isTalking) logon();
			if ( isTalking ) 
			{ 
				handle_pac(false);
				aptus.log_bug("handle end");
			}
		} else if  (pius.ordo == Pius.CMD_DBFETCH ) {
			aptus.log_bug("facio CMD_DBFETCH");
			handle_pac(true);
		} else if  (pius.ordo == Pius.CMD_SET_DBFACE ) {
			aptus.log_bug("facio CMD_SET_DBFACE");
			face = (DBFace ) pius.indic;
		} else if  (pius.ordo == Pius.IGNITE_ALL_READY ) {
			aptus.log_bug("facio IGNITE_ALL_READY");
		} else if  (pius.ordo == Pius.DMD_END_SESSION ) {
			aptus.log_bug("facio DMD_END_SESSION");
			logout();
		} else if  (pius.ordo == Pius.DMD_START_SESSION ) {
			aptus.log_bug("facio DMD_START_SESSION");
			logon();
		} else if  (pius.ordo == Pius.CLONE_ALL_READY ) {
			aptus.log_bug("facio CLONE_ALL_READY");
		} else if  (pius.ordo == Pius.SET_UNIPAC ) {
			aptus.log_bug("facio SET_UNIPAC");
			PacketData[] tbs = (PacketData[])pius.indic;
			rcv_pac = tbs[0];
			snd_pac = tbs[1];
		} else if  (pius.ordo == Pius.CMD_DB_CANCEL ) {
			aptus.log_bug("facio CMD_DB_CANCEL");
		} else {
			aptus.log_bug("JdbCli facio " + pius.ordo);
			return false;
		}
		return true;
	}
	
	public boolean sponte(Pius ps ) {
		return true;
	}
	
	public Object clone() {
		JdbCli child;
		child = new JdbCli();
		child.connect_url = connect_url;
		child.username = username;
		child.password = password; 
		return (Object)child;
	}

	void logon() {
     		try{   
			if ( username != null && password != null )
			{
    				connection = DriverManager.getConnection(connect_url , username , password ) ;   
			} else {
    				connection = DriverManager.getConnection(connect_url) ;   
			}
			isTalking = true;
     		} catch (SQLException se){   
			isTalking = false;
			aptus.log_err(se.getMessage());
    			se.printStackTrace() ;   
		}
	}

	void logout(){
		isTalking = false;
     		try{   
    			connection.close();
     		}catch(SQLException se){   
			aptus.log_err(se.getMessage());
    			se.printStackTrace() ;   
		}
	}

	void stmt_set_input(PreparedStatement p_stmt, int j, DBFace.Para para) throws SQLException {
		int rNo = para.fld + face.offset;
		if ( para.inout == DBFace.PARA_OUT) return; 
		switch ( para.data_type )
		{
		case DBFace.Integer:
			p_stmt.setInt(j, rcv_pac.getInt(rNo));
			break;

		case DBFace.SmallInt:
			p_stmt.setShort(j, rcv_pac.getShort(rNo));
			break;

		case DBFace.TinyInt:
			p_stmt.setByte(j,  rcv_pac.getfld(rNo)[0]);
			break;

		case DBFace.Char:
		case DBFace.String:
		case DBFace.Text:
			p_stmt.setString(j,  rcv_pac.getString(rNo));
			break;

		case DBFace.Decimal:
		case DBFace.Numeric:
			p_stmt.setBigDecimal(j,  rcv_pac.getBigDecimal(rNo));
			break;

		case DBFace.Double:
			p_stmt.setDouble(j,  rcv_pac.getDouble(rNo));
			break;

		case DBFace.Float:
			p_stmt.setFloat(j,  rcv_pac.getFloat(rNo));
			break;

		case DBFace.VarBinary:
		case DBFace.Binary:
		case DBFace.LongBinary:
			p_stmt.setBytes(j,  rcv_pac.getfld(rNo));
			break;

		case DBFace.Long:
			p_stmt.setLong(j,  rcv_pac.getLong(rNo));
			break;

		case DBFace.Date:
			p_stmt.setDate(j,  rcv_pac.getDate(rNo));
			break;

		case DBFace.Time:
			p_stmt.setTime(j,  rcv_pac.getTime(rNo));
			break;

		case DBFace.TimeStamp:
			p_stmt.setTimestamp(j,  rcv_pac.getTimestamp(rNo));
			break;

		case DBFace.Boolean:
			p_stmt.setBoolean(j,  rcv_pac.getBool(rNo));
			break;
/*
		case DBFace.Currency:
			p_stmt.(j,  rcv_pac.get(rNo));
			break;
*/
		default:
			aptus.log_crit("Unknown data type "+para.data_type);
			break;
		}
	}

	void stmt_reg_output(CallableStatement p_stmt, int j, DBFace.Para para)throws SQLException {
		switch ( para.data_type )
		{
		case DBFace.Integer:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.INTEGER);
			else
				p_stmt.registerOutParameter(j, Types.INTEGER);
			break;

		case DBFace.SmallInt:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.SMALLINT);
			else
				p_stmt.registerOutParameter(j, Types.SMALLINT);
			break;

		case DBFace.TinyInt:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.TINYINT);
			else
				p_stmt.registerOutParameter(j, Types.TINYINT);
			break;

		case DBFace.Char:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.CHAR);
			else
				p_stmt.registerOutParameter(j, Types.CHAR);
			break;

		case DBFace.String:
		case DBFace.Text:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.VARCHAR);
			else
				p_stmt.registerOutParameter(j, Types.VARCHAR);
			break;

		case DBFace.Decimal:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.DECIMAL, para.scale);
			else
				p_stmt.registerOutParameter(j, Types.DECIMAL, para.scale);
			break;

		case DBFace.Numeric:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.NUMERIC, para.scale);
			else
				p_stmt.registerOutParameter(j, Types.NUMERIC, para.scale);
			break;

		case DBFace.Double:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.DOUBLE);
			else
				p_stmt.registerOutParameter(j, Types.DOUBLE);
			break;

		case DBFace.Float:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.FLOAT);
			else
				p_stmt.registerOutParameter(j, Types.FLOAT);
			break;

		case DBFace.VarBinary:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.VARBINARY);
			else
				p_stmt.registerOutParameter(j, Types.VARBINARY);
			break;

		case DBFace.Binary:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.BINARY);
			else
				p_stmt.registerOutParameter(j, Types.BINARY);
			break;

		case DBFace.LongBinary:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.LONGVARBINARY);
			else
				p_stmt.registerOutParameter(j, Types.LONGVARBINARY);
			break;

		case DBFace.Long:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.BIGINT);
			else
				p_stmt.registerOutParameter(j, Types.BIGINT);
			break;

		case DBFace.Date:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.DATE);
			else
				p_stmt.registerOutParameter(j, Types.DATE);
			break;

		case DBFace.Time:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.TIME);
			else
				p_stmt.registerOutParameter(j, Types.TIME);
			break;

		case DBFace.TimeStamp:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.TIMESTAMP);
			else
				p_stmt.registerOutParameter(j, Types.TIMESTAMP);
			break;

		case DBFace.Boolean:
			if ( para.namelen > 0 ) 
				p_stmt.registerOutParameter(para.name, Types.BOOLEAN);
			else
				p_stmt.registerOutParameter(j, Types.BOOLEAN);
			break;
/*
		case DBFace.Currency:
			break;
*/
		default:
			aptus.log_crit("Unknown data type "+para.data_type);
			break;
		}
	}

	void proc_get(int j, DBFace.Para para, CallableStatement stmt)throws SQLException 
	{
		if ( para.inout == DBFace.PARA_IN ) return ;
		switch ( para.data_type )
		{
		case DBFace.Integer:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getInt(para.name));
			else
				snd_pac.input(para.fld, stmt.getInt(j));
			break;

		case DBFace.SmallInt:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getShort(para.name));
			else
				snd_pac.input(para.fld, stmt.getShort(j));
			break;

		case DBFace.TinyInt:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getByte(para.name));
			else
				snd_pac.input(para.fld, stmt.getByte(j));
			break;

		case DBFace.String:
		case DBFace.Text:
		case DBFace.Char:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getString(para.name));
			else
				snd_pac.input(para.fld, stmt.getString(j));
			break;

		case DBFace.Decimal:
		case DBFace.Numeric:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getBigDecimal(para.name));
			else
				snd_pac.input(para.fld, stmt.getBigDecimal(j));
			break;

		case DBFace.Double:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getDouble(para.name));
			else
				snd_pac.input(para.fld, stmt.getDouble(j));
			break;

		case DBFace.Float:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getFloat(para.name));
			else
				snd_pac.input(para.fld, stmt.getFloat(j));
			break;

		case DBFace.VarBinary:
		case DBFace.Binary:
		case DBFace.LongBinary:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getBytes(para.name));
			else
				snd_pac.input(para.fld, stmt.getBytes(j));
			break;

		case DBFace.Long:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getLong(para.name));
			else
				snd_pac.input(para.fld, stmt.getLong(j));
			break;

		case DBFace.Date:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getDate(para.name));
			else
				snd_pac.input(para.fld, stmt.getDate(j));
			break;

		case DBFace.Time:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getTime(para.name));
			else
				snd_pac.input(para.fld, stmt.getTime(j));
			break;

		case DBFace.TimeStamp:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getTimestamp(para.name));
			else
				snd_pac.input(para.fld, stmt.getTimestamp(j));
			break;

		case DBFace.Boolean:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, stmt.getBoolean(para.name));
			else
				snd_pac.input(para.fld, stmt.getBoolean(j));
			break;
/*
		case DBFace.Currency:
			break;
*/
		default:
			aptus.log_crit("Unknown data type "+para.data_type);
			break;
		}
	}
	
	void rs_get(int j, DBFace.Para para)throws SQLException 
	{
		if ( para.inout == DBFace.PARA_IN ) return ;
		switch ( para.data_type )
		{
		case DBFace.Integer:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getInt(para.name));
			else
				snd_pac.input(para.fld, rSet.getInt(j));
			break;

		case DBFace.SmallInt:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getShort(para.name));
			else
				snd_pac.input(para.fld, rSet.getShort(j));
			break;

		case DBFace.TinyInt:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getByte(para.name));
			else
				snd_pac.input(para.fld, rSet.getByte(j));
			break;

		case DBFace.String:
		case DBFace.Text:
		case DBFace.Char:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getString(para.name));
			else
				snd_pac.input(para.fld, rSet.getString(j));
			break;

		case DBFace.Decimal:
		case DBFace.Numeric:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getBigDecimal(para.name));
			else
				snd_pac.input(para.fld, rSet.getBigDecimal(j));
			break;

		case DBFace.Double:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getDouble(para.name));
			else
				snd_pac.input(para.fld, rSet.getDouble(j));
			break;

		case DBFace.Float:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getFloat(para.name));
			else
				snd_pac.input(para.fld, rSet.getFloat(j));
			break;

		case DBFace.VarBinary:
		case DBFace.Binary:
		case DBFace.LongBinary:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getBytes(para.name));
			else
				snd_pac.input(para.fld, rSet.getBytes(j));
			break;

		case DBFace.Long:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getLong(para.name));
			else
				snd_pac.input(para.fld, rSet.getLong(j));
			break;

		case DBFace.Date:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getDate(para.name));
			else
				snd_pac.input(para.fld, rSet.getDate(j));
			break;

		case DBFace.Time:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getTime(para.name));
			else
				snd_pac.input(para.fld, rSet.getTime(j));
			break;

		case DBFace.TimeStamp:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getTimestamp(para.name));
			else
				snd_pac.input(para.fld, rSet.getTimestamp(j));
			break;

		case DBFace.Boolean:
			if ( para.namelen > 0 ) 
				snd_pac.input(para.fld, rSet.getBoolean(para.name));
			else
				snd_pac.input(para.fld, rSet.getBoolean(j));
			break;
/*
		case DBFace.Currency:
			break;
*/
		default:
			aptus.log_crit("Unknown data type "+para.data_type);
			break;
		}
	}
	
	void proc_call (String n_sentence ) {
		int i,rows;
		CallableStatement c_stmt;
		try {
			c_stmt = connection.prepareCall(n_sentence);
			set_para_inout(c_stmt);
			c_stmt.execute();
			for ( i = 0 ; i <  face.num; i++ )
			{
				proc_get(i+1,  face.paras[i], c_stmt);
			}
			c_stmt.close();
			aptus.sponte(dopac_ps);
    		} catch(SQLException se) { 
			aptus.log_err(se.getMessage());
			snd_pac.input(face.errCode_field, se.getErrorCode());
			snd_pac.input(face.errStr_field, se.toString()); 
			aptus.sponte(dopac_ps);
		}
	}

	void dml_pro (String n_sentence ) {
		int i,rows;
		PreparedStatement p_stmt;
		try {
			p_stmt = connection.prepareStatement(n_sentence);
			/* 输入值的设定 */
			for ( i = 0 ; i <  face.num; i++ )
			{
				stmt_set_input(p_stmt, i+1,  face.paras[i]);
			}
			rows = p_stmt.executeUpdate();
			snd_pac.input(face.cRows_field, rows); 
			p_stmt.close();
			aptus.sponte(dopac_ps);
    		} catch(SQLException se) { 
			aptus.log_err(se.getMessage());
			snd_pac.input(face.errCode_field, se.getErrorCode());
			snd_pac.input(face.errStr_field, se.toString()); 
			aptus.sponte(dopac_ps);
		}
	}

	void set_para_inout (CallableStatement c_stmt)throws SQLException {
		DBFace.Para para;
		int rNo;
		int i;
		for ( i = 0 ; i <  face.num; i++ )
		{
			para = face.paras[i]; 
			rNo = para.fld + face.offset;
			if ( para.inout == DBFace.PARA_IN || para.inout == DBFace.PARA_INOUT)
			{
				stmt_set_input(c_stmt, i+1, para);
			}
			if ( para.inout == DBFace.PARA_OUT || para.inout == DBFace.PARA_INOUT)
			{
				stmt_reg_output(c_stmt, i+1, para);
			}
		}
	}

	String obtain_call_str (boolean isFun) {
		String n_sentence;
		int i;
		if ( isFun ) { 
			n_sentence = "{?=";
		} else {
			n_sentence = "{";
		}
		n_sentence += " call " +  face.sentence + " (" ;
		for ( i = 1 ; i <  face.num-1; i++ )
		{
			n_sentence += "?,";
		}
		if ( face.num > 1)
		{
			n_sentence += "?";
		}
		n_sentence += ")}";
		aptus.log_bug("statement is " + n_sentence);
		return n_sentence;
	}

	void fetch_result(boolean isFirst) {
		int i;
		int f_num;
		f_num = face.rowset.chunk ;
		try {
			if ( isFirst && face.cRows_field >=0 )
			{
				rSet.last();
				snd_pac.input(face.cRows_field, rSet.getRow()); 
				rSet.beforeFirst();
			}
			while (rSet.next() && f_num >0) 
			{
				for ( i = 0 ; i <  face.num; i++)
				{
					rs_get(i+1,  face.paras[i]);
				}
				aptus.sponte(dopac_ps);
				f_num--;
			}
			snd_pac.input(face.cRowsObt_fld, face.rowset.chunk - f_num); 
			if ( f_num == face.rowset.chunk )	//没有一行
				aptus.sponte(dopac_ps);
			else if (face.rowset.useEnd )
				aptus.sponte(end_ps);
    		} catch(SQLException se) { 
			aptus.log_err(se.getMessage());
			snd_pac.input(face.errCode_field, se.getErrorCode());
			snd_pac.input(face.errStr_field, se.toString()); 
			aptus.sponte(dopac_ps);
		}
	}

	boolean query_pro (String n_sentence ) {
		int i;
		PreparedStatement p_stmt;
		boolean ret = false;
		try {
			p_stmt = connection.prepareStatement(n_sentence);
			p_stmt.setFetchSize(face.rowset.chunk);	//这个的确是这样
			/* 输入值的设定 */
			for ( i = 0 ; i <  face.num; i++ )
			{
				stmt_set_input(p_stmt, i+1,  face.paras[i]);
			}
			rSet = p_stmt.executeQuery();
			ret = true;
			p_stmt.close();
    		} catch(SQLException se) { 
			aptus.log_err(se.getMessage());
			snd_pac.input(face.errCode_field, se.getErrorCode());
			snd_pac.input(face.errStr_field, se.toString()); 
			aptus.sponte(dopac_ps);
		}
		return ret;
	}

	void handle_pac(boolean isDBFetch) 
	{
		DBFace.Para para;
		int i;
		String n_sentence;
		snd_pac.input(face.errCode_field, 0); //首先假定结果OK，把值设到返回域中

		if ( isDBFetch)
		{
			fetch_result(false) ;	//取结果集的, 不是第一次
			return ;
		}

		//凡是最近QUERY的, 要cancel,to do....
		try {
			if ( rSet != null )
			{
				rSet.close();
				rSet = null;
			}
    		} catch(SQLException se) { 
			aptus.log_err(se.getMessage());
		}

		switch ( face.pro )
		{
			case DBFace.DBPROC:
				n_sentence = obtain_call_str(false);
				proc_call (n_sentence);
				break;

			case DBFace.FUNC:
				aptus.log_bug("handle DBPROC/FUNC " + face.sentence +" para num "+face.num);
				n_sentence = obtain_call_str(true);
				proc_call (n_sentence);
				break;

			case DBFace.DML:
				aptus.log_bug("handle DML (\"" + face.sentence +" \")" + " param num " + face.num);
				dml_pro(face.sentence);
				break;

			case DBFace.QUERY:
				aptus.log_bug("handle QUERY (\"" + face.sentence +" \")" + " param num " + face.num);
				if ( query_pro(face.sentence) )
					fetch_result(true);	//第一次取结果集
				break;
				
			case DBFace.CURSOR:
				aptus.log_bug("handle CURSOR");
				break;
			default:
				break;
		}
	}
}
