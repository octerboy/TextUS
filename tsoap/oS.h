/* oS属性 */

char *targetns;	/* WSDL文档的"targetNamespace" */
struct Att {
	char *alias;
	char *value;
};
Att *ns;/* alias="xsd":一般为"http://www.w3.org/2001/XMLSchema" */
	/* alias=其它, 从WSDL文档的第一个节点(definitions)以xmlns为前缀的属性而得, 不包括"xmlns"属性 */
	/* alias="xsi", 默认为"http://www.w3.org/2001/XMLSchema-instance"*/

	
Att *qlt;
	/*
	oS.qlt["soapenc"] = "http://schemas.xmlsoap.org/soap/encoding/";
       	oS.qlt["wsdl"] = "http://schemas.xmlsoap.org/wsdl/";
       	oS.qlt["soap"] = "http://schemas.xmlsoap.org/wsdl/soap/";
       	oS.qlt["SOAP-ENV"] = 'http://schemas.xmlsoap.org/soap/envelope/'
       	这是默认值, 如果WSDL文档的definitions有内容，即对如"http://schemas.xmlsoap.org/wsdl/soap/"作了其它别名, 
       	则qlt["soap"]就是这个别名了, 否则qlt[]的值就是这个默认别名。 qlt也要加入到Att *ns中 */
	*/
struct Element {
	char *name;	/* */
	bool fArray;	/* 是否为阵列? 即: maxOccurs值 > 1 */
	char *type;	/* 数据类型？ type="s:float"中的float, 如果s是指xsd的命名空间, 则对于"s:anyType"则为string
				对于wsdl文档的element节点没有"type"及"xsi:type"属性的, 则内容为name + "_" +name
			 */
	char *ns;	/* type="s:float"中的s, 如果为空, 则取schema的qdef */
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
	char *name;	/* 对于simpleType, 在wsdl文档须有定义, 一般是所在element节点name属性的内容(name_name),这是个索引，每个应不同 */
	char *ns;	/* 对于simpleType的子节点为"restriction'：
				如果子节点没有base属性, 则ns="xsd", data_kind="string";
				如果有base属性,则取base内容的前缀,data_kind
			   
			   对于simpleType的子节点为list或union：ns="xsd", data_kind="string"
			*/
	char *data_kind;
	
};

struct ComplexType {
	char *name;	/* 在wsdl文档须有定义, 一般是所在element节点的name属性的内容(name_name),这是个索引，每个应不同 */
	int lex;	/* Element阵列, Attribute阵列 */
	void *arr;
	
};

struct Schema {
	char *uri;	/* WSDL文档的types元素的每个子节点(schema)的targetNamespace属性内容, 这是每个都不同的 */
	char *efd;	/* "elementFormDefault"属性 */
	char *afd;	/* "attributeFormDefault"属性 */
	char *qdef;	/* targetNamespace所指内容的别名, 从Att *ns中寻找 */
	char *namespaceURI;	/* 从types元素的子节点(schema)名称的前缀(查询Att *ns)可知 */
	char *service;	/* asmx所指URL */
	Element *elems;	/* 每个element */
	Type	*stypes;/* 每个simpleType */
	ComplexType	*types;	/* 每个Type */
	
};

Schema schemas[];	/* 再说 */

struct Part {
	char *name;	/* 节点中的name属性内容, 这是索引, 每个都不同 */
	char *type;	/* 取基本名, 如果节点中内容为"anyType",且前缀为"xsd"则将其改为"string" */
	char *elem;	/* element属性的基本名 */
	char *ns;	/* 前缀， 在part节点中, type属性内容的前缀为优先,否则取element属性内容的前缀 */
	
};

struct HDRInfo {
	char *ns;
	char *es;
	bool fLiteral;
	bool fRequired;
	Part *type;
};

struct Msg {
	char *sName;	/* message节点的name属性内容, 这是索引 */
	Part *args;	/* message节点下的part阵列 */
	char *opname;	/* portType/operation节点的name属性,  portType/operation/input只能指向一个message. 
				其对应关系是input子节点的message属性内容的基本名是这里的sName.  */
				
	bool fWrapped;	/* 如果只有一个Part(args),且非空，并且(Part->name为"parameter" 或 typet和elem有一个与opname相同)*/
	
	bool fOneWay;	/* 如果portType/operation节点没有output子节点, */
	Msg  *response;	/* 指向portType/operation/output节点中message属性内容所指明的Msg */
	
	char *ns;	/* 在相应的binding/operation/input节点中namespace属性内容 */
	char *es;	/* 在相应的binding/operation/input节点中encodingStyle属性内容 */
			/* binding/operation/input只能指向一个portType/operation/input */
	bool *fLiteral;	/* 在相应的binding/operation/input节点中use属性内容为"literal" */
	HDRInfo *hdrsIn; /* 在相应的binding/operation/input/soap:header内容 */
	HDRInfo *hdrsOut;/* 在相应的binding/operation/output/soap:header内容 */
	bool fRpc;	/* 在相应的binding/operation/soap:operation的style属性内容为"rpc"*/
	
	
	
	
};

Msgs msgs[];		/* 对应于WSDL文档中所有的message节点 */

struct APort {
	char *szname;	/* service/port的name属性内容, 这个索引值*/
	char *location;	/* service/port/soap:address的location属性内容 */
	
	struct {	
		char *sOpName;	/* 某个portType下的各operation的name属性*/
		char *sin;	/* 某个portType下的各operation/input的name属性, 一般因缺省就是sOpName */
		Msg *msg;	/* service/port的binding属性内容基本名指明采用哪一个binding元素. 
				此binding元素(type属性指明)相应一个portType, 该portType的operation/input指明了
				相应的message。*/
	} oops;
};

char *defPortName;		/* service下第一个port的name属性内容 */
APort *soapPort;		/* service下各个port*/
HDRInfo *headers;		/* service下各个port的soap:header */



