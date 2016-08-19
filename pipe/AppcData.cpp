/**
 ID: Hesa-AppcData.cpp
 Title: 请求与响应数据结构类实现
 Build: B008
 Modify history:
	B001:created by liqr 2002/12/12
	B002:modified by octerboy 2002/12/13,加上assert。
	B003:modified by octerboy 2002/12/17,增加extendLen,hsmGrpID.
	B004:modified by octerboy 2002/12/23,取消next
	B005:modified by octerboy 2002/12/31,增加重量
	B006:modified by octerboy 2002/01/02,时间标志初始化
	B007:modified by octerboy 2002/03/24,增加next, 采用宏定义MAXHSMGRPID
	B008:modified by octerboy 2002/03/25,增加nxtApp,nxtSvr，供AppcCon与SvrCo
n分别使用, 互不干扰

*/
#include "AppcData.h"
#include <assert.h>
void AppcData::init()
{
	dataReq = new char[4096];
	assert(dataReq > 0 );
 	lenReq = 0;
 	maxReqLen = 4096;
 	dataAnsing = new char[1024];
	assert(dataAnsing > 0 );
 	lenAnsing = 0;
 	maxAnsLen = 1024;
 	pacID = -1;
 	connectID = -1;
 	indexOfCons = -1;
  	extendData = (char*) 0;
	extendLen = 0;
	hsmGrpID = 0;
	next = (AppcData*) 0;
	nxtApp = (AppcData*) 0;
	nxtSvr = (AppcData*) 0;
	weight = 1;
	hasAppcTime = false;
	hasSvrTime = false;
}
