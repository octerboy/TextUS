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
import jetus.jvmport.Amor;
import jetus.jvmport.Pius;
import jetus.jvmport.TBuffer;
import jetus.jvmport.PacketData;

import java.io.ByteArrayInputStream;
import java.io.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.*;
    
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.*;
import java.util.Properties;
import textor.jvmport.DBFace;

public class JdbCli {
	public Amor aptus;
  
	public static final String JETUS_MODTIME = "$Date$";
	public static final String JETUS_BUILDNO =  "$Revision$";

	Element db_cfg;	
	String connect_url;	
     	String username ;   
     	String password ;   
	DBFace face;

    	Connection connection;
	bool shared_session;

	PacketData rcv_pac;
	PacketData snd_pac;
	boolean isTalking;

	public JdbCli () { 
		isTalking = false;
		face = null;
		connect_url = null;
		password = null;
		username = null;
		rcv_pac = null;
		snd_pac = null;
		shared_session = null;
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
