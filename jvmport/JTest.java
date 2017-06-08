import textor.jvmport.Amor;
import textor.jvmport.Pius;
import textor.jvmport.TBuffer;
import textor.jvmport.PacketData;
import textor.jvmport.TiXML;
import textor.jvmport.DBFace;
import java.io.ByteArrayInputStream;
import java.io.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import org.xml.sax.*;
    
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.*;
import java.util.Properties;
//import org.apache.jasper.compiler.*;
//import org.apache.jasper.*;
//import javax.servlet.jsp.*;

public class JTest {
	public Amor aptus = new Amor("ISO8859-1");
	public Element im;
	public int count;
	public DBFace face;

	public static final String JETUS_MODTIME = "$Date: 07-11-16 8:39 $";
	public static final String JETUS_BUILDNO =  "$Revision: 3 $";

	public JTest () { count = 0;}

	public void ignite (Document doc) {
		String args[]={"-uriroot",  "/tmp/jsp", "-source","/tmp/jsp", "-d", "/tmp/let", "m.jsp"};
		//PrintStream log = new PrintStream(System.out, true);
		//JspC agc=new JspC();
		//JspC.main(args);
		//System.out.println("JspC path : " + agc.getClassPath());
		im = doc.getDocumentElement();
		try {
		
		PrintStream log = new PrintStream(System.out, true, "ISO8859-1");
		Text txt = (Text) im.getFirstChild();
		face = new DBFace();
		System.out.println("my text : " + txt.getData() );
		log.println("my text : " + txt.getData() );
		
		} catch ( Exception e) {
		}
		return;
	}

	public boolean facio(Pius ps ) {
		NamedNodeMap atts;
		Pius nps = new Pius();
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
		aptus.log_bug("ordo " + ps.ordo + " count " + count + " I 对象 am.. "+ ((Text) im.getFirstChild()).getData());
		aptus.log_bug("ordo " + ps.ordo + " count " + count + " I 对象 am "+ ((Attr) im.getAttributes().getNamedItem("ver")).getValue());
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

        public Document parseXmlFile( String xmlstr, boolean validating) {
            try {
                // Create a builder factory
		ByteArrayInputStream is = new ByteArrayInputStream(xmlstr.getBytes("UTF-8"));
                DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
                //factory.setValidating(validating);
    
                // Create the builder and parse the file
                Document doc = factory.newDocumentBuilder().parse(is);
                return doc;
            } catch (SAXException e) {
                // A parsing error occurred; the xml input is not valid
		System.out.println("error1 " + e.getMessage());
            } catch (ParserConfigurationException e) {
		System.out.println("error2 " + e.getMessage());
            } catch (IOException e) {
		System.out.println("error3 " + e.getMessage());
            }
            return null;
        }

public static void main(String[] args) {
	JTest test  = new JTest();
	Pius ps = new Pius();
				TiXML ti = new TiXML();
				ti.alloc();
	ps.ordo = 100;
	test.aptus = new Amor();
	String nstr = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?><root>this is OK root</root>";
	test.ignite(test.parseXmlFile(nstr,true));
	test.facio(ps);
        }

}

