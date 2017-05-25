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
	bool fArray;	/* 是否为阵列? 即: maxOccurs值 > 1 */
	char *type;	/* 数据类型？ type="s:float"中的float, 如果s是指xsd的命名空间, 则对于"s:anyType"则为string
				对于wsdl文档的element节点没有"type"及"xsi:type"属性的, 则内容为name + "_" +name
			 */
	char *ns;	/* type="s:float"中的s, 如果为空, 则取schema的qdef */
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
	//char *name;	/* 对于simpleType, 在wsdl文档须有定义, 一般是所在element节点name属性的内容(name_name),这是个索引，每个应不同 */
	char *ns;	/* 对于simpleType的子节点为"restriction'：
				如果子节点没有base属性, 则ns="xsd", data_kind="string";
				如果有base属性,则取base内容的前缀,data_kind
			   
			   对于simpleType的子节点为list或union：ns="xsd", data_kind="string"
			*/
	char *data_kind;
	inline Type (char *nm):VName(nm) { };
	inline Type () {
		name = ns = data_kind = 0;
	};
};

class ComplexType:public VName {
public:
	//char *name;	/* 在wsdl文档须有定义,一般是所在element节点的name属性的内容(name_name),这是个索引，每个应不同 */
	int lex;	/* Element阵列, Attribute阵列 */
	void *arr;
	
	inline ComplexType ( char*nm):VName(nm) {};
	inline ComplexType () {
		name = 0;
	};
};

class Part: public VName {
public:
	//char *name;	/* 节点中的name属性内容, 这是索引, 每个都不同 */
	char *type;	/* 取基本名, 如果节点中内容为"anyType",且前缀为"xsd"则将其改为"string" */
	char *elem;	/* element属性的基本名 */
	char *ns;	/* 前缀， 在part节点中, type属性内容的前缀为优先,否则取element属性内容的前缀 */
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
	//char *name;	/* message节点的name属性内容, 这是索引 */
	Array<Part> args;	/* message节点下的part阵列 */
	const char *opname;	/* portType/operation节点的name属性,  portType/operation/input只能指向一个message. */
	const char *soapAction;	
	char *soapAction2;	
				
	bool fWrapped;	/* 如果只有一个Part(args),且非空，并且(Part->name为"parameter" 或 typet和elem有一个与opname相同)*/
	
	bool fOneWay;	/* 如果portType/operation节点没有output子节点, */
	Message  *response;	/* 指向portType/operation/output节点中message属性内容所指明的Msg */
	
	const char *ns;	/* 在相应的binding/operation/input节点中namespace属性内容 */
	const char *es;	/* 在相应的binding/operation/input节点中encodingStyle属性内容 */
			/* binding/operation/input只能指向一个portType/operation/input */
	bool fLiteral;	/* 在相应的binding/operation/input节点中use属性内容为"literal" */
	Array<HDRInfo> hdrsIn; /* 在相应的binding/operation/input/soap:header内容 */
	Array<HDRInfo> hdrsOut;/* 在相应的binding/operation/output/soap:header内容 */
	bool fRpc;	/* 在相应的binding/operation/soap:operation的style属性内容为"rpc"*/

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
	const char *siname;	/* 某个portType下的各operation/input的name属性, 一般缺省, 就取sOpName */
	Message *msgp;		/* service/port的binding属性内容基本名指明采用哪一个binding元素.  此binding元素(type属性指明)相应一个portType, 该portType的operation/input指明了相应的message。*/

	inline Operation ( const char *nm) :VName(nm) { clear();};
	inline Operation () { clear(); };

	inline void clear() {
		siname = 0;
		msgp = 0;
	};
};

class PortType:public VName {
public:
	//char *name;	/* service/port的name属性内容, 这个索引值*/
	char *location;	/* service/port/soap:address的location属性内容 */
	Array<Operation>	ops;	/* 每个operation */
	
	inline void clear () { location = 0; };

	inline PortType ( const char*nm):VName(nm) { clear(); };
	inline  PortType () { clear(); };
};

class Schema {
public:
	char *uri;	/* WSDL文档的types元素的每个子节点(schema)的targetNStringace属性内容, 这是每个都不同的 */
	char efd[N_SIZE];	/* "elementFormDefault"属性 */
	char afd[N_SIZE];	/* "attributeFormDefault"属性 */
	char *qdef;	/* targetNStringace所指内容的别名, 从Att *ns中寻找 */
	char *namespURI;	/* 从types元素的子节点(schema)名称的前缀(查询Att *ns)可知 */
	char *service;	/* asmx所指URL */
	Array<Element> 	elems;	/* 每个element */
	Array<Type>	stypes;/* 每个simpleType */
	Array<ComplexType> types;	/* 每个Type */

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
	Array<Operation>	*ops;	/* 每个operation */
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
	const char *location;	/* service/port/soap:address的location属性内容 */
	Array<Operation>	*ops;	/* 每个operation */
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
	Array<HDRInfo> headers;		/* service/port/下的每一个soap:header内容 */
	Array<Binding> abinding;		/* service/port/下的每一个soap:header内容 */
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
