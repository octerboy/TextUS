/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Every Java Module should contain the TBuffer
 Build: Created by octerboy 2007/11/21, Panyu
 $Date: 07-11-30 23:01 $
 $Revision: 4 $
*/

/* $NoKeywords: $ */

package textor.jvmport;
import java.lang.Object;

public class TBuffer {
	
	public  static final int DEFAULT_TBUFFER_SIZE = 4096;
	static  { System.loadLibrary("jvmport"); } 

        public  byte[] portPtr;	/* c++ pointer of  TBuffer */

	public native void grant(long space) ;
	public native void input(byte[] val) ;
	public native long commit(long len) ;
	public native void reset();
	public  static native void exchange( TBuffer a, TBuffer b);
	public native byte[] getBytes();

	public  static native void pour( TBuffer a, TBuffer b);
	public  static native void pour( TBuffer a, TBuffer b, long len);

	public TBuffer() { }
	public native void alloc(long size) ;
	public void alloc() { alloc( DEFAULT_TBUFFER_SIZE); }
	public native void free() ;
}
