/**
 ID: Hesa-Queue.h
 Title: 数据队列类定义
 Build: B008
 Modify history:
	B001:created by liqr 2002/12/12
	B002:modified by octerboy 2002/12/20,改为循环数组实现.
	B003:modified by octerboy 2002/12/24, 使之符合编码指导	
	B004:modified by octerboy 2003/01/02,增加AppcData的weight统计
	B005:modified by octerboy 2003/01/08,分组计算“重量”
	B006:modified by octerboy 2002/03/25,采用宏定义MAXHSMGRPID
	B007:modified by octerboy 2002/03/25,不用循环数组, 成为一个虚类
	B008:modified by octerboy 2002/03/26,clear()函数设为虚函数
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
