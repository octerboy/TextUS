
struct AppcData
{
	Amor *pupa;
	struct AppcData *next;
	inline AppcData() {
		pupa = 0;
		next = 0;
	};
};

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

#include <assert.h>
DataStack::DataStack()
{
	top = 0;
	max = 128;
	appcDataArray = new (AppcData*)[max];
	for(int i = 0; i < max; i++)
	{
		appcDataArray[i] = new AppcData;
 	}
}

AppcData * DataStack::pop()// ³öÕ»
{
	int mid = top;
	if (mid >= max)
	{
		int old = max;
		delete[] appcDataArray;
		max += 64;
		appcDataArray = new (AppcData*)[max];
		for(int i = old; i < max; i++)
		{
			appcDataArray[i] = new AppcData;
			assert(appcDataArray[i] > 0);
 		}
	}
	top++;
	return appcDataArray[mid];
}

void DataStack::push(AppcData * element)// ÈëÕ»
{
	if (top > 0)
	{
		top--;
		appcDataArray[top] = element;
	}
}
