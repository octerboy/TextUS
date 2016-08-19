#include <string.h>

class Queue:public AppcData
{
public:
	AppcData * remove();
	bool append(AppcData * data);
	void clear();
};

bool Queue::append(AppcData * data)
{
	/* 入队, DR#:CppFifo-002 */
	if (next!= 0)
	{
		data->next = next->next;
		next->next = data;
	} else
	{
		data->next = data;
	}
	next = data;	
	/* End of 入队 */

	return true;
}

AppcData * Queue::remove()
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

void Queue::clear()
{
	Queue::clear();
	next = (AppcData*) 0;
	
}
