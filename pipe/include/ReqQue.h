/**
 ID: Hesa-ReqQue.h
 Title: ���ݶ����ඨ��
 Build: B004
 Modify history:
	B001:created by octerboy 2002/03/12
	B002:modified by octerboy 2002/03/24,��������ʵ��
	B003:modified by octerboy 2002/03/25,�̳�Queue
	B004:modified by octerboy 2002/03/26,�ٶ���clear()����
*/
#ifndef ReqQueue__H
#define ReqQueue__H
#include "Queue.h"
class ReqQue:public Queue 
{
private:
	static ReqQue* _instance;

protected:
	ReqQue();
public:
	static ReqQue* instance();
	AppcData * remove();
	bool append(AppcData * data);
	void clear();
};
#endif
