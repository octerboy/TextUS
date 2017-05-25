/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: Network information
 Build: 2005/7/25
 $Header: /textus/sysadm/netrs.cpp 5     07-01-15 10:19 Octerboy $
*/

#define SCM_MODULE_ID  "$Workfile: netrs.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
#include "version_2.c"
/* $NoKeywords: $ */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/bitypes.h>
#include <arpa/nameser.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <resolv.h>
#include <string.h>
#include <unistd.h>

#define INIT_IF_NUM     5
#define SOCK_OPEN_ERR   -100
#define MALLOC_ERROR    -101
#define IOCTL_GETCONF_ERR       -102
#define STRUCT_NICINFO_LEN_ERR  -103
#define OPEN_PROCNET_ROUTE_ERR  -200
#define ERROR_ROUTEINFO_NUM     -201

#define _PATH_PROCNET_ROUTE  "/proc/net/route"

#include "netrs.h"

static unsigned long atl(char *buf);
static int check_digist(char *buf);
static char *mac_to_dotted(unsigned char *s);

struct Netrs::NicInfo* Netrs::getNicInfo()
{
	struct ifconf ifr;
	struct ifreq *freq;
	int skhandle;
	int ifnum,i, ret;
	struct sockaddr_in *sin;
	static struct NicSubInfo *subinfo = NULL; 


	if((skhandle = socket(AF_INET, SOCK_DGRAM,0)) < 0)
	{
		sprintf(errMsg,"netrs: %s", strerror(errno));
		err_lev = 3;
		return NULL;
	}

	memset(&ifr,0,sizeof(struct ifconf));

	ifnum = INIT_IF_NUM;
	ifr.ifc_len = sizeof(struct ifreq) * ifnum ;
	ifr.ifc_req = (struct ifreq *)malloc(sizeof(struct ifreq)*ifnum);
	if(ifr.ifc_req == NULL)
	{
		sprintf(errMsg,"netrs: out of memory");
		err_lev = 1;
		return NULL;
	}

	for(;;)
	{
		ifr.ifc_len = sizeof(struct ifreq) * ifnum;
  		ifr.ifc_buf = (char *)realloc(ifr.ifc_buf, ifr.ifc_len);
  		
		if(ioctl(skhandle, SIOCGIFCONF,&ifr) < 0)
		{
			sprintf(errMsg,"netrs: %s", strerror(errno));
			err_lev = 3;
			free(ifr.ifc_buf);
			return NULL;
		}
  		if (ifr.ifc_len == (int ) sizeof(struct ifreq) * ifnum) {
  			ifnum += INIT_IF_NUM;
  			continue;
  		}
  		break;
	}
	
	ifnum = (ifr.ifc_len / sizeof(struct ifreq));
	subinfo = (struct NicSubInfo * ) realloc ((void*) subinfo,sizeof(struct NicSubInfo)*ifnum);
	nic.subinfo =subinfo;
	nic.nicNum = ifnum;
	
	freq = ifr.ifc_req;
	for( i = 0; i < ifnum; i++)
	{
		strncpy(subinfo[i].iface,freq[i].ifr_name,IF_NAMESIZE);
		ret = ioctl(skhandle, SIOCGIFADDR, &freq[i]);

		sin = (struct sockaddr_in *) &freq[i].ifr_addr;
		strcpy(subinfo[i].ip,inet_ntoa(sin->sin_addr));

		ret = ioctl(skhandle, SIOCGIFNETMASK, &freq[i]);
		sin = (struct sockaddr_in *) &freq[i].ifr_netmask;
		strcpy(subinfo[i].netmask,inet_ntoa(sin->sin_addr));

		ret = ioctl(skhandle, SIOCGIFHWADDR, &freq[i]);
		strcpy(subinfo[i].mac,mac_to_dotted((unsigned char *)freq[i].ifr_hwaddr.sa_data));

		ret = ioctl(skhandle, SIOCGIFFLAGS, &freq[i]);
		subinfo[i].flag = freq[i].ifr_flags;
	}	

	free(ifr.ifc_buf);
	close(skhandle);

	return &nic;
}


static char *mac_to_dotted(unsigned char *s)
{
	static char buf[30];

	memset(buf,0,20);
	sprintf (buf, "%02X:%02X:%02X:%02X:%02X:%02X", 
		s[0], s[1], s[2], s[3], s[4], s[5]);
	return buf;
}



struct Netrs::RouteInfo* Netrs::getRouteInfo()
{
	FILE *fp;
	char buff[1024],*ptr;
	int count;
	//struct in_addr daddr;
	unsigned long addrL;
	static struct RouteSubInfo *subinfo = NULL; 
	

	if(( fp = fopen(_PATH_PROCNET_ROUTE,"r")) == NULL)
	{
		sprintf(errMsg, "netrs: %s", strerror(errno));
		err_lev = 1;
		return NULL;
	}

	count = 0;


    	while (fgets(buff, 1023, fp)) 
	{
		// First is name
		subinfo= (struct RouteSubInfo * ) realloc ((void*) subinfo,sizeof(struct RouteSubInfo)*(count+1));
		if((ptr = strtok(buff," 	")) != NULL)
			strncpy(subinfo[count].iface,ptr, IF_NAMESIZE);
		else
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}

		// second is Destination
		if((ptr = strtok(NULL," 	")) != NULL)
		{
			if(check_digist(ptr))
				continue;	

			addrL = atl(ptr);
			ptr = (char *)&addrL;
			sprintf(subinfo[count].netip, "%d.%d.%d.%d", 
				(unsigned char)ptr[3], (unsigned char)ptr[2], 
				(unsigned char)ptr[1], (unsigned char)ptr[0]);
		}
		else
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}

		// Gateway
		if((ptr = strtok(NULL," 	")) != NULL)
		{
			if(check_digist(ptr))
				continue;	
			addrL = atl(ptr);
			ptr = (char *)&addrL;
			sprintf(subinfo[count].gateway, "%d.%d.%d.%d", 
				(unsigned char)ptr[3], (unsigned char)ptr[2], 
				(unsigned char)ptr[1], (unsigned char)ptr[0]);
		}
		else
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}
		//  Flags   
		if((ptr = strtok(NULL," 	")) == NULL)
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}
		//RefCnt  
		if((ptr = strtok(NULL," 	")) == NULL)
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}
		//Use
		if((ptr = strtok(NULL," 	")) == NULL)
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}
		// Metric	
		if((ptr = strtok(NULL," 	")) == NULL)
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}

		// NetMask
		if((ptr = strtok(NULL," 	")) == NULL)
		{
			sprintf(errMsg, "netrs(%d), ptr is NULL",__LINE__);
			err_lev = 4;
			continue;
		}
		if(check_digist(ptr))
			continue;	
		addrL = atl(ptr);
		ptr = (char *)&addrL;
		sprintf(subinfo[count].netmask, "%d.%d.%d.%d", 
			(unsigned char)ptr[3], (unsigned char)ptr[2], 
			(unsigned char)ptr[1], (unsigned char)ptr[0]);
		
		count++;
	}
	ri.routeNum = count;
	ri.subinfo=subinfo;
	fclose(fp);
	return &ri;
}


static unsigned long atl(char *buf)
{
	unsigned long bl;
	char *ptr;
	int i;

	ptr = (char *)&bl;
	for( bl = 0L,i = 0; i < 4; i++)
	{
		*(ptr+i) = ((buf[i*2]>'9')? (buf[i*2]+10-'A'):(buf[i*2]-'0'))<<4;
		*(ptr+i) |= (buf[i*2+1]>'9')? (buf[i*2+1]+10-'A'):(buf[i*2+1]-'0');
	}
	return bl;
}


static int check_digist(char *buf)
{

	int i,len;
	int ret;

	len = strlen(buf);
	
	ret = 0;
	for( i = 0; i < len; i++)
	{
		if( (buf[i]>= '0'&& buf[i] <='9') || 
		    (buf[i] >= 'A' && buf[i] <= 'F')
		  )
			continue;
		else
		{
			ret = -1;
			break;
		}
	}
	return ret;
}
