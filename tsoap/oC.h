static int _nextId = 0;
/* oC�Ľṹ */
bool fDone;
int id;
oC *next;

struct CO {
	char *funcName;		
	bool async;	/* ͬ�����첽, ͨ��true: �첽 */
	char *portName;	/* ��WSDL�ĵ�service�ĵ�һ��port��name */
	char *args;	/* */
} *co;

char *cb;/* ָ���һ��������������������һ��function֮��� */
char *service;	/* asmx��URL */
char *args[];	/* ��ҳ���ò����� */

struct OXmlHttp {
	bool fFree;	/* */
	void *xmlHttp;
} oXmlHttp;


