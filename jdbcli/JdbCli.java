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
	public Element db_cfg;	
	public JdbCli () { }

	public void ignite (Document doc) 
	{
		String drv_cls;
		db_cfg = doc.getDocumentElement();
		drv_cls = ((Attr) db_cfg.getFirstChild().getAttributes().getNamedItem("driver")).getValue();
		try {   //加载数据库的驱动类   
    			//Class.forName("com.mysql.jdbc.Driver") ;   
    			Class.forName(drv_cls);   
    		} catch(ClassNotFoundException e) {   
    			System.out.println("找不到驱动程序类 ，加载驱动失败！");   
    			e.printStackTrace() ;   
		} 

	}

	public boolean facio(Pius pius ) {
		Pius nps = new Pius();
		switch(pius.ordo) 
		{ 
   			case 1: 
       				System.out.println(1); 
       			break; 	
			default: 
       			break; 
		}
		if ( ps.ordo == Pius.TIMER)
		{
			//System.out.println("timer " + (Integer) ps.indic);
		}
		if ( ps.ordo == Pius.MAIN_PARA)
		{
			String argv[] = (String[])ps.indic;
			System.out.print("main paras ");
			for ( int j = 0 ; j < argv.length; j++ )
			System.out.print(" " + argv[j]);
			System.out.print("\n");
			Pius myps = new Pius();
			//myps.ordo = Pius.DMD_SET_ALARM;
			myps.ordo = Pius.DMD_SET_TIMER;
			//myps.indic = new Integer(1);
			aptus.sponte(myps);
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
