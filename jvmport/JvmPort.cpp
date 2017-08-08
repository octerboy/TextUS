/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
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
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textor_jvmport_TiXML.h"
#include "textor_jvmport_Amor.h"
#include "textor_jvmport_PacketData.h"
#include "textor_jvmport_DBFace.h"
#include "PacData.h"
#include "textor_jvmport_TBuffer.h"
#include "TBuffer.h"
#include "DBFace.h"
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
static void toJFace (JNIEnv *env, DBFace *dface, jobject face_obj, jclass face_cls, const char *encoding);

static jobject getIntegerObj(JNIEnv *env, int val);
static void toInt (JNIEnv *env, int *val, jobject jInt );

static void getPiusIndic (JNIEnv *env,  Amor::Pius &pius, jobject ps, jobject amr);
static void freePiusIndic (Amor::Pius &pius);

typedef struct _FaceList {
		DBFace *face;
		jobject face_obj;
		struct _FaceList *prev, *next;

		inline _FaceList () {
			prev = 0;
			next = 0;
			face = 0;
			face_obj = 0;
		};

		inline void put ( struct _FaceList *neo ) 
		{
			if( !neo ) return;
			neo->next = next;
			neo->prev = this;
			if ( next != 0 )
				next->prev = neo;
			next = neo;
		};

		inline jobject look(DBFace* me) 
		{
			struct _FaceList *obj = 0;
	
			for ( obj = next; obj; obj = obj->next )
			{
				if ( obj->face == me )
					break;
			}
			if ( !obj ) return 0;
			return obj->face_obj;
		};

} FaceList;

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

			//vm_args.version=JNI_VERSION_1_8;/* 这个字段必须设置为该值, 版本号设置不能漏*/
			vm_args.version=JNI_VERSION_1_2;
			vm_args.ignoreUnrecognized = JNI_FALSE;;
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
#ifdef JNI_VERSION_1_6
				if (strcmp(comm_str, "1.6") == 0 )
					vm_args.version=JNI_VERSION_1_6;
#endif
#ifdef JNI_VERSION_1_8
				if (strcmp(comm_str, "1.8") == 0 )
					vm_args.version=JNI_VERSION_1_8;
#endif
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
				//printf("args %d %s\n", strlen(vm_args.options[k].optionString),vm_args.options[k].optionString);
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
		FaceList f_list;

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
	bool pius2Java( Pius *ps, jmethodID mid);
	
	//PacketObj *rcv_pac, *snd_pac;	/* 来自左节点的PacketObj */
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
		res = JNI_GetDefaultJavaVMInitArgs(&(jvmcfg->vm_args));
		if (res != JNI_OK)
			return;
		//printf("vm_args.nOptions %d\n", jvmcfg->vm_args.nOptions);
		res = JNI_CreateJavaVM(&(jvmcfg->jvm), (void**)&(jvmcfg->env), &(jvmcfg->vm_args));
		if (res != JNI_OK)
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
	jobject document;
	assert(pius);
	assert(jvmcfg);
	jint jret;
	//PacketObj **tmp;
	if ( !jvmcfg->env )
	{
		WLOG(ERR,"JVM not created!");
		return false;
	}

	switch ( pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		if ( !gCFG->bean_cls)
		{
  			WLOG(ERR, "Not found class of %s", gCFG->cls_name);
			break;
		}

		gCFG->bean_init = jvmcfg->env->GetMethodID(gCFG->bean_cls, "<init>", "()V");

		gCFG->pius_cls = jvmcfg->env->FindClass("textor/jvmport/Pius");
		if ( !gCFG->pius_cls)
		{
			jvmError();
  			WLOG(ERR, "Not found class of textor.jvmport.Pius");
			break;
		}

		gCFG->amor_cls = jvmcfg->env->FindClass("textor/jvmport/Amor");
		if ( !gCFG->amor_cls) {
			jvmError();
  			WLOG(ERR, "Not found class of textor.jvmport.Amor");
			break;
		}

		gCFG->amor_init = jvmcfg->env->GetMethodID(gCFG->amor_cls, "<init>", "()V");

		gCFG->facio_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "facio", "(Ltextor/jvmport/Pius;)Z");
		if ( !gCFG->facio_mid ) {
			jvmError();
			WLOG(ERR,"not found method facio(Pius)");
			break;
		}

		gCFG->sponte_mid = jvmcfg->env->GetMethodID(gCFG->bean_cls, "sponte", "(Ltextor/jvmport/Pius;)Z");
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
				document = getDocumentObj(jvmcfg->env, gCFG->xmlstr, gCFG->encoding);
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
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		return facioJava(pius);
		break;

	case Notitia::JUST_START_THREAD:
		WBUG("facio JUST_START_THREAD");
		if ( JNI_OK == (jret = jvmcfg->jvm->AttachCurrentThread((void**)&(jvmcfg->env), 0) ))
		{
			WBUG("JVM AttachCurrentThread OK.");
			return facioJava(pius);
		} else {
			WLOG(EMERG, "JVM AttachCurrentThread failed! error=%d", jret);
		}
		break;

	case Notitia::FINAL_END_THREAD:
		WBUG("facio FINAL_END_THREAD");
		facioJava(pius);
		if ( JNI_OK == (jret = jvmcfg->jvm->DetachCurrentThread() ))
		{
			WBUG("JVM DetachCurrentThread  OK.");
		} else {
			WLOG(EMERG, "JVM DetachCurrentThread failed! error=%d", jret);
		}
		break;

/*
	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else {
				WBUG("facio SET_UNIPAC rcv_pac null");
			}
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else {
				WBUG("facio SET_UNIPAC snd_pac null");
			}

		} else {
			WBUG("facio SET_UNIPAC null");
		}
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");

		snd_pac->input(4, "166", 3);
		snd_pac->input(5, "TEST", 4);
		snd_pac->input(6, "B9E3B6AB440100011603440186015001008810382000010120100101DC41303030303100000000010000010100", 90);
		aptus->sponte(pius);
		break;
	case Notitia::CMD_SET_DBFACE:
		WBUG("facio CMD_SET_DBFACE");
		break;
*/
	default:
		WBUG("facio %ld in JvmPort", pius->ordo);
		return facioJava(pius);
	}
	return true;
}

bool JvmPort::sponte( Amor::Pius *pius) 
{
	bool ret = false;
	assert(pius);
	assert(jvmcfg);
	if ( !jvmcfg->env )
	{
		WLOG(ERR,"JVM not created!");
		return false;
	}
	if ( gCFG->sponte_mid && gCFG->pius_cls && owner_obj)
	{
		ret = pius2Java(pius, gCFG->sponte_mid);
		if( jvmError() ) return false;
	}
	return ret;
}

bool JvmPort::facioJava( Amor::Pius *pius)
{
	bool ret = false;
	if ( gCFG->facio_mid && gCFG->pius_cls && owner_obj )
	{
		ret = pius2Java(pius, gCFG->facio_mid);
		if( jvmError() ) return false;
	}
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
		own_obj = jvmcfg->env->NewObject(gCFG->bean_cls, gCFG->bean_init);
	apt_fld   = jvmcfg->env->GetFieldID(gCFG->bean_cls, "aptus", "Ltextor/jvmport/Amor;");
	apt_obj = jvmcfg->env->NewObject(gCFG->amor_cls, gCFG->amor_init);
	if ( jvmError() || !apt_fld || !own_obj || !apt_obj)
	{
		WLOG(ERR,"not found member aptus");
		return false;
	} else {
		jvmcfg->env->SetObjectField(own_obj, apt_fld, apt_obj);

		ptr_fld = jvmcfg->env->GetFieldID(gCFG->amor_cls, "portPtr", "[B");
		/* Java的Amor对象中的portPtr, 即为本对象的指针 */
		selfPtr = jvmcfg->env->NewByteArray(sizeof(me));
		jvmcfg->env->SetByteArrayRegion(selfPtr, 0, sizeof(me), (jbyte*)&me);
		jvmcfg->env->SetObjectField(apt_obj, ptr_fld, selfPtr);
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
	env->DeleteLocalRef(port);
	env->DeleteLocalRef(o_cls);
	return pointer;
}

static void setPointer (JNIEnv *env,  jobject obj, void *p)
{
	jclass o_cls = env->GetObjectClass(obj); 
	jfieldID port_fld = env->GetFieldID(o_cls, "portPtr", "[B");	
	jbyteArray port =  buf2ba(env, (unsigned char*)&p , sizeof(p));
	env->SetObjectField(obj, port_fld, (jobject)port);
	env->DeleteLocalRef(o_cls);
	return ;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_TiXML_getDocument (JNIEnv *env, jobject tio)
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

JNIEXPORT void JNICALL Java_textor_jvmport_TiXML_putDocument (JNIEnv *env, jobject tio, jobject jdoc)
{
	TiXmlDocument *docp = (TiXmlDocument *)getPointer(env, tio);
	if ( !docp || !jdoc) return ;

	toDocument(env, docp, jdoc);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_TiXML_alloc (JNIEnv *env , jobject tbo)
{
	TiXmlDocument *neo = new TiXmlDocument();
	setPointer(env, tbo, neo);
}

JNIEXPORT void JNICALL Java_textor_jvmport_TiXML_free
  (JNIEnv *env, jobject tbo)
{
	TiXmlDocument *docp = (TiXmlDocument *)getPointer(env, tbo);
	if ( docp)
		delete docp;
}

JNIEXPORT jboolean JNICALL Java_textor_jvmport_Amor_facio (JNIEnv *env, jobject amor, jobject ps) 
{
	Amor::Pius pius;
	Amor *port;
	jboolean ret;
	
	port = (Amor*) getPointer (env,  amor);
	if ( !port )
		return false;

	getPiusIndic (env, pius, ps, amor);
	ret = (jboolean) port->aptus->facio(&pius);
	if ( (TEST_NOTITIA_FLAG & pius.ordo) == JAVA_NOTITIA_DOM )
	{
		env->DeleteLocalRef((jobject)pius.indic);
		return ret;
	}
	freePiusIndic(pius);
	return ret;
}

JNIEXPORT jboolean JNICALL Java_textor_jvmport_Amor_sponte (JNIEnv *env, jobject amor, jobject ps) 
{
	Amor::Pius pius;
	Amor *port;
	jboolean ret;
	
	port = (Amor*) getPointer (env,  amor);
	if ( !port )
		return false;

	getPiusIndic (env, pius, ps, amor);

	ret = (jboolean) port->aptus->sponte(&pius);
	if ( (TEST_NOTITIA_FLAG & pius.ordo) == JAVA_NOTITIA_DOM )
	{
		env->DeleteLocalRef((jobject)pius.indic);
		return ret;
	}
	freePiusIndic(pius);
	return ret;
}

JNIEXPORT void JNICALL Java_textor_jvmport_Amor_log (JNIEnv *env, jobject amor, jlong ordo, jstring jmsg)
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
	pius.subor = 0;
	
	port->aptus->sponte(&pius);
	env->ReleaseStringUTFChars(jmsg, msg);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_alloc
  (JNIEnv *env, jobject paco) {
	struct PacketObj *neo = new  struct PacketObj();
	setPointer (env,  paco, neo);
	return ;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_produce (JNIEnv *env, jobject paco, jint fld_num) 
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		pcp->produce(fld_num);
	return ;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_reset (JNIEnv *env, jobject paco)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		pcp->reset();
	return ;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_free (JNIEnv *env, jobject paco)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
		delete pcp;
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_grant (JNIEnv *env, jobject paco, jint space)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	//printf("in jni pacdata %08x\n", (unsigned int)pcp);
	if ( pcp)
		pcp->grant(space);
	return;
}

JNIEXPORT jbyteArray JNICALL Java_textor_jvmport_PacketData_getfld (JNIEnv *env, jobject paco, jint no)
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

JNIEXPORT jstring JNICALL Java_textor_jvmport_PacketData_getString (JNIEnv *env, jobject paco, jint no)
{
	jbyteArray reta;
	jstring jstr =NULL;
	jmethodID strInit_mid;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass str_cls = env->FindClass("java/lang/String");
	strInit_mid = env->GetMethodID(str_cls, "<init>", "([B)V");
	reta = 0;
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val)
		{
			reta =  buf2ba(env, val, (int) len);
			jstr = (jstring) env->NewObject(str_cls, strInit_mid, reta);
			env->DeleteLocalRef(reta);
		}
	}
	env->DeleteLocalRef(str_cls);
	return jstr;
}

JNIEXPORT jint JNICALL Java_textor_jvmport_PacketData_getInt (JNIEnv *env, jobject paco, jint no)
{
	jint iVal = 0;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && sizeof(jint) == len)
		{
			memcpy(&iVal, val, len);
		}
	}
	return iVal;
}

JNIEXPORT jlong JNICALL Java_textor_jvmport_PacketData_getLong (JNIEnv *env, jobject paco, jint no)
{
	jlong lVal = 0;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if (pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && sizeof(jlong) == len)
		{
			memcpy(&lVal, val, len);
		}
	}
	return lVal;
}

JNIEXPORT jboolean JNICALL Java_textor_jvmport_PacketData_getBool (JNIEnv *env, jobject paco, jint no)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jboolean ret = JNI_TRUE;
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && len == 4)
		{
			if (strncasecmp((const char*)val, "true",4) ==0 )
				ret = JNI_TRUE;
		}
		if ( val && len == 5)
		{
			if (strncasecmp((const char*)val, "false",5) ==0 )
				ret = JNI_FALSE;
		}
		if ( val && len == sizeof(jboolean))
		{
			memcpy(&ret, val, sizeof(jboolean));
		}
	}
	return ret;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_PacketData_getBoolean (JNIEnv *env, jobject paco, jint no)
{
	jmethodID bInit_mid;
	jobject ret_o;
	jclass b_cls = env->FindClass("java/lang/Boolean");
	bInit_mid = env->GetMethodID(b_cls, "<init>", "(Z)V");
	if ( b_cls != 0 &&  bInit_mid != 0) 
		ret_o =  env->NewObject(b_cls, bInit_mid, Java_textor_jvmport_PacketData_getBool(env, paco, no));
	else
		ret_o = 0;
	env->DeleteLocalRef(b_cls);
	return ret_o;
}

JNIEXPORT jshort JNICALL Java_textor_jvmport_PacketData_getShort (JNIEnv *env, jobject paco, jint no)
{
	jshort sVal = 0;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && sizeof(jshort) == len)
		{
			memcpy(&sVal, val, len);
		}
	}
	return sVal;
}

JNIEXPORT jfloat JNICALL Java_textor_jvmport_PacketData_getFloat (JNIEnv *env, jobject paco, jint no)
{
	jfloat fVal = 0;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && sizeof(jfloat) == len)
		{
			memcpy(&fVal, val, len);
		}
	}
	return fVal;
}

JNIEXPORT jdouble JNICALL Java_textor_jvmport_PacketData_getDouble (JNIEnv *env, jobject paco, jint no)
{
	jdouble dVal = 0;
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( val && sizeof(jdouble) == len)
		{
			memcpy(&dVal, val, len);
		}
	}
	return dVal;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_PacketData_getBigDecimal (JNIEnv *env, jobject paco, jint no)
{
	jclass b_cls = env->FindClass("java/math/BigDecimal");
	jmethodID bInit_mid = env->GetMethodID(b_cls, "<init>", "(Ljava/math/BigInteger;I)V");
	jclass bi_cls = env->FindClass("java/math/BigInteger");
	jmethodID biInit_mid = env->GetMethodID(bi_cls, "<init>", "([B)V");
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jint scale;
	jbyteArray bi_arr;
	jobject bigInter, biDec;
	biDec = NULL;

	if ( pcp)
	{
		unsigned long len;
		unsigned char *val;
		val = pcp->getfld(no, &len);
		if ( !val || len <= 4 ) 
			return NULL;
		memcpy(&scale, val, 4);
		bi_arr =  buf2ba(env, &val[len-4], len-4);
		bigInter = env->NewObject(bi_cls, biInit_mid, bi_arr);
		biDec = env->NewObject(b_cls, bInit_mid, bigInter, scale);
		env->DeleteLocalRef(bi_arr);
		env->DeleteLocalRef(bigInter);
	}
	env->DeleteLocalRef(bi_cls);
	env->DeleteLocalRef(b_cls);
	return biDec;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_PacketData_getDate (JNIEnv *env, jobject paco, jint no)
{
	jmethodID dInit_mid;
	jobject ret;
	jclass d_cls = env->FindClass("java/sql/Date");
	dInit_mid = env->GetMethodID(d_cls, "<init>", "(J)V");
	if ( d_cls != 0 &&  dInit_mid != 0) 
		ret =  env->NewObject(d_cls, dInit_mid, Java_textor_jvmport_PacketData_getLong(env, paco, no));
	else
		ret = 0;
	env->DeleteLocalRef(d_cls);
	return ret;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_PacketData_getTime (JNIEnv *env, jobject paco, jint no)
{
	jmethodID tInit_mid;
	jobject ret;
	jclass t_cls = env->FindClass("java/sql/Time");
	tInit_mid = env->GetMethodID(t_cls, "<init>", "(J)V");
	if ( t_cls != 0 &&  tInit_mid != 0) 
		ret = env->NewObject(t_cls, tInit_mid, Java_textor_jvmport_PacketData_getLong(env, paco, no));
	else
		ret = 0;

	env->DeleteLocalRef(t_cls);
	return ret;
}

JNIEXPORT jobject JNICALL Java_textor_jvmport_PacketData_getTimestamp (JNIEnv *env, jobject paco, jint no)
{
	jmethodID tInit_mid;
	jobject ret;
	jclass t_cls = env->FindClass("java/sql/Timestamp");
	tInit_mid = env->GetMethodID(t_cls, "<init>", "(J)V");
	if ( t_cls != 0 &&  tInit_mid != 0) 
		ret = env->NewObject(t_cls, tInit_mid, Java_textor_jvmport_PacketData_getLong(env, paco, no));
	else
		ret = 0;

	env->DeleteLocalRef(t_cls);
	return ret;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__I_3B (JNIEnv *env, jobject paco, jint no , jbyteArray val)
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

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__IB (JNIEnv *env, jobject paco, jint no, jbyte val)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&val, sizeof(jbyte));
	}
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__II (JNIEnv *env, jobject paco, jint no , jint iVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&iVal, sizeof(jint));
	}
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__IJ (JNIEnv *env, jobject paco, jint no, jlong jVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&jVal, sizeof(jlong));
	}
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_lang_String_2 (JNIEnv *env, jobject paco, jint no , jstring sVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass str_cls = env->FindClass("java/lang/String");
	jmethodID getBytes_mid = env->GetMethodID(str_cls, "getBytes", "()[B");
	jbyteArray strBytes;
	int len;
	
	if (!sVal ) return;
	if ( pcp)
	{
		strBytes = (jbyteArray)env->CallObjectMethod(sVal, getBytes_mid);
		len = env->GetArrayLength(strBytes);
		pcp->grant(len);
		env->GetByteArrayRegion(strBytes, 0, len, (jbyte*)pcp->buf.point);
		pcp->commit(no, len);
		env->DeleteLocalRef(strBytes);
	}
	env->DeleteLocalRef(str_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_lang_Boolean_2 (JNIEnv *env, jobject paco, jint no, jobject bVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass bl_cls = env->FindClass("java/lang/Boolean");
	jmethodID getVal_mid = env->GetMethodID(bl_cls, "booleanValue", "()Z");
	jboolean *yes;
	int len;

	if ( pcp)
	{
		yes = (jboolean*)env->CallObjectMethod(bVal, getVal_mid);
		pcp->grant(sizeof(jboolean));
		pcp->input(no, (unsigned char*)yes, sizeof(jboolean));
	}
	env->DeleteLocalRef(bl_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__IZ (JNIEnv *env, jobject paco, jint no, jboolean blVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&blVal, sizeof(jboolean));
	}
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__IS (JNIEnv *env, jobject paco, jint no, jshort sVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&sVal, sizeof(jshort));
	}
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__IF (JNIEnv *env, jobject paco, jint no, jfloat fVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&fVal, sizeof(jfloat));
	}
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ID (JNIEnv *env, jobject paco, jint no, jdouble dVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	if ( pcp)
	{
		pcp->input(no, (unsigned char*)&dVal, sizeof(jdouble));
	}
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_math_BigDecimal_2 (JNIEnv *env, jobject paco, jint no, jobject dVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass bd_cls = env->FindClass("java/math/BigDecimal");
	jclass bi_cls = env->FindClass("java/math/BigInteger");
	jmethodID biGetBytes_mid = env->GetMethodID(bi_cls, "toByteArray", "()[B");
	jbyteArray bi_bytes;
	jmethodID getScale_mid = env->GetMethodID(bd_cls, "scale", "()I");
	jmethodID getUnScale_mid = env->GetMethodID(bd_cls, "unscaledValue", "())Ljava/math/BigDecimal;");
	jint *scale;
	jobject bigInt;
	int len;

	if ( pcp)
	{
		scale = (jint*)env->CallObjectMethod(dVal, getScale_mid);
		bigInt = env->CallObjectMethod(dVal, getUnScale_mid);
		bi_bytes = (jbyteArray) env->CallObjectMethod(bigInt, biGetBytes_mid);
		len = env->GetArrayLength(bi_bytes);
		pcp->grant(sizeof(jint)+len);
		env->GetByteArrayRegion(bi_bytes, 0, len, (jbyte*)&(pcp->buf.point[sizeof(jint)]));
		memcpy(pcp->buf.point, scale, sizeof(jint));
                pcp->commit(no, sizeof(jint)+len);
		env->DeleteLocalRef(bi_bytes);
	}
	env->DeleteLocalRef(bd_cls);
	env->DeleteLocalRef(bi_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_sql_Date_2 (JNIEnv *env, jobject paco, jint no, jobject dtVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass d_cls = env->FindClass("java/sql/Date");
	jmethodID getVal_mid = env->GetMethodID(d_cls, "getTime", "()J");
	jlong *time;
	if ( pcp)
	{
		time = (jlong*)env->CallObjectMethod(dtVal, getVal_mid);
		pcp->grant(sizeof(jlong));
		pcp->input(no, (unsigned char*)time, sizeof(jlong));
	}
	env->DeleteLocalRef(d_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_sql_Time_2 (JNIEnv *env, jobject paco, jint no, jobject tmVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass d_cls = env->FindClass("java/sql/Time");
	jmethodID getVal_mid = env->GetMethodID(d_cls, "getTime", "()J");
	jlong *time;
	if ( pcp)
	{
		time = (jlong*)env->CallObjectMethod(tmVal, getVal_mid);
		pcp->grant(sizeof(jlong));
		pcp->input(no, (unsigned char*)time, sizeof(jlong));
	}
	env->DeleteLocalRef(d_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_PacketData_input__ILjava_sql_Timestamp_2 (JNIEnv *env, jobject paco, jint no, jobject stmVal)
{
	struct PacketObj *pcp = (struct PacketObj *) getPointer(env,paco);
	jclass d_cls = env->FindClass("java/sql/Timestamp");
	jmethodID getVal_mid = env->GetMethodID(d_cls, "getTime", "()J");
	jlong *time;
	if ( pcp)
	{
		time = (jlong*)env->CallObjectMethod(stmVal, getVal_mid);
		pcp->grant(sizeof(jlong));
		pcp->input(no, (unsigned char*)time, sizeof(jlong));
	}
	env->DeleteLocalRef(d_cls);
	return;
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_alloc
  (JNIEnv *env , jobject tbo, jint size)
{
	TBuffer *neo = new TBuffer(size);
	setPointer(env, tbo, neo);
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_free
  (JNIEnv *env, jobject tbo)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp)
		delete tbp;
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_grant
  (JNIEnv * env, jobject tbo, jint size) 
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	//printf("in jnitbuffer %08x\n", tbp);
	if ( tbp)
		tbp->grant(size);
}

JNIEXPORT jint JNICALL Java_textor_jvmport_TBuffer_commit
  (JNIEnv *env, jobject tbo, jbyteArray ptr, jint len)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp ) 
		return tbp->commit(len);
	else 
		return 0;
	
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_reset
  (JNIEnv *env, jobject tbo, jbyteArray ptr)
{
	TBuffer *tbp = (TBuffer *)getPointer(env, tbo);
	if ( tbp )
		tbp->reset();
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_exchange
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::exchange(*tbpa, *tbpb);
	return ;
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_pour___3B_3B
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::pour(*tbpa, *tbpb);
	return ;
}

JNIEXPORT void JNICALL Java_textor_jvmport_TBuffer_pour___3B_3BI
  (JNIEnv *env, jclass tb_cls, jobject tba, jobject tbb, jint len)
{
	TBuffer *tbpa = (TBuffer *)getPointer(env, tba);
	TBuffer *tbpb = (TBuffer *)getPointer(env, tbb);
	if ( tbpa && tbpb)
		TBuffer::pour(*tbpa, *tbpb, len);
	return ;
}

JNIEXPORT jbyteArray JNICALL Java_textor_jvmport_TBuffer_getBytes
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
	jobject jstr, encStr ;
	jobject factory, docBuilder, document;
	jclass str_cls, dbFac_cls, doc_cls, docBuild_cls;
	jmethodID strInit_mid, getBytes_mid, newFac_mid, newDocBuilder_mid, parse_mid;

	jclass strRd_cls, inputSrc_cls;
	jmethodID strRdInit_mid, inputSrcInit_mid;
	jobject strRd_obj, inputSrc_obj;

	str_cls = env->FindClass("java/lang/String");
	dbFac_cls = env->FindClass("javax/xml/parsers/DocumentBuilderFactory");
	docBuild_cls = env->FindClass("javax/xml/parsers/DocumentBuilder");
	doc_cls = env->FindClass("org/w3c/dom/Document");
	strRd_cls = env->FindClass("java/io/StringReader");
	inputSrc_cls = env->FindClass("org/xml/sax/InputSource");
	if ( jvmError(env) )
		return 0;

	strInit_mid = env->GetMethodID(str_cls, "<init>", "([BLjava/lang/String;)V");
	strRdInit_mid = env->GetMethodID(strRd_cls, "<init>", "(Ljava/lang/String;)V");
	inputSrcInit_mid = env->GetMethodID(inputSrc_cls, "<init>", "(Ljava/io/Reader;)V");
	if ( jvmError(env) )
		return 0;

	getBytes_mid = env->GetMethodID(str_cls, "getBytes", "(Ljava/lang/String;)[B");
	newFac_mid = env->GetStaticMethodID(dbFac_cls, "newInstance", "()Ljavax/xml/parsers/DocumentBuilderFactory;");
	newDocBuilder_mid = env->GetMethodID(dbFac_cls, "newDocumentBuilder", "()Ljavax/xml/parsers/DocumentBuilder;");
	parse_mid = env->GetMethodID(docBuild_cls, "parse", "(Lorg/xml/sax/InputSource;)Lorg/w3c/dom/Document;");
				
	if ( jvmError(env) )
		return 0;
				
	encStr = env->NewStringUTF(encoding);
			
	/* 生成String对象jstr, 即其xml */	
	len = strlen(xmlstr);
	args =  env->NewByteArray(len);
	env->SetByteArrayRegion(args, 0, len, (jbyte*)xmlstr);
	jstr = env->NewObject(str_cls, strInit_mid, args, encStr);
	
	strRd_obj = env->NewObject(strRd_cls, strRdInit_mid, jstr);
	inputSrc_obj = env->NewObject(inputSrc_cls, inputSrcInit_mid, strRd_obj);
	
	factory = env->CallStaticObjectMethod(dbFac_cls, newFac_mid);
	docBuilder = env->CallObjectMethod(factory, newDocBuilder_mid);
	document = env->CallObjectMethod(docBuilder, parse_mid, inputSrc_obj);

	if ( jvmError(env)) return 0;
			
	env->DeleteLocalRef(encStr);
	env->DeleteLocalRef(args);
	env->DeleteLocalRef(jstr);

	env->DeleteLocalRef(strRd_obj);
	env->DeleteLocalRef(inputSrc_obj);
	env->DeleteLocalRef(factory);
	env->DeleteLocalRef(docBuilder);

	env->DeleteLocalRef(str_cls);
	env->DeleteLocalRef(dbFac_cls);
	env->DeleteLocalRef(docBuild_cls);
	env->DeleteLocalRef(doc_cls);
	env->DeleteLocalRef(strRd_cls);
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

	env->DeleteLocalRef(tFac_cls);
	env->DeleteLocalRef(trans_cls);
	env->DeleteLocalRef(dsrc_cls);
	env->DeleteLocalRef(bos_cls);
	env->DeleteLocalRef(rslt_cls);
	delete[] buf;

	return;
}

/* 从Java程序到C++程序, 在调用C++程序时, 生成相应的indic指针 */
static void getPiusIndic (JNIEnv *env,  Amor::Pius &pius, jobject ps, jobject amor)
{
	jclass pius_cls;
	jfieldID ordo_fld, indic_fld, sub_fld;

	pius.ordo = Notitia::TEXTUS_RESERVED;
	pius.indic = 0;
	pius.subor = 0;
	pius_cls = env->FindClass("textor/jvmport/Pius");
	if ( jvmError(env) )
		return;

	ordo_fld = env->GetFieldID(pius_cls, "ordo", "J");
	sub_fld = env->GetFieldID(pius_cls, "subor", "I");
	indic_fld = env->GetFieldID(pius_cls, "indic", "Ljava/lang/Object;");
	pius.ordo = env->GetLongField(ps, ordo_fld);
	pius.subor = env->GetIntField(ps, sub_fld);

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
			tbuf_cls = env->FindClass("textor/jvmport/PacketData");
		else if ( pius.ordo == Notitia::SET_TBUF ||  pius.ordo == Notitia::PRO_TBUF )
			tbuf_cls = env->FindClass("textor/jvmport/TBuffer");
		else
			tbuf_cls = env->FindClass("textor/jvmport/TiXML");
			
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
		env->DeleteLocalRef(tbo1);
		env->DeleteLocalRef(tbo2);
		env->DeleteLocalRef(tbuf_cls);
		env->DeleteLocalRef(indic);
		env->DeleteLocalRef(fir);
		env->DeleteLocalRef(sec);
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

	case Notitia::CMD_SET_DBFACE:	
		/* 这个由DBPort发出, 不会有Java到C++的情况*/
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
	env->DeleteLocalRef(pius_cls);
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

/* 从C++程序到Java程序, 为Java程序生成合适的Pius对象, and facio or sponte */
bool JvmPort::pius2Java (Pius *pius, jmethodID fs_mid)
{
	jobject ps_obj;
	bool fs_ret = false;
	jmethodID psInit;
	jfieldID ordo_fld, indic_fld, sub_fld;
	jobjectArray indic;

	psInit = jvmcfg->env->GetMethodID(gCFG->pius_cls, "<init>", "()V");
	ps_obj = jvmcfg->env->NewObject(gCFG->pius_cls, psInit);

	ordo_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "ordo", "J");
	sub_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "subor", "I");
	jvmcfg->env->SetLongField(ps_obj, ordo_fld, pius->ordo);		/* java的pius的ordo已设定 */
	jvmcfg->env->SetIntField(ps_obj, sub_fld, pius->subor);		/* java的pius的subor已设定 */

	indic_fld = jvmcfg->env->GetFieldID(gCFG->pius_cls, "indic", "Ljava/lang/Object;");
	if ( jvmError() )
		return false;

	if ( (TEST_NOTITIA_FLAG & pius->ordo) == JAVA_NOTITIA_DOM )
	{
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, (jobject)(pius->indic));		/* 对于Java的Object直接设置 */
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
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
				tbuf_cls = jvmcfg->env->FindClass("textor/jvmport/TBuffer");
			else if ( pius->ordo == Notitia::SET_UNIPAC)
				tbuf_cls = jvmcfg->env->FindClass("textor/jvmport/PacketData");
			else 
				tbuf_cls = jvmcfg->env->FindClass("textor/jvmport/TiXML");

			if ( jvmError()) break;
			port_fld  = jvmcfg->env->GetFieldID(tbuf_cls, "portPtr", "[B");
			if ( jvmError()) break;

			/* 这里, tbuf_cls也指 PacketData.class 和 TiXML.class */
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
			fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
			jvmcfg->env->DeleteLocalRef(fir);
			jvmcfg->env->DeleteLocalRef(sec);
			jvmcfg->env->DeleteLocalRef(tbo1);
			jvmcfg->env->DeleteLocalRef(tbo2);
			jvmcfg->env->DeleteLocalRef(indic);
			jvmcfg->env->DeleteLocalRef(tbuf_cls);
			//printf("in jvmport %08x %08x\n", first, second);
		}
	}
		break;

	case Notitia::CMD_SET_DBFACE:	
	{
		jclass face_cls;
		jobject face_obj;

		face_obj = gCFG->f_list.look((DBFace*)pius->indic);
		if ( !face_obj ) 
		{
			face_cls = jvmcfg->env->FindClass("textor/jvmport/DBFace");
			FaceList *neo = new FaceList();
			face_obj = jvmcfg->env->AllocObject(face_cls); 
			toJFace (jvmcfg->env, (DBFace*)pius->indic, face_obj, face_cls, gCFG->encoding);
			neo->face = (DBFace*)pius->indic;
			neo->face_obj = face_obj;
			gCFG->f_list.put(neo);
			WBUG("new face %p, face_obj %p", (DBFace*)pius->indic, face_obj);
		}  else {
			WBUG("find face %p, face_obj %p", (DBFace*)pius->indic, face_obj);
		}
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, face_obj);
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
	}
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
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
		jvmcfg->env->DeleteLocalRef(document);
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
		int num,i;
		char **argv;
		void **ps;
		jstring *jv;
		jclass str_cls = jvmcfg->env->FindClass("java/lang/String");
		jobjectArray strArr;

		ps = (void**)pius->indic;
 		num = (*(int *)ps[0]);
		argv = (char **)ps[1];
		strArr = jvmcfg->env->NewObjectArray(num, str_cls, 0);
		jv = new jstring[num];
		for ( i = 0 ; i < num; i++)
		{
			jv[i] = jvmcfg->env->NewStringUTF(argv[i]);
			jvmcfg->env->SetObjectArrayElement(strArr, i, jv[i]);
		}
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, strArr);
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
		for ( i = 0 ; i < num; i++)
		{
			jvmcfg->env->DeleteLocalRef(jv[i]);
		}
		jvmcfg->env->DeleteLocalRef(str_cls);
		jvmcfg->env->DeleteLocalRef(strArr);
	}
		break;

	case Notitia::TIMER:
		/* 这些要转一个java.lang.Integer */
	{
		jobject integer;

		integer = getIntegerObj(jvmcfg->env,*((int*) (pius->indic)));
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, integer);
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
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
	case Notitia::START_SERVICE:
		/* 这些本来就是不需要indic的 */
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, 0);
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
		break;

	default :
		jvmcfg->env->SetObjectField(ps_obj, indic_fld, 0);
		fs_ret = (bool) jvmcfg->env->CallBooleanMethod(owner_obj, fs_mid, ps_obj);
		break;
	}

RETURN:
	jvmcfg->env->DeleteLocalRef(ps_obj);
	return fs_ret;
}

jobject getIntegerObj(JNIEnv *env, int val) 
{
	jobject jInt;
	jclass integer_cls;
	jmethodID intInit_mid;

	integer_cls = env->FindClass("java/lang/Integer");
	intInit_mid = env->GetMethodID(integer_cls, "<init>", "(I)V");

	jInt = env->NewObject(integer_cls, intInit_mid, val);
	env->DeleteLocalRef(integer_cls);
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
	env->DeleteLocalRef(integer_cls);
	jvmError(env);
}

void toJFace (JNIEnv *env, DBFace *dface, jobject face_obj, jclass face_cls,  const char *encoding)
{
	jobject jstr, encStr, rowset_obj;
	jbyteArray  args;
	int len; 
	jmethodID strInit_mid;
	jclass str_cls = env->FindClass("java/lang/String");
	jclass rowset_cls = env->FindClass("textor/jvmport/DBFace$RowSet");
	if ( jvmError(env)) return;
	jclass dbpara_cls = env->FindClass("textor/jvmport/DBFace$Para");
	if ( jvmError(env)) return;

	strInit_mid = env->GetMethodID(str_cls, "<init>", "([BLjava/lang/String;)V");
	encStr = env->NewStringUTF(encoding);
	
#define OBJ_SET_INT(X,Y) \
	env->SetIntField(face_obj, env->GetFieldID(face_cls, X, "I"), dface->Y);

#define OBJ_SET_STR(X,Y) \
	if ( dface->Y == 0 ) { \
		env->SetObjectField(face_obj, env->GetFieldID(face_cls, X, "Ljava/lang/String;"), 0);	\
	} else {	\
		len = strlen((const char *)dface->Y);	\
		args =  env->NewByteArray(len);		\
		env->SetByteArrayRegion(args, 0, len, (jbyte*)dface->Y);	\
		jstr = env->NewObject(str_cls, strInit_mid, args, encStr);	\
		env->SetObjectField(face_obj, env->GetFieldID(face_cls, X, "Ljava/lang/String;"), jstr);	\
		env->DeleteLocalRef(args);	\
		env->DeleteLocalRef(jstr);	\
	}

#define OBJ_SET_STATIC_INT(X,Y) \
	env->SetIntField(face_obj, env->GetFieldID(face_cls, X, "I"), env->GetStaticIntField(face_cls,  env->GetStaticFieldID(face_cls, Y, "I")));
	OBJ_SET_INT("num", num);
	OBJ_SET_INT("outNum", outNum);
	OBJ_SET_INT("outSize", outSize);
	switch (dface->in )
	{
	case DBFace::FIRST:
		OBJ_SET_STATIC_INT("in","FIRST")
		break;

	case DBFace::SECOND:
		OBJ_SET_STATIC_INT("in","SECOND")
		break;
	}

	switch (dface->out )
	{
	case DBFace::FIRST:
		OBJ_SET_STATIC_INT("out","FIRST")
		break;

	case DBFace::SECOND:
		OBJ_SET_STATIC_INT("out","SECOND")
		break;
	}
	OBJ_SET_STR("sentence", sentence) 
	switch (dface->pro )
	{
	case DBFace::DBPROC:
		OBJ_SET_STATIC_INT("pro","DBPROC")
		break;

	case DBFace::QUERY:
		OBJ_SET_STATIC_INT("pro","QUERY")
		break;

	case DBFace::FUNC:
		OBJ_SET_STATIC_INT("pro","FUNC")
		break;

	case DBFace::FETCH:
		OBJ_SET_STATIC_INT("pro","FETCH")
		break;

	case DBFace::DML:
		OBJ_SET_STATIC_INT("pro","DML")
		break;

	case DBFace::CURSOR:
		OBJ_SET_STATIC_INT("pro","CURSOR")
		break;
	}
	OBJ_SET_STR("id_name", id_name) 
	OBJ_SET_INT("offset", offset);
	OBJ_SET_INT("ref_fldNo", ref.fldNo);
	OBJ_SET_STR("ref_content", ref.content) 
	OBJ_SET_INT("ref_len", ref.len)
	OBJ_SET_INT("cRows_field", cRows_field)
	OBJ_SET_INT("cRowsObt_fld", cRowsObt_fld)
	OBJ_SET_INT("errCode_field", errCode_field)
	OBJ_SET_INT("errStr_field", errStr_field)
	if ( dface->rowset )
	{
		rowset_obj = env->AllocObject(rowset_cls); 
		env->SetObjectField(face_obj, env->GetFieldID(face_cls, "rowset", "Ltextor/jvmport/DBFace$RowSet;"), rowset_obj);
		env->SetIntField(rowset_obj, env->GetFieldID(rowset_cls, "para_pos", "I"), dface->rowset->para_pos);
		env->SetIntField(rowset_obj, env->GetFieldID(rowset_cls, "trace_field", "I"), dface->rowset->trace_field);
		env->SetIntField(rowset_obj, env->GetFieldID(rowset_cls, "chunk", "I"), dface->rowset->chunk);
		env->SetBooleanField(rowset_obj, env->GetFieldID(rowset_cls, "useEnd", "Z"), dface->rowset->useEnd);
	}
	if ( dface->num > 0 )
	{
		jobjectArray para_obj_arr;
		jobject para_obj;
		unsigned int i;
		para_obj_arr = env->NewObjectArray(dface->num, dbpara_cls, 0); 
		for ( i = 0 ; i < dface->num; i++)
		{
#define OBJ_PARA_SET_INT(X,Y) \
	env->SetIntField(para_obj, env->GetFieldID(dbpara_cls, X, "I"), (int)dface->paras[i].Y);

#define OBJ_PARA_SET_STR(X,Y) \
	if ( dface->paras[i].Y ) \
	{	\
	len = strlen((const char *)dface->paras[i].Y);	\
	args =  env->NewByteArray(len);		\
	env->SetByteArrayRegion(args, 0, len, (jbyte*)dface->paras[i].Y);	\
	jstr = env->NewObject(str_cls, strInit_mid, args, encStr);	\
	env->SetObjectField(para_obj, env->GetFieldID(dbpara_cls, X, "Ljava/lang/String;"), jstr);	\
	env->DeleteLocalRef(args);	\
	env->DeleteLocalRef(jstr);	\
	} else {	\
		env->SetObjectField(para_obj, env->GetFieldID(dbpara_cls, X, "Ljava/lang/String;"), 0);	\
	}

#define OBJ_PARA_SET_STATIC_INT(X,Y) \
	env->SetIntField(para_obj, env->GetFieldID(dbpara_cls, X, "I"), env->GetStaticIntField(face_cls,  env->GetStaticFieldID(face_cls, Y, "I")));
			para_obj = env->AllocObject(dbpara_cls);
			OBJ_PARA_SET_INT("pos", pos)
			OBJ_PARA_SET_INT("fld", fld)
			OBJ_PARA_SET_STR("name",name)
			OBJ_PARA_SET_INT("namelen", namelen)
			switch ( dface->paras[i].inout)
			{
			case DBFace::PARA_IN:
				OBJ_PARA_SET_STATIC_INT("inout", "PARA_IN")
				break;
			case DBFace::PARA_OUT:
				OBJ_PARA_SET_STATIC_INT("inout", "PARA_OUT")
				break;
			case DBFace::PARA_INOUT:
				OBJ_PARA_SET_STATIC_INT("inout", "PARA_INOUT")
				break;
			case DBFace::UNKNOWN:
				OBJ_PARA_SET_STATIC_INT("inout", "PARA_UNKNOWN")
				break;
			}
			env->SetLongField(para_obj, env->GetFieldID(dbpara_cls, "outlen", "J"), dface->paras[i].outlen);
			OBJ_PARA_SET_INT("scale", scale)
			OBJ_PARA_SET_INT("precision", precision)
			switch ( dface->paras[i].type)
			{
#define OBJ_DAT_TYPE_SET(X) \
			case DBFace::X:	\
				OBJ_PARA_SET_STATIC_INT("data_type", #X)	\
				break;
				
			OBJ_DAT_TYPE_SET(BigInt)
			OBJ_DAT_TYPE_SET(SmallInt)
			OBJ_DAT_TYPE_SET(TinyInt)
			OBJ_DAT_TYPE_SET(Binary)
			OBJ_DAT_TYPE_SET(Boolean)
			OBJ_DAT_TYPE_SET(Byte)
			OBJ_DAT_TYPE_SET(Char)
			OBJ_DAT_TYPE_SET(String)
			OBJ_DAT_TYPE_SET(Currency)
			OBJ_DAT_TYPE_SET(Date)
			OBJ_DAT_TYPE_SET(Decimal)
			OBJ_DAT_TYPE_SET(Double)
			OBJ_DAT_TYPE_SET(Float)
			OBJ_DAT_TYPE_SET(GUID)
			OBJ_DAT_TYPE_SET(Integer)
			OBJ_DAT_TYPE_SET(Long)
			OBJ_DAT_TYPE_SET(LongBinary)
			OBJ_DAT_TYPE_SET(Memo)
			OBJ_DAT_TYPE_SET(Numeric)
			OBJ_DAT_TYPE_SET(Single)
			OBJ_DAT_TYPE_SET(Text)
			OBJ_DAT_TYPE_SET(Time)
			OBJ_DAT_TYPE_SET(TimeStamp)
			OBJ_DAT_TYPE_SET(VarBinary)
			OBJ_DAT_TYPE_SET(UNKNOWN_TYPE)
			}
			env->SetObjectArrayElement(para_obj_arr, i, para_obj); 
		}
		env->SetObjectField(face_obj, env->GetFieldID(face_cls, "paras", "[Ltextor/jvmport/DBFace$Para;"), para_obj_arr);
	}
	env->DeleteLocalRef(encStr);
}
