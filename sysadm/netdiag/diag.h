/**
 ����:ϵͳ���
 ��ʶ:XmlHttp-diag.h
 �汾:B002
	B001:created on 2003/11/20
	B002:modified on 2003/11/21,���������const����
*/
#ifndef __DIAG__H
#define __DIAG__H
class Diag {
public:
	char *dns(const char *dns_name);
	int ping(const char *ip_dot_addr);	
};

#endif
