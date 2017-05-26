/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: 
 Build:created by octerboy 2005/04/12
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "PacData.h"
#include "casecmp.h"
#include <assert.h>
#if !defined (_WIN32)
#include <errno.h>
#endif
/* 包含JS引擎的API头文件 */
#include "jsapi.h"

#include "function.c"

static JSBool
global_enumerate(JSContext *cx, JSObject *obj)
{
#ifdef LAZY_STANDARD_CLASSES
    return JS_EnumerateStandardClasses(cx, obj);
#else
    return JS_TRUE;
#endif
}

static JSBool
global_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
               JSObject **objp)
{
#ifdef LAZY_STANDARD_CLASSES
    JSBool resolved;

    if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
        return JS_FALSE;
    if (resolved) {
        *objp = obj;
        return JS_TRUE;
    }
#endif

#if defined(SHELL_HACK) && defined(DEBUG) && defined(XP_UNIX)
    if ((flags & (JSRESOLVE_QUALIFIED | JSRESOLVE_ASSIGNING)) == 0) {
        /*
         * Do this expensive hack only for unoptimized Unix builds, which are
         * not used for benchmarking.
         */
        char *path, *comp, *full;
        const char *name;
        JSBool ok, found;
        JSFunction *fun;

        if (!JSVAL_IS_STRING(id))
            return JS_TRUE;
        path = getenv("PATH");
        if (!path)
            return JS_TRUE;
        path = JS_strdup(cx, path);
        if (!path)
            return JS_FALSE;
        name = JS_GetStringBytes(JSVAL_TO_STRING(id));
        ok = JS_TRUE;
        for (comp = strtok(path, ":"); comp; comp = strtok(NULL, ":")) {
            if (*comp != '\0') {
                full = JS_smprintf("%s/%s", comp, name);
                if (!full) {
                    JS_ReportOutOfMemory(cx);
                    ok = JS_FALSE;
                    break;
                }
            } else {
                full = (char *)name;
            }
            found = (access(full, X_OK) == 0);
            if (*comp != '\0')
                free(full);
            if (found) {
                fun = JS_DefineFunction(cx, obj, name, Exec, 0,
                                        JSPROP_ENUMERATE);
                ok = (fun != NULL);
                if (ok)
                    *objp = obj;
                break;
            }
        }
        JS_free(cx, path);
        return ok;
    }
#else
    return JS_TRUE;
#endif
}

JSClass global_class = {
	"global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
	JS_PropertyStub,  JS_PropertyStub,
	JS_PropertyStub,  JS_PropertyStub,
	global_enumerate, (JSResolveOp) global_resolve,
	JS_ConvertStub,   JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool aptus_sponte (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool aptus_facio  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSBool tbuffer_peek (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool tbuffer_write (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool tbuffer_read (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSBool packet_get  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool packet_getInt  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool packet_set  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool packet_setnlen  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static JSBool packet_reset  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSBool alloc_myobj (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static void* get_cpp_obj_pointer  (JSContext *cx, JSObject *obj);

static JSBool getTBuffProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp);
static JSBool getPacProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp);
static JSBool setPacProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp);

void set_indic_obj(JSContext *cx,JSObject *js_ind, const char *name, void *here, JSFunctionSpec *methods, JSPropertySpec *prop);
static void *get_indic_obj(JSContext *cx, JSObject *js_ind, const char *name);
enum tag_Prop {TB_LENGTH, PAC_MAXIUM};

/*定义TBuffer js对象的属性*/
static JSPropertySpec tbuffProperties[] =
{
	{"length", TB_LENGTH, JSPROP_ENUMERATE, getTBuffProperty, 0},
	{0}
};

/*定义PacketObj js对象的属性*/
static JSPropertySpec packetProperties[] =
{
	{"maxium", PAC_MAXIUM, JSPROP_ENUMERATE, getPacProperty, setPacProperty},
	{0}
};

static JSFunctionSpec amor_methods[] =
{
	{"aptus_sponte",aptus_sponte,	1,0,0},
	{"aptus_facio", aptus_facio,	1,0,0},
	{"alloc",	alloc_myobj,	1,0,0},
	{0}
};

static JSFunctionSpec tbuffer_methods[] =
{
	{"peek", tbuffer_peek,    1,0,0},
	{"write",  tbuffer_write,  1,0,0},
	{"read",  tbuffer_read, 1,0,0},
	{0}
};

static JSFunctionSpec packet_methods[] =
{
	{"get", packet_get, 1,0,0},
	{"getInt", packet_getInt, 1,0,0},
	{"set", packet_set, 2,0,0},
	{"setnl", packet_setnlen, 2,0,0},
	{"reset", packet_reset, 1,0,0},
	{0}
};

#define CPP_OBJ_POINTER "cpp_object_pointer"
#define AMOR_OBJ_POINTER "textus_amor_pointer"

JSBool alloc_myobj (JSContext *cx, JSObject *host_obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval aval;
	JSObject *obj;
	JSBool bOk;
	void *native_p = (void*) 0;
	char *bytes;
	JSFunctionSpec *methods;
	JSPropertySpec *prop;

	if ( argc < 1 )
		return JS_FALSE;

	bytes = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	if ( strcasecmp(bytes, "tbuffer")== 0 ) {
		native_p = new TBuffer();
		methods = tbuffer_methods;
		prop = tbuffProperties;
	} else if ( strcasecmp(bytes, "packet")== 0 ) {
		native_p = new PacketObj();
		methods = packet_methods;
		prop = packetProperties;
	} else 
		return JS_FALSE;

	obj = JS_NewObject(cx, NULL, NULL, NULL);
	*rval = OBJECT_TO_JSVAL(obj);
	aval= OBJECT_TO_JSVAL(native_p);
	bOk = JS_SetProperty(cx, obj, CPP_OBJ_POINTER, &aval);
	bOk = JS_DefineFunctions (cx, obj, methods);
	bOk = JS_DefineProperties (cx, obj, prop);

	return bOk;
}

/*
 get_pius()返回
 0: OK
 1: obj中无AMOR_OBJ_POINTER属性, 
 2: 脚本中的参数太少
 3: pius无"ordo"属性
 4: pius无"indic"属性
 5: indic无"pac0"属性
 6: indic无"pac1"属性
 7: indic无"tb0"属性
 8: indic无"tb1"属性
*/

int get_pius(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval, Amor** amr, Amor::Pius &ps)
{
	JSBool bOk;
	jsval aval, jordo, jdic;
	JSObject *js_ind, *js_ps;
	void **tb=0;

	bOk= JS_GetProperty(cx, obj, AMOR_OBJ_POINTER, &aval);
	if (!bOk || aval == JSVAL_VOID ) 
		return 1;
	*amr = (Amor *)JSVAL_TO_OBJECT(aval);
	if ( argc < 1 )
		return 2;

	js_ps = JSVAL_TO_OBJECT(argv[0]);
	bOk = JS_GetProperty(cx, js_ps, "ordo", &jordo);
	if (!bOk || jordo == JSVAL_VOID ) 
		return 3;
	ps.ordo = JSVAL_TO_INT(jordo);

	bOk = JS_GetProperty(cx, js_ps, "indic", &jdic);
	switch ( ps.ordo)
	{
	case Notitia::SET_UNIPAC:
		if (!bOk || jdic == JSVAL_VOID ) 
			return 4;
		js_ind = JSVAL_TO_OBJECT(jdic);

		tb=new void* [2];
		tb[0] = get_indic_obj(cx, js_ind, "pac0");
		if ( tb[0] == 0 ) 
			return 5;
		tb[1] = get_indic_obj(cx, js_ind, "pac1");
		if ( tb[1] == 0 ) 
			return 6;
		break;

	case Notitia::SET_TBUF:
		if (!bOk || jdic == JSVAL_VOID ) 
			return 4;
		js_ind = JSVAL_TO_OBJECT(jdic);

		tb=new void* [2];
		tb[0] = get_indic_obj(cx, js_ind, "tb0");
		if ( tb[0] == 0 ) 
			return 7;
		tb[1] = get_indic_obj(cx, js_ind, "tb1");
		if ( tb[1] == 0 ) 
			return 8;
		break;

	default:
		ps.indic = (void*)0;
		break;
	}
	ps.indic = tb;
	*rval = JSVAL_TRUE;

	return 0;
}
/*
	输入为0个到2个参数:
	第1个:peek内容的长度;
	第2个:peek内容的偏移量, 默认为0 
	如果参数为0, 则peek长度为1个字节
*/
static JSBool tbuffer_peek (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int from, length, len;
	char *src;
	TBuffer *tbp = (TBuffer *)get_cpp_obj_pointer(cx, obj);

	if ( argc < 1 )
		return JS_FALSE;
	
	length = JSVAL_TO_INT(argv[0]);
	from = 0 ;
	if ( argc > 1 )
		from = JSVAL_TO_INT(argv[1]);
	len = tbp->point - tbp->base;
	if ( from > len )
		goto END;
	
	src = (char*) &tbp->base[from];
	if ( from + length > len )
		length = len - from;
	*rval=STRING_TO_JSVAL (JS_NewStringCopyN(cx, src, length));
END:
	return JS_TRUE;
}

/*
	第一个参数为要输入的内容
*/
static JSBool tbuffer_write (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *bytes;
	int length;
	TBuffer *tbp = (TBuffer *)get_cpp_obj_pointer(cx, obj);
	if ( argc < 1 )
		return JS_FALSE;

	bytes = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	length = JS_GetStringLength(JSVAL_TO_STRING(argv[0]));
	tbp->input((unsigned char*)bytes, length);

	return JS_TRUE;
}

static JSBool tbuffer_read (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int length;
	TBuffer *tbp = (TBuffer *)get_cpp_obj_pointer(cx, obj);

	if ( argc == 0 )
		length =  tbp->point - tbp->base;
	else {
		length = JSVAL_TO_INT(argv[0]);
		long len = tbp->point - tbp->base;
		if ( length > len )
			length = len;
	}

	*rval=STRING_TO_JSVAL (JS_NewStringCopyN(cx, (char*) tbp->base, length));
	tbp->commit(-length);
	return JS_TRUE;
}

static JSBool getTBuffProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	TBuffer *tbp = (TBuffer *)get_cpp_obj_pointer(cx, obj);
	if (JSVAL_IS_INT(id)) 
	{
		switch (JSVAL_TO_INT(id)) 
		{
		case TB_LENGTH:
                	*vp=INT_TO_JSVAL (tbp->point - tbp->base);
                	break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static void* get_cpp_obj_pointer  (JSContext *cx, JSObject *obj)
{
	jsval aval;	
	void *p;
	JSBool bOk;

	bOk = JS_GetProperty(cx, obj, CPP_OBJ_POINTER, &aval);
	if (!bOk || aval == JSVAL_VOID )
		return (void*) 0;
	p = (void *)aval;
	return p;

}

static JSBool packet_get  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int no;
	unsigned char *value;
	unsigned long length;
	
	*rval = JSVAL_NULL; /*  先假定是空的 */
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if ( argc < 1 )
		goto END;
	
	if ( JS_TypeOfValue(cx, argv[0]) != JSTYPE_NUMBER)
		goto END;
		
	no = JSVAL_TO_INT(argv[0]);
	value = pacp->getfld(no, &length);
	if ( !value )
		goto END;
	
	*rval=STRING_TO_JSVAL (JS_NewStringCopyN(cx, (char*)value, length));
END:
	return JS_TRUE;
}

static JSBool packet_getInt  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int no, ival;
	unsigned char *value;
	unsigned long length;
	
	*rval = JSVAL_NULL; /*  先假定是空的 */
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if ( argc < 1 )
		goto END;
	
	if ( JS_TypeOfValue(cx, argv[0]) != JSTYPE_NUMBER)
		goto END;
		
	no = JSVAL_TO_INT(argv[0]);
	value = pacp->getfld(no, &length);
	if ( !value )
		goto END;
	
	if ( length != sizeof(ival))
		goto END;

	memcpy(&ival, value, length);
	*rval=INT_TO_JSVAL (ival);
END:
	return JS_TRUE;
}

static JSBool packet_set  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *bytes;
	int length;
	int no;
	int ival;
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if ( argc < 2)
		return JS_FALSE;

	if ( JS_TypeOfValue(cx, argv[0]) != JSTYPE_NUMBER)
		return JS_FALSE;
		
	no = JSVAL_TO_INT(argv[0]);
	switch ( JS_TypeOfValue(cx, argv[1]) )
	{
	case JSTYPE_NUMBER:
		ival = JSVAL_TO_INT(argv[1]);
		pacp->input(no, (unsigned char*)&ival, (unsigned long)sizeof(ival));
		break;

	case JSTYPE_STRING:
		bytes = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
		length = JS_GetStringLength(JSVAL_TO_STRING(argv[1]));
		pacp->input(no, (unsigned char*)bytes, (unsigned long)length);
		break;
	default:
		break;
	}
	return JS_TRUE;
}

static JSBool packet_setnlen  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int no;
	int ival;
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if ( argc < 2)
		return JS_FALSE;

	if ( JS_TypeOfValue(cx, argv[0]) != JSTYPE_NUMBER)
		return JS_FALSE;
		
	no = JSVAL_TO_INT(argv[0]);
	switch ( JS_TypeOfValue(cx, argv[1]) )
	{
	case JSTYPE_NUMBER:
		ival = JSVAL_TO_INT(argv[1]);
		if ( no >= 0 && no <= pacp->max )
		{
			pacp->fld[no].len = ival;
		}
		break;

	default:
		break;
	}
	return JS_TRUE;
}

static JSBool packet_reset  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	pacp->reset();
	return JS_TRUE;
}

static JSBool getPacProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if (JSVAL_IS_INT(id)) 
	{
		switch (JSVAL_TO_INT(id)) 
		{
		case PAC_MAXIUM:
                	*vp=INT_TO_JSVAL (pacp->max);
                	break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool setPacProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	int maxf;
	struct PacketObj *pacp = (struct PacketObj *)get_cpp_obj_pointer(cx, obj);
	if (JSVAL_IS_INT(id)) 
	{
		switch (JSVAL_TO_INT(id)) 
		{
		case PAC_MAXIUM:
			maxf=JSVAL_TO_INT(*vp);
			pacp->produce(maxf);
                	break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

class JSPort :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	JSPort();
	~JSPort();

	void myerr(const char *msg);
	static JSRuntime *rt;
	static size_t total_stack;	
	typedef struct _GCFG  {
		JSContext *context;
		JSVersion run_ver;	//外部设定的版本, 如果不设定, 就用最新版本
		size_t cx_stack;
		JSObject  *globalObj;
		JSFunction *alloc_ps;

		char *script_txt;
		char *script_file;
		int script_len;

		inline void update(TiXmlElement *cfg )
		{
			const char *comm_str = (const char*) 0;
			TiXmlElement *scr;

			context = 0;
			globalObj = 0;
			script_len = 0;

			if (script_txt) delete[] script_txt;
			if (script_file) delete[] script_file;
			script_txt = 0;
			script_file = 0;

			if (cfg)
				comm_str = cfg->Attribute("context_stack");
			if ( comm_str)		
				cx_stack = atol(comm_str);
			else
				cx_stack = 25600;

			scr = cfg->FirstChildElement("script");
			if(scr)
			{
				const char *scr_txt;
				const char *scr_file;
				
				scr_file = scr->Attribute("file");
				if ( scr_file )
				{
					script_file = new char[strlen(scr_file)+1];
					memcpy(script_file, scr_file, strlen(scr_file)+1);
				}

				scr_txt = scr->GetText();
				if ( scr_txt )
				{
					script_len = strlen(script_txt);
					script_txt = new char[script_len+1];
					memcpy(script_txt, scr_txt, script_len+1);
				}
			}
		};

		inline _GCFG( TiXmlElement *cfg )
		{
			script_txt = 0;
			script_file = 0;
			update(cfg);
		}
	} GCFG;

	GCFG *gcfg;
	bool has_config;

	JSObject  *js_amor;
	bool to_jsfunc(Amor::Pius *ps, const char *name);

private:
#include "wlog.h"
};

JSRuntime *JSPort::rt= (JSRuntime*) 0;
size_t JSPort::total_stack = 1000000L;

void JSPort::ignite(TiXmlElement *cfg) 
{
	const char *comm_str;
	if ( !gcfg )
	{
		gcfg = new GCFG (cfg);
		has_config = true;
	} else {
		gcfg->update(cfg);
	}

	comm_str = cfg->Attribute("total_stack");
	if ( comm_str)		
		total_stack = atol(comm_str);

	gcfg->run_ver = JSVERSION_LATEST;
	comm_str = cfg->Attribute("js_version");
	if ( comm_str)		
		gcfg->run_ver = JS_StringToVersion(comm_str);
}

bool JSPort::facio( Amor::Pius *pius)
{
	JSBool builtins;
	TBuffer buf;
	jsval rval, aval;
	JSString* jss;
	FILE *fp=NULL;
	JSBool bOk;
	JSFunction *func;
	const char* fbody="this.ignite=ignite; this.facio=facio; this.sponte=sponte; this.clone=clone;";

	JSFunction *alloc_amr;
	const char* fbody2="var neo = new Amor(); return neo;";
	const char* fbody3="var neo = new Pius(); return neo;";

	const char *scrpt;
	int len;

	assert(pius);
	switch(pius->ordo)
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");		
		/* 初始化JS RunTime，返回结果给rt */
		if ( !rt )
			rt = JS_NewRuntime(total_stack);

		if (!rt) 
		{
			WLOG(ERR,"JS_NewRuntime failed of %u", total_stack);
			goto END;
		}

		/* 创建一个上下文，并将其与JS RunTime关联起来 */
		gcfg->context = JS_NewContext(rt, gcfg->cx_stack);
		if (!gcfg->context) 
		{
			WLOG(ERR,"JS_NewContext failed of %u", gcfg->cx_stack);
			goto END;
		}
		JS_SetVersion(gcfg->context, gcfg->run_ver);

		/* 创建全局对象 */

		if (!(gcfg->globalObj = JS_NewObject (gcfg->context, &global_class, NULL, NULL)))
		{
			WLOG(ERR,"JS_NewObject globalObj failed.");
			goto END;
		}

		/* 实例化内置对象和全局对象*/
		/**//* 初始化内置JS对象和全局对象 */
		builtins = JS_InitStandardClasses(gcfg->context, gcfg->globalObj);
		if ( builtins != JS_TRUE )
		{
			WLOG(ERR,"JS_InitStandardClasses failed.");
			goto END;
		}

		if (!JS_DefineFunctions(gcfg->context, gcfg->globalObj, shell_functions))
		{
			WLOG(ERR,"JS_DefineFunctions shell_functions failed.");
			goto END;
		}

		if ( gcfg->script_txt)
		{
			scrpt = gcfg->script_txt;	
			len = gcfg->script_len;
			goto EXEC_SCR;
		}

		/** 打开文件，读入脚本 */
		TEXTUS_FOPEN(fp,gcfg->script_file,"r");
		if (!fp)
		{
			WLOG_OSERR("open script file");
			goto END;
		}
		len = 0;
		while ( !ferror(fp) && !feof(fp) )
		{
			size_t n;
			buf.grant(2048);
			n = fread (buf.point, 1, 2048, fp);
			len += n;
			buf.commit(n);
		}
		if ( ferror(fp) ) 
		{
			WLOG_OSERR("read script file");
			goto END;
		}
		fclose (fp);

		scrpt = (const char*) buf.base;
EXEC_SCR:
		/** 执行脚本 */
		bOk = JS_EvaluateScript(gcfg->context, gcfg->globalObj, scrpt, len, "", 1, &rval);
		if (bOk == JS_TRUE)
		{
			jss = JS_ValueToString (gcfg->context, rval);
		} else {
			WLOG(ERR, "JS_EvaluateScript failed!");
			goto END;
		}

		func = JS_CompileFunction(gcfg->context, gcfg->globalObj, "Pius", 0, NULL, NULL, 0, NULL, 0);
		if ( !func )
		{
			WLOG(ERR,"JS_CompileFunction of \"Pius()\"  failed.");
			goto END;
		}

		gcfg->alloc_ps = JS_CompileFunction(gcfg->context, gcfg->globalObj, NULL, 0, NULL, fbody3, strlen(fbody3), NULL, 0);
		if ( !gcfg->alloc_ps )
		{
			WLOG(ERR,"JS_CompileFunction of \"new Pius()\"  failed.");
			goto END;
		}
		
		func=JS_CompileFunction(gcfg->context, gcfg->globalObj, "Amor", 0, NULL, fbody, strlen(fbody),NULL, 0);
		if ( !func )
		{
			WLOG(ERR,"JS_CompileFunction of \"Amor()\"  failed.");
			goto END;
		}

		alloc_amr=JS_CompileFunction(gcfg->context, gcfg->globalObj, NULL, 0, NULL, fbody2, strlen(fbody2),NULL, 0);
		if ( !alloc_amr )
		{
			WLOG(ERR,"JS_CompileFunction of \"new Amor()\"  failed.");
			goto END;
		}

		bOk = JS_CallFunction(gcfg->context, gcfg->globalObj, alloc_amr, 0, NULL, &rval);
		if ( !bOk )
		{
			WLOG(ERR,"JS_CallFunction of \"new Amor()\"  failed.");
			goto END;
		}
		js_amor = JSVAL_TO_OBJECT(rval);

		JS_DefineFunctions (gcfg->context, js_amor, amor_methods);
		aval = OBJECT_TO_JSVAL(this);
		JS_SetProperty(gcfg->context, js_amor, AMOR_OBJ_POINTER, &aval);
		//bOk = JS_CallFunctionName(gcfg->context, js_amor, "ignite", 1, &aval, &rval);
		bOk = JS_CallFunctionName(gcfg->context, js_amor, "ignite", 0, NULL, &rval);
		goto DEFAULT;
END:
		break;

	default:
DEFAULT:
		return to_jsfunc(pius, "facio");
	}
	return true;
}

bool JSPort::sponte( Amor::Pius *pius)
{ 
	assert(pius);
	WBUG("sponte Notitia::%lu", pius->ordo);
	return to_jsfunc(pius, "sponte");
}

Amor* JSPort::clone()
{
	jsval aval, rval;
	JSBool bOk;
	JSPort *child = new JSPort();
	child->gcfg = gcfg;
	if (!js_amor )
		goto END;
	bOk = JS_CallFunctionName(gcfg->context, js_amor, "clone", 0, NULL, &rval);
	if ( bOk == JS_TRUE)
	{
		child->js_amor = JSVAL_TO_OBJECT(rval);
		JS_DefineFunctions (gcfg->context, child->js_amor, amor_methods);
	} else {
		child->js_amor = JS_NewObject(gcfg->context, NULL, js_amor, NULL);
	}

	if ( child->js_amor)
	{
		aval = OBJECT_TO_JSVAL(child);
		JS_SetProperty(gcfg->context, child->js_amor, AMOR_OBJ_POINTER, &aval);
	}
END:
	return  (Amor*)child;
}

JSPort::JSPort() { 
	gcfg = 0; 
	js_amor = 0;
	has_config = false;
}

JSPort::~JSPort() { 
	if ( has_config )
		delete gcfg;
} 

void JSPort::myerr(const char *msg)
{
	WLOG(ERR,msg);
}

bool JSPort::to_jsfunc(Amor::Pius *pius, const char *name)
{
	jsval rval,aval;
	JSBool bOk;
	JSObject  *js_ps, *js_ind;
	bool ret = false;

	if ( !js_amor)
		return false;

	bOk = JS_CallFunction(gcfg->context, gcfg->globalObj, gcfg->alloc_ps, 0, NULL, &rval);
	if ( !bOk )
	{
		WLOG(ERR,"JS_CallFunction of \"new Pius()\"  failed.");
		return false;
	}
	js_ps = JSVAL_TO_OBJECT(rval);
	aval = INT_TO_JSVAL(pius->ordo);
	bOk = JS_SetProperty(gcfg->context, js_ps, "ordo", &aval);
	if ( bOk != JS_TRUE )
	{
		WLOG(ERR,"JS_SetProperty ordo of \"pius\"  failed.");
		return false;
	}

	js_ind = JS_NewObject(gcfg->context, NULL, NULL, NULL);
	if ( !js_ind )
	{
		WLOG(ERR,"JS_NewObject of \"indic\"  failed.");
		return false;
	}
	aval = OBJECT_TO_JSVAL(js_ind);
	bOk = JS_SetProperty(gcfg->context, js_ps, "indic", &aval);
	if ( bOk != JS_TRUE )
	{
		WLOG(ERR,"JS_SetProperty ordo of \"indic\"  failed.");
		return false;
	}

	/* 根据不同的ordo, 生成相应的indic */
	switch ( pius->ordo )
	{
	case Notitia::SET_UNIPAC:
		#define SET_PACP(X,Y) \
		set_indic_obj(gcfg->context,js_ind, X,  ((PacketObj **)(pius->indic))[Y], packet_methods, packetProperties);
		SET_PACP("pac0", 0)
		SET_PACP("pac1", 1)
		break;
	case Notitia::SET_TBUF:
		#define SET_TBP(X,Y) \
		set_indic_obj(gcfg->context,js_ind, X,  ((TBuffer **)(pius->indic))[Y], tbuffer_methods,tbuffProperties);
		SET_TBP("tb0", 0)
		SET_TBP("tb1", 1)
		break;

	default:
		break;
	}

	/* 这里调用js_amor的函数 */
	aval = OBJECT_TO_JSVAL(js_ps);
	bOk = JS_CallFunctionName(gcfg->context, js_amor, name, 1, &aval, &rval);
	if ( bOk != JS_TRUE )
	{
		WLOG(ERR,"JS_CallFunctionName of \"%s()\"  failed.", name);
	} else {
		bOk = JSVAL_TO_BOOLEAN(rval);
		ret = (bOk == JS_TRUE);
	}
	return ret;
}

void *get_indic_obj (
	JSContext *cx,
	JSObject *js_ind, 	/* 即pius.indic对象 */
	const char *mName 	/* 在indic中的成员名, 比如tb0 */
) {
	jsval aval;
	JSObject  *obj;
	JSBool bOk;

	bOk = JS_GetProperty(cx, js_ind, mName, &aval);
	if (!bOk || aval == JSVAL_VOID )
		return (void*) 0;
	obj = JSVAL_TO_OBJECT(aval);
	return get_cpp_obj_pointer(cx, obj);
}

void set_indic_obj (
	JSContext *cx,
	JSObject *js_ind, 	/* 即pius.indic对象 */
	const char *mName, 	/* 在indic中的成员名, 比如tb0 */
	void *native_p, 	/* 相应的如TBuffer这样的指针 */
	JSFunctionSpec *methods,	/* 操作数据对象的方法 */
	JSPropertySpec *prop
) {
	jsval aval;
	JSObject  *obj;
	JSBool bOk;

	obj = JS_NewObject(cx, NULL, NULL, NULL);
	aval = OBJECT_TO_JSVAL(obj);
	bOk = JS_SetProperty(cx, js_ind, mName, &aval);

	aval= OBJECT_TO_JSVAL(native_p);
	bOk = JS_SetProperty(cx, obj, CPP_OBJ_POINTER, &aval);

	bOk = JS_DefineFunctions (cx, obj, methods);
	if ( prop)
		JS_DefineProperties (cx, obj, prop);
}

static JSBool aptus_sponte (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Amor *owner;
	Amor::Pius ps;
	int ret;
	ret =  get_pius(cx, obj, argc, argv, rval, &owner, ps) ;
	if ( ret == 0  )
	{
		owner->aptus->sponte(&ps);
		if ( ps.ordo == Notitia::SET_UNIPAC 
			|| ps.ordo == Notitia::SET_TBUF)
			delete[] (void**)ps.indic;
		return JS_TRUE;
	} else if ( ret != 1 )
	{
		JSPort *jsprt = (JSPort *)owner;
		switch (ret )
		{
		case 2:
			jsprt->myerr("aptus_sponte absence of parameter 'Pius'");
			break;
		case 3:
			jsprt->myerr("aptus_sponte Pius has no property 'ordo' ");
			break;
		case 4:
			jsprt->myerr("aptus_sponte Pius has no property 'indic' ");
			break;
		case 5:
			jsprt->myerr("aptus_sponte Pius.indic has no property 'pac0' ");
			break;
		case 6:
			jsprt->myerr("aptus_sponte Pius.indic has no property 'pac1' ");
			break;
		case 7:
			jsprt->myerr("aptus_sponte Pius.indic has no property 'tb0' ");
			break;
		case 8:
			jsprt->myerr("aptus_sponte Pius.indic has no property 'tb1' ");
			break;
		default:
			break;
		}
	}
	return JS_FALSE;
}

static JSBool aptus_facio  (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	Amor *owner;
	Amor::Pius ps;
	int ret;
	ret =  get_pius(cx, obj, argc, argv, rval, &owner, ps) ;
	if ( ret == 0  )
	{
		owner->aptus->facio(&ps);
		if ( ps.ordo == Notitia::SET_UNIPAC 
			|| ps.ordo == Notitia::SET_TBUF)
			delete[] (void**)ps.indic;
		return JS_TRUE;
	} else if ( ret != 1 )
	{
		JSPort *jsprt = (JSPort *)owner;
		switch (ret )
		{
		case 2:
			jsprt->myerr("aptus_facio absence of parameter 'Pius'");
			break;
		case 3:
			jsprt->myerr("aptus_facio Pius has no property 'ordo' ");
			break;
		case 4:
			jsprt->myerr("aptus_facio Pius has no property 'indic' ");
			break;
		case 5:
			jsprt->myerr("aptus_facio Pius.indic has no property 'pac0' ");
			break;
		case 6:
			jsprt->myerr("aptus_facio Pius.indic has no property 'pac1' ");
			break;
		case 7:
			jsprt->myerr("aptus_facio Pius.indic has no property 'tb0' ");
			break;
		case 8:
			jsprt->myerr("aptus_facio Pius.indic has no property 'tb1' ");
			break;
		default:
			break;
		}
	}
	return JS_FALSE;
}

#include "hook.c"
