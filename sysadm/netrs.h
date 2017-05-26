/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.

 $Id$
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
        	int nicNum;     /*　网络接口数，也是interface的数目，包括lo */
        	struct NicSubInfo *subinfo; /* 接口信息子结构 */
        };

	struct RouteSubInfo {
        	char iface[20];
        	char netip[20];
        	char netmask[20];
        	char gateway[20];
        };

	struct RouteInfo {
        	int routeNum; 	/*　网络接口数，也是interface的数目，包括lo */
        	struct RouteSubInfo *subinfo; 	/* 接口信息子结构指针 */
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
