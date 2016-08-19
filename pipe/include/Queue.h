/**
 ID: Hesa-Queue.h
 Title: ���ݶ����ඨ��
 Build: B008
 Modify history:
	B001:created by liqr 2002/12/12
	B002:modified by octerboy 2002/12/20,��Ϊѭ������ʵ��.
	B003:modified by octerboy 2002/12/24, ʹ֮���ϱ���ָ��	
	B004:modified by octerboy 2003/01/02,����AppcData��weightͳ��
	B005:modified by octerboy 2003/01/08,������㡰������
	B006:modified by octerboy 2002/03/25,���ú궨��MAXHSMGRPID
	B007:modified by octerboy 2002/03/25,����ѭ������, ��Ϊһ������
	B008:modified by octerboy 2002/03/26,clear()������Ϊ�麯��
*/
#ifndef Queue__H
#define Queue__H
#include "AppcData.h"
class Queue: public AppcData
{
protected:
	int max;
	int current;
	int weight;
	int grpWeight[MAXHSMGRPID];
public:
	void setMax(int num);
	int getCurrent();	
	int getWeight();	
	int getWeight(int grpID);	
	virtual AppcData * remove();
	virtual bool append(AppcData * data);
	virtual void clear();
	Queue();
};
#endif
