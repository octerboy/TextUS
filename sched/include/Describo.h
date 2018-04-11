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
 Title: ��Sched�ཻ������������, Pius��ordo��Ϊ1, indicΪHowfd
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
	Criptor��Sched����ͨѶ������(Tcpsvr��)֮�䴫��, 
	����scanfd����ҪTcpsvr���ำֵ��,
	��index����Sched�ദ��, Tcpsvr��������������κη���.

	Criptor��Tcpsvr��������, ��scanfd��ֵ֮��, 
	��Ҫ�ٶԴ�Criptor���κθı�, ֱ��scanfd����ʹ��, ������Sched������
	CLRRD��CLRWR����֮��, ��Criptor�ſ����仯��
	����,Tcpsvr������ʵ���������ڼ�, Criptor��ָ��ĵ�ַҪ���ֲ���.

	ע��:Tcpsvr����, ����read��write��Criptor, 
	*/
	struct Criptor{
		Amor *pupa;	//������facio�Ķ���
		int scanfd;	//������
		int rd_index;	//ָʾ�����е�λ��, �����Sched��������
		int wr_index;	//ָʾ�����е�λ��, �����Sched��������
		int ex_index;	//ָʾ�����е�λ��, �����Sched��������
		inline Criptor() {
			rd_index = -1;
			wr_index = -1;
			ex_index = -1;
			pupa = 0;
		};
	};
	struct Pendor{
		Amor *pupa;	//������facio�Ķ���
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
