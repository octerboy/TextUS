/* oS���� */

char *targetns;	/* WSDL�ĵ���"targetNamespace" */
struct Att {
	char *alias;
	char *value;
};
Att *ns;/* alias="xsd":һ��Ϊ"http://www.w3.org/2001/XMLSchema" */
	/* alias=����, ��WSDL�ĵ��ĵ�һ���ڵ�(definitions)��xmlnsΪǰ׺�����Զ���, ������"xmlns"���� */
	/* alias="xsi", Ĭ��Ϊ"http://www.w3.org/2001/XMLSchema-instance"*/

	
Att *qlt;
	/*
	oS.qlt["soapenc"] = "http://schemas.xmlsoap.org/soap/encoding/";
       	oS.qlt["wsdl"] = "http://schemas.xmlsoap.org/wsdl/";
       	oS.qlt["soap"] = "http://schemas.xmlsoap.org/wsdl/soap/";
       	oS.qlt["SOAP-ENV"] = 'http://schemas.xmlsoap.org/soap/envelope/'
       	����Ĭ��ֵ, ���WSDL�ĵ���definitions�����ݣ�������"http://schemas.xmlsoap.org/wsdl/soap/"������������, 
       	��qlt["soap"]�������������, ����qlt[]��ֵ�������Ĭ�ϱ����� qltҲҪ���뵽Att *ns�� */
	*/
struct Element {
	char *name;	/* */
	bool fArray;	/* �Ƿ�Ϊ����? ��: maxOccursֵ > 1 */
	char *type;	/* �������ͣ� type="s:float"�е�float, ���s��ָxsd�������ռ�, �����"s:anyType"��Ϊstring
				����wsdl�ĵ���element�ڵ�û��"type"��"xsi:type"���Ե�, ������Ϊname + "_" +name
			 */
	char *ns;	/* type="s:float"�е�s, ���Ϊ��, ��ȡschema��qdef */
	sizeArray;
	
	
};

struct Attribute {
	char *name;
	char *fixed;
	bool allowed;
	bool fAttribute;
	char *type;
	char *ns;
};


struct Type {
	char *name;	/* ����simpleType, ��wsdl�ĵ����ж���, һ��������element�ڵ�name���Ե�����(name_name),���Ǹ�������ÿ��Ӧ��ͬ */
	char *ns;	/* ����simpleType���ӽڵ�Ϊ"restriction'��
				����ӽڵ�û��base����, ��ns="xsd", data_kind="string";
				�����base����,��ȡbase���ݵ�ǰ׺,data_kind
			   
			   ����simpleType���ӽڵ�Ϊlist��union��ns="xsd", data_kind="string"
			*/
	char *data_kind;
	
};

struct ComplexType {
	char *name;	/* ��wsdl�ĵ����ж���, һ��������element�ڵ��name���Ե�����(name_name),���Ǹ�������ÿ��Ӧ��ͬ */
	int lex;	/* Element����, Attribute���� */
	void *arr;
	
};

struct Schema {
	char *uri;	/* WSDL�ĵ���typesԪ�ص�ÿ���ӽڵ�(schema)��targetNamespace��������, ����ÿ������ͬ�� */
	char *efd;	/* "elementFormDefault"���� */
	char *afd;	/* "attributeFormDefault"���� */
	char *qdef;	/* targetNamespace��ָ���ݵı���, ��Att *ns��Ѱ�� */
	char *namespaceURI;	/* ��typesԪ�ص��ӽڵ�(schema)���Ƶ�ǰ׺(��ѯAtt *ns)��֪ */
	char *service;	/* asmx��ָURL */
	Element *elems;	/* ÿ��element */
	Type	*stypes;/* ÿ��simpleType */
	ComplexType	*types;	/* ÿ��Type */
	
};

Schema schemas[];	/* ��˵ */

struct Part {
	char *name;	/* �ڵ��е�name��������, ��������, ÿ������ͬ */
	char *type;	/* ȡ������, ����ڵ�������Ϊ"anyType",��ǰ׺Ϊ"xsd"�����Ϊ"string" */
	char *elem;	/* element���ԵĻ����� */
	char *ns;	/* ǰ׺�� ��part�ڵ���, type�������ݵ�ǰ׺Ϊ����,����ȡelement�������ݵ�ǰ׺ */
	
};

struct HDRInfo {
	char *ns;
	char *es;
	bool fLiteral;
	bool fRequired;
	Part *type;
};

struct Msg {
	char *sName;	/* message�ڵ��name��������, �������� */
	Part *args;	/* message�ڵ��µ�part���� */
	char *opname;	/* portType/operation�ڵ��name����,  portType/operation/inputֻ��ָ��һ��message. 
				���Ӧ��ϵ��input�ӽڵ��message�������ݵĻ������������sName.  */
				
	bool fWrapped;	/* ���ֻ��һ��Part(args),�ҷǿգ�����(Part->nameΪ"parameter" �� typet��elem��һ����opname��ͬ)*/
	
	bool fOneWay;	/* ���portType/operation�ڵ�û��output�ӽڵ�, */
	Msg  *response;	/* ָ��portType/operation/output�ڵ���message����������ָ����Msg */
	
	char *ns;	/* ����Ӧ��binding/operation/input�ڵ���namespace�������� */
	char *es;	/* ����Ӧ��binding/operation/input�ڵ���encodingStyle�������� */
			/* binding/operation/inputֻ��ָ��һ��portType/operation/input */
	bool *fLiteral;	/* ����Ӧ��binding/operation/input�ڵ���use��������Ϊ"literal" */
	HDRInfo *hdrsIn; /* ����Ӧ��binding/operation/input/soap:header���� */
	HDRInfo *hdrsOut;/* ����Ӧ��binding/operation/output/soap:header���� */
	bool fRpc;	/* ����Ӧ��binding/operation/soap:operation��style��������Ϊ"rpc"*/
	
	
	
	
};

Msgs msgs[];		/* ��Ӧ��WSDL�ĵ������е�message�ڵ� */

struct APort {
	char *szname;	/* service/port��name��������, �������ֵ*/
	char *location;	/* service/port/soap:address��location�������� */
	
	struct {	
		char *sOpName;	/* ĳ��portType�µĸ�operation��name����*/
		char *sin;	/* ĳ��portType�µĸ�operation/input��name����, һ����ȱʡ����sOpName */
		Msg *msg;	/* service/port��binding�������ݻ�����ָ��������һ��bindingԪ��. 
				��bindingԪ��(type����ָ��)��Ӧһ��portType, ��portType��operation/inputָ����
				��Ӧ��message��*/
	} oops;
};

char *defPortName;		/* service�µ�һ��port��name�������� */
APort *soapPort;		/* service�¸���port*/
HDRInfo *headers;		/* service�¸���port��soap:header */



