/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 ID: Textus-Describo.h
 Title: 与Sched类交互的数据描述, Pius的ordo定为1, indic为Howfd
 Build: 
	B01:created by octerboy, 2005/06/10
*/
#ifndef DESCRIBO__H
#define DESCRIBO__H
#include "Amor.h"
class Describo
{
public:
	/*
	Criptor在Sched类与通讯处理类(Tcpsvr等)之间传递, 
	其中scanfd是需要Tcpsvr等类赋值的,
	而index则由Sched类处理, Tcpsvr等类对其无需作任何访问.

	Criptor由Tcpsvr等类生成, 在scanfd赋值之后, 
	则不要再对此Criptor作任何改变, 直到scanfd不再使用, 并且向Sched传递了
	CLRRD或CLRWR动作之后, 此Criptor才可作变化。
	所以,Tcpsvr等类在实例的生存期间, Criptor所指向的地址要保持不变.

	注意:Tcpsvr类中, 用于read或write的Criptor, 
	*/
	struct Criptor{
		Amor *pupa;	//被调用facio的对象
		int scanfd;	//描述符
		int rd_index;	//指示数组中的位置, 这对于Sched类有意义
		int wr_index;	//指示数组中的位置, 这对于Sched类有意义
		int ex_index;	//指示数组中的位置, 这对于Sched类有意义
		inline Criptor() {
			rd_index = -1;
			wr_index = -1;
			ex_index = -1;
			pupa = 0;
		};
	};
	struct Pendor{
		Amor *pupa;	//被调用facio的对象
		Amor::Pius *pius;
		int dir;
		int from;
		inline Pendor() {
			pius = 0;
			pupa = 0;
			dir = -1;
			from = 0;
		};
	};
};
#endif
