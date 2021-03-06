/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Every Java Module should contain the Amor
 Build: Created by octerboy 2007/10/27
 $Date: 07-11-30 23:00 $
 $Revision: 7 $
*/

/* $NoKeywords: $ */

package textor.jvmport;
import java.lang.Object;
import textor.jvmport.Pius;
import java.io.ByteArrayOutputStream;
import java.io.PrintStream;

public class Amor {
	
	static { System.loadLibrary("jvmport"); } 

        public byte[] portPtr;

	public  native boolean facio(Pius ps) ;
	public  native void log(long ordo, String msg);

	public  native boolean sponte(Pius ps) ;
	
	private ByteArrayOutputStream logbuf;
	private PrintStream out;

	public Amor() {
		logbuf =  new ByteArrayOutputStream();
		out = new PrintStream(logbuf);
	}
	public Amor(String dec) {
		logbuf =  new ByteArrayOutputStream();
		try {
		out = new PrintStream(logbuf, true, dec);
		} catch (Exception e) {
		}
	}
	
	public void log(long ordo, Object obj)
	{
		out.print(obj);
		//System.out.println(logbuf);
		String s = logbuf.toString();
		log(ordo, s);
		logbuf.reset();
	}

	public void log_bug(Object obj) 	{ log(17, obj); }
	public void log_info(Object obj) 	{ log(16, obj); }
	public void log_notice(Object obj) 	{ log(15, obj); }
	public void log_warn(Object obj) 	{ log(14, obj); }
	public void log_err(Object obj) 	{ log(13, obj); }
	public void log_crit(Object obj) 	{ log(12, obj); }
	public void log_alert(Object obj) 	{ log(11, obj); }
	public void log_emerg(Object obj) 	{ log(10, obj); }
}
