/**
 ID: Hesa-AppcData.cpp
 Title: ��������Ӧ���ݽṹ��ʵ��
 Build: B008
 Modify history:
	B001:created by liqr 2002/12/12
	B002:modified by octerboy 2002/12/13,����assert��
	B003:modified by octerboy 2002/12/17,����extendLen,hsmGrpID.
	B004:modified by octerboy 2002/12/23,ȡ��next
	B005:modified by octerboy 2002/12/31,��������
	B006:modified by octerboy 2002/01/02,ʱ���־��ʼ��
	B007:modified by octerboy 2002/03/24,����next, ���ú궨��MAXHSMGRPID
	B008:modified by octerboy 2002/03/25,����nxtApp,nxtSvr����AppcCon��SvrCo
n�ֱ�ʹ��, ��������

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
