/**
 标题:系统诊断
 标识:XmlHttp-diag.h
 版本:B002
	B001:created on 2003/11/20
	B002:modified on 2003/11/21,输入参数加const声明
*/
#ifndef __DIAG__H
#define __DIAG__H
class Diag {
public:
	char *dns(const char *dns_name);
	int ping(const char *ip_dot_addr);	
};

#endif
