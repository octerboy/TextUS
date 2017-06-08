import textor.jvmport.Amor;
import textor.jvmport.Pius;
import org.w3c.dom.*;

public class MTest {
	public Amor aptus;
	public Document im;
	public int count;
	public int mcount;

	public static final String JETUS_MODTIME = "$Date: 07-11-16 8:38 $";
	public static final String JETUS_BUILDNO =  "$Revision: 3 $";

	public MTest () { count = 0; mcount = 0;}

	public void ignite (Document doc) {
		im = doc;
		try {
		
		Text txt = (Text) doc.getDocumentElement().getFirstChild();
		System.out.println("my text : " + txt.getData() );
		
		} catch ( Exception e) {
		}
		return;
	}

	public boolean facio(Pius ps ) {
		Pius nps = new Pius();
		aptus.log_bug("ordo " + ps.ordo + " count " + count + " I ∂‘œÛ am "+ ((Text) im.getDocumentElement().getFirstChild()).getData());
		
		try {
			if ( ps.ordo == 5 )
			{
				String[] tstr =  new String[]{"First String", "Second String"};
			nps.ordo = 2000 | 0x00100000;
			nps.indic = (Object) tstr;
			aptus.facio(nps);
			} else if ( ps.ordo == (2000 | Pius.JAVA_NOTITIA_DOM ) )
			{
				String[] tstr = (String[] )ps.indic;
				System.out.println(tstr[0] + " " + tstr[1] );       
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
		MTest child;
		child = new MTest();
		child.im = im;
		mcount = mcount+1;
		child.count = mcount ;
		return (Object)child;
	}
}

