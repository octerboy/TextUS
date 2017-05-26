/* Copyright (c) 2010-2012 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: DWave
 Build:created by octerboy 2011/06/08
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "Amor.h"
#include "casecmp.h"
#include "Notitia.h"
#include "TBuffer.h"
#include <assert.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "NIDAQmx.h"
#include "dsrc_wv.h"
#define TOTAL_SAMPLE 6000	//以K为单位
#define EVERY_SAMPLE 500	//以K为单位
#define RATE_MULTI 5		//采样速率，以M为单位
#define WV_MAX 128		//波形帧缓冲数, 如果一个交易中, 有超过128帧数据（包括无效的）， 后面的就不记了
#define HIGH_LEV = 1010;
#define	LOW_LEV = 990;
#define	TRIG_LEV = 700;

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

class DWave :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	DWave();
	~DWave();
	int32 CVICALLBACK analyze(TaskHandle task,uInt32 nSamples);
	void log_dmq_err(char *s);

	bool outDone;

#include "PacData.h"
private:
	PacketObj rcv_pac;
	PacketObj snd_pac;	/* 传递至右节点的PacketObj */
	PacketObj *pa[3];

	Amor::Pius local_p;

	uInt16  *data;
	int total;
	int wv_index, wv_maxium;
	int high_level, low_level, trig_level;

	typedef struct _WV_REC{
		int start_at;
		int end_at;				
	} WV_REC;
	WV_REC *wv_recs;
	
	UNI_WAVE wave_fr;
	int file_no;
	int total_sample, every_sample, rate_multi;
	void run();
	void deliver(Notitia::HERE_ORDO aordo);
	int s_fld_no, e_fld_no, d_fld_no, gb_fld_no;
	TaskHandle  taskIn , taskOut;
	int emulation;	/* 0:OBU方式， 1：RSU方式 */
	int out_num;

	bool crc_ok;

#include "wlog.h"
};

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *me);
int32 CVICALLBACK InDoneCallback(TaskHandle taskHandle, int32 status, void *me);
int32 CVICALLBACK OutDoneCallback(TaskHandle taskHandle, int32 status, void *me);

void DWave::ignite(TiXmlElement *cfg) 
{
	TiXmlElement *w_ele, *p_ele;
	int maxium_fldno=32;

	total_sample = TOTAL_SAMPLE;
	every_sample = EVERY_SAMPLE;
	rate_multi = RATE_MULTI;
	wv_maxium = WV_MAX;
	high_level = 1020;
	low_level = 980;
	trig_level = 700;
	maxium_fldno = 32;

	cfg->QueryIntAttribute("emulation", &emulation);
	w_ele = cfg->FirstChildElement("wave");
	if ( w_ele ) {
		w_ele->QueryIntAttribute("sampling", &total_sample);	//这是采样时间, 以ms为单位
		w_ele->QueryIntAttribute("every", &every_sample); 	//以微秒为单位
		w_ele->QueryIntAttribute("multi", &rate_multi);		//multi是以M为单位
		w_ele->QueryIntAttribute("wave_num", &wv_maxium);		//一次交易波形数
		w_ele->QueryIntAttribute("threshold_1", &high_level);
		w_ele->QueryIntAttribute("threshold_0", &low_level);
		w_ele->QueryIntAttribute("trigger", &trig_level);
	}
	p_ele = cfg->FirstChildElement("fields");
	if ( p_ele ) {
		p_ele->QueryIntAttribute("maxium", &maxium_fldno);
		p_ele->QueryIntAttribute("start_at", &s_fld_no);
		p_ele->QueryIntAttribute("end_at", &e_fld_no);
		p_ele->QueryIntAttribute("data", &d_fld_no);
		p_ele->QueryIntAttribute("standard", &gb_fld_no);
	}
	maxium_fldno += maxium_fldno/2;

	crc_ok = true;
	p_ele = cfg->FirstChildElement("abnormal");
	if ( p_ele ) {
		if (strcasecmp(p_ele->Attribute("crc"), "err") ==0 )
			crc_ok = false;
	}


	total_sample *= (2*1000*rate_multi);	//从采样时间ms， 转换为采样点数。 采样4M，1ms就要4000点。 并考虑取半
	every_sample *= rate_multi;	//从采样时间us， 转换为采样点数。

	rcv_pac.produce(maxium_fldno) ;
	snd_pac.produce(maxium_fldno) ;
	wv_index = 0;
	if ( !wv_recs ) 
		wv_recs = new WV_REC[wv_maxium];

	wave_fr.pre_set(rate_multi, false, high_level, low_level, false);
	wave_fr.init();

	out_num = 0;
}

DWave::DWave()
{
	pa[0] = &rcv_pac;
	pa[1] = &snd_pac;
	pa[2] = 0;
	
	local_p.ordo = Notitia::PRO_UNIPAC;
	local_p.indic = 0;
	wv_recs = (WV_REC*)0;
}

DWave::~DWave() 
{ 
	if ( wv_recs ) 
		delete[] wv_recs;
} 

bool DWave::facio( Amor::Pius *pius)
{
	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::WINMAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::WINMAIN_PARA");
		goto MainPro;
		break;

	case Notitia::MAIN_PARA:	/* 在整个系统中, 这应是最后被通知到的。 */
		WBUG("facio Notitia::MAIN_PARA");
MainPro:
		deliver(Notitia::SET_UNIPAC);
		run();
		break;

	default:
		return false;
	}
	return true;
}

#define LEV_SIZE 400000

bool DWave::sponte( Amor::Pius *pius) 
{ 
	int32 wmode;
	int32       error=0;
	char        errBuff[2048]={'\0'};
	unsigned char *d_type;
	DB44_WAVE dbw;
	unsigned short levs[LEV_SIZE];
	unsigned char *inp;
	unsigned char in[512];
	unsigned short crc;
	unsigned long in_len;
	int levs_num = LEV_SIZE;
	char msg[2048];

	assert(pius);
	switch ( pius->ordo )
	{
	case Notitia::PRO_UNIPAC:	/* 有数据要发送 */
		WBUG("sponte PRO_UNIPAC");
		d_type = snd_pac.getfld(gb_fld_no);
		inp = snd_pac.getfld(d_fld_no, &in_len);
		memcpy(in, inp,in_len);
		crc = do_crc(0xffff, in, in_len);//算CRC

		if ( !crc_ok  ) 
			crc = ~crc;

		in[in_len] = crc&0x00ff;   //低字节在前
		in_len++;
		in[in_len] = crc>>8&0x00ff; //高字节在后
		in_len++;
		

		switch ( *d_type )
		{
		case FR_TYPE_OBU:
			TEXTUS_SPRINTF(msg, "%s", "FR_TYPE_OBU ");
			goto PRO;
		case FR_TYPE_RSU:
			TEXTUS_SPRINTF(msg, "%s", "FR_TYPE_RSU ");
PRO:
#ifndef NDEBUG
		{
			unsigned long i;
			for ( i = 0 ;i < in_len; i++)
			{
				char tmp[16];
				TEXTUS_SPRINTF(tmp, "%02X ", in[i]);
				TEXTUS_STRCAT(msg, tmp);
			}
			WBUG(msg);
		}
#endif

			dbw.multi = 1;
			dbw.mode(*d_type);
			dbw.generate(levs, levs_num, in, in_len, 600, 65536 - 100);

			error = DAQmxStopTask(taskOut);
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxStopTask Error: %s\n",errBuff);
				goto THIS_END;
			}

			error = DAQmxCfgSampClkTiming(taskOut,"", dbw.multi * 1000000.0,DAQmx_Val_Rising, DAQmx_Val_FiniteSamps,levs_num);
			//error = DAQmxCfgSampClkTiming(taskOut,"", dbw.multi * 1000000.0,DAQmx_Val_Rising, DAQmx_Val_HWTimedSinglePoint ,levs_num);
	/*
			error = DAQmxGetWriteWaitMode(taskOut, &wmode); 
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxGetWriteWaitMode error: %s\n",errBuff);
				goto THIS_END;
			} else {
				WLOG(INFO, "Write_WaitMode %d", wmode);
			}
	*/
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxCfgSampClkTiming error: %s\n",errBuff);
				goto THIS_END;
			}
			outDone = false;

			error = DAQmxWriteBinaryU16(taskOut, levs_num,true,10.0,DAQmx_Val_GroupByChannel, levs, NULL,NULL);	
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxWriteBinaryU16  Error: %s\n",errBuff);
				goto THIS_END;
			}
			WBUG("Analog out end %d", out_num);
/*
			error = DAQmxStartTask(taskOut);
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxStartTask(out) Error: %s\n",errBuff);
				goto THIS_END;
			}
			
*/
			error = DAQmxWaitUntilTaskDone(taskOut,10.0);
			if( DAQmxFailed(error) )
			{
				DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
				WLOG(ERR,"DAQmxWaitUntilTaskDone(out) Error: %s\n",errBuff);
				goto THIS_END;
			}

			WBUG("Analog out end %d", out_num);


			out_num++;

THIS_END:
			break;
		default:
			break;
		}
		break;
	default:
		return false;	
	}
	return true;
}

Amor* DWave::clone()
{
	DWave *child = new DWave();
	return  (Amor*)child;
}

void DWave::deliver(Notitia::HERE_ORDO aordo)
{
	Amor::Pius tmp_pius;
	tmp_pius.ordo = aordo;
	tmp_pius.indic = 0;
	
	switch (aordo)
	{
	case Notitia::SET_UNIPAC:
		WBUG("deliver SET_UNIPAC");
		tmp_pius.indic = &pa[0];
		break;

	default:
		WBUG("deliver Notitia::%d",aordo);
		break;
	}

	aptus->facio(&tmp_pius);
	return ;
}

void DWave:: run()
{
	int32       error=0;
	taskIn=0 ;
	taskOut=0;
	int32 wmode,wmode2;
	unsigned short samd[]={1,2,3,4,5,6,7,8,9,10,11};

	char        errBuff[2048]={'\0'};

	data = (uInt16 *)malloc(sizeof(uInt16 )*total_sample);
	if ( data == 0 ) 
	{
		printf("out of memory!(data)\n");
		return ;
	}
	wave_fr.init();
	total = 0 ;
	file_no = 0;
	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("",&taskIn));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(taskIn,"Dev1/ai0","",DAQmx_Val_Cfg_Default,0.0,2.0,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCfgSampClkTiming(taskIn,"", rate_multi*1000000, DAQmx_Val_Rising, DAQmx_Val_ContSamps, (int(16*1000*1000/every_sample))*every_sample));
	DAQmxErrChk (DAQmxCfgAnlgEdgeStartTrig(taskIn,"Dev1/ai0",DAQmx_Val_Rising,((double)trig_level)/1000));
	DAQmxErrChk (DAQmxSetAnlgEdgeStartTrigHyst(taskIn, 0.05));

	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(taskIn,DAQmx_Val_Acquired_Into_Buffer,every_sample,0,EveryNCallback,(void*)this));
	DAQmxErrChk (DAQmxRegisterDoneEvent(taskIn,0,InDoneCallback, this));


	DAQmxErrChk (DAQmxCreateTask("",&taskOut));
	DAQmxErrChk (DAQmxCreateAOVoltageChan(taskOut,"Dev1/ao0","",-1,2,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxCfgSampClkTiming(taskOut,"", 2 * 1000000.0,DAQmx_Val_Rising,DAQmx_Val_FiniteSamps,10));
	//DAQmxErrChk (DAQmxWriteBinaryU16(taskOut, 10,true,10.0,DAQmx_Val_GroupByChannel, levs, NULL,NULL);	
	//DAQmxErrChk (DAQmxClearTask(taskOut));
	
/*
	
	DAQmxErrChk (DAQmxCfgOutputBuffer (taskOut, 8000000));

	DAQmxErrChk (DAQmxGetWriteWaitMode(taskOut, &wmode)); 
	wmode2 = 12524;
	DAQmxErrChk (DAQmxSetWriteWaitMode(taskOut, wmode2)); 
	DAQmxErrChk (DAQmxGetWriteWaitMode(taskOut, &wmode2)); 

	WLOG(INFO, "Out channel Write_WaitMode old is %d, now set to %d.", wmode, wmode2);
*/
	//DAQmxErrChk (DAQmxRegisterDoneEvent(taskOut,0,OutDoneCallback, this));

	DAQmxErrChk (DAQmxStartTask(taskIn));
	printf("Acquiring samples continuously. Press Enter to interrupt\n");
	getchar();

	/*********************************************/

Error:
	if( DAQmxFailed(error) ) 
	{
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		WLOG(ERR, "DAQmx Error: %s",errBuff);
	}
	if( taskIn!=0 )  {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskIn);
		DAQmxClearTask(taskIn);
	}

	if( taskOut!=0 )  {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskOut);
		DAQmxClearTask(taskOut);
	}

	//printf("End of program, press Enter key to quit\n");
	//getchar();
}

int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* _me)
{
	DWave *me = (DWave*)_me;
	return me->analyze(taskHandle,nSamples);
}

int32 CVICALLBACK DWave::analyze(TaskHandle taskIn,uInt32 nSamples)
{
	int32       error=0;
	char        errBuff[2048]={'\0'};
	int32       read=0;
	FILE *fp;
	unsigned short *levarr;
	int lev_num;
	int i,j,done;
	char msg[1024];
	long *posp=0;

	// DAQmx Read Code
	/*********************************************/
	
	if ( total >= total_sample/2) total = 0;
	DAQmxErrChk (DAQmxReadBinaryU16 (taskIn,-1,2,DAQmx_Val_GroupByChannel,&data[total], (total_sample/2-total), &read,NULL));
	if ( read <= 0 )  return 0;

	lev_num = read;
	levarr = &data[total];
	posp = 0;

	while ( lev_num > 0 ) 
	{
		done = wave_fr.feed_points(levarr, posp, lev_num, -1,1);
		
		if ( done == 1 ) 
		{
			wv_recs[wv_index].start_at = wave_fr.db44_wave.start_at;
			wv_recs[wv_index].end_at = wave_fr.db44_wave.end_at;
			TEXTUS_SPRINTF(msg, "%d DB44 %d, %d, crc %s, ", wv_index, wave_fr.db44_wave.start_at/rate_multi, wave_fr.db44_wave.end_at/rate_multi, wave_fr.db44_wave.crc_ok? "ok":"err" );
			for ( i = 0 ; i < wave_fr.db44_wave.length; i++ )
			{
				char tmp[16];
				TEXTUS_SPRINTF(tmp, "%02X ", wave_fr.db44_wave.data[i]);
				TEXTUS_STRCAT(msg, tmp);
			}
			WBUG(msg);

			if ( emulation == 0 )	//以OBU方式
			{
				rcv_pac.reset();
				snd_pac.reset();
			}
			rcv_pac.input(s_fld_no, (unsigned char*)&wave_fr.db44_wave.start_at, sizeof(wave_fr.db44_wave.start_at));
			rcv_pac.input(e_fld_no, (unsigned char*)&wave_fr.db44_wave.end_at, sizeof(wave_fr.db44_wave.end_at));
			rcv_pac.input(gb_fld_no, (unsigned char*)&wave_fr.db44_wave.dsrc_type, sizeof(wave_fr.db44_wave.dsrc_type));
			rcv_pac.input(d_fld_no, (unsigned char*)wave_fr.db44_wave.data, wave_fr.db44_wave.length);

			if (wave_fr.db44_wave.crc_ok) aptus->facio(&local_p);
			wv_index++;
			wave_fr.init();
			if ( wv_index >= wv_maxium)
				wv_index = 0;
		}

		if ( done == 2 ) 
		{
			wv_recs[wv_index].start_at = wave_fr.gb_wave.start_at;
			wv_recs[wv_index].end_at = wave_fr.gb_wave.end_at;
			printf ("-----GB_%s, %.2f, %.2f, ", wave_fr.gb_wave.dsrc_type == FR_TYPE_GB_OBU ?"OBU":"RSU", wave_fr.gb_wave.start_at/1000.0,wave_fr.gb_wave.end_at/1000.0);
			if (wave_fr.gb_wave.fm_dat.c_14K > 5) 
			{
				printf ("14KWave, %d", wave_fr.gb_wave.fm_dat.c_14K);
			} else {
				printf ("crc %s, ", wave_fr.gb_wave.fm_dat.crc_ok ? "ok":"err");
			}

			for ( i = 0 ; i < wave_fr.gb_wave.fm_dat.len; i++ )
			{
				printf("%02X ", wave_fr.gb_wave.fm_dat.data[i]);
			}
			printf ("\n");

			if ( emulation == 0 )	//以OBU方式
			{
				rcv_pac.reset();
				snd_pac.reset();
			}
			rcv_pac.input(s_fld_no, (unsigned char*)&wave_fr.gb_wave.start_at, sizeof(wave_fr.gb_wave.start_at));
			rcv_pac.input(e_fld_no, (unsigned char*)&wave_fr.gb_wave.end_at, sizeof(wave_fr.gb_wave.end_at));
			rcv_pac.input(gb_fld_no, (unsigned char*)&wave_fr.db44_wave.dsrc_type, sizeof(wave_fr.db44_wave.dsrc_type));
			if ( wave_fr.gb_wave.dsrc_type == FR_TYPE_14K)
			{
				rcv_pac.input(d_fld_no, (unsigned char*)&wave_fr.gb_wave.fm_dat.c_14K, sizeof(wave_fr.gb_wave.fm_dat.c_14K));
			} else {
				rcv_pac.input(d_fld_no, (unsigned char*)wave_fr.gb_wave.fm_dat.data, wave_fr.gb_wave.fm_dat.len);
			}

			if (wave_fr.gb_wave.fm_dat.crc_ok) aptus->facio(&local_p);
			wv_index++;
			wave_fr.init();
			if ( wv_index >= wv_maxium)
				wv_index = 0;
		}
	}

	total += read;

	if ( total >= total_sample/2) 
	{
		DAQmxStopTask(taskIn);
		deliver(Notitia::END_SESSION);
		printf ("\n");
		sprintf_s(msg, "acin%d.txt", file_no);
		fopen_s(&fp, msg, "a+");
		if ( fp ) 
		{
			int rec_s, rec_e, rec_last_e=-1;
			for ( i = 0 ; i < wv_index;  i++ )
			{
				rec_s = wv_recs[i].start_at;	
				rec_e = wv_recs[i].end_at;

				if (rec_s > (rec_last_e + 10) )
					rec_s  -= 10;
				else
					rec_s = rec_last_e ;

				rec_e += 10;		//波形结束点再向后延一点， 
				rec_last_e = rec_e;
					
				for ( j =  rec_s; j < rec_e; j++)
				fprintf_s(fp, "% 9d %04d\n", j, data[j]);
			}
			fclose(fp);
			WLOG(INFO, "file %s out", msg)
		} else {
			printf("file error\n");
			WLOG(ERR, "file %s error", msg)
		}
		total = 0;
		wv_index = 0;
		file_no++;
		wave_fr.pre_set(rate_multi, false, high_level, low_level, false);
		wave_fr.init();
		DAQmxErrChk (DAQmxStartTask(taskIn));
	}

Error:
	if( DAQmxFailed(error) ) {
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskIn);
		DAQmxClearTask(taskIn);
		WLOG(ERR, "DAQmx Error: %s",errBuff);
		//printf("DAQmx Error: %s\n",errBuff);
	}
	return 0;
}

void DWave::log_dmq_err(char *s)
{
	WLOG(ERR, "DAQmx Error: %s", s);
}

int32 CVICALLBACK InDoneCallback(TaskHandle task, int32 status, void *_me)
{
	DWave *me = (DWave*)_me;
	int32   error=0;
	char    errBuff[2048]={'\0'};

	me->log_dmq_err("Task Done");
	DAQmxErrChk (status);

Error:
	if( DAQmxFailed(error) ) 
	{
		DAQmxGetExtendedErrorInfo(errBuff,2048);
		DAQmxClearTask(task);
		me->log_dmq_err(errBuff);
	}
	return 0;
}

int32 CVICALLBACK OutDoneCallback(TaskHandle task, int32 status, void *_me)
{
	DWave *me = (DWave*)_me;
	int32   error=0;
	char    errBuff[2048]={'\0'};
	me->outDone = true;

	error = DAQmxStopTask(task);
	if( DAQmxFailed(error) )
	{
		DAQmxGetExtendedErrorInfo(errBuff, sizeof(errBuff)-1);
		DAQmxClearTask(task);
		me->log_dmq_err(errBuff);
	}
	return 0;
}

#include "hook.c"
