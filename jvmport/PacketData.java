/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: To Unipac Data of PacketData
 Build: Created by octerboy 2007/11/25, Panyu
 $Date: 07-11-30 23:01 $
 $Revision: 2 $
*/

/* $NoKeywords: $ */

package textor.jvmport;
import java.lang.Object;

public class PacketData {
	
	static  { System.loadLibrary("jvmport"); } 
        public  byte[] portPtr;	/* C++ PacketData 的指针, 以字节数组方式表达 */

	public native void alloc() ;
	public native void produce(int max_index) ;
	public native void reset();
	public native void free();
	public native void grant(int space);
	public native byte[] getfld(int no);
	public native void input(int no, byte[] val);
	public native void input(int no, int iVal);
	public native void input(int no, long lVal);
	public native void input(int no, String str);
	public PacketData() { }
}
