/*
 ID: Hesa-ReqQue.cpp
 Title: 请求队列类实现
 Build: B004
 Modify history:
	B001:created by octerboy 2002/03/12
	B002:modified by octerboy 2002/03/24,改用链表实现
	B003:modified by octerboy 2002/03/25,继承Queue
	B004:modified by octerboy 2002/03/26,再定义clear()函数
*/
#include <string.h>
#include "ReqQue.h"
ReqQue* ReqQue::_instance = 0;

ReqQue::ReqQue()
{
}

ReqQue* ReqQue::instance ()
{
	if (_instance == 0)
	{
		_instance = new ReqQue;
	}
	return _instance;
}

bool ReqQue::append(AppcData * data)
{
	if ((current == max)&&(max > 0))
	{
		return false;
	}

	/* 入队, DR#:CppFifo-002 */
	if (next!= 0)
	{
		data->next = next->next;
		next->next = data;
	}
	else
	{
		data->next = data;
	}
	next = data;	
	/* End of 入队 */

	current++;	/* 队列元素个数增加 */

	weight += data->weight;	/* 总重量增加 */
	/* 分组计算重量 */
	if (data->hsmGrpID > -1 && data->hsmGrpID < MAXHSMGRPID )
	{
		grpWeight[data->hsmGrpID] += data->weight;
	}

	return true;
}

AppcData * ReqQue::remove()
{
	/* 出队, DR#:CppFifo-002 */
	AppcData *list = next;
	if (list != 0)
	{
		list =list->next;
		if (list != next)
		{
			next->next = list->next;
		}
		else
		{
			next = 0;
		}
		list->next = 0;
		/* End of 出队 */

	}
	return list;
}

void ReqQue::clear()
{
	Queue::clear();
	next = (AppcData*) 0;
	
}
