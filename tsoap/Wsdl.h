/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Wsdl class
 Build: created by octerboy, 2006/12/01
 $Header: /textus/tsoap/Wsdl.h 12    08-01-02 0:00 Octerboy $
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef WSDL_H
#define WSDL_H
#include "tinyxml.h"
#include "Array.h"
#include "casecmp.h"
#define N_SIZE 128
class VName {
public:
	char *name;
	VName () { name = 0;}
	inline ~VName () { if (name) delete[] name; }
	VName (const char *nm);
};

class UData:public VName {
public:
	int type;
	void operator = (int val) { type =  val; };
	inline bool operator == (int val) {
		return ( (val ==type )  );
	};

	bool operator != (int val) {
		return ( (val != type )  );
	}; 

	inline void clear () {
		type = -1;
	};

	inline UData () {
		clear();
	};

	UData ( const char *nm):VName(nm) { clear(); }
};

class NString {
public:
	char *nm;
	char *value;
	char *result;
	void operator = (const char *val) ;
	void operator = (NString &) ;
	char* operator + ( const char *val);
	inline bool operator == (NString &val) {
		return *this == val.value;
	};
	inline bool operator == (const char *val) {

		if ( val == value )
			return true;
		else if ( val && value )
			return ( (strcmp(val, value) == 0) );
		else if ( !val && !value )
			return true;
		else
			return false;
	};

	bool operator != (const char *val) {
		if ( val == value )
			return false;
		else if ( val && value )
			return ( (strcmp(val, value) != 0) );
		else 
			return true;
	}; 

	inline void clear() {
		value = 0;
		nm = 0;
		result = 0;
	};

	inline NString() {
		clear();
	};

	NString( const char *nm);

	inline ~NString() {
		if ( value)
			delete[] value;

		if ( nm)
			delete[] nm;
		clear();
	};
};

class Element: public VName {
public:
	bool fArray;	/* �Ƿ�Ϊ����? ��: maxOccursֵ > 1 */
	char *type;	/* �������ͣ� type="s:float"�е�float, ���s��ָxsd�������ռ�, �����"s:anyType"��Ϊstring
				����wsdl�ĵ���element�ڵ�û��"type"��"xsi:type"���Ե�, ������Ϊname + "_" +name
			 */
	char *ns;	/* type="s:float"�е�s, ���Ϊ��, ��ȡschema��qdef */
	//sizeArray;
	inline Element ( char *nm ):VName(nm) {} ;
	inline Element () {
		clear();
	};
	inline void clear () {
		fArray = false;
		type = 0;
		ns = 0;
	};
};

class Attribute: public VName {
public:
	char *fixed;
	bool allowed;
	bool fAttribute;
	char *type;
	char *ns;
	inline Attribute (char* nm ):VName(nm) { };
	inline Attribute () {
		name = 0;
	};
};

class Type: public VName {
public:
	//char *name;	/* ����simpleType, ��wsdl�ĵ����ж���, һ��������element�ڵ�name���Ե�����(name_name),���Ǹ�������ÿ��Ӧ��ͬ */
	char *ns;	/* ����simpleType���ӽڵ�Ϊ"restriction'��
				����ӽڵ�û��base����, ��ns="xsd", data_kind="string";
				�����base����,��ȡbase���ݵ�ǰ׺,data_kind
			   
			   ����simpleType���ӽڵ�Ϊlist��union��ns="xsd", data_kind="string"
			*/
	char *data_kind;
	inline Type (char *nm):VName(nm) { };
	inline Type () {
		name = ns = data_kind = 0;
	};
};

class ComplexType:public VName {
public:
	//char *name;	/* ��wsdl�ĵ����ж���,һ��������element�ڵ��name���Ե�����(name_name),���Ǹ�������ÿ��Ӧ��ͬ */
	int lex;	/* Element����, Attribute���� */
	void *arr;
	
	inline ComplexType ( char*nm):VName(nm) {};
	inline ComplexType () {
		name = 0;
	};
};

class Part: public VName {
public:
	//char *name;	/* �ڵ��е�name��������, ��������, ÿ������ͬ */
	char *type;	/* ȡ������, ����ڵ�������Ϊ"anyType",��ǰ׺Ϊ"xsd"�����Ϊ"string" */
	char *elem;	/* element���ԵĻ����� */
	char *ns;	/* ǰ׺�� ��part�ڵ���, type�������ݵ�ǰ׺Ϊ����,����ȡelement�������ݵ�ǰ׺ */
	inline Part ( const char* nm):VName(nm) { clear(); } ;
	inline Part () {
		clear();
	};

	inline void clear() {
		type = 0;
		elem = 0;
		ns = 0;
	};
	void setType ( const char*) ;
	void setElem ( const char*) ;
	void setNs ( const char*) ;

	inline ~Part() {
		if ( type) delete []type;
		if ( elem) delete []elem;
		if ( ns) delete []ns;
	};
};
class Wsdl ;
class HDRInfo {
public:
	char *name;
	const char *ns;
	const char *es;
	bool fLiteral;
	bool fRequired;
	Part *type;
	inline HDRInfo ( const char* nm) { clear();};
	inline  HDRInfo () {
		clear();
	};
	inline void clear() {
		name = 0;
		ns = 0;
		es = 0;
		fLiteral = false;
		fRequired = false;
		type = 0;
	};
	void parse(Wsdl *, TiXmlElement *);
};

class Message:public VName {
public:
	//char *name;	/* message�ڵ��name��������, �������� */
	Array<Part> args;	/* message�ڵ��µ�part���� */
	const char *opname;	/* portType/operation�ڵ��name����,  portType/operation/inputֻ��ָ��һ��message. */
	const char *soapAction;	
	char *soapAction2;	
				
	bool fWrapped;	/* ���ֻ��һ��Part(args),�ҷǿգ�����(Part->nameΪ"parameter" �� typet��elem��һ����opname��ͬ)*/
	
	bool fOneWay;	/* ���portType/operation�ڵ�û��output�ӽڵ�, */
	Message  *response;	/* ָ��portType/operation/output�ڵ���message����������ָ����Msg */
	
	const char *ns;	/* ����Ӧ��binding/operation/input�ڵ���namespace�������� */
	const char *es;	/* ����Ӧ��binding/operation/input�ڵ���encodingStyle�������� */
			/* binding/operation/inputֻ��ָ��һ��portType/operation/input */
	bool fLiteral;	/* ����Ӧ��binding/operation/input�ڵ���use��������Ϊ"literal" */
	Array<HDRInfo> hdrsIn; /* ����Ӧ��binding/operation/input/soap:header���� */
	Array<HDRInfo> hdrsOut;/* ����Ӧ��binding/operation/output/soap:header���� */
	bool fRpc;	/* ����Ӧ��binding/operation/soap:operation��style��������Ϊ"rpc"*/

	inline Message ( const char*nm):VName(nm) { clear();};
	inline  Message () {
		clear();
	};

	inline  ~Message () {
		if ( soapAction2) delete[] soapAction2;
	};

	inline void clear() {
		opname = 0;
		soapAction = 0;
		soapAction2 = 0;
		fWrapped = false;
		fOneWay = 0;
		response = 0;
		ns = es = 0;
		fLiteral = false;
		fRpc = false;
	};
};
#ifdef USEMESSP
class MessP:public VName {
public:
	Message *msgp;
	inline MessP ( char *nm) :VName(nm) { clear();};
	inline MessP () { clear(); };

	inline void clear() { msgp = 0; };
};
#endif

class Operation:public VName {	
public :
	const char *siname;	/* ĳ��portType�µĸ�operation/input��name����, һ��ȱʡ, ��ȡsOpName */
	Message *msgp;		/* service/port��binding�������ݻ�����ָ��������һ��bindingԪ��.  ��bindingԪ��(type����ָ��)��Ӧһ��portType, ��portType��operation/inputָ������Ӧ��message��*/

	inline Operation ( const char *nm) :VName(nm) { clear();};
	inline Operation () { clear(); };

	inline void clear() {
		siname = 0;
		msgp = 0;
	};
};

class PortType:public VName {
public:
	//char *name;	/* service/port��name��������, �������ֵ*/
	char *location;	/* service/port/soap:address��location�������� */
	Array<Operation>	ops;	/* ÿ��operation */
	
	inline void clear () { location = 0; };

	inline PortType ( const char*nm):VName(nm) { clear(); };
	inline  PortType () { clear(); };
};

class Schema {
public:
	char *uri;	/* WSDL�ĵ���typesԪ�ص�ÿ���ӽڵ�(schema)��targetNStringace��������, ����ÿ������ͬ�� */
	char efd[N_SIZE];	/* "elementFormDefault"���� */
	char afd[N_SIZE];	/* "attributeFormDefault"���� */
	char *qdef;	/* targetNStringace��ָ���ݵı���, ��Att *ns��Ѱ�� */
	char *namespURI;	/* ��typesԪ�ص��ӽڵ�(schema)���Ƶ�ǰ׺(��ѯAtt *ns)��֪ */
	char *service;	/* asmx��ָURL */
	Array<Element> 	elems;	/* ÿ��element */
	Array<Type>	stypes;/* ÿ��simpleType */
	Array<ComplexType> types;	/* ÿ��Type */

	Schema ( const char*);
	void parse(TiXmlElement *sma);

	inline Schema () {
		clear();
	};

	inline void clear () {
		memset(efd, 0, sizeof(efd));
		memset(afd, 0, sizeof(afd));
		qdef = 0;
		namespURI = 0;
	};
};

class Binding:public VName {
public:
	Array<Operation>	*ops;	/* ÿ��operation */
	inline void clear () {
		ops = 0;
	};

	inline Binding ( const char*nm):VName(nm) { clear(); };
	inline  Binding () {
		clear();
	};
};

class APort:public VName {
public:
	const char *location;	/* service/port/soap:address��location�������� */
	Array<Operation>	*ops;	/* ÿ��operation */
	inline void clear () {
		ops = 0;
		location = 0;
	};

	inline APort ( const char*nm):VName(nm) { clear(); };
	inline  APort () {
		clear();
	};
};

class Wsdl {
public:
	char *asmx;
	int asm_len;

	Array<NString> ns;
	Array<NString> nsalias;
	Array<NString> qlt;
	Array<NString> env_attr;
	Array<UData> _st;
	Array<Schema> schemas;
	Array<Message> msgs;
	Array<PortType> ports;
	Array<HDRInfo> headers;		/* service/port/�µ�ÿһ��soap:header���� */
	Array<Binding> abinding;		/* service/port/�µ�ÿһ��soap:header���� */
	Array<APort> soapPorts;		/* service/port */

	const char *targetns;
	const char *namespaceURI;
	char *defportname;

	Wsdl();
	~Wsdl();
	bool parse(TiXmlElement *);
	static const char* getBaseName(const char *);
	static char* getQualifier(const char *);
	//char* getnsValue( char *ns );
	char* getUniqueNsq(TiXmlElement *ps, char *ns);

};
#endif
