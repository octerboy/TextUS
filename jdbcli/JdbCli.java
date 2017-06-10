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

public class JdbCli {
	public Amor aptus;
  
	public static final String JETUS_MODTIME = "$Date$";
	public static final String JETUS_BUILDNO =  "$Revision$";

	Element db_cfg;	
	String connect_url;	
     	String username ;   
     	String password ;   

	PacketData rcv_pac;
	PacketData snd_pac;

	public JdbCli () { }

	public void ignite (Document doc) 
	{
		String drv_cls;
		NamedNodeMap atts;
		Attr attr;
		db_cfg = doc.getDocumentElement();
		drv_cls = ((Attr) db_cfg.getAttributes().getNamedItem("driver")).getValue();
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
			if ( drv_cls != null)
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
       				break; 	

   			case Pius.CMD_DBFETCH: 
				aptus.log_bug("facio CMD_DBFETCH");
       				break; 	

   			case Pius.CMD_SET_DBFACE: 
				aptus.log_bug("facio CMD_SET_DBFACE");
       				break; 	

   			case Pius.IGNITE_ALL_READY: 
				aptus.log_bug("facio IGNITE_ALL_READY");
       				break; 	

   			case Pius.DMD_END_SESSION: 
				aptus.log_bug("facio DMD_END_SESSION");
       				break; 	

   			case Pius.DMD_START_SESSION: 
				aptus.log_bug("facio DMD_START_SESSION");
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
       				break; 
		}
		aptus.log_bug("ordo " + ps.ordo + " count " + count + " I 对象 am "+ ((Text) im.getFirstChild()).getData());
		try {
			if ( ps.ordo == Pius.SET_TBUF )
			{	/* SET_TBUF */
			System.out.println("测试数据, 来自Java程序");
/*
				TiXML ti = new TiXML();
				ti.alloc();
				ti.putDocument(Hell.parseXmlFile("demo.xml", false));
				Document doc = ti.getDocument();
		try{ 
			TransformerFactory tFactory = TransformerFactory.newInstance(); 
			Transformer transformer = tFactory.newTransformer(); 

			DOMSource source = new DOMSource(doc); 
			ByteArrayOutputStream outs = new ByteArrayOutputStream();
			StreamResult result = new StreamResult(outs); 
			
			Properties properties = transformer.getOutputProperties(); 
			properties.setProperty(OutputKeys.ENCODING,"GB2312"); 
			transformer.setOutputProperties(properties); 
			transformer.transform(source,result); 
			
			System.out.println(outs.toString());
		} catch(Exception e){
			System.out.println("error4 " + e.getMessage());
		} 
*/
				TBuffer[] tbs = (TBuffer[])ps.indic;
				tbs[0].grant(100);
				tbs[1].grant(100);
				aptus.facio(ps);
			}

			if ( ps.ordo == Pius.SET_UNIPAC )
			{	/* SET_UNIPAC */
				PacketData[] tbs = (PacketData[])ps.indic;
				tbs[0].grant(100);
				tbs[1].grant(100);
				aptus.facio(ps);
			}
			if ( ps.ordo != (2000 | Pius.JAVA_NOTITIA_DOM) )
			{
			//System.out.println("in java pius.ordo "+ ps.ordo);       
			//nps.ordo = 2000 | 0x00100000;
			//nps.indic = new String("完全是是Java的String对象");
			//aptus.facio(nps);
			} else {
				//String str = (String)ps.indic;
				//System.out.println("Java String is "+ str);       
			}
		} catch (Exception e) {
                        System.err.println("excption: " + e.getMessage());
                }

		return true;
	}
	
	public boolean sponte(Pius ps ) {
		return true;
	}
	
	public Object clone() {
		JTest child;
		child = new JTest();
		count = count+1;
		child.im = im;
		child.count = count;
		return (Object)child;
	}

String url = "jdbc:mysql://localhost:3306/test" ;    
     String username = "root" ;   
     String password = "root" ;   
     try{   
    Connection con =    
             DriverManager.getConnection(url , username , password ) ;   
     }catch(SQLException se){   
    System.out.println("数据库连接失败！");   
    se.printStackTrace() ;   
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
