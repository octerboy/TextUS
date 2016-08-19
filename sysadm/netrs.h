/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 $Header: /textus/sysadm/netrs.h 3     07-01-13 22:59 Octerboy $
*/

/* $NoKeywords: $ */

#ifndef __NETRS__H
#define __NETRS__H
class Netrs {
public:
	struct NicSubInfo {
        	char iface[20];
        	char ip[20];
        	char netmask[20];
        	char mac[20];
		short int flag;
		int bandwidth;
        };

	struct NicInfo {
        	int nicNum;     /*������ӿ�����Ҳ��interface����Ŀ������lo */
        	struct NicSubInfo *subinfo; /* �ӿ���Ϣ�ӽṹ */
        };

	struct RouteSubInfo {
        	char iface[20];
        	char netip[20];
        	char netmask[20];
        	char gateway[20];
        };

	struct RouteInfo {
        	int routeNum; 	/*������ӿ�����Ҳ��interface����Ŀ������lo */
        	struct RouteSubInfo *subinfo; 	/* �ӿ���Ϣ�ӽṹָ�� */
        };

	struct RouteInfo* getRouteInfo();
	struct NicInfo* getNicInfo();

	char errMsg[1024];
	int err_lev;
private:
	struct NicInfo nic;
	struct RouteInfo ri;
};

#endif
