/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: To Unipac Data of TiXML
 Build: Created by octerboy 2007/11/25, Panyu
 $Date: 07-11-30 23:01 $
 $Revision: 2 $
*/

/* $NoKeywords: $ */

package jetus.jvmport;
import java.lang.Object;
import org.w3c.dom.*;

public class TiXML {
	
	static  { System.loadLibrary("jvmport"); } 
        public  byte[] portPtr;	/* C++ TiXmlElement 的指针, 以字节数组方式表达 */

	public native Document getDocument();
	public native void putDocument(Document doc);
	public native void alloc();
	public native void free();

	public TiXML() { }
}
