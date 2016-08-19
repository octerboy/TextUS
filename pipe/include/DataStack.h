/**
 ID: Hesa-DataStack.h
 Title: ���ݶ�ջ����
 Build: B002
 Modify history:
	B001:created by liqr 2002/12/12
	B002:modified by octerboy 2003/03/12,����Singletonģʽ
*/
#ifndef DataStack__H
#define DataStack__H
#include "AppcData.h"
class DataStack
{
private:
	int top;
	int max;
	AppcData **appcDataArray;
public:
	DataStack();
	AppcData * pop();
	void push(AppcData * element);
};
#endif
