/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: PyPort
 Build:created by octerboy 2018/07/19
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "Notitia.h"
#include "textus_string.h"
#include "PacData.h"
#include "TBuffer.h"
#include "DBFace.h"
#include "casecmp.h"
#include <time.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <python2.7/Python.h>
//#include <python3.4m/Python.h>

class PyPort :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	PyPort();
	~PyPort();

private:
	PyObject *pInstance;
	PyObject * fun_ignite, *fun_facio, *fun_sponte, *fun_clone;
	struct G_CFG {
		PyObject * pModule, *pClass;
	 	TiXmlElement *run_simpleStr;
		const char *run_tag;
		const char *pyMod_str, *pyClass_str;
		inline G_CFG () {
			run_tag="run";
			run_simpleStr = 0;
			pyMod_str = pyClass_str = 0;
			pModule = pClass = 0;
		};
	};
	struct G_CFG *gCFG;
	bool has_config;
	bool pius2py (Pius *pius, char *method);
	bool facioPy( Amor::Pius *pius);
	char fac_method[16], spo_method[16];
#include "wlog.h"
};

static PyObject *aptus_error;
bool get_aps(Amor::Pius &aps, PyObject *args)
{
	aps.indic = 0;
	if (!PyArg_ParseTuple(args, "ki", &aps.ordo, &aps.subor)) 
		return false;
	return true;
}

typedef struct {
	PyObject_HEAD
	TBuffer *tb;
} PyTBufferObj;

typedef struct {
	PyObject_HEAD
	PyPort *owner;
} PyAmorObj;

static PyObject *python_facio(PyObject *self, PyObject *args)
{
	bool ret;
	Amor::Pius aps;
	PyPort *c_owner = ((PyAmorObj*)self)->owner;

	ret = get_aps(aps, args);
	if (!ret ) return Py_BuildValue("i", 0);
//	printf("facio PyPort %p  ordo=%lu subor=%d \n", c_owner, aps.ordo, aps.subor);
	ret =  c_owner->aptus->facio(&aps);
	return Py_BuildValue("i", ret ? 1:0);
}

static PyObject *python_sponte(PyObject *self, PyObject *args)
{
	bool ret;
	Amor::Pius aps;
	PyPort *c_owner = ((PyAmorObj*)self)->owner;

	ret = get_aps(aps, args);
	if (!ret ) return Py_BuildValue("i", 0);
//	printf("sponte PyPort %p  ordo=%lu subor=%d \n", c_owner, aps.ordo, aps.subor);
	ret =  c_owner->aptus->sponte(&aps);
	return Py_BuildValue("i", ret ? 1:0);
}

static PyMethodDef py_amor_methods[] = {
	{"aptus_facio", python_facio, METH_VARARGS, "Execute facio command"},
	{"aptus_sponte", python_sponte, METH_VARARGS, "Execute sponte command"},
	{NULL,NULL,0,NULL}
};


//#include "structmember.h"
static void PyTBuffer_dealloc(PyTBufferObj* self)
{
	//printf("++++++ PyTBuffer_dealloc\n");
	delete self->tb;
	self->tb = 0;
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyTBuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTBufferObj *self;

//	printf("++++++ PyTBuffer_new type=%p\n", type);
	self = (PyTBufferObj *)type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		//printf("++++ self PyTBuffer_new %p\n", self);
		//self->tb = new TBuffer(8192);
		self->tb = 0;
	}

	return (PyObject *)self;
}

static int PyTBuffer_init(PyTBufferObj *self, PyObject *args, PyObject *kwds)
{
	if ( !self->tb )
	{
		self->tb = new TBuffer(8192);
	//	printf("++++++ PyTBuffer_init self %p, tb %p\n", self, self->tb);
	}
	return 0;
}

static void PyAmor_dealloc(PyAmorObj* self)
{
//	printf("++++++ PyAmor_dealloc self=%p\n" ,self);
	self->owner = 0;
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyAmor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyAmorObj *self;

	self = (PyAmorObj *)type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		//printf("++++ self PyAmor_new %p\n", self);
		self->owner = 0;
	}

	return (PyObject *)self;
}

static int PyAmor_init(PyAmorObj *self, PyObject *args, PyObject *kwds)
{
	if ( !self->owner )
	{
		//printf("++++++ PyAmor_init self %p, owner %p\n", self, self->owner);
	}
	return 0;
}

#ifdef TTT
static PyMemberDef Noddy_members[] = {
    {"first", T_OBJECT_EX, offsetof(Noddy, first), 0,
    "first name"},
    {"last", T_OBJECT_EX, offsetof(Noddy, last), 0,
     "last name"},
    {"number", T_INT, offsetof(Noddy, number), 0,
     "noddy number"},
    {NULL} 
};

static PyObject *
Noddy_name(Noddy* self)
{
    static PyObject *format = NULL;
    PyObject *args, *result;

    if (format == NULL) {
        format = PyString_FromString("%s %s");
        if (format == NULL)
            return NULL;
    }

    if (self->first == NULL) {
        PyErr_SetString(PyExc_AttributeError, "first");
        return NULL;
    }

    if (self->last == NULL) {
        PyErr_SetString(PyExc_AttributeError, "last");
        return NULL;
    }

    args = Py_BuildValue("OO", self->first, self->last);
    if (args == NULL)
        return NULL;

    result = PyString_Format(format, args);
    Py_DECREF(args);

    return result;
}

static PyMethodDef Noddy_methods[] = {
    {"name", (PyCFunction)Noddy_name, METH_NOARGS,
     "Return the name, combining the first and last name"
    },
    {NULL} 
};

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initnoddy2(void)
{
    PyObject* m;

    m = Py_InitModule3("noddy2", module_methods,
                       "Example module that creates an extension type.");

    if (m == NULL)
        return;

    if (PyType_Ready(&NoddyType) < 0)
        return;

    Py_INCREF(&NoddyType);
    PyModule_AddObject(m, "Noddy", (PyObject *)&NoddyType);
}
#endif

static PyObject *py_tb_input(PyObject *self, PyObject *args)
{
	char *str;
	PyObject *o;
	//printf("--tbuffer ------self %p, tbuffer %p\n", self, ((PyTBufferObj*)self)->tb);
	if (!PyArg_ParseTuple(args, "O", &o)) 
	{
		printf("!!!!valid Object\n");
		return Py_BuildValue("i", 0);
	} else {
		printf("tbuff Object type %s\n", o->ob_type->tp_name);
	}
	if (!PyArg_ParseTuple(args, "s", &str)) 
		return Py_BuildValue("i", 0);
	((PyTBufferObj*)self)->tb->input((unsigned char*)str, strlen(str));
	return Py_BuildValue("i", 1);
}

static PyMethodDef pytb_methods[] = {
	{"input", py_tb_input, METH_VARARGS, "PyTBuffer input ascii string"},
	{NULL,NULL,0,NULL}
};

static PyTypeObject PyTBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textus.TBuffer",             /* tp_name */
    sizeof(PyTBufferObj),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyTBuffer_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "PyTBuffer objects",       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pytb_methods,		/* tp_methods */
    0,				/* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyTBuffer_init,	/* tp_init */
    0,                         /* tp_alloc */
    PyTBuffer_new                 /* tp_new */
};

static PyTypeObject PyAmorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textus.Amor",             /* tp_name */
    sizeof(PyAmorObj),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyAmor_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_compare */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE,   /* tp_flags */
    "PyAmor objects",       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    py_amor_methods,		/* tp_methods */
    0,				/* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyAmor_init,	/* tp_init */
    0,                         /* tp_alloc */
    PyAmor_new                 /* tp_new */
};

static PyMethodDef module_null_methods[] = {
    {NULL}  /* Sentinel */
};

void PyPort::ignite(TiXmlElement *cfg) 
{ 
	if (!cfg) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}

	gCFG->run_simpleStr = cfg->FirstChildElement(gCFG->run_tag);
	gCFG->pyMod_str = cfg->Attribute("module");
	gCFG->pyClass_str = cfg->Attribute("class");
}

bool PyPort::facio( Amor::Pius *pius)
{
	assert(pius);
	const char *run_str;
	TiXmlElement *run_ele;
	PyObject *m_name = 0;
	PyObject *m, *mth;
	int ret;

	switch ( pius->ordo )
	{
	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
		if ( !gCFG->pClass ) 
		{
			WLOG(WARNING,"pClass is null");
			break;
		}
		pInstance = PyObject_CallObject(gCFG->pClass, NULL);
		if ( !pInstance) 
		{
			WLOG(WARNING,"PyInstance_New of class (%s) failed", gCFG->pyClass_str);
			break;
		} else {
			WBUG("PyInstance_New of class (%s) %p", gCFG->pyClass_str, pInstance);
		}
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		Py_Initialize();
		if ( !Py_IsInitialized() )
		{
			WLOG(WARNING,"Py_IsInitialized failed");
			break;
    		}
		if (PyType_Ready(&PyTBufferType) < 0) break;
		if (PyType_Ready(&PyAmorType) < 0) break;

		m = Py_InitModule("textus", module_null_methods);
		if ( !m ) {
			WLOG(WARNING,"Py_InitModule textus failed");
			break;
		} else {
			WBUG("Py_InitModule textus ok!");
		}

		aptus_error = PyErr_NewException((char*)"textus.error", 0, 0);
		Py_INCREF(aptus_error);
		PyModule_AddObject(m, "error", aptus_error);

		Py_INCREF(&PyAmorType);
		PyModule_AddObject(m, "Amor", (PyObject *)&PyAmorType);

		Py_INCREF(&PyTBufferType);
		PyModule_AddObject(m, "TBuffer", (PyObject *)&PyTBufferType);

		run_ele =  gCFG->run_simpleStr;
		while ( run_ele ) 
		{
			run_str= run_ele->GetText();
			if ( run_str && strlen(run_str) > 0 )
			{
				WBUG("run_str \"%s\"", run_str);
				ret = PyRun_SimpleString(run_str);
				if ( ret ) 
				{
					WLOG(WARNING,"python run %s return %d", run_str, ret);
				}
				//PyRun_SimpleString("import sys");
				//PyRun_SimpleString("sys.path.append('./')");
			}
			run_ele = run_ele->NextSiblingElement(gCFG->run_tag);
		}
/*
		ret = PyRun_SimpleString("import textus");
		if ( ret ) 
		{
			WLOG(WARNING,"python run %s return %d (failed!)", "import textus", ret);
		} else {
			WBUG("import aptus ok!");
		}
*/

		m_name = PyString_FromString(gCFG->pyMod_str);
		if ( !m_name ) 
		{
			WLOG(WARNING,"PyString_FromString (%s) failed", gCFG->pyMod_str);
			break;
		}
		gCFG->pModule = PyImport_Import(m_name);      
		if ( !gCFG->pModule) 
		{
			WLOG(WARNING,"PyImport_Import module of (%s) failed", gCFG->pyMod_str);
			break;
		}
		gCFG->pClass = PyObject_GetAttrString(gCFG->pModule, gCFG->pyClass_str);  
		if ( !gCFG->pClass ) 
		{
			WLOG(WARNING,"PyObject_GetAttrString of class (%s) is null", gCFG->pyClass_str);
			break;
		}

		if ( !PyType_Check(gCFG->pClass) ) 
		{
			WLOG(WARNING,"class (%s) is not type", gCFG->pyClass_str);
			break;
		}

		if ( !PyType_IsSubtype((PyTypeObject*)gCFG->pClass, &PyAmorType) ) 
		{
			WLOG(WARNING,"class (%s) is not subtype of (textus.Amor)", gCFG->pyClass_str);
			break;
		}

		pInstance = PyObject_CallObject(gCFG->pClass, NULL);
		if ( !pInstance) 
		{
			WLOG(WARNING,"PyInstance_New of class (%s) failed", gCFG->pyClass_str);
			break;
		} else {
			((PyAmorObj*) pInstance)->owner = this;
			WBUG("PyInstance_New of class (%s) %p", gCFG->pyClass_str, pInstance);
		}
/*
		printf("==== class %p\n", PyMethod_Class(mth));
		ret = PyObject_SetAttrString(pInstance, "aptus_facio", mth );
		if ( ret ) 
		{
			WLOG(WARNING,"PyObject_SetAttrString(pInstance)  %s return %d (failed!)", "facio", ret);
		} else {
			WBUG("set aptus_facio(pInstance) method ok!");
		}

		m = Py_InitModule4("aptus", aptus_methods, NULL, pInstance, PYTHON_API_VERSION);
		if ( !m ) {
			WLOG(WARNING,"Py_InitModule aptus failed");
			break;
		} else {
			WBUG("Py_InitModule aptus ok!");
		}
		aptus_error = PyErr_NewException((char*)"aptus.error", 0, 0);
		Py_INCREF(aptus_error);
		PyModule_AddObject(m, "error", aptus_error);
		ret = PyRun_SimpleString("import aptus");
*/
		break;
	default:
		//WBUG("ordo = %d ========== %p\n", pius->ordo, pInstance);
		return facioPy(pius);
	}

	return true;
}

bool PyPort::sponte( Amor::Pius *pius) { 
	assert(pius);
	WBUG("sponte Notitia::%lu", pius->ordo);
	return false; 
}

Amor* PyPort::clone()
{
	PyPort *child = new PyPort();
	child->gCFG = gCFG;	
	return  (Amor*)child;
}

PyPort::PyPort() 
{
	gCFG=0;
	has_config = false;
	pInstance = 0;
	memset(fac_method, 0, sizeof(fac_method));
	memset(spo_method, 0, sizeof(spo_method));
	memcpy(fac_method, "facio", 5);
	memcpy(spo_method, "sponte", 6);
}

PyPort::~PyPort() 
{ 
	Py_DECREF(pInstance);
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
		Py_Finalize();
	}
} 

/*
static struct PyModuleDef aptus_mod = {
	PyModuleDef_HEAD_INIT;
	"python_facio",
	null,
	-1
};

PyMODINIT_FUNC PyInit_aptus_facio(void)
{
	PyObject *m;
	m = PyModule_Create(&fac_mod);
	if ( !m ) return 0;
}

PyMODINIT_FUNC PyInit_aptus_sponte(void)
{
}
*/

bool PyPort::facioPy( Amor::Pius *pius)
{
	bool ret = false;
	if ( pInstance )
	{
		ret = pius2py(pius, fac_method);
		//if( jvmError() ) return false;
	}
	return ret;
}

/* 从C++程序到Python脚本, 为python生成合适的对象, and facio or sponte */
bool PyPort::pius2py (Pius *pius, char *py_method)
{
	/* 下面根据ordo来生成 ps_obj中的indic, 对付各种TBuffer等 */
	switch ( pius->ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::PRO_TBUF:
	case Notitia::SET_UNIPAC:
	case Notitia::SET_TINY_XML:
	{
	}
		break;

	case Notitia::CMD_SET_DBFACE:	
	{
	}
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		/* indic 指向一个指针TiXmlElement* */
	{
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
	}
		break;

	case Notitia::TIMER:
		/* 这些要转一个java.lang.Integer */
	{
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
	case Notitia::DMD_END_SERVICE:
		/* 这些本来就是不需要indic的 */
		PyObject_CallMethod(pInstance, py_method, (char*)"ki", pius->ordo, pius->subor);
		break;

	default :
		break;
	}

	return true;
}
#include "hook.c"

