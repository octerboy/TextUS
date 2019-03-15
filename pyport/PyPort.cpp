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
#include <python2.7/structmember.h>

class PyPort :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	PyPort();
	~PyPort();

	bool get_aps(Amor::Pius &aps, PyObject *arg, const char *err_msg);
	void free_aps(Amor::Pius &aps);
private:
	Amor::Pius clr_timer_pius;  //timer handle
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
	bool pius2py (Pius *pius, char *method ,const char *str);
	bool facioPy( Amor::Pius *pius);
	char fac_method[16], spo_method[16];
	void fetch_error();
#include "wlog.h"
};

static PyObject *aptus_error;

typedef struct {
	PyObject_HEAD
	TEXTUS_ORDO ordo;
	int subor;
	PyObject *indic;
} PyPiusObj;

typedef struct {
	PyObject_HEAD
	PyPort *owner;
} PyAmorObj;

typedef struct {
	PyObject_HEAD
	TBuffer *tb;
	int ref;	/* 0: tb是自己的, 最后要释放;  1: tb是外来的, 不管 */
} PyTBufferObj;

typedef struct {
	PyObject_HEAD
	PacketObj *pac;
	int ref;	/* 0: tb是自己的, 最后要释放;  1: tb是外来的, 不管 */
} PyPacketObj;

static void PyPius_dealloc(PyPiusObj* self)
{
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyPius_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPiusObj *self;

	self = (PyPiusObj *)type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		//printf("++++ self PyAmor_new %p\n", self);
		self->indic = Py_None;
		self->ordo = Notitia::TEXTUS_RESERVED;
		self->subor = Amor::CAN_ALL;
	}
	return (PyObject *)self;
}

static int PyPius_init(PyPiusObj *self, PyObject *args, PyObject *kwds)
{
	int j=-1;
	if ( PyArg_ParseTuple (args, "|i:textor.Pius.init", &j) )
	{
		if ( j  > 0 ) 
		{
			self->ordo = (TEXTUS_ORDO)j;
		}
	} else return -1;
	return 0;
}

static PyMemberDef PyPius_members[] = {
	{(char*)"ordo", T_LONG, offsetof(PyPiusObj, ordo), 0, (char*)"pius ordo" },
	{(char*)"subor", T_INT, offsetof(PyPiusObj, subor), 0, (char*)"pius sub ordo" },
	{(char*)"indic", T_OBJECT_EX, offsetof(PyPiusObj, indic), 0, (char*)"pius indic" },
	{NULL}
};

static PyTypeObject PyPiusType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textor.Pius",             /* tp_name */
    sizeof(PyPiusObj),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyPius_dealloc, /* tp_dealloc */
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
    "Pius objects for python",	/* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,				/* tp_methods */
    PyPius_members,		/* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyPius_init,	/* tp_init */
    0,                         /* tp_alloc */
    PyPius_new                 /* tp_new */
};

static PyObject *python_facio(PyObject *self, PyObject *args)
{
	bool ret;
	Amor::Pius aps;
	PyPort *c_owner = ((PyAmorObj*)self)->owner;

	if ( !c_owner ) 
	{
		PyErr_SetString(aptus_error, "c_owner is null when textor.aptus_facio()");
		return NULL;
	}
	ret = c_owner->get_aps(aps, args, "O:textor.aptus_facio");
	if ( !ret ) return NULL;
//	printf("facio PyPort %p  ordo=%lu subor=%d \n", c_owner, aps.ordo, aps.subor);
	ret =  c_owner->aptus->facio(&aps);
	c_owner->free_aps(aps);
	return ret ? Py_True :Py_False;
	//return Py_BuildValue("i", ret ? 1:0);
}

static PyObject *python_sponte(PyObject *self, PyObject *args)
{
	bool ret;
	Amor::Pius aps;
	PyPort *c_owner = ((PyAmorObj*)self)->owner;

	if ( !c_owner ) 
	{
		PyErr_SetString(aptus_error, "c_owner is null when textor.aptus_sponte()");
		return NULL;
	}
	ret = c_owner->get_aps(aps, args, "O:textor.aptus_sponte");
	if ( !ret ) return NULL;
//	printf("sponte PyPort %p  ordo=%lu subor=%d \n", c_owner, aps.ordo, aps.subor);
	ret =  c_owner->aptus->sponte(&aps);
	c_owner->free_aps(aps);
	return ret ? Py_True :Py_False;
}

static PyObject *python_log(PyObject *self, PyObject *args, TEXTUS_ORDO lev)
{
	bool ret;
	Amor::Pius aps;
	char **msg_arr=0;
	Py_ssize_t  args_num,i;
	TBuffer buf;
	
	PyPort *c_owner = ((PyAmorObj*)self)->owner;

	if ( !c_owner ) 
	{
		PyErr_SetString(aptus_error, "c_owner is null when textor.aptus_log()");
		return NULL;
	}
	
	aps.ordo = lev;
	aps.subor = Amor::CAN_ALL;
	/* msg from args */	
	args_num = PyTuple_Size(args);
	msg_arr = new char* [args_num];
	for ( i = 0 ; i < args_num; i++)
	{
		msg_arr[i] = (char*)PyString_AsString(PyObject_Str(PyTuple_GetItem(args, i)));
		buf.input((unsigned char*)msg_arr[i], strlen(msg_arr[i]));
	}
	buf.input((unsigned char*)"\0",1);
	delete [] (char*)msg_arr;
//	printf("sponte PyPort %p  ordo=%lu subor=%d \n", c_owner, aps.ordo, aps.subor);
	aps.indic = (void*)buf.base;
	ret =  c_owner->aptus->sponte(&aps);
	/* release msg */
	return Py_True;
}

static PyObject *python_log_bug(PyObject *self, PyObject *args)
{
#ifndef NDEBUG
		return python_log(self, args, Notitia::LOG_DEBUG);
#endif
}

static PyObject *python_log_info(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_INFO);
}

static PyObject *python_log_notice(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_NOTICE);
}

static PyObject *python_log_warn(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_WARNING);
}

static PyObject *python_log_err(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_ERR);
}

static PyObject *python_log_crit(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_CRIT);
}

static PyObject *python_log_alert(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_ALERT);
}

static PyObject *python_log_emerg(PyObject *self, PyObject *args)
{
	return python_log(self, args, Notitia::LOG_EMERG);
}

static PyMethodDef py_amor_methods[] = {
	{"aptus_facio", python_facio, METH_VARARGS, "Execute facio command"},
	{"aptus_sponte", python_sponte, METH_VARARGS, "Execute sponte command"},
	{"aptus_log_bug", python_log_bug, METH_VARARGS, "Execute log bug command"},
	{"aptus_log_info", python_log_info, METH_VARARGS, "Execute log infomation command"},
	{"aptus_log_notice", python_log_notice, METH_VARARGS, "Execute log notice command"},
	{"aptus_log_warn", python_log_warn, METH_VARARGS, "Execute log warning command"},
	{"aptus_log_err", python_log_err, METH_VARARGS, "Execute log error command"},
	{"aptus_log_crit", python_log_crit, METH_VARARGS, "Execute log critical command"},
	{"aptus_log_alert", python_log_alert, METH_VARARGS, "Execute log alert command"},
	{"aptus_log_emerg", python_log_emerg, METH_VARARGS, "Execute log emerg command"},
	{NULL,NULL,0,NULL}
};

static void PyAmor_dealloc(PyAmorObj* self)
{
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

static PyTypeObject PyAmorType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textor.Amor",             /* tp_name */
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
    "Amor objects for python",       /* tp_doc */
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

static void PyTBuffer_dealloc(PyTBufferObj* self)
{
	if ( self->tb  && self->ref == 0 )
	{
		delete self->tb;
		self->tb = 0;
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyTBuffer_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyTBufferObj *self=NULL;

	//printf("++++++ PyTBuffer_new type=%p args %p kwds %p\n", type, args, kwds);
	self = (PyTBufferObj *)type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		self->tb = 0;
	}

	return (PyObject *)self;
}

static int PyTBuffer_init(PyTBufferObj *self, PyObject *args, PyObject *kwds)
{
	int j=-1;
	if ( PyArg_ParseTuple (args, "|i:textor.TBuffer.init", &j) )
	{
		if ( j  ==0 ) 
			self->ref = 1;
	} else return -1;
	if( j != 0 && !self->tb ) 
	{
		self->tb = new TBuffer( j > 0? j:8192);
		self->ref = 0;
	}
	//printf("+###++ PyTBuffer_init self=%p self tb=%p ref=%d\n", self, self->tb, self->ref);
	return 0;
}

static PyObject *py_tb_input(PyObject *self, PyObject *args)
{
	PyObject *o;
	if (!PyArg_ParseTuple(args, "O:textor.TBuffer.input", &o)) 
	{
		return  NULL;
	} else {
		if ( PyByteArray_Check(o) )
		{
			((PyTBufferObj*)self)->tb->input((unsigned char*)PyByteArray_AsString(o), PyByteArray_Size(o));
		}  else if ( PyString_Check(o) )
		{
			((PyTBufferObj*)self)->tb->input((unsigned char*)PyString_AsString(o), PyString_Size(o));
		}  else {
			PyErr_SetString(aptus_error, "not supported data type when textor.TBuffer.input()");
			return NULL;
		}
	}
	return Py_BuildValue("i", 0); /* It will core_dumped if 'return Py_True'. Why? But it will not when python_facio */
}

static PyObject *py_tb_get(PyObject *self)
{
	return PyString_FromStringAndSize((const char*)(((PyTBufferObj*)self)->tb->base), ((PyTBufferObj*)self)->tb->point - ((PyTBufferObj*)self)->tb->base);
}

static PyObject *py_tb_getbytes(PyObject *self)
{
	return PyByteArray_FromStringAndSize((const char*)(((PyTBufferObj*)self)->tb->base), ((PyTBufferObj*)self)->tb->point - ((PyTBufferObj*)self)->tb->base);
}

static PyMethodDef pytb_methods[] = {
	{"input", py_tb_input, METH_VARARGS, "PyTBuffer input ascii string"},
	{"get", (PyCFunction)py_tb_get, METH_NOARGS, "PyTBuffer get ascii string"},
	{"getstr", (PyCFunction)py_tb_get, METH_NOARGS, "PyTBuffer get ascii string"},
	{"getbytes", (PyCFunction)py_tb_getbytes, METH_NOARGS, "PyTBuffer get bytes"},
	{NULL,NULL,0,NULL}
};

static PyTypeObject PyTBufferType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textor.TBuffer",             /* tp_name */
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
    "TBuffer objects for python",       /* tp_doc */
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

static void PyPacket_dealloc(PyPacketObj* self)
{
	if ( self->pac && self->ref == 0 )
	{
		//printf("PyPacket dealloc %p ref = %d\n", self->pac, self->ref);
		delete self->pac;
		self->pac = 0;
	}
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *PyPacket_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPacketObj *self = (PyPacketObj *)type->tp_alloc(type, 0);
	if (self != NULL) 
	{
		self->pac = 0;
		self->ref = 0;
	}
	return (PyObject *)self;
}

static int PyPacket_init(PyPacketObj *self, PyObject *args, PyObject *kwds)
{
	int j=-1;
	if ( PyArg_ParseTuple (args, "|i:textor.Packet.init", &j) )
	{
		if ( j  ==0 ) 
			self->ref = 1;
	} else return -1;
	if( j != 0 && !self->pac )
	{
		self->pac = new PacketObj();
		self->ref = 0;
		if ( j > 0 ) 
		{
			self->pac->produce(j);
		}
	}
	return 0;
}

static PyObject *py_pac_set_ajp(PyObject *self, int fld, PyObject *on, PyObject *ov )
{
	const char* sc, *val;
	unsigned short nlen, vlen;
	if ( PyByteArray_Check(on) )
	{
		sc = (const char*)PyByteArray_AsString(on);
		nlen = (unsigned short)PyByteArray_Size(on) & 0xFF;
	}  else if ( PyString_Check(on) )
	{
		sc = (const char*)PyString_AsString(on);
		nlen = (unsigned short)PyString_Size(on) & 0xFF;
	}  else {
		PyErr_SetString(aptus_error, "not supported data type of the 2nd parameter when textor.Packet.set()");
		return NULL;
	}

	if ( PyByteArray_Check(ov) )
	{
		val = (const char*)PyByteArray_AsString(ov);
		vlen = (unsigned short)PyByteArray_Size(ov) & 0xFF;
	}  else if ( PyString_Check(ov) )
	{
		val = (const char*)PyString_AsString(ov);
		vlen = (unsigned short)PyString_Size(ov) & 0xFF;
	}  else {
		PyErr_SetString(aptus_error, "not supported data type of the 3rd parameter when textor.Packet.set()");
		return NULL;
	}
	((PyPacketObj*)self)->pac->inputAJP(fld, nlen, sc, vlen, val);
	return Py_BuildValue("i", 0);
}

static PyObject *py_pac_set(PyObject *self, PyObject *args)
{
	PyObject *o=0, *v=0;
	int fld;
	if (!PyArg_ParseTuple(args, "iO|O:textor.Packet.set ", &fld, &o, &v)) 
	{
		return NULL;
	} else {
		if ( v ) return py_pac_set_ajp(self, fld, o, v);	
		if ( PyByteArray_Check(o) )
		{
			((PyPacketObj*)self)->pac->input(fld, (unsigned char*)PyByteArray_AsString(o), PyByteArray_Size(o));
		}  else if ( PyString_Check(o) )
		{
			((PyPacketObj*)self)->pac->input(fld, (unsigned char*)PyString_AsString(o), PyString_Size(o));
		}  else {
			PyErr_SetString(aptus_error, "not supported data type when textor.Packet.set()");
			return NULL;
		}
	}
	return Py_BuildValue("i", 0);
}

static PyObject *py_pac_get(PyObject *self, PyObject *args)
{
	int fld, len;
	unsigned char*p = 0;
	if (PyArg_ParseTuple(args, "i", &fld))
	{
		p = ((PyPacketObj*)self)->pac->getfld(fld, &len);
		if ( p )
			return PyString_FromStringAndSize((const char*)p, (Py_ssize_t)len);
		else {
			PyErr_SetString(aptus_error, "no such field when textor.Packet.get()");
			return NULL;
		}
	} else return NULL;
	return PyString_FromStringAndSize((const char*)p, (Py_ssize_t)0);
}

static PyObject *py_pac_getbytes(PyObject *self, PyObject *args)
{
	int fld, len;
	unsigned char*p = 0;
	if (PyArg_ParseTuple(args, "i", &fld))
	{
		unsigned char*p = 0; 
		p  = ((PyPacketObj*)self)->pac->getfld(fld, &len);
		if ( p )
			return PyByteArray_FromStringAndSize((const char*)p, (Py_ssize_t)len);
		else {
			PyErr_SetString(aptus_error, "no such field when textor.Packet.get()");
			return NULL;
		}
	} else return NULL;
	return PyString_FromStringAndSize((const char*)p, (Py_ssize_t)0);
}

static PyMethodDef pypac_methods[] = {
	{"set", py_pac_set, METH_VARARGS, "Packet set field"},
	{"get", py_pac_get, METH_VARARGS, "Packet get field"},
	{"getstr", py_pac_get, METH_VARARGS, "Packet get field"},
	{"getbytes", py_pac_getbytes, METH_VARARGS, "Packet get field"},
	{NULL,NULL,0,NULL}
};

static PyTypeObject PyPacketType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "textor.Packet",             /* tp_name */
    sizeof(PyPacketObj),      /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)PyPacket_dealloc, /* tp_dealloc */
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
    "Packet objects for python",       /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    pypac_methods,		/* tp_methods */
    0,				/* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)PyPacket_init,	/* tp_init */
    0,                         /* tp_alloc */
    PyPacket_new                 /* tp_new */
};

static PyMethodDef module_null_methods[] = {
    {NULL}  
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
	PyObject *m_name = 0, *m;
	int ret;

	switch ( pius->ordo )
	{
	case Notitia::TIMER_HANDLE:
		WBUG("facio TIMER_HANDLE");
		clr_timer_pius.indic = pius->indic;
		break;

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
			((PyAmorObj*) pInstance)->owner = this;
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
		if (PyType_Ready(&PyPiusType) < 0) break;
		if (PyType_Ready(&PyAmorType) < 0) break;
		if (PyType_Ready(&PyTBufferType) < 0) break;
		if (PyType_Ready(&PyPacketType) < 0) break;

		m = Py_InitModule("textor", module_null_methods);
		if ( !m ) {
			WLOG(WARNING,"Py_InitModule of (textor) failed");
			break;
		} else {
			WBUG("Py_InitModule of (textor) ok!");
		}

		aptus_error = PyErr_NewException((char*)"textor.TextorError", 0, 0);
		Py_INCREF(aptus_error);
		PyModule_AddObject(m, "error", aptus_error);

#define ADD_NOTI(X) PyModule_AddIntConstant(m, "_"#X, Notitia::X);

		ADD_NOTI(MAIN_PARA) 
		ADD_NOTI(WINMAIN_PARA)
		ADD_NOTI(CMD_MAIN_EXIT)
		ADD_NOTI(CLONE_ALL_READY)
		ADD_NOTI(CMD_GET_OWNER)
		ADD_NOTI(SET_SAME_PRIUS)
		ADD_NOTI(WHO_AM_I)
		ADD_NOTI(IGNITE_ALL_READY)
		ADD_NOTI(LOG_EMERG)
		ADD_NOTI(LOG_ALERT)
		ADD_NOTI(LOG_CRIT)
		ADD_NOTI(LOG_ERR)
		ADD_NOTI(LOG_WARNING)
		ADD_NOTI(LOG_NOTICE)
		ADD_NOTI(LOG_INFO)
		ADD_NOTI(LOG_DEBUG)
		ADD_NOTI(FAC_LOG_EMERG)
		ADD_NOTI(FAC_LOG_ALERT)
		ADD_NOTI(FAC_LOG_CRIT)
		ADD_NOTI(FAC_LOG_ERR)
		ADD_NOTI(FAC_LOG_WARNING)
		ADD_NOTI(FAC_LOG_NOTICE)
		ADD_NOTI(FAC_LOG_INFO)
		ADD_NOTI(FAC_LOG_DEBUG)
		ADD_NOTI(CMD_GET_VERSION)
		ADD_NOTI(CMD_ZERO_FILE)
		ADD_NOTI(CMD_CLOSE_FILE)
		ADD_NOTI(CMD_GET_SCHED)
		ADD_NOTI(CMD_PUT_PENDOR)
		ADD_NOTI(DMD_SCHED_RUN)
		ADD_NOTI(CMD_GET_PIUS)
		ADD_NOTI(DMD_CONTINUE_SELF)
		ADD_NOTI(DMD_STOP_NEXT)
		ADD_NOTI(DMD_CONTINUE_NEXT)
		ADD_NOTI(CMD_ALLOC_IDLE)
		ADD_NOTI(CMD_FREE_IDLE)
		ADD_NOTI(DMD_CLONE_OBJ)
		ADD_NOTI(CMD_INCR_REFS)
		ADD_NOTI(CMD_DECR_REFS)
		ADD_NOTI(JUST_START_THREAD)
		ADD_NOTI(FINAL_END_THREAD)

		ADD_NOTI(SET_EVENT_HD)
		ADD_NOTI(CLR_EVENT_HD)
		ADD_NOTI(PRO_EVENT_HD)
		ADD_NOTI(ERR_EVENT_HD)

		ADD_NOTI(LOG_VAR_EMERG)
		ADD_NOTI(LOG_VAR_ALERT)
		ADD_NOTI(LOG_VAR_CRIT)
		ADD_NOTI(LOG_VAR_ERR)
		ADD_NOTI(LOG_VAR_WARNING)
		ADD_NOTI(LOG_VAR_NOTICE)
		ADD_NOTI(LOG_VAR_INFO)
		ADD_NOTI(LOG_VAR_DEBUG)

		ADD_NOTI(MORE_DATA_EPOLL)
		ADD_NOTI(POST_EPOLL)
		ADD_NOTI(SIGNAL_EPOLL)
		ADD_NOTI(AIO_EPOLL)
		ADD_NOTI(ACCEPT_EPOLL)
		ADD_NOTI(SET_EPOLL)
		ADD_NOTI(CLR_EPOLL)
		ADD_NOTI(PRO_EPOLL)
		ADD_NOTI(WR_EPOLL)
		ADD_NOTI(RD_EPOLL)
		ADD_NOTI(EOF_EPOLL)
		ADD_NOTI(ERR_EPOLL)

		ADD_NOTI(NEW_SESSION)
		ADD_NOTI(END_SERVICE)
		ADD_NOTI(CMD_RELEASE_SESSION)
		ADD_NOTI(NT_SERVICE_PAUSE)
		ADD_NOTI(NT_SERVICE_RESUME)

		ADD_NOTI(CHANNEL_TIMEOUT)
		ADD_NOTI(CMD_CHANNEL_PAUSE)
		ADD_NOTI(CMD_CHANNEL_RESUME)
		ADD_NOTI(CHANNEL_NOT_ALIVE)
		ADD_NOTI(CMD_NEW_SERVICE)
		ADD_NOTI(START_SERVICE)
		ADD_NOTI(DMD_END_SERVICE)
		ADD_NOTI(DMD_BEGIN_SERVICE)
		ADD_NOTI(END_SESSION)
		ADD_NOTI(DMD_END_SESSION)
		ADD_NOTI(START_SESSION)
		ADD_NOTI(DMD_START_SESSION)
		ADD_NOTI(SET_TBUF)
		ADD_NOTI(PRO_TBUF)
		ADD_NOTI(GET_TBUF)
		ADD_NOTI(ERR_FRAME_LENGTH)
		ADD_NOTI(ERR_FRAME_TIMEOUT)

		ADD_NOTI(FD_SETRD)
		ADD_NOTI(FD_SETWR)
		ADD_NOTI(FD_SETEX)
		ADD_NOTI(FD_CLRRD)
		ADD_NOTI(FD_CLRWR)
		ADD_NOTI(FD_CLREX)
		ADD_NOTI(FD_PRORD)
		ADD_NOTI(FD_PROWR)
		ADD_NOTI(FD_PROEX)

		ADD_NOTI(TIMER)
		ADD_NOTI(DMD_SET_TIMER)
		ADD_NOTI(DMD_CLR_TIMER)
		ADD_NOTI(DMD_SET_ALARM)
		ADD_NOTI(TIMER_HANDLE)
		ADD_NOTI(PRO_HTTP_HEAD)
		ADD_NOTI(CMD_HTTP_GET)
		ADD_NOTI(CMD_HTTP_SET)
		ADD_NOTI(CMD_GET_HTTP_HEADBUF)
		ADD_NOTI(CMD_GET_HTTP_HEADOBJ)
		ADD_NOTI(CMD_SET_HTTP_HEAD)
		ADD_NOTI(PRO_HTTP_REQUEST)
		ADD_NOTI(PRO_HTTP_RESPONSE)
		ADD_NOTI(HTTP_Request_Complete)
		ADD_NOTI(HTTP_Response_Complete)
		ADD_NOTI(HTTP_Request_Cleaned)
		ADD_NOTI(GET_COOKIE)
		ADD_NOTI(SET_COOKIE)
		ADD_NOTI(GET_DOMAIN)
		ADD_NOTI(HTTP_ASKING)
		ADD_NOTI(WebSock_Start)
		ADD_NOTI(WebSock_End)
		ADD_NOTI(Get_WS_MsgType)
		ADD_NOTI(Set_WS_MsgType)
		ADD_NOTI(SET_TINY_XML)
		ADD_NOTI(PRO_TINY_XML)
		ADD_NOTI(PRO_SOAP_HEAD)
		ADD_NOTI(PRO_SOAP_BODY)
		ADD_NOTI(ERR_SOAP_FAULT)
		ADD_NOTI(CMD_GET_FD)
		ADD_NOTI(CMD_SET_PEER)
		ADD_NOTI(CMD_GET_PEER)
		ADD_NOTI(CMD_GET_SSL)
		ADD_NOTI(CMD_GET_CERT_NO)
		ADD_NOTI(SET_WEIGHT_POINTER)
		ADD_NOTI(TRANS_TO_SEND)
		ADD_NOTI(TRANS_TO_RECV)
		ADD_NOTI(TRANS_TO_HANDLE)
		ADD_NOTI(CMD_BEGIN_TRANS)
		ADD_NOTI(CMD_CANCEL_TRANS)
		ADD_NOTI(CMD_FAIL_TRANS)
		ADD_NOTI(CMD_RETAIN_TRANS)
		ADD_NOTI(CMD_END_TRANS)
		ADD_NOTI(CMD_FORK)
		ADD_NOTI(FORKED_PARENT)
		ADD_NOTI(FORKED_CHILD)
		ADD_NOTI(Pro_File_Err_Op)
		ADD_NOTI(Pro_File_Open)
		ADD_NOTI(Pro_File_Err_Rd)
		ADD_NOTI(Pro_File_End)
		ADD_NOTI(Move_File_From_Current)
		ADD_NOTI(Move_File_From_Begin)
		ADD_NOTI(Move_File_From_End)
		ADD_NOTI(NEW_HOLDING)
		ADD_NOTI(AUTH_HOLDING)
		ADD_NOTI(HAS_HOLDING)
		ADD_NOTI(CMD_SET_HOLDING)
		ADD_NOTI(CMD_CLR_HOLDING)
		ADD_NOTI(CLEARED_HOLDING)
		ADD_NOTI(PRO_FILE)
		ADD_NOTI(GET_FILE)
		ADD_NOTI(SET_UNIPAC)
		ADD_NOTI(PRO_UNIPAC)
		ADD_NOTI(ERR_UNIPAC_COMPOSE)
		ADD_NOTI(ERR_UNIPAC_RESOLVE)
		ADD_NOTI(ERR_UNIPAC_INFO)
		ADD_NOTI(MULTI_UNIPAC_END)
		ADD_NOTI(Set_InsWay)
		ADD_NOTI(Pro_InsWay)
		ADD_NOTI(Ans_InsWay)
		ADD_NOTI(Pro_TranWay)
		ADD_NOTI(Ans_TranWay)
		ADD_NOTI(Log_InsWay)
		ADD_NOTI(Comm_Recv_Timeout)
		ADD_NOTI(Comm_Event_Break)
		ADD_NOTI(Comm_Event_CTS)
		ADD_NOTI(Comm_Event_DSR)
		ADD_NOTI(Comm_Event_Err)
		ADD_NOTI(Comm_Event_Ring)
		ADD_NOTI(Comm_Event_RLSD)
		ADD_NOTI(Comm_Event_RxChar)
		ADD_NOTI(Comm_Event_RxFlag)
		ADD_NOTI(Comm_Event_TxEmpty)
		ADD_NOTI(Comm_Err_Break)
		ADD_NOTI(Comm_Err_Frame)
		ADD_NOTI(Comm_Err_OverRun)
		ADD_NOTI(Comm_Err_RxOver)
		ADD_NOTI(Comm_Err_RxParity)
		ADD_NOTI(CMD_SET_DBFACE)
		ADD_NOTI(CMD_SET_DBCONN)
		ADD_NOTI(CMD_DBFETCH)
		ADD_NOTI(CMD_GET_DBFACE)
		ADD_NOTI(CMD_DB_CANCEL)
		ADD_NOTI(PRO_DBFACE)
		ADD_NOTI(IC_DEV_INIT_BACK)
		ADD_NOTI(IC_DEV_INIT)
		ADD_NOTI(IC_DEV_QUIT)
		ADD_NOTI(IC_OPEN_PRO)
		ADD_NOTI(IC_CLOSE_PRO)
		ADD_NOTI(IC_PRO_COMMAND)
		ADD_NOTI(IC_SAM_COMMAND)
		ADD_NOTI(IC_RESET_SAM)
		ADD_NOTI(IC_PRO_PRESENT)
		ADD_NOTI(ICC_Authenticate)
		ADD_NOTI(ICC_Read_Sector)
		ADD_NOTI(ICC_Write_Sector)
		ADD_NOTI(ICC_Reader_Version)
		ADD_NOTI(ICC_Led_Display)
		ADD_NOTI(ICC_Audio_Control)
		ADD_NOTI(ICC_GetOpInfo)
		ADD_NOTI(ICC_Get_Card_RFID)
		ADD_NOTI(ICC_Get_CPC_RFID)
		ADD_NOTI(ICC_Get_Flag_RFID)
		ADD_NOTI(ICC_Get_Power_RFID)
		ADD_NOTI(ICC_Set433_Mode_RFID)
		ADD_NOTI(ICC_Get433_Mode_RFID)
		ADD_NOTI(ICC_CARD_open)
		ADD_NOTI(URead_ReLoad_Dll)
		ADD_NOTI(URead_UnLoad_Dll)
		ADD_NOTI(URead_Load_Dll)

		Py_INCREF(&PyPiusType);
		PyModule_AddObject(m, "Pius", (PyObject *)&PyPiusType);

		Py_INCREF(&PyAmorType);
		PyModule_AddObject(m, "Amor", (PyObject *)&PyAmorType);

		Py_INCREF(&PyTBufferType);
		PyModule_AddObject(m, "TBuffer", (PyObject *)&PyTBufferType);

		Py_INCREF(&PyPacketType);
		PyModule_AddObject(m, "Packet", (PyObject *)&PyPacketType);

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
		ret = PyRun_SimpleString("import textor");
		if ( ret ) 
		{
			WLOG(WARNING,"python run %s return %d (failed!)", "import textor", ret);
		} else {
			WBUG("import textor ok!");
		}
*/

		m_name = PyString_FromString(gCFG->pyMod_str);
		if ( !m_name ) 
		{
			WLOG(WARNING,"PyString_FromString (%s) failed", gCFG->pyMod_str);
			break;
		}
		gCFG->pModule = PyImport_Import(m_name);      
		Py_DECREF(m_name);
		if ( !gCFG->pModule) 
		{
			WLOG(WARNING,"PyImport_Import module of (%s) failed", gCFG->pyMod_str);
			fetch_error();
			//PyErr_Print();
			//PyErr_Clear();
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
			WLOG(WARNING,"PyObject_CallObject of class (%s) failed", gCFG->pyClass_str);
			break;
		} else {
			((PyAmorObj*) pInstance)->owner = this;
			WBUG("PyObject_CallObject of class (%s) %p", gCFG->pyClass_str, pInstance);
		}
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
	clr_timer_pius.ordo = Notitia::DMD_CLR_TIMER;
	clr_timer_pius.indic = this;
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

bool PyPort::facioPy( Amor::Pius *pius)
{
	bool ret = false;
	if ( pInstance )
	{
		ret = pius2py(pius, fac_method, "facio");
	}
	return ret;
}

void PyPort::free_aps(Amor::Pius &aps)
{
	/* 接下去要做 ps_obj.indic(Python)到pius.indic(C++)的工作 */
	if ( !aps.indic ) return;
	switch ( aps.ordo )
	{
	case Notitia::SET_TBUF:
	case Notitia::SET_UNIPAC:
		delete [](void**)aps.indic;
		break;
	case Notitia::PRO_TBUF:
	case Notitia::SET_TINY_XML:
		break;

	case Notitia::ERR_SOAP_FAULT:
	case Notitia::PRO_SOAP_HEAD:
	case Notitia::PRO_SOAP_BODY:	
		break;	

	case Notitia::CMD_SET_DBFACE:	
		/* 这个由DBPort发出, 不会有Java到C++的情况*/
		break;	

	case Notitia::DMD_SET_TIMER:
		/* ps.indic 是一个java.lang.integer, 转成int, 并且还要加一个jvmport的指针 */
		break;

	case Notitia::Get_WS_MsgType:
	case Notitia::Set_WS_MsgType:
		/* ps.indic 是一个java.lang.integer, 转成unsigned char*, 并且还要加一个jvmport的指针 */
		delete (unsigned char*)aps.indic;
		break;

	case Notitia::DMD_SET_ALARM:
		/* ps.indic 是一个java.lang.integer, 转成int, 并且还要加一个jvmport的指针 */
		delete[] (int*)(((void **)aps.indic)[1]);
		delete [](void**)aps.indic;
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

/* 从Python脚本到C++程序, 为C++生成Pius, 调用的还需要调用 free_aps*/
bool PyPort::get_aps(Amor::Pius &aps, PyObject *args, const char *err_msg)
{
	PyPiusObj *ps_obj;
	PyTBufferObj *a_tb = 0, *b_tb=0;
	PyPacketObj *a_pac = 0, *b_pac=0;

	if (!PyArg_ParseTuple(args, err_msg, &ps_obj)) 
		return false;
	aps.ordo = ps_obj->ordo;
	aps.subor = ps_obj->subor;
	aps.indic = 0;
	if (!ps_obj->indic )  {
		WBUG("aps.indic is null"); 
		return true;
	}
	/* 接下去要做 ps_obj.indic(Python)到pius.indic(C++)的工作 */
	switch ( aps.ordo )
	{
	case Notitia::SET_TBUF:
		WBUG("aps.indic get_aps SET_TBUF");
		if ( !PyList_Check(ps_obj->indic) ) {
			PyErr_SetString(aptus_error, "Pius.indic is not PyListObject!");
			return false;
		}
		a_tb = (PyTBufferObj *)PyList_GetItem(ps_obj->indic, 0);
		b_tb = (PyTBufferObj *)PyList_GetItem(ps_obj->indic, 1);
		if ( !a_tb || !b_tb )
		{
			WLOG(WARNING,"aps.indic a_tb or b_tb is null when SET_TBUF");
			break;
		}
		if ( !PyObject_IsInstance((PyObject*)a_tb, (PyObject*)&PyTBufferType)
			 || !PyObject_IsInstance((PyObject*)b_tb, (PyObject*)&PyTBufferType) )
		{
			WLOG(WARNING,"aps.indic a_tb or b_tb is not of PyTBufferObj when SET_TBUF");
			break;
		}
		//printf("a_tb.tb = %p, b_tb.tb=%p\n", a_tb->tb, b_tb->tb);
		aps.indic = new void*[2];
		((void**)(aps.indic))[0] = (void*)(a_tb->tb);
		((void**)(aps.indic))[1] = (void*)(b_tb->tb);
		break;

	case Notitia::SET_UNIPAC:
		WBUG("aps.indic get_aps SET_UNIPAC");
		if ( !PyList_Check(ps_obj->indic) ) {
			PyErr_SetString(aptus_error, "Pius.indic is not PyListObject!");
			return false;
		}
		a_pac = (PyPacketObj *)PyList_GetItem(ps_obj->indic, 0);
		b_pac = (PyPacketObj *)PyList_GetItem(ps_obj->indic, 1);
		if ( !a_pac || !b_pac )
		{
			WLOG(WARNING,"aps.indic a_pac or b_pac is null when SET_UNIPAC");
			break;
		}
		if ( !PyObject_IsInstance((PyObject*)a_pac, (PyObject*)&PyPacketType)
			 || !PyObject_IsInstance((PyObject*)b_pac, (PyObject*)&PyPacketType) )
		{
			WLOG(WARNING,"aps.indic a_pac or b_pac is not of PyPacketObj when SET_UNIPAC");
			break;
		}
//		printf("a_pac.pac = %p, b_pac.pac=%p\n", a_pac->pac, b_pac->pac);
		aps.indic = new void*[2];
		((void**)(aps.indic))[0] = (void*)(a_pac->pac);
		((void**)(aps.indic))[1] = (void*)(b_pac->pac);
		break;

	case Notitia::Get_WS_MsgType:
	case Notitia::Set_WS_MsgType:
		/* ps.indic 是一个PyObject*, 转成unsigned char*, 并且还要加一个jvmport的指针 */
	{
		unsigned char *opcode = new unsigned char;
		if ( !PyInt_Check(ps_obj->indic) ) {
			WLOG(WARNING,"aps.indic is not of PyInt_Type! when Get/Set_WS_MsgType");
			return false;
		}
		*opcode = (unsigned char)(PyInt_AS_LONG(ps_obj->indic)&0xFF);
		aps.indic = opcode;
	}
		break;

	case Notitia::DMD_SET_ALARM:
		/* ps_obj.indic 是一个PyInt_Type, 转成int, 并且还要加一个this指针 */
	{
		void **indp = new void* [3];
		int *click = new int[2];
		PyObject *c1, *c2;
		if ( !PyList_Check(ps_obj->indic) ) {
			PyErr_SetString(aptus_error, "Pius.indic is not PyListObject!");
			return false;
		}
		c1 = PyList_GetItem(ps_obj->indic, 0);
		c2 = PyList_GetItem(ps_obj->indic, 1);

		if ( !PyInt_Check(c1)  || !PyInt_Check(c2) ) {
			WLOG(WARNING,"aps.indic is not of PyInt_Type! when DMD_SET_ALARM");
			return false;
		}
		click[0] = (int)(PyInt_AS_LONG(c1)&0xFFFFFFFF);
		click[1] = (int)(PyInt_AS_LONG(c2)&0xFFFFFFFF);
		indp[0] = this;
		indp[1] = &click[0];
		indp[2] = &click[1];
		aps.indic = indp;
	}
		break;

	case Notitia::DMD_SET_TIMER:
		/* ps.indic  */
		aps.indic = this;
		break;

	case Notitia::DMD_CLR_TIMER:
		/* ps.indic 指向 timer_handle */
		aps.indic = clr_timer_pius.indic;
		break;

	case Notitia::PRO_TBUF:
	case Notitia::PRO_UNIPAC:
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
		break;

#ifdef TTT
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

	case Notitia::CMD_HTTP_GET:
		/* indic 指向一个指针struct GetRequestCmd* */
		break;
	
	case Notitia::CMD_HTTP_SET:
		/* indic 指向一个指针struct SetResponseCmd* */
		break;
#endif

	default :
		break;
	}
	return true;
}

/* 从C++程序到Python脚本, 为python生成合适的对象, and facio or sponte */
bool PyPort::pius2py (Pius *pius, char *py_method , const char *meth_str)
{
	PyObject *t, *obj_list=0, *ret_obj=0;
	PyTBufferObj *a_tb = 0, *b_tb=0;
	PyPacketObj *a_pac= 0, *b_pac=0;
	TBuffer **tmt=0;
	PacketObj **tmp=0;
	PyPiusObj *ps_obj = 0;

	ps_obj = (PyPiusObj *)PyObject_CallObject((PyObject*)&PyPiusType, NULL);
	if ( !ps_obj ) 
	{
		WLOG(WARNING, "PyObject_CallObject(PyPiusType) failed when %s ordo=%d", meth_str, pius->ordo);
		return false;
	} 
	ps_obj->ordo = pius->ordo;
	ps_obj->subor = pius->subor;
	/* 下面根据ordo来生成 ps_obj中的indic, 对付各种TBuffer等 */
	switch ( pius->ordo )
	{
	case Notitia::SET_TBUF:
		WBUG("%s SET_TBUF", meth_str);
		t = PyTuple_New(1);
		PyTuple_SetItem(t, 0, PyInt_FromLong(0L));
		a_tb =  (PyTBufferObj *)PyObject_CallObject((PyObject*)&PyTBufferType, t);
		b_tb =  (PyTBufferObj *)PyObject_CallObject((PyObject*)&PyTBufferType, t);
		//printf("!! a_tb %p, b_tb %p ---t = %p---\n", a_tb, b_tb, t);
		Py_DECREF(t);
		if ( (tmt = (TBuffer **)(pius->indic)))
		{
			if ( *tmt) a_tb->tb = *tmt; 
			else
				WLOG(WARNING, "%s SET_TBUF first is null", meth_str);
			tmt++;
			if ( *tmt) b_tb->tb = *tmt;
			else
				WLOG(WARNING, "%s SET_TBUF second is null", meth_str);
		} else 
			WLOG(WARNING, "%s SET_TBUF null", meth_str);
		obj_list = PyList_New(0);
		if ( !obj_list ) 
		{
			WLOG(WARNING, "PyList_New return NULL when %s SET_TBUF", meth_str);
			goto FAIL;
		}
		if ( PyList_Append(obj_list, (PyObject*)a_tb) == -1 || PyList_Append(obj_list, (PyObject*)b_tb) ==-1 )
		{
			WLOG(WARNING, "PyList_Append failed when %s SET_TBUF", meth_str);
			goto NFAIL;
		}
		ps_obj->indic = obj_list;
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
	NFAIL:
		Py_DECREF(obj_list);
	FAIL:
		Py_DECREF(a_tb);
		Py_DECREF(b_tb);
		break;

	case Notitia::SET_UNIPAC:
		WBUG("%s SET_UNIPAC", meth_str);
		t = PyTuple_New(1);
		PyTuple_SetItem(t, 0, PyInt_FromLong(0L));
		a_pac =  (PyPacketObj *)PyObject_CallObject((PyObject*)&PyPacketType, t);
		b_pac =  (PyPacketObj *)PyObject_CallObject((PyObject*)&PyPacketType, t);
		//printf("!! a_pac %p, b_pac %p ---t = %p---\n", a_pac, b_pac, t);
		Py_DECREF(t);
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) a_pac->pac = *tmp; 
			else
				WLOG(WARNING, "%s SET_UNIPAC first is null", meth_str);
			tmp++;
			if ( *tmp) b_pac->pac = *tmp;
			else
				WLOG(WARNING, "%s SET_UNIPAC second is null", meth_str);
		} else 
			WLOG(WARNING, "%s SET_UNIPAC null", meth_str);
		obj_list = PyList_New(0);
		if ( !obj_list ) 
		{
			WLOG(WARNING, "PyList_New return NULL when %s SET_UNIPAC", meth_str);
			goto PFAIL;
		}
		if ( PyList_Append(obj_list, (PyObject*)a_pac) == -1 || PyList_Append(obj_list, (PyObject*)b_pac) ==-1 )
		{
			WLOG(WARNING, "PyList_Append failed when %s SET_UNIPAC", meth_str);
			goto QFAIL;
		}
		ps_obj->indic = obj_list;
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
	QFAIL:
		Py_DECREF(obj_list);
	PFAIL:
		Py_DECREF(a_pac);
		Py_DECREF(b_pac);
		break;

	case Notitia::PRO_TBUF:
	case Notitia::PRO_UNIPAC:
		WBUG("%s PRO_TBUF/PRO_UNIPAC", meth_str);
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
		break;

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

	case Notitia::WebSock_Start:
		WBUG("%s WebSock_Start", meth_str);
		ps_obj->indic = PyString_FromString((const char*)pius->indic);
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
		Py_DECREF(ps_obj->indic); 
		break;

	case Notitia::MAIN_PARA:
		/*  *indic[0] = argc, *indic[1] = argv, 将此转为String[] */
		WBUG("%s MAIN_PARA", meth_str);
	{
		void **ps = (void**)pius->indic;
		int num = (*(int *)ps[0]);
		char **argv = (char **)ps[1];
		obj_list = PyList_New(0);
		PyObject *num_obj;
		if ( !obj_list ) 
		{
			WLOG(WARNING, "PyList_New return NULL when %s MAIN_PARA", meth_str);
			goto PFAIL1;
		}
		num_obj = PyInt_FromLong((long)num);
		if ( PyList_Append(obj_list, num_obj) == -1 ) 
		{
			goto QFAIL1;
		}
		for ( int i = 0; i < num ; i++ )
		{
			if ( PyList_Append(obj_list, PyString_FromString((const char*)argv[i])) == -1 )
			{
				WLOG(WARNING, "PyList_Append argv failed when %s MAIN_PARA", meth_str);
				goto QFAIL1;
			}
		}
		ps_obj->indic = obj_list;
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
	QFAIL1:
		Py_DECREF(num_obj);
		Py_DECREF(obj_list);
	PFAIL1:
		break;
	}

	case Notitia::TIMER:
		/* 这些要转一个PyInt_Type, 这个不需要了 */
		/*
		ps_obj->indic = PyInt_FromLong((long)*((int*) (pius->indic)));
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
		Py_DECREF(ps_obj->indic);
		*/
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
		ret_obj = PyObject_CallMethod(pInstance, py_method, (char*)"O", ps_obj);
		break;

	default :
		break;
	}
LAST:
	Py_DECREF(ps_obj);
	if ( ret_obj && PyObject_Compare(ret_obj, Py_True) == 0 )
	{
		return true;
	} else {
		if (PyErr_Occurred() ) 
		{
			fetch_error();
			//PyErr_Print();
			//PyErr_Clear();
		}
		return false;
	}
}

void PyPort::fetch_error()
{
	PyObject *type =NULL, *value =NULL, *traceback =NULL;
	PyErr_Fetch(&type, &value, &traceback);
	char *str, *ptr;
	TBuffer buf;
	PyObject *pystr, *module_name, *pyth_module, *pyth_func;
	if(type)
	{
		ptr = 0; str = 0;
		str = PyExceptionClass_Name(type);
		if ( str ) { 
			ptr = strpbrk(str,".");
			if ( ptr ) { 
				ptr++;
				buf.input((unsigned char*)ptr, strlen(ptr));
			} else {
				buf.input((unsigned char*)str, strlen(str));
			}
			buf.input((unsigned char*)": ", 2);
		}
	}

	if(value)
	{
		PyObject *line = PyObject_Str(value);
		if ( line ) {
			str = PyString_AsString(PyObject_Str(value)); 
			buf.input((unsigned char*)str, strlen(str));
		}
	}
	if(!traceback) goto LAST;
/*
		for (PyTracebackObject *tb = (PyTracebackObject *)traceback; NULL!= tb; tb = tb->tb_next)
		{
			PyObject *line = PyUnicode_FromFormat(" File \"%U\", line %d, in %U\n",
				tb->tb_frame->f_code->co_filename,
				tb->tb_lineno,
				tb->tb_frame->f_code->co_name);
				//PyUnicode_1BYTE_DATA(line);
		}
*/
	/* See if we can get a full traceback */
	module_name = PyString_FromString("traceback");
	pyth_module = PyImport_Import(module_name);
	Py_DECREF(module_name);

	if (pyth_module == NULL) goto LAST;

	pyth_func = PyObject_GetAttrString(pyth_module, "format_exception");
	if (pyth_func && PyCallable_Check(pyth_func)) 
	{
		PyObject *pyth_val;
		pyth_val = PyObject_CallFunctionObjArgs(pyth_func, type, value, traceback, NULL);

		pystr = PyObject_Str(pyth_val);
		str = PyString_AsString(pystr);
		buf.input((unsigned char*)"\t", 1);
		buf.input((unsigned char*)str, strlen(str));
		Py_DECREF(pyth_val);
    	}
LAST:
	buf.input((unsigned char*)"\0", 1);
	WLOG(ERR, (char*)buf.base);

}
#include "hook.c"

