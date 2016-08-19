static int _nextId = 0;
/* oC的结构 */
bool fDone;
int id;
oC *next;

struct CO {
	char *funcName;		
	bool async;	/* 同步或异步, 通常true: 异步 */
	char *portName;	/* 找WSDL文档service的第一个port的name */
	char *args;	/* */
} *co;

char *cb;/* 指向第一个参数，如果这个参数是一个function之类的 */
char *service;	/* asmx的URL */
char *args[];	/* 网页调用参数表 */

struct OXmlHttp {
	bool fFree;	/* */
	void *xmlHttp;
} oXmlHttp;


