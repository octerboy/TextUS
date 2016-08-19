/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Port to the Java world
 Build:created by octerboy 2007/05/01, Panyu
 $Header: /textus/jvmport/JvmPort.cpp 20    08-05-04 22:21 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: JvmPort.cpp $"
#define TEXTUS_MODTIME  "$Date: 08-05-04 22:21 $"
#define TEXTUS_BUILDNO  "$Revision: 20 $"
/* $NoKeywords: $ */

#include "Notitia.h"
#include "Amor.h"
#include "jetus_jvmport_TiXML.h"
#include "jetus_jvmport_Amor.h"
#include "jetus_jvmport_PacketData.h"
#include "PacData.h"
#include "jetus_jvmport_TBuffer.h"
#include "TBuffer.h"
#include <time.h>
#include <jni.h>
#include <string.h>
#include <assert.h>

static bool jvmError(JNIEnv *env)
{
	jthrowable excp = 0;
	excp = env->ExceptionOccurred();
	if ( excp) { 
		env->ExceptionDescribe();
		env->ExceptionClear();
		return true;
	}
	return false;
}

static jobject getDocumentObj(JNIEnv *env, const char*xmlstr, const char *encod);
static void toDocument (JNIEnv *env, TiXmlDocument *docp, jobject jdoc);

static jobject getIntegerObj(JNIEnv *env, int val);
static void toInt (JNIEnv *env, int *val, jobject jInt );

static void allocPiusIndic (JNIEnv *env,  Amor::Pius &pius, jobject ps, jobject amr);
static void freePiusIndic (Amor::Pius &pius);

class JvmPort :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	JvmPort();
	~JvmPort();

#define BUF_Z 128
	struct VerList {
		char cls_name[BUF_Z];
		char mod_time[BUF_Z];
		char build_no[BUF_Z];
		struct VerList *next;
		inline VerList () {
			memset(cls_name, 0, sizeof(cls_name));
			memset(mod_time, 0, sizeof(mod_time));
			memset(build_no, 0, sizeof(build_no));
			next = 0;
		};

		inline void push(struct VerList *list ) 
		{
			if (next != 0)
			{
				list->next = next->next;
				next->next = list;
			} else {
				list->next = list;
			}
			next = list;
		};

		inline struct VerList *pop() {
			struct VerList *list;
			list = next;
			if (list != 0)
			{
				list = list->next;
				if (list != next)
					next->next = list->next;
				else
					next = 0; 
				list->next=0;	
			}
			return list;
		};
	};

	struct JVM_CFG {
		JavaVM *jvm;
		JNIEnv *env;
		JavaVMInitArgs vm_args;
		char *opt_string;
		struct VerList ver;
		
		inline JVM_CFG ( TiXmlElement *cfg) {
			int total_len = 0, k;
			TiXmlElement *o_ele;
			const char *comm_str;

			vm_args.version=JNI_VERSION_1_2;/* 这个字段必须设置为该值, 版本号设置不能漏*/
			//vm_args.version=JNI_VERSION_1_6;
			vm_args.ignoreUnrecognized = JNI_TRUE;
			vm_args.nOptions = 0;	
			vm_args.options = 0;
			opt_string = 0;

			jvm = 0;
			env = 0;

			if (!cfg) return ;
		
			comm_str = cfg->Attribute("version");
			if ( comm_str )
			{
				/* 这个字段必须设置为该值, 版本号设置不能漏 */
				if (strcmp(comm_str, "1.1") == 0 )
					vm_args.version=JNI_VERSION_1_1;

				if (strcmp(comm_str, "1.2") == 0 )
					vm_args.version=JNI_VERSION_1_2;

				if (strcmp(comm_str, "1.4") == 0 )
					vm_args.version=JNI_VERSION_1_4;

				if (strcmp(comm_str, "1.6") == 0 )
					vm_args.version=JNI_VERSION_1_6;
			}

			/* 先计算有多少个options */
			vm_args.nOptions = 0;
			for ( o_ele = cfg->FirstChildElement("option");
				o_ele;
				o_ele = o_ele->NextSiblingElement("option"))
			{
				vm_args.nOptions++;
				total_len += 1;
				if ( o_ele->GetText())
					total_len += strlen(o_ele->GetText());
			}

			if ( vm_args.nOptions == 0 ) 
				goto NEXT_STEP1;

			vm_args.options = new JavaVMOption [vm_args.nOptions];
			opt_string = new char[total_len];
			memset(opt_string, 0, total_len);
			vm_args.options[0].optionString = opt_string;

			for ( o_ele = cfg->FirstChildElement("option"), k =0;
				o_ele;
				o_ele = o_ele->NextSiblingElement("option"), k++)
			{
				int len;
				const char *con = o_ele->GetText();
				len = strlen(con);
				memcpy(vm_args.options[k].optionString, con, len);
				vm_args.options[k].extraInfo =  0;
				if ( k+1 < vm_args.nOptions )
					vm_args.options[k+1].optionString =  vm_args.options[k].optionString+ len +1;
			}
	NEXT_STEP1:
			return;
		};

		inline ~JVM_CFG () {
			if (vm_args.options ) 
				delete [] vm_args.options;
			vm_args.nOptions = 0;	
			vm_args.options = 0;
			if ( opt_string ) 
				delete []opt_string;
			if ( jvm )
				jvm->DestroyJavaVM();
		};
	};
	static struct JVM_CFG *jvmcfg;

	inline bool jvmError() { return ::jvmError(jvmcfg->env); };
private:
	struct G_CFG {
		char *xmlstr;
		const char *encoding;	/* xmlstr的编码方式 */
		char *cls_name;	/* Java类的名称,含路径 */
		jclass bean_cls, pius_cls, amor_cls;
		jmethodID bean_init, amor_init,  ignite_mid, facio_mid, sponte_mid, clone_mid;

		inline G_CFG ( TiXmlElement *cfg) {
			TiXmlDocument doc;
			TiXmlPrinter printer;
			TiXmlElement *xml;
			const char *bean;
			int len;

			xmlstr = 0;
			encoding = 0;
			cls_name = 0;
			bean_cls = pius_cls =  amor_cls = 0;
			ignite_mid = facio_mid = sponte_mid = clone_mid = 0;
			bean_init = amor_init = 0;
			bean =  cfg->Attribute("class");
			if ( bean)
			{
				int i;
				int l = strlen(bean);
				cls_name = new char [l + 1];
				memcpy(cls_name, bean, l+1);
				for ( i = 0 ; i < l ; i++)
					if ( cls_name[i] == '.' )
						cls_name[i] = '/';
			}
				
			xml= cfg->FirstChildElement("parameter");
			if ( !xml )
				return;
			doc.Clear();
			doc.InsertEndChild(*xml);
			doc.Accept( &printer );
			len = (int) printer.Size();
			xmlstr = new char [len+1];
			memset(xmlstr,0, len+1);
			memcpy(xmlstr, printer.CStr(), len);
			doc.Clear();
		};

		inline ~G_CFG () {
			if ( xmlstr) 
				delete[] xmlstr;
		}
	};

	Amor::Pius local_pius;  //仅用于传回数据
	Amor::Pius end_pius;  //仅用于传回数据

	struct G_CFG *gCFG;
	bool has_config;

	jobject owner_obj, aptus_obj;
	bool neo(jobject parent, JvmPort *me, jobject &own_obj, jobject &apt_obj);
	bool facioJava( Pius *ps);
	bool sponteJava( Pius *ps);
	jobject allocPiusObj( Pius *ps);
	void freePiusObj(jobject pso);
/*
options[0].optionString = "-Djava.compiler=NONE"; 
options[1].optionString = "-Djava.class.path=."; 
options[2].optionString = "-verbose:jni";
*/
#include "wlog.h"
};
struct JvmPort::JVM_CFG *JvmPort::jvmcfg=0;

void JvmPort::ignite(TiXmlElement *cfg) 
{ 
	TiXmlDeclaration *dec;
	TiXmlDocument *doc;
	TiXmlNode *fnode;
	int res;

	jstring ver_obj;
	jfieldID ver_fld;
	const char *ver_str;

	struct VerList *aver=new struct VerList;

	if (!cfg) return;

	doc = cfg->GetDocument ();
	fnode = doc->FirstChild () ;
	dec = fnode->ToDeclaration () ;
	if (  !gCFG  ) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	if ( dec)
		gCFG->encoding = dec->Encoding();

	if (  !jvmcfg  ) 
	{
		jvmcfg = new struct JVM_CFG(cfg->FirstChildElement("jvm"));
		res = JNI_CreateJavaVM(&(jvmcfg->jvm), (void**)&(jvmcfg->env), &(jvmcfg->vm_args));
		if (res < 0)
			return;
	}

	if ( !jvmcfg->env ) return;

	if ( !gCFG->cls_name) return;

	gCFG->bean_cls = jvmcfg->env->FindClass(gCFG->cls_name);
	if( jvmError() ) return;

	/* 取得Java类的时间信息 */
	ver_fld = jvmcfg->env->GetStaticFieldID(gCFG->bean_cls, "JETUS_MODTIME", "Ljava/lang/String;");
	if( jvmError() ) return;

	ver_obj = (jstring )jvmcfg->env->GetStaticObjectField(gCFG->bean_cls, ver_fld);	
	if( jvmError() ) return;

	ver_str = jvmcfg->env->GetStringUTFChars(ver_obj,0);
	if( jvmError() ) return;
	memcpy(aver->mod_time, ver_str, strlen(ver_str) > BUF_Z-1 ? BUF_Z-1 : strlen(ver_str) );
	jvmcfg->env->ReleaseStringUTFChars(ver_obj, ver_str);	

	/* 取得Java类的版本序号 */
	ver_fld = jvmcfg->env->GetStaticFieldID(gCFG->bean_cls, "JETUS_BUILDNO", "Ljava/lang/String;");
	if( jvmError() ) return;

	ver_obj = (jstring) jvmcfg->env->GetStaticObjectField(gCFG->bean_cls, ver_fld);	
	if( jvmError() ) return;

	ver_str = jvmcfg->env->GetStringUTFChars(ver_obj,0);
	if( jvmError() ) return;
	memcpy(aver->build_no, ver_str, strlen(ver_str) > BUF_Z-1 ? BUF_Z-1 : strlen(ver_str) );
	jvmcfg->env->ReleaseStringUTFChars(ver_obj, ver_str);	

	ver_str = (const char*) gCFG->cls_name;
	memcpy(aver->cls_name, ver_str, strlen(ver_str) > BUF_Z-1 ? BUF_Z-1 : strlen(ver_str) );
	jvmcfg->ver.push(aver);
}

bool JvmPort::facio( Amor::Pius *pius)
{
	assert(pius);
	assert(jvmcfg);
	if ( !jvmcfg->env )
	{
		WLOG(ERR,"JVM not created!");
		return false;
	}

	switch ( pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		gCFG->bean_init = jvmcfg->env->GetMethodID(gCFG->bean_cls, "<init>", "()V");

		gCFG->pius_cls = jvmcfg->env->FindClass("jetus/jvmport/Pius");
		if ( !gCFG->pius_cls)
		{
			jvmError();
  			WLOG(ERR, "Not found class of textus.jvmport.Pius");
			break;
		}

		gCFG->amor_cls = jvmcfg->env->FindClass("jetus/jvmport/Amor");
		if ( !gCFG->amor_cls) {
			jvmError();
  			WLOG(ERR, "Not found class of textus.jvmport.Amor");
			break;
		}

		gCFG->amor_init = jvmcfg->env->GetMethodID(gCFG->amor_cls, "<init>", "()V");

		gCFG->facio_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "facio", "(Ljetus/jvmport/Pius;)Z");
		if ( !gCFG->facio_mid ) {
			jvmError();
			WLOG(ERR,"not found method facio(Pius)");
			break;
		}

		gCFG->sponte_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "sponte", "(Ljetus/jvmport/Pius;)Z");
		if ( !gCFG->sponte_mid ) {
			jvmError();
			WLOG(ERR,"not found method sponte(Pius)");
			break;
		}

		gCFG->clone_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "clone", "()Ljava/lang/Object;");
		if ( !gCFG->clone_mid ) {
			jvmError();
			WLOG(ERR,"not found method clone()");
			break;
		}

		gCFG->ignite_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "ignite", "(Lorg/w3c/dom/Document;)V");
		if ( !gCFG->ignite_mid ) {
			jvmError();
			WLOG(ERR,"not found method ignite(Element)");
			break;
		}

		if ( jvmError() || !neo(0, this, owner_obj, aptus_obj) )
			break;

		if ( gCFG->ignite_mid )
		{
			if ( gCFG->xmlstr )
			{
				jobject document = getDocumentObj(jvmcfg->env, gCFG->xmlstr, gCFG->encoding);
				if ( document)
				{
					jvmcfg->env->CallVoidMethod(owner_obj, gCFG->ignite_mid, document); /* 调用Java模块的ignite函数 */
					jvmError();
					jvmcfg->env->DeleteLocalRef(document);
				} 
			} else {
				jvmcfg->env->CallVoidMethod(owner_obj,gCFG->ignite_mid,0);
			}
		} else {
  			WLOG(ERR, "Not found method ignite(String)");
			break;
		}
		return facioJava(pius);
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		return facioJava(pius);
		break;

	default:
		return facioJava(pius);
	}
	return true;
}

bool JvmPort::sponte( Amor::Pius *pius) 
{
	assert(pius);
	assert(jvmcfg);
	if ( !jvmcfg->env )
	{
		WLOG(ERR,"JVM not created!");
		return false;
	}
	return sponteJava(pius);
}

bool JvmPort::facioJava( Amor::Pius *pius)
{
	jobject ps_obj;
	bool ret = false;
	if ( gCFG->facio_mid && gCFG->pius_cls && (ps_obj = allocPiusObj(pius) ) && owner_obj )
	{
		ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj,gCFG->facio_mid, ps_obj);
		freePiusObj(ps_obj);
	}
	jvmError() ;
	return ret;
}

bool JvmPort::sponteJava( Amor::Pius *pius) 
{
	jobject ps_obj;
	bool ret = false;
	if ( gCFG->sponte_mid && gCFG->pius_cls && (ps_obj = allocPiusObj(pius) ) && owner_obj)
	{
		ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj,gCFG->sponte_mid, ps_obj);
		freePiusObj(ps_obj);
	}
	jvmError() ;
	return ret;
}

bool JvmPort:: neo(jobject parent, JvmPort *me, jobject &own_obj, jobject &apt_obj)
{
	jfieldID apt_fld, ptr_fld;
	jbyteArray  selfPtr;
	
	if ( !gCFG->bean_cls || ! gCFG->amor_cls || !gCFG->clone_mid || !gCFG->bean_init || !gCFG->amor_init )
		return false;

	if ( parent )
		own_obj = jvmcfg->env->CallObjectMethod(parent, gCFG->clone_mid);
	else
		own_obj = jvmcfg->env->NewObject(gCFG->bean_cls, gCFG->bean_init, 0);
	apt_fld   = jvmcfg->env->GetFieldID(gCFG->bean_cls, "aptus", "Ljetus/jvmport/Amor;");
	apt_obj = jvmcfg->env->NewObject(gCFG->amor_cls, gCFG->amor_init, 0);
	if ( jvmError() || !apt_fld || !own_obj || !apt_obj)
	{
		WLOG(ERR,"not found member aptus");
		return false;
	} else {
		jvmcfg->env->SetObjectField(own_obj, apt_fld, aptus_obj);

		ptr_fld = jvmcfg->env->GetFieldID(gCFG->amor_cls, "portPtr", "[B");
		/* Java的Amor对象中的portPtr, 即为本对象的指针 */
		selfPtr = jvmcfg->env->NewByteArray(sizeof(me));
		jvmcfg->env->SetByteArrayRegion(selfPtr, 0, sizeof(me), (jbyte*)&me);
		jvmcfg->env->SetObjectField(aptus_obj, ptr_fld, selfPtr);
	} 
	return true;
}

Amor* JvmPort::clone()
{
	JvmPort *child = new JvmPort();
	child->gCFG = gCFG;
	neo(owner_obj, child, child->owner_obj , child->aptus_obj);
	return  (Amor*)child;
}

JvmPort::JvmPort()
{
	gCFG = 0;
	has_config = false;

	owner_obj = aptus_obj = 0;
}

JvmPort::~JvmPort() 
{ 
	if ( has_config )
	{
		delete gCFG;
		gCFG = 0;
	}
	if (jvmcfg && jvmcfg->env )
	{
		jvmcfg->env->DeleteLocalRef(aptus_obj);
		jvmcfg->env->DeleteLocalRef(owner_obj);
	}
} 
#include "hook.c"

extern "C" TEXTUS_AMOR_EXPORT void textus_get_version_1(char *scm_id, char *time_str, char *ver_no, int len) 
{
	JvmPort::JVM_CFG *jcfg;
	JvmPort::VerList *aver;
	jcfg = JvmPort::jvmcfg;
	aver = jcfg->ver.pop();
	memset(scm_id,0, len);
	memset(time_str,0, len);
	memset(ver_no,0, len);
	memset(scm_id,'1', 1);
	memset(time_str,'1', 1);
	memset(ver_no, '1', 1);
	if ( aver )
	{
		int mlen ;
	        char tmp[1024];
		char *p, *q;
		int flen;
		if ( len > 1023 ) len = 1023;

		mlen = strlen(aver->cls_name);
		memcpy(scm_id, aver->cls_name, mlen > len ? len : mlen);

		GET_SCM_VERSION_INFO(aver->mod_time, time_str)
		mlen = strlen(aver->build_no);
		GET_SCM_VERSION_INFO(aver->build_no, ver_no)
		delete aver;
	}
}

static int ba2buf(JNIEnv * env, jbyteArray args, unsigned char*buf, int buf_len)
{
        int len, r_len;
        len = env->GetArrayLength(args);
	r_len = (len > buf_len ? buf_len:len);
	memset(buf, 0, buf_len);
        env->GetByteArrayRegion(args, 0, r_len, (jbyte*)buf);
        return len;
}

static jbyteArray buf2ba(JNIEnv * env, unsigned char*buf , int len)
{
        jbyteArray  args;
        if ( len <=0 ) return 0;
        args = env->NewByteArray(len);
        env->SetByteArrayRegion(args, 0, len, (jbyte*)buf);
        return args;
}

static void *getPointer (JNIEnv *env,  jobject obj)
{
	void *pointer;
	jclass o_cls = env->GetObjectClass(obj); 
	jfieldID port_fld = env->GetFieldID(o_cls, "portPtr", "[B");	
	jbyteArray port = (jbyteArray) env->GetObjectField(obj, port_fld);
	int psize = env->GetArrayLength(port);
	if ( psize !=  sizeof(pointer))
		return 0;
	ba2buf(env, port, (unsigned char*)(&pointer), sizeof(pointer));
	return pointer;
}

static void setPointer (JNIEnv *env,  jobject obj, void *p)
{
	jclass o_cls = env->GetObjectClass(obj); 
	jfieldID port_fld = env->GetFieldID(o_cls, "portPtr", "[B");	
	jbyteArray port =  buf2ba(env, (unsigned char*)&p , sizeof(p));
	env->SetObjectField(obj, port_fld, (jobject)port);
	return ;
}

JNIEXPORT jobject JNICALL Java_jetus_jvmport_TiXML_getDocument (JNIEnv *env, jobject tio)
{
	TiXmlDocument *docp = (TiXmlDocument *)getPointer(env, tio);
	jobject document ;
	TiXmlPrinter printer;
	
	if ( !docp ) return 0;

	/* 先将数据合成一个XML串 */
	docp->Accept( &printer );
	document = getDocumentObj(env, printer.CStr(), docp->FirstChild()->ToDeclaration()->Encoding());
	return document;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TiXML_putDocument (JNIEnv *env, jobject tio, jobject jdoc)
{
	TiXmlDocument *docp = (TiXmlDocument *)getPointer(env, tio);
	if ( !docp || !jdoc) return ;

	toDocument(env, docp, jdoc);
	return;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TiXML_alloc (JNIEnv *env , jobject tbo)
{
	TiXmlDocument *neo = new TiXmlDocument();
	setPointer(env, tbo, neo);
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TiXML_free
  (JNIEnv *env, jobject tbo)
{
	TiXmlDocument *docp = (TiXmlDocument *)getPointer(env, tbo);
	if ( docp)
		delete docp;
}

JNIEXPORT jboolean JNICALL Java_jetus_jvmport_Amor_facio (JNIEnv *env, jobject amor, jobject ps) 
{
	Amor::Pius pius;
	Amor *port;
	jboolean ret;
	
	port = (Amor*) getPointer (env,  amor);
	if ( !port )
		return false;

	allocPiusIndic (env, pius, ps, amor);
	ret = (jboolean) port->aptus->facio(&pius);
	freePiusIndic(pius);
	return ret;
}

JNIEXPORT jboolean JNICALL Java_jetus_jvmport_Amor_sponte (JNIEnv *env, jobject amor, jobject ps) 
{
	Amor::Pius pius;
	Amor *port;
	jboolean ret;
	
	port = (Amor*) getPointer (env,  amor);
	if ( !port )
		return false;

	allocPiusIndic (env, pius, ps, amor);

	ret = (jboolean) port->aptus->sponte(&pius);
	freePiusIndic(pius);
	return ret;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_Amor_log (JNIEnv *env, jobject amor, jint ordo, jstring jmsg)
{
	Amor::Pius pius;
	Amor *port;
	const char *msg;

	port = (Amor*) getPointer (env,  amor);
	if ( !port )
		return ;

	msg = env->GetStringUTFChars(jmsg,0);
	pius.ordo = ordo;
	pius.indic = (void*)msg;
	
	port->aptus->sponte(&pius);
	env->ReleaseStringUTFChars(jmsg, msg);
	return;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_alloc
  (JNIEnv *env, jobject paco) {
	struct PacketObj *neo = new  struct PacketObj();
	setPointer (env,  paco, neo);
	return ;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_produce (JNIEnv *env, jobject paco, jint fld_num) 
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		pcp->produce(fld_num);
	return ;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_reset (JNIEnv *env, jobject paco)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		pcp->reset();
	return ;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_free (JNIEnv *env, jobject paco)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		delete pcp;
	return;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_grant (JNIEnv *env, jobject paco, jint space)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	//printf("in jni pacdata %08x\n", (unsigned int)pcp);
	if ( pcp)
		pcp->grant(space);
	return;
}

JNIEXPORT jbyteArray JNICALL Java_jetus_jvmport_PacketData_getfld (JNIEnv *env, jobject paco, jint no)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jbyteArray reta;
	reta = 0;
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val)
			reta =  buf2ba(env, val, (int) len);
	}
	return reta;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_PacketData_input (JNIEnv *env, jobject paco, jint no , jbyteArray val)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
        	int len = env->GetArrayLength(val);
		pcp->grant(len);
        	env->GetByteArrayRegion(val, 0, len, (jbyte*)pcp->buf.point);
		pcp->commit(no, len);
	}
	return;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_alloc
  (JNIEnv *env , jobject tbo, jint size)
{
	TBuffer *neo = new TBuffer(size);
	setPointer(env, tbo, neo);
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_free
  (JNIEnv *env, jobject tbo)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp)
		delete tbp;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_grant
  (JNIEnv * env, jobject tbo, jint size) 
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	//printf("in jnitbuffer %08x\n", tbp);
	if ( tbp)
		tbp->grant(size);
}

JNIEXPORT jint JNICALL Java_jetus_jvmport_TBuffer_commit
  (JNIEnv *env, jobject tbo, jbyteArray ptr, jint len)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp ) 
		return tbp->commit(len);
	else 
		return 0;
	
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_reset
  (JNIEnv *env, jobject tbo, jbyteArray ptr)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp )
		tbp->reset();
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_exchange
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::exchange(*tbpa, *tbpb);
	return ;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_pour___3B_3B
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::pour(*tbpa, *tbpb);
	return ;
}

JNIEXPORT void JNICALL Java_jetus_jvmport_TBuffer_pour___3B_3BI
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb, jint len)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::pour(*tbpa, *tbpb, len);
	return ;
}

JNIEXPORT jbyteArray JNICALL Java_jetus_jvmport_TBuffer_getBytes
  (JNIEnv *env, jobject tbo)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	jbyteArray bts;
	int len ;
	if ( !tbp)
		return 0;
	len = tbp->point - tbp->base;
	bts = env->NewByteArray(len);
	env->SetByteArrayRegion(bts, 0, len, (jbyte*)tbp->base);
	return bts;
}


/* 从一个字符串xmlstr, 获得一个org.w3c.dom.Document对象 */
jobject getDocumentObj(JNIEnv *env, const char*xmlstr, const char *encoding )
{
	jbyteArray  args;
	int len; 
	jobject jstr, encStr , utfBytes ;
	jobject input, factory, docBuilder, document;
	jclass str_cls, is_cls, dbFac_cls, doc_cls, docBuild_cls;
	jmethodID strInit_mid, getBytes_mid, isInit_mid, newFac_mid, newDocBuilder_mid, parse_mid;
	jstring string;

	str_cls = env->FindClass("java/lang/String");
	is_cls = env->FindClass("java/io/ByteArrayInputStream");
	dbFac_cls = env->FindClass("javax/xml/parsers/DocumentBuilderFactory");
	docBuild_cls = env->FindClass("javax/xml/parsers/DocumentBuilder");
	doc_cls = env->FindClass("org/w3c/dom/Document");

	strInit_mid = env->GetMethodID(str_cls, "<init>", "([BLjava/lang/String;)V");
	getBytes_mid = env->GetMethodID(str_cls, "getBytes", "(Ljava/lang/String;)[B");
	isInit_mid = env->GetMethodID(is_cls, "<init>", "([B)V");
	newFac_mid = env->GetStaticMethodID(dbFac_cls, "newInstance", "()Ljavax/xml/parsers/DocumentBuilderFactory;");
	newDocBuilder_mid = env->GetMethodID(dbFac_cls, "newDocumentBuilder", "()Ljavax/xml/parsers/DocumentBuilder;");
	parse_mid = env->GetMethodID(docBuild_cls, "parse", "(Ljava/io/InputStream;)Lorg/w3c/dom/Document;");
				
	if ( jvmError(env) )
		return 0;
				
	encStr = env->NewStringUTF(encoding);
			
	/* 生成String对象jstr, 即其xml */	
	len = strlen(xmlstr);
	args =  env->NewByteArray(len);
	env->SetByteArrayRegion(args, 0, len, (jbyte*)xmlstr);
	jstr = env->NewObject(str_cls, strInit_mid, args, encStr);
	
	/* 按UTF8返回bytes */
	string = env->NewStringUTF("UTF-8");
	utfBytes = env->CallObjectMethod(jstr, getBytes_mid, string);
	env->DeleteLocalRef(string);

	/* 生成InputStream, 及一系列 */
	input = env->NewObject(is_cls, isInit_mid, utfBytes);
	factory = env->CallStaticObjectMethod(dbFac_cls, newFac_mid);
	docBuilder = env->CallObjectMethod(factory, newDocBuilder_mid);
	document = env->CallObjectMethod(docBuilder, parse_mid, input);

	if ( jvmError(env)) return 0;
			
	env->DeleteLocalRef(encStr);
	env->DeleteLocalRef(args);
	env->DeleteLocalRef(jstr);

	env->DeleteLocalRef(utfBytes);
	env->DeleteLocalRef(input);
	env->DeleteLocalRef(factory);
	env->DeleteLocalRef(docBuilder);

	return document;
}

/* 从一个org.w3c.dom.Document对象, 生成一个TiXmlDocument对象 */
void toDocument (JNIEnv *env, TiXmlDocument *docp, jobject jdoc)
{
	jobject factory, transformer, source, bos, result;
	jbyteArray  xmlbytes;	
	jclass tFac_cls, trans_cls, dsrc_cls, bos_cls, rslt_cls;
	jmethodID newFac_mid, newTrans_mid, dsrcInit_mid, bosInit_mid, rsltInit_mid;
	jmethodID transform_mid, toBytes_mid;

	int len;
	char *buf;
	tFac_cls = env->FindClass("javax/xml/transform/TransformerFactory");
	trans_cls = env->FindClass("javax/xml/transform/Transformer");
	dsrc_cls = env->FindClass("javax/xml/transform/dom/DOMSource");
	bos_cls = env->FindClass("java/io/ByteArrayOutputStream");
	rslt_cls = env->FindClass("javax/xml/transform/stream/StreamResult");

	if ( jvmError(env) ) return ;
	newFac_mid = env->GetStaticMethodID(tFac_cls, "newInstance", "()Ljavax/xml/transform/TransformerFactory;");
	newTrans_mid = env->GetMethodID(tFac_cls, "newTransformer", "()Ljavax/xml/transform/Transformer;");
	dsrcInit_mid = env->GetMethodID(dsrc_cls, "<init>", "(Lorg/w3c/dom/Node;)V");
	bosInit_mid = env->GetMethodID(bos_cls, "<init>", "()V");
	rsltInit_mid = env->GetMethodID(rslt_cls, "<init>", "(Ljava/io/OutputStream;)V");
	transform_mid = env->GetMethodID(trans_cls, "transform", "(Ljavax/xml/transform/Source;Ljavax/xml/transform/Result;)V");
	toBytes_mid = env->GetMethodID(bos_cls, "toByteArray", "()[B");
	if ( jvmError(env) ) return ;

	factory = env->CallStaticObjectMethod(tFac_cls, newFac_mid);
	transformer = env->CallObjectMethod(factory, newTrans_mid);
	source = env->NewObject(dsrc_cls, dsrcInit_mid, jdoc);
	bos =  env->NewObject(bos_cls, bosInit_mid);
	result = env->NewObject(rslt_cls, rsltInit_mid, bos);
	env->CallVoidMethod(transformer, transform_mid, source, result);
	xmlbytes = (jbyteArray)env->CallObjectMethod(bos, toBytes_mid);
	if ( jvmError(env) ) return ;

        len = env->GetArrayLength(xmlbytes) ;
	buf = new char[len+1];
        env->GetByteArrayRegion(xmlbytes, 0, len, (jbyte*)buf);
	buf[len] = 0;
	docp->Parse((const char*)buf);

	env->DeleteLocalRef(xmlbytes);
	env->DeleteLocalRef(result);
	env->DeleteLocalRef(bos);
	env->DeleteLocalRef(source);
	env->DeleteLocalRef(transformer);
	env->DeleteLocalRef(factory);
	delete[] buf;

	return;
}

/* 从Java程序到C++程序, 在调用C++程序时, 生成相应的indic指针 */
static void allocPiusIndic (JNIEnv *env,  Amor::Pius &pius, jobject ps, jobject amor)
{
	jclass pius_cls;
	jfieldID ordo_fld, indic_fld;

	pius.ordo = Notitia::TEXTUS_RESERVED;
	pius.indic = 0;
	pius_cls = env->FindClass("jetus/jvmport/Pius");
	if ( jvmError(env) )
		return;

	ordo_fld = env->GetFieldID(pius_cls, "ordo", "I");
	indic_fld = env->GetFieldID(pius_cls, "indic", "Ljava/lang/Object;");
	pius.ordo = env->GetIntField(ps, ordo_fld);

	if ( (TEST_NOTITIA_FLAG & pius.ordo) == JAVA_NOTITIA_DOM )
	{
		pius.indic = (void*) env->GetObjectField(ps, indic_fld);
		return ;
	}
	/* 接下去要做 ps.indic(Java)到pius.indic(C++)的工作 */
	switch ( pius.ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::PRO_TBUF:
	case Notitia::SET_UNIPAC:
	case Notitia::SET_TINY_XML:
	{
		jclass tbuf_cls;
		jobject tbo1, tbo2;
		jbyteArray fir, sec;
		jfieldID port_fld;
		jobjectArray indic;

		indic = (jobjectArray) env->GetObjectField(ps, indic_fld);
		if ( !indic ) break;

		pius.indic = new void*[2];
		if ( pius.ordo == Notitia::SET_UNIPAC )
			tbuf_cls = env->FindClass("jetus/jvmport/PacketData");
		else if ( pius.ordo == Notitia::SET_TBUF ||  pius.ordo == Notitia::PRO_TBUF )
			tbuf_cls = env->FindClass("jetus/jvmport/TBuffer");
		else
			tbuf_cls = env->FindClass("jetus/jvmport/TiXML");
			
		if ( jvmError(env)) break;
		port_fld  = env->GetFieldID(tbuf_cls, "portPtr", "[B");
		if ( jvmError(env)) break;

		tbo1 = env->GetObjectArrayElement(indic, 0); 
		tbo2 = env->GetObjectArrayElement(indic, 1); 

		if ( !tbo1 || !tbo2 ) break;
		fir = (jbyteArray) env->GetObjectField(tbo1, port_fld);
		sec = (jbyteArray) env->GetObjectField(tbo2, port_fld);

		ba2buf(env, fir, (unsigned char*)&(((void**)pius.indic)[0]), sizeof(void *));
		ba2buf(env, sec, (unsigned char*)&(((void**)pius.indic)[1]), sizeof(void *));
		//printf("in jniamor %08x %08x \n", ((void**)pius.indic)[0], ((void**)pius.indic)[1]);
	}
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		/* indic 指向一个指针TiXmlElement*, 将此转为org.w3c.dom.Document对象 */
	{
		TiXmlDocument *docp = new TiXmlDocument();
		toDocument(env, docp, env->GetObjectField(ps, indic_fld));
		pius.indic = docp->RootElement();	
	}
		break;	

	case Notitia::DMD_SET_TIMER:
		/* ps.indic 是一个java.lang.integer, 转成int, 并且还要加一个jvmport的指针 */
	{
		pius.indic = getPointer(env, amor);
	}
		break;

	case Notitia::DMD_SET_ALARM:
		/* ps.indic 是一个java.lang.integer, 转成int, 并且还要加一个jvmport的指针 */
	{
		void **indp = new void* [2];
		int *click = new int;

		indp[0] = getPointer(env, amor);
		indp[1] = click;
		toInt(env, click,  env->GetObjectField(ps, indic_fld));
		pius.indic = indp;
	}
		break;

	case Notitia::CMD_HTTP_GET:
		/* indic 指向一个指针struct GetRequestCmd* */
		break;
	
	case Notitia::CMD_HTTP_SET:
		/* indic 指向一个指针struct SetResponseCmd* */
		break;

	default :
		break;
	}
}

/* 从Java程序到C++程序, 在调用C++程序后, 释放相应的indic指针 */
static void freePiusIndic (Amor::Pius &pius) 
{
	void **ptrArray;
	TiXmlDocument *docp;
	TiXmlElement *root;
	
	switch ( pius.ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::PRO_TBUF:
	case Notitia::SET_UNIPAC:
	case Notitia::SET_TINY_XML:
		ptrArray = (void**) pius.indic;
		if ( ptrArray )
			delete[] ptrArray;
		break;
		/* indic指向两个指针的指针组 */
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		/* indic 指向一个指针TiXmlElement* */
		root = (TiXmlElement *)pius.indic;
		if ( root )
		{
			docp = root->Parent()->ToDocument();
			if ( docp)
				delete docp;
		}
		break;	
	
	case Notitia::DMD_SET_ALARM:
	{
		void **indp = (void**)pius.indic;
		int *click = (int*)indp[1];
		delete click;
		delete[] indp;
	}
		break;

	case Notitia::CMD_HTTP_GET:
		/* indic 指向一个指针struct GetRequestCmd* */
		break;
	
	case Notitia::CMD_HTTP_SET:
		/* indic 指向一个指针struct SetResponseCmd* */
		break;

	default :
		break;
	}
}

/* 从C++程序到Java程序, 为Java程序生成合适的Pius对象 */
jobject JvmPort::allocPiusObj( Pius *pius)
{
	jobject ps_obj;
	jmethodID psInit;
	jfieldID ordo_fld, indic_fld;
	jobjectArray indic;

	psInit = jvmcfg->env->GetMethodID(gCFG->pius_cls, "<init>", "()V");
	ps_obj = jvmcfg->env->NewObject(gCFG->pius_cls, psInit);

	ordo_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "ordo", "I");
	jvmcfg->env->SetIntField(ps_obj, ordo_fld, pius->ordo);		/* java的pius的ordo已设定 */

	indic_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "indic", "Ljava/lang/Object;");
	if ( jvmError() )
		return 0;

	if ( (TEST_NOTITIA_FLAG & pius->ordo) == JAVA_NOTITIA_DOM )
	{
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, (jobject)(pius->indic));		/* 对于Java的Object直接设置 */
		goto RETURN;
	}
	/* 下面根据ordo来生成 ps_obj中的indic, 对付各种TBuffer等 */
	switch ( pius->ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::PRO_TBUF:
	case Notitia::SET_UNIPAC:
	case Notitia::SET_TINY_XML:
	{
		void **tb, *first, *second;
		jclass tbuf_cls;
		jobject tbo1, tbo2;
		jbyteArray fir, sec;
		jfieldID port_fld;

		tb = (void **)(pius->indic);
		if (tb) 
		{
			if ( *tb) first = *tb; 
			tb++;
			if ( *tb) second = *tb;

			if ( pius->ordo == Notitia::SET_TBUF || pius->ordo == Notitia::PRO_TBUF )
				tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/TBuffer");
			else if ( pius->ordo == Notitia::SET_UNIPAC)
				tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/PacketData");
			else 
				tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/TiXML");

			if ( jvmError()) break;
			port_fld  = jvmcfg->env->GetFieldID(tbuf_cls, "portPtr", "[B");
			if ( jvmError()) break;

			indic = jvmcfg->env->NewObjectArray(2, tbuf_cls, 0); 
			tbo1 = jvmcfg->env->AllocObject(tbuf_cls); 
			tbo2 = jvmcfg->env->AllocObject(tbuf_cls); 

			/* 对于32位和64位系统, first、second这此指针的值分别为32位和64位, 
			用sizeof来决定其所占空间. 这样,Java中的ByteArray就分别是4字节或8字节
			*/
			fir = jvmcfg->env->NewByteArray(sizeof(first));
			sec = jvmcfg->env->NewByteArray(sizeof(second));
			jvmcfg->env->SetByteArrayRegion(fir, 0, sizeof(first), (jbyte*)&first);
			jvmcfg->env->SetByteArrayRegion(sec, 0, sizeof(second), (jbyte*)&second);
			jvmcfg->env->SetObjectField(tbo1, port_fld, (jobject)fir);
			jvmcfg->env->SetObjectField(tbo2, port_fld, (jobject)sec);
			jvmcfg->env->SetObjectArrayElement(indic, 0, tbo1); 
			jvmcfg->env->SetObjectArrayElement(indic, 1, tbo2); 
			jvmcfg->env->SetObjectField(ps_obj, indic_fld, (jobject)indic);
			//printf("in jvmport %08x %08x\n", first, second);
		}
	}
		break;
		/* indic指向两个指针的指针组 */
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		/* indic 指向一个指针TiXmlElement* */
	{
		TiXmlDocument doc;
		TiXmlPrinter printer;
		jobject document;
		TiXmlElement *xml = (TiXmlElement *)pius->indic;
		if ( !xml ) 
			break;
		doc.Clear();
		doc.InsertEndChild(*xml);
		doc.Accept( &printer );
		document = getDocumentObj( jvmcfg->env, printer.CStr(), gCFG->encoding);
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, document);
	}
		break;	

	case Notitia::CMD_HTTP_GET:
		/* indic 指向一个指针struct GetRequestCmd* */
		break;
	
	case Notitia::CMD_HTTP_SET:
		/* indic 指向一个指针struct SetResponseCmd* */
		break;

	case Notitia::MAIN_PARA:
		/*  *indic[0] = argc, *indic[1] = argv, 将此转为String[] */
	{
		int num;
		char **argv;
		void **ps;
		jclass str_cls = jvmcfg->env->FindClass("java/lang/String");
		jobjectArray strArr;

		ps = (void**)pius->indic;
 		num = (*(int *)ps[0]);
		argv = (char **)ps[1];
		strArr = jvmcfg->env->NewObjectArray(num, str_cls, 0);
		for ( int i = 0 ; i < num; i++)
		{
			jstring jv;
			jv = jvmcfg->env->NewStringUTF(argv[i]);
			jvmcfg->env->SetObjectArrayElement(strArr, i, jv);
		}
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, strArr);
	}
		break;

	case Notitia::TIMER:
		/* 这些要转一个java.lang.Integer */
	{
		jobject integer;

		integer = getIntegerObj(jvmcfg->env,*((int*) (pius->indic)));
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, integer);
	}
		break;

	case Notitia::DMD_SET_TIMER:
		/* indic指向this, 通知给Java的, 有Java程序进行时间片管理吗? 没有, 所以暂不实现. */
		break;
	
	case Notitia::DMD_SET_ALARM:
		/* indic指向this及一个int*, 通知给Java的, 有Java程序进行时间片管理吗? 没有, 所以暂不实现. */
		break;

	case Notitia::SET_SAME_PRIUS:	/* 这个其实不会出现 */
	case Notitia::DMD_CONTINUE_SELF:
	case Notitia::DMD_CONTINUE_NEXT:
	case Notitia::CMD_FREE_IDLE:

	case Notitia::FD_SETRD:
	case Notitia::FD_SETWR:
	case Notitia::FD_SETEX:
	case Notitia::FD_CLRRD:
	case Notitia::FD_CLRWR:
	case Notitia::FD_CLREX:
	case Notitia::FD_PRORD:
	case Notitia::FD_PROWR:
	case Notitia::FD_PROEX: /* 指向 Describo::Criptor指针 */
		/* 这些是不支持的 */
		break;

	case Notitia::CMD_GET_OWNER:	/* 返回时指向一个Amor对象 */
	case Notitia::CMD_ALLOC_IDLE:	/* 返回时指向一个Amor对象 */
	case Notitia::DMD_CLONE_OBJ:	/* 返回时指向一个Amor对象 */
	case Notitia::CMD_GET_PIUS:	/* 返回时指向一个Pius对象 */
	case Notitia::CMD_GET_VERSION:	/* 返回时指向一个TiXmlElement */
		/* 这些是调用返回时设置的, 可以支持, 如何处理? */
		break;

	case Notitia::DMD_STOP_NEXT:
	case Notitia::CMD_INCR_REFS:
	case Notitia::CMD_DECR_REFS:
	case Notitia::JUST_START_THREAD:
	case Notitia::FINAL_END_THREAD:
	case Notitia::NEW_SESSION:
	case Notitia::END_SERVICE:
	case Notitia::CHANNEL_TIMEOUT:
	case Notitia::CMD_CHANNEL_PAUSE:
	case Notitia::CMD_CHANNEL_RESUME:
	case Notitia::CHANNEL_NOT_ALIVE:
	case Notitia::END_SESSION:
	case Notitia::DMD_END_SESSION:
	case Notitia::START_SESSION:
	case Notitia::DMD_START_SESSION:
	case Notitia::ERR_FRAME_TIMEOUT:
	case Notitia::ERR_FRAME_LENGTH:
		/* 这些本来就是不需要indic的 */
		break;

	default :
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, 0);
		break;
	}

RETURN:
	return ps_obj;
}

/* 从C++程序到Java程序, 在调用Java程序后, 释放相应的Pius对象 */
void JvmPort::freePiusObj( jobject ps_obj)
{
	jobjectArray indic;
	jfieldID ordo_fld, indic_fld;
	int ordo;

	jobject sm_obj;
	int loopi;

	ordo_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "ordo", "I");
	indic_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "indic", "Ljava/lang/Object;");
	ordo = jvmcfg->env->GetIntField(ps_obj, ordo_fld);

	/* 下面根据ordo来处理 ps_obj中的indic, 释放各种对象 */
	switch ( ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::PRO_TBUF:
	case Notitia::SET_UNIPAC:
	case Notitia::SET_TINY_XML:
	{
		jclass tbuf_cls;
		jobject tbo1, tbo2;
		jbyteArray fir, sec;
		jfieldID port_fld;

		indic = (jobjectArray) jvmcfg->env->GetObjectField(ps_obj, indic_fld);
		if ( !indic ) break;

		if ( ordo == Notitia::SET_TBUF || ordo == Notitia::PRO_TBUF )
			tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/TBuffer");
		else if ( ordo == Notitia::SET_UNIPAC)
			tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/PacketData");
		else 
			tbuf_cls = jvmcfg->env->FindClass("jetus/jvmport/TiXML");

		if ( jvmError()) break;
		port_fld  = jvmcfg->env->GetFieldID(tbuf_cls, "portPtr", "[B");
		if ( jvmError()) break;

		tbo1 = jvmcfg->env->GetObjectArrayElement(indic, 0); 
		tbo2 = jvmcfg->env->GetObjectArrayElement(indic, 1); 

		fir = (jbyteArray) jvmcfg->env->GetObjectField(tbo1, port_fld);
		sec = (jbyteArray) jvmcfg->env->GetObjectField(tbo2, port_fld);
		if ( jvmError()) break;

		jvmcfg->env->DeleteLocalRef(fir);
		jvmcfg->env->DeleteLocalRef(sec);
		jvmcfg->env->DeleteLocalRef(tbo1);
		jvmcfg->env->DeleteLocalRef(tbo2);
	}
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		/* indic 指向一个Document */
	case Notitia::TIMER:
		/* indic 指向一个Integer */
	{
		jobject doc = jvmcfg->env->GetObjectField(ps_obj, indic_fld);
		jvmcfg->env->DeleteLocalRef(doc);
	}
		break;	

	case Notitia::MAIN_PARA:
		/* indic 指向一个String[] */
		indic = (jobjectArray) jvmcfg->env->GetObjectField(ps_obj, indic_fld);
		if ( !indic ) break;

		loopi = jvmcfg->env->GetArrayLength(indic); 
		while ( loopi >0 )
		{
			sm_obj = jvmcfg->env->GetObjectArrayElement(indic, loopi-1);
			jvmcfg->env->DeleteLocalRef(sm_obj);
			--loopi;
		}

		break;

	case Notitia::CMD_HTTP_GET:
		/* indic 指向一个指针struct GetRequestCmd* */
		break;
	
	case Notitia::CMD_HTTP_SET:
		/* indic 指向一个指针struct SetResponseCmd* */
		break;

	default :
		break;
	}

	jvmcfg->env->DeleteLocalRef(ps_obj);
}

jobject getIntegerObj(JNIEnv *env, int val) 
{
	jobject jInt;
	jclass integer_cls;
	jmethodID intInit_mid;

	integer_cls = env->FindClass("java/lang/Integer");
	intInit_mid = env->GetMethodID(integer_cls, "<init>", "(I)V");

	jInt = env->NewObject(integer_cls, intInit_mid, val);
	if ( jvmError(env) ) return 0;
	return jInt;
}

void toInt (JNIEnv *env, int *val, jobject jInt )
{
	jclass integer_cls;
	jmethodID getInt_mid;

	integer_cls = env->FindClass("java/lang/Integer");

	getInt_mid = env->GetMethodID(integer_cls, "intValue", "()I");
	*val = env->CallIntMethod(jInt, getInt_mid);
	jvmError(env);
}
