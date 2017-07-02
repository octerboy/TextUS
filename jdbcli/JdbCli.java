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
import java.sql.Timestamp
import java.math.*;
import jetus.jvmport.Amor;
import jetus.jvmport.Pius;
import jetus.jvmport.TBuffer;
import jetus.jvmport.PacketData;
import jetus.jvmport.DBFace;

import java.io.ByteArrayInputStream;
import java.io.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.*;
    
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.*;
import java.util.Properties;

public class JdbCli 
{
	public Amor aptus;
  
	public static final String JETUS_MODTIME = "$Date$";
	public static final String JETUS_BUILDNO =  "$Revision$";

	Element db_cfg;	
	String connect_url;	
     	String username ;   
     	String password ;   
	DBFace face;

	bool shared_session;

	PacketData rcv_pac;
	PacketData snd_pac;
	boolean isTalking;

    	Connection connection;
	ResultSet  rSet;

	public JdbCli () { 
		isTalking = false;
		face = null;
		connect_url = null;
		password = null;
		username = null;
		rcv_pac = null;
		snd_pac = null;
		shared_session = false;
		rSet = null;
	}

	public void ignite (Document doc) 
	{
		String drv_str;
		NamedNodeMap atts;
		Attr attr;
		db_cfg = doc.getDocumentElement();
		drv_str = ((Attr) db_cfg.getAttributes().getNamedItem("driver")).getValue();
		try {   //加载数据库的驱动类   
    			//Class.forName("com.mysql.jdbc.Driver") ;   
			atts = db_cfg.getAttributes();
			if ( atts != null ) 
			{
				attr = atts.getNamedItem("connect");
				if ( attr != null )
					connect_url = attr.getValue();
				attr = atts.getNamedItem("user");
				if ( attr != null )
					username = attr.getValue();
				attr = atts.getNamedItem("password");
				if ( attr != null )
					password = attr.getValue();
				attr = atts.getNamedItem("driver");
				if ( attr != null )
					drv_cls = attr.getValue();
			}
			if ( drv_str != null)
			{
    				Class.forName(drv_cls);   
			} else {
				aptus.log_err("no database driver class");
			}
    		} catch(ClassNotFoundException e) {   
			aptus.log_err("Can not found class of " + drv_cls);
    			e.printStackTrace() ;   
		} 

	}

	public boolean facio(Pius pius ) {
		Pius nps = new Pius();
		switch(pius.ordo) 
		{ 
   			case Pius.MAIN_PARA: 
				aptus.log_bug("facio MAIN_PARA");
       				break; 	

   			case Pius.PRO_UNIPAC: 
				aptus.log_bug("facio PRO_UNIPAC");
				if ( !isTalking) logon();
				if ( isTalking)) 
				{ 
					handle_pac();
					aptus.log_bug("handle end");
				}
       				break; 	

   			case Pius.CMD_DBFETCH: 
				aptus.log_bug("facio CMD_DBFETCH");
       				break; 	

   			case Pius.CMD_SET_DBFACE: 
				aptus.log_bug("facio CMD_SET_DBFACE");
				face = (DBFace ) pius.indic;
       				break; 	

   			case Pius.IGNITE_ALL_READY: 
				aptus.log_bug("facio IGNITE_ALL_READY");
       				break; 	

   			case Pius.DMD_END_SESSION: 
				aptus.log_bug("facio DMD_END_SESSION");
				logout();
       				break; 	

   			case Pius.DMD_START_SESSION: 
				aptus.log_bug("facio DMD_START_SESSION");
				logon();
       				break; 	

   			case Pius.CLONE_ALL_READY: 
				aptus.log_bug("facio CLONE_ALL_READY");
       				break; 	

   			case Pius.SET_UNIPAC: 
				aptus.log_bug("facio SET_UNIPAC");
				PacketData[] tbs = (PacketData[])ps.indic;
				rcv_pac = tbs[0];
				snd_pac = tbs[1];
       				break; 	

   			case Pius.CMD_DB_CANCEL: 
				aptus.log_bug("facio CMD_DB_CANCEL");
       				break; 	

			default: 
				aptus.log_bug("facio " + pius.ordo);
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
     		}catch(SQLException se){   
			isTalking = false;
    			System.out.println("数据库连接失败！");   
    			se.printStackTrace() ;   
		}
	}

	void logout(){
		isTalking = false;
     		try{   
    			connection.close();
     		}catch(SQLException se){   
    			System.out.println("数据库关闭失败！");   
    			se.printStackTrace() ;   
		}
	}

	void stmt_set_input(PreparedStatement p_stmt, int j, DBFace:Para para) throws SQLException {
		int rNo = para.fld + face.offset;
		if ( para.inout == DBFace.PARA_OUT) return; 
		switch ( para.type )
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
			WLOG(CRIT, "Unknown data type %d!", para.type);
			break;
		}
	}

	void stmt_reg_output(CallableStatement p_stmt, int j, DBFace:Para para)throws SQLException {
		switch ( para.type )
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
			WLOG(CRIT, "Unknown data type %d!", para.type);
			break;
		}
	}
	
	void rs_get(int j, DBFace:Para para)throws SQLException {
		switch ( para.type )
		{
		case DBFace.Integer:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getInt(para.name));
			else
				snd_pac.intput(para.fld, rSet.getInt(j));
			break;

		case DBFace.SmallInt:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getShort(para.name));
			else
				snd_pac.intput(para.fld, rSet.getShort(j));
			break;

		case DBFace.TinyInt:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getByte(para.name));
			else
				snd_pac.intput(para.fld, rSet.getByte(j));
			break;

		case DBFace.String:
		case DBFace.Text:
		case DBFace.Char:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getString(para.name));
			else
				snd_pac.intput(para.fld, rSet.getString(j));
			break;

		case DBFace.Decimal:
		case DBFace.Numeric:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.geBigDecimalt(para.name));
			else
				snd_pac.intput(para.fld, rSet.geBigDecimalt(j));
			break;

		case DBFace.Double:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getDouble(para.name));
			else
				snd_pac.intput(para.fld, rSet.getDouble(j));
			break;

		case DBFace.Float:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getFloat(para.name));
			else
				snd_pac.intput(para.fld, rSet.getFloat(j));
			break;

		case DBFace.VarBinary:
		case DBFace.Binary:
		case DBFace.LongBinary:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getBytes(para.name));
			else
				snd_pac.intput(para.fld, rSet.getBytes(j));
			break;

		case DBFace.Long:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getLong(para.name));
			else
				snd_pac.intput(para.fld, rSet.getLong(j));
			break;

		case DBFace.Date:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getDate(para.name));
			else
				snd_pac.intput(para.fld, rSet.getDate(j));
			break;

		case DBFace.Time:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getTime(para.name));
			else
				snd_pac.intput(para.fld, rSet.getTime(j));
			break;

		case DBFace.TimeStamp:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getTimeStamp(para.name));
			else
				snd_pac.intput(para.fld, rSet.getTimeStamp(j));
			break;

		case DBFace.Boolean:
			if ( para.namelen > 0 ) 
				snd_pac.intput(para.fld, rSet.getBoolean(para.name));
			else
				snd_pac.intput(para.fld, rSet.getBoolean(j));
			break;
/*
		case DBFace.Currency:
			break;
*/
		default:
			WLOG(CRIT, "Unknown data type %d!", para.type);
			break;
		}
	}
	
	void set_para_inout (CallableStatement c_stmt)throws SQLException {
		DBFace:Para para;
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

	void handle_pac(boolean isDBFetch) throws SQLException 
	{
		PreparedStatement p_stmt;
		CallableStatement c_stmt;
		DBFace:Para para;
		int i;
		snd_pac.input(face.errCode_field, 0); //首先假定结果OK，把值设到返回域中

		if ( isDBFetch)
		{
			Fetch_data ( CS_ROW_RESULT ) ;	//取结果集的
			return ;
		}

		//凡是最近QUERY的, 要cancel,to do....

		switch ( face.pro )
		{
			case DBFace.DBPROC:
				n_sentence = obtain_call_str(false);
				c_stmt = connection.prepareCall(n_sentence);
				set_para_inout(c_stmt);
				c_stmt.execute();
				break;

			case DBFace.FUNC:
				aptus.log_bug("handle DBPROC/FUNC " + face.sentence +" para num "+face.num);
				n_sentence = obtain_call_str(true);
				c_stmt = connection.prepareCall(n_sentence);
				set_para_inout(c_stmt);
				c_stmt.execute();
				break;

			case DBFace.DML:
				aptus.log_bug("handle DML (\"" + face.sentence +" \")" + " param num " + face.num);
				p_stmt = connection.prepareStatement(face.sentence);
				/* 输入值的设定 */
				for ( i = 0 ; i <  face.num; i++ )
				{
					stmt_set_input(p_stmt, i+1,  face.paras[i]);
				}
				p_stmt.executeUpdate();
				break;

			case DBFace.QUERY:
				aptus.log_bug("handle QUERY (\"" + face.sentence +" \")" + " param num " + face.num);
				p_stmt = connection.prepareStatement(face.sentence);
				/* 输入值的设定 */
				for ( i = 0 ; i <  face.num; i++ )
				{
					stmt_set_input(p_stmt, i+1,  face.paras[i]);
				}
				rSet = p_stmt.executeQuery();
				while (rSet.next()) 
				{
					for ( i = 0 ; i <  face.num; i++ )
					{
						rs_get(i+1,  face.paras[i]);
					}
					/* .. */
				}
				
				break;
				
			case DBFace.CURSOR:
				aptus.log_bug("handle CURSOR");
				break;
		}
	}

	void fetch_data ()
	{
	}

	void handle_pac()
	{
		handle_pac(false);

	}

String url = "jdbc:mysql://localhost:3306/test" ;    
     String username = "root" ;   
     String password = "root" ;   
     }   


       Statement stmt = con.createStatement() ;   
       PreparedStatement pstmt = con.prepareStatement(sql) ;   
       CallableStatement cstmt =    
                            con.prepareCall("{CALL demoSp(? , ?)}") ;   



          ResultSet rs = stmt.executeQuery("SELECT * FROM ...") ;   
    int rows = stmt.executeUpdate("INSERT INTO ...") ;   
    boolean flag = stmt.execute(String sql) ;  


    while(rs.next()){   
         String name = rs.getString("name") ;   
    String pass = rs.getString(1) ; // 此方法比较高效   
     }   



    if(rs != null){   // 关闭记录集   
        try{   
            rs.close() ;   
        }catch(SQLException e){   
            e.printStackTrace() ;   
        }   
          }   
          if(stmt != null){   // 关闭声明   
        try{   
            stmt.close() ;   
        }catch(SQLException e){   
            e.printStackTrace() ;   
        }   
          }   
          if(conn != null){  // 关闭连接对象   
         try{   
            conn.close() ;   
         }catch(SQLException e){   
            e.printStackTrace() ;   
         }   
          }  

}
