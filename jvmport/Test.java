import jetus.jvmport.Amor;
import jetus.jvmport.Pius;

public class Test {
	public Amor aptus;
	public static final String JETUS_MODTIME = "$Date: 07-11-13 10:36 $";
	public static final String JETUS_BUILDNO =  "$Revision: 2 $";

	public Test () { }
	public void ignite (String xmlstr) {
		/* xmlstr为XML文本, 包含模块启动时需要的参数 */
		return;
	}

	public boolean facio(Pius ps ) {
		/* 如果函数认知ps中的ordo并作了处理, 则返回true */
		return false;
	}
	
	public boolean sponte(Pius ps ) {
		/* 如果函数认知ps中的ordo并作了处理, 则返回true */
		return false;
	}
	
	public Object clone() {
		Test child;
		child = new Test();
		/* 这个类可以有很多属性, 在这里可以对新建的对象进行设置 */
		return (Object)child;
	}
}

