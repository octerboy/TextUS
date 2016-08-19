/*
 ID: Hesa-ReqQue.cpp
 Title: ���������ʵ��
 Build: B004
 Modify history:
	B001:created by octerboy 2002/03/12
	B002:modified by octerboy 2002/03/24,��������ʵ��
	B003:modified by octerboy 2002/03/25,�̳�Queue
	B004:modified by octerboy 2002/03/26,�ٶ���clear()����
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

	/* ���, DR#:CppFifo-002 */
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
	/* End of ��� */

	current++;	/* ����Ԫ�ظ������� */

	weight += data->weight;	/* ���������� */
	/* ����������� */
	if (data->hsmGrpID > -1 && data->hsmGrpID < MAXHSMGRPID )
	{
		grpWeight[data->hsmGrpID] += data->weight;
	}

	return true;
}

AppcData * ReqQue::remove()
{
	/* ����, DR#:CppFifo-002 */
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
		/* End of ���� */

	}
	return list;
}

void ReqQue::clear()
{
	Queue::clear();
	next = (AppcData*) 0;
	
}
