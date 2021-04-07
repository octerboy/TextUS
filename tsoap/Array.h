/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
Title: Array Template
Build: created by octerboy, 2006/12/01
 $Id$
 $Date$
 $Revision$
*/

/* $NoKeywords: $ */

#ifndef ARRAY_H
#define ARRAY_H
template <class Type>
class ArrayEle {
public:
	char *name;
	Type val;
	ArrayEle *next;
	inline ArrayEle():val() {
		name = 0;
		next = 0;
	};

	inline ~ArrayEle() {
		if ( name ) delete[] name;
		name = 0;
	};
	ArrayEle( const char *nm);
};

template <class T>
ArrayEle<T>:: ArrayEle(const char *nm): val(nm)
{
	if ( nm )
	{
		int len;
		len = static_cast<int>(strlen(nm));
		name = new char [len + 1];
		memcpy ( name, nm, len);
		name[len] = 0;
	} else
		name = 0;
	next = 0;
}

template <class Type>
class Array: public ArrayEle<Type>
{
public:
	
	ArrayEle<Type> * remove();
	bool append(ArrayEle<Type> * data);
	Type& operator [] (const char *nm) ;
	Type& operator [] (int i) ;
	Type* get();

	ArrayEle<Type> *cursor;
	int length;
	inline Array () {
		cursor=0;
		length  = 0;
	};
	void rewind() {
		cursor = 0;
	};
	inline ~Array ();
};

template <class T>
Array<T>::~Array() 
{
	ArrayEle<T> *rm = this->remove();
	while( rm )
	{
		delete rm;
		rm = this->remove();
	}
}

template <class T>
T* Array<T>::get() 
{
	if( cursor )
		cursor = cursor->next;
	else {	/* First get */
		cursor = this->next;
		if ( cursor )
			return &(cursor->val);	
		else
			return 0;
	}

	if( !cursor || cursor == this->next)
	{
		cursor = 0;
		return 0;
	} else
		return &(cursor->val);	
}


template <class T>
T& Array<T>::operator [] (const char *nm) 
{
	ArrayEle<T> *neo, *list = this->next;
	while ( list )
	{
		if ( list->name == nm 
			|| (list->name && nm && strcmp(list->name , nm ) == 0 ))
			return list->val;
		list = list->next;
		if (list == this->next )
			break;
	}
	neo = new ArrayEle<T>(nm);
	append(neo);
	return neo->val;	
}

template <class T>
T& Array<T>::operator [] (int i) 
{
	ArrayEle<T> *neo, *list = this->next;
	if ( i >= length )
	{
		neo = new ArrayEle<T>();
		append(neo);
		return neo->val;	
	} else {
		list = list->next;
		for ( int j = 0; j < i; j++)
			list = list->next;
	}
	return list->val;
}

template <class T>
bool Array<T>::append(ArrayEle<T> * data)
{
	/* 入队, DR#:CppFifo-002 */
	if (this->next!= 0)
	{
		data->next = this->next->next;
		this->next->next = data;
	} else
	{
		data->next = data;
	}
	this->next = data;	
	/* End of 入队 */
	length++;

	return true;
}

template <class T>
ArrayEle<T> * Array<T>::remove()
{
	/* 出队, DR#:CppFifo-002 */
	ArrayEle<T> *list = this->next;
	if (list != 0)
	{
		list =list->next;
		if (list != this->next)
		{
			this->next->next = list->next;
		} else
		{
			this->next = 0;
		}
		list->next = 0;
	}
	/* End of 出队 */
	length--;
	return list;
}
#endif

