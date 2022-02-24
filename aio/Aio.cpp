/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/**
 Title: aio file
 Build: created by octerboy, 2019/02/13
 $Id$
*/
#define SCM_MODULE_ID	"$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "DPoll.h"
#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "PacData.h"
#include "textus_string.h"
#include "casecmp.h"
#include "BTool.h"
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

#if !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#ifdef AIO_WRITE_TEST
#if !defined(_WIN32)
#if defined(TEXTUS_PLATFORM_64) && !defined(_WIN32)
#include <sys/time.h>
#else
#include <sys/timeb.h>
#endif
#else
#include <time.h>
#include <realtimeapiset.h>
#endif
#endif

class Aio: public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	Aio();
	~Aio();
private:
	bool should_spo;
	bool get_file_all;
	Amor::Pius epl_set_ps, epl_clr_ps, pro_tbuf_ps, tmp_ps;

	char file_name[992];
	int block_size;
	PacketObj *fname_pac;
	void epoll_launch();
#if defined(_WIN32)
	OVERLAPPED ovlpW, ovlpR;
	TEXTUS_LONG ovlpW_offset, ovlpR_offset;
	
	HANDLE hdev, *hdevPtr,  hEvent;		/* �ļ���� */
	DPoll::Pollor pollor; /* �����¼����, ����ʵ����ͬ */
#else
	DPoll::PollorAio pollor; /* �����¼����, ����ʵ����ͬ */
	int fd, *fdPtr;
#if defined(__linux__)
	struct iocb *aiocbp_W, *aiocbp_R;
	struct iocb **iocbp_W, **iocbp_R;
	inline int io_submit(aio_context_t ctx, TEXTUS_LONG nr,  struct iocb **iocbpp) 
	{
		return syscall(__NR_io_submit, ctx, nr, iocbpp);
	}
#else
	struct aiocb *aiocbp_W, *aiocbp_R;
#endif
#endif
	bool a_open();
	void a_close();
	
	struct G_CFG {
		bool on_start;	/* ����ʺϹܵ�����, ������session���� */
		Amor *sch;
		struct DPoll::PollorBase lor; /* ̽ѯ */
		int block_size;
		int pac_fld_num;
		unsigned char start_seq[1024];
		unsigned int seq_len;
#if defined(_WIN32)
		DWORD                 dwDesiredAccess;
		DWORD                 dwShareMode;
		LPSECURITY_ATTRIBUTES lpSecurityAttributes;
		DWORD                 dwCreationDisposition;
		DWORD                 dwFlagsAndAttributes;
#else
		int oflag;
		mode_t mode;
#endif
		void set_cfg (TiXmlElement *cfg) {
			const char *str;
			TiXmlElement *ele;
#if defined(_WIN32)
			TiXmlElement *ele2;
			for (	ele= cfg->FirstChildElement("access"); ele; ele = ele->NextSiblingElement("access"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "Read" ) == 0 )	
				{
					dwDesiredAccess |= GENERIC_READ;
				}
				if ( strcasecmp(str, "EXECUTE" ) == 0 )	
				{
					dwDesiredAccess |= GENERIC_EXECUTE;
				}
				if ( strcasecmp(str, "WRITE" ) == 0 )	
				{
					dwDesiredAccess |= GENERIC_WRITE;
				}
				if ( strcasecmp(str, "All" ) == 0 )	
				{
					dwDesiredAccess |= GENERIC_ALL;
				}
			}
			for ( ele= cfg->FirstChildElement("share"); ele; ele = ele->NextSiblingElement("share"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "DELETE" ) == 0 )	
				{
					dwShareMode |= FILE_SHARE_DELETE;
				}
				if ( strcasecmp(str, "READ" ) == 0 )	
				{
					dwShareMode |= FILE_SHARE_READ;
				}
				if ( strcasecmp(str, "WRITE" ) == 0 )	
				{
					dwShareMode |= FILE_SHARE_WRITE;
				}
			}
			for ( ele= cfg->FirstChildElement("mode"); ele; ele = ele->NextSiblingElement("mode"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "create" ) == 0 )	
				{
					dwCreationDisposition = CREATE_NEW;
				}
				if ( strcasecmp(str, "create_always" ) == 0 )	
				{
					dwCreationDisposition = CREATE_ALWAYS;
				}
				if ( strcasecmp(str, "open" ) == 0 )	
				{
					dwCreationDisposition = OPEN_ALWAYS;
				}
				if ( strcasecmp(str, "exist" ) == 0 )	
				{
					dwCreationDisposition = OPEN_EXISTING;
				}
				if ( strcasecmp(str, "truncate" ) == 0 )	
				{
					dwCreationDisposition = TRUNCATE_EXISTING;
				}
			}

			dwFlagsAndAttributes |= FILE_FLAG_OVERLAPPED;
			ele2= cfg->FirstChildElement("Flag");
			if ( ele2)
			for (	ele= ele2->FirstChildElement("security"); ele; ele = ele->NextSiblingElement("security"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				dwFlagsAndAttributes |= SECURITY_SQOS_PRESENT;
				if ( strcasecmp(str, "ANONYMOUST" ) == 0 )	
					dwFlagsAndAttributes |= SECURITY_ANONYMOUS;
				if ( strcasecmp(str, "CONTEXT_TRACKING" ) == 0 )	
					dwFlagsAndAttributes |= SECURITY_CONTEXT_TRACKING;
				if ( strcasecmp(str, "EFFECTIVE_ONLY" ) == 0 )	
					dwFlagsAndAttributes |= SECURITY_EFFECTIVE_ONLY;
				if ( strcasecmp(str, "IDENTIFICATION" ) == 0 )	
					dwFlagsAndAttributes |= SECURITY_IDENTIFICATION;
				if ( strcasecmp(str, "IMPERSONATION" ) == 0 )	
					dwFlagsAndAttributes |= SECURITY_IMPERSONATION;
			}
			if ( ele2)
			for (	ele= ele2->FirstChildElement("flag"); ele; ele = ele->NextSiblingElement("flag"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "BACKUP_SEMANTICS" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_BACKUP_SEMANTICS;
				if ( strcasecmp(str, "DELETE_ON_CLOSE" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;
				if ( strcasecmp(str, "NO_BUFFERING" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
				if ( strcasecmp(str, "OPEN_NO_RECALL" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_OPEN_NO_RECALL;
				if ( strcasecmp(str, "OPEN_REPARSE_POINT" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_OPEN_REPARSE_POINT;
				if ( strcasecmp(str, "POSIX_SEMANTICS" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_POSIX_SEMANTICS;
				if ( strcasecmp(str, "RANDOM_ACCESS" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;
/*
				if ( strcasecmp(str, "SESSION_AWARE" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_SESSION_AWARE;
*/
				if ( strcasecmp(str, "SEQUENTIAL_SCAN" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;
				if ( strcasecmp(str, "WRITE_THROUGH" ) == 0 )	
					dwFlagsAndAttributes |= FILE_FLAG_WRITE_THROUGH;

			}
			if ( ele2)
			for (	ele= ele2->FirstChildElement("attribute"); ele; ele = ele->NextSiblingElement("attribute"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "ARCHIVE" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_ARCHIVE;
				if ( strcasecmp(str, "ENCRYPTED" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_ENCRYPTED;
				if ( strcasecmp(str, "HIDDEN" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_HIDDEN;
				if ( strcasecmp(str, "NORMAL" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;
				if ( strcasecmp(str, "OFFLINE" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_OFFLINE;
				if ( strcasecmp(str, "READONLY" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_READONLY;
				if ( strcasecmp(str, "SYSTEM" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_SYSTEM;
				if ( strcasecmp(str, "TEMPORARY" ) == 0 )	
					dwFlagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY;
			}
#else
			for ( ele= cfg->FirstChildElement("mode"); ele; ele = ele->NextSiblingElement("mode"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "read_write_execute_owner")== 0 ) 	mode |= S_IRWXU;
				if ( strcasecmp(str, "read_owner" ) == 0 ) 		mode |= S_IRUSR;
				if ( strcasecmp(str, "write_owner" ) == 0 ) 		mode |= S_IWUSR;
				if ( strcasecmp(str, "execute_owner" ) == 0 )		mode |= S_IXUSR;
				if ( strcasecmp(str, "read_write_execute_group")== 0 )	mode |= S_IRWXG;
				if ( strcasecmp(str, "read_group" ) == 0 )		mode |= S_IRGRP;
				if ( strcasecmp(str, "write_group" ) == 0 )		mode |= S_IWGRP;
				if ( strcasecmp(str, "execute_group" ) == 0 )		mode |= S_IXGRP;
				if ( strcasecmp(str, "read_write_execute_other")== 0 )	mode |= S_IRWXO;
				if ( strcasecmp(str, "read_other" ) == 0 )		mode |= S_IROTH;
				if ( strcasecmp(str, "write_other" ) == 0 )		mode |= S_IWOTH;
				if ( strcasecmp(str, "execute_other" ) == 0 )		mode |= S_IXOTH;
			}
			for ( ele= cfg->FirstChildElement("flag"); ele; ele = ele->NextSiblingElement("flag"))
			{
				str = ele->GetText();
				if ( !str || strlen(str) == 0 ) continue;
				if ( strcasecmp(str, "read_only" ) == 0 )	
					oflag |= O_RDONLY;
				if ( strcasecmp(str, "write_only" ) == 0 )	
					oflag |= O_WRONLY;
				if ( strcasecmp(str, "read_write" ) == 0 || strcasecmp(str, "write/read") == 0 
					|| strcasecmp(str, "write_read") == 0 || strcasecmp(str, "read/write") == 0 )	
					oflag |= O_RDWR;
#if defined(__sun) 
				if ( strcasecmp(str, "execute" ) == 0 || strcasecmp(str, "exec" ) == 0 )	
					oflag |= O_EXEC;
				if ( strcasecmp(str, "search" ) == 0 )	
					oflag |= O_SEARCH;
				if ( strcasecmp(str, "close_on_fork" ) == 0 )	
					oflag |= O_CLOFORK;
				if ( strcasecmp(str, "d_sync" ) == 0 || strcasecmp(str, "d_synchronize" ) == 0)	
					oflag |= O_DSYNC;
				if ( strcasecmp(str, "rsync" ) == 0 ) 
					oflag |= O_RSYNC;
				if ( strcasecmp(str, "tpd_safe" ) == 0 )	
					oflag |= O_TPDSAFE;
				if ( strcasecmp(str, "x_attr" ) == 0 )	
					oflag |= O_XATTR;
				if ( strcasecmp(str, "no_links" ) == 0 )	
					oflag |= O_NOLINKS;
#endif
#if defined(__sun) || defined(__linux__) || defined(__FreeBSD__) 
				if ( strcasecmp(str, "directory" ) == 0 )	
					oflag |= O_DIRECTORY;
				if ( strcasecmp(str, "no_delay" ) == 0 )	
					oflag |= O_NDELAY;
#endif

#if defined(__linux__)
				if ( strcasecmp(str, "no_update_time" ) == 0 )	
					oflag |= O_NOATIME;
				if ( strcasecmp(str, "path" ) == 0 )	
					oflag |= O_PATH;
				if ( strcasecmp(str, "async" ) == 0 ) 
					oflag |= O_ASYNC;
#endif
#if defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
				if ( strcasecmp(str, "fsync" ) == 0 ) 
					oflag |= O_FSYNC;
				if ( strcasecmp(str, "shlock" ) == 0 ) 
					oflag |= O_SHLOCK;
				if ( strcasecmp(str, "exlock" ) == 0 ) 
					oflag |= O_EXLOCK;
/*
				if ( strcasecmp(str, "verify" ) == 0 ) 
					oflag |= O_VERIFY;
*/
#endif
#if defined(__linux__) || defined(__NetBSD__) 
				if ( strcasecmp(str, "direct" ) == 0 )	
					oflag |= O_DIRECT;
#endif
				if ( strcasecmp(str, "append" ) == 0 )	
					oflag |= O_APPEND;
				if ( strcasecmp(str, "close_on_exec" ) == 0 )	
					oflag |= O_CLOEXEC;
				if ( strcasecmp(str, "create" ) == 0 || strcasecmp(str, "creat" ) == 0)	
					oflag |= O_CREAT;
				if ( strcasecmp(str, "excl" ) == 0 )	
					oflag |= O_EXCL;
#if !defined(__APPLE__) && !defined(__FreeBSD__)  && !defined(__NetBSD__)  && !defined(__OpenBSD__)
				if ( strcasecmp(str, "largefile" ) == 0 )	
					oflag |= O_LARGEFILE;
#endif
#if defined(__APPLE__)
				if ( strcasecmp(str, "event_only" ) == 0 )	
					oflag |= O_EVTONLY;
				if ( strcasecmp(str, "symbol" ) == 0 )	
					oflag |= O_SYMLINK;
#endif
				if ( strcasecmp(str, "noc_tty" ) == 0 )	
					oflag |= O_NOCTTY;
				if ( strcasecmp(str, "no_follow" ) == 0 )	
					oflag |= O_NOFOLLOW;
				if ( strcasecmp(str, "no_block" ) == 0 )	
					oflag |= O_NONBLOCK;
				if ( strcasecmp(str, "sync" ) == 0 || strcasecmp(str, "synchronize" ) == 0)	
					oflag |= O_SYNC;
				if ( strcasecmp(str, "trunc" ) == 0 || strcasecmp(str, "truncate" ) == 0)	
					oflag |= O_TRUNC;
			}
#endif	/* end for non-windows */
		}
		inline G_CFG(TiXmlElement *cfg) {
			const char *comm_str;
			sch = 0;
			lor.type = DPoll::NotUsed;
                	on_start = true; /* default to start */
			if ( (comm_str = cfg->Attribute("start") ) && strcasecmp(comm_str, "no") ==0 )
                		on_start = false; /* ����һ��ʼ������ */
			block_size = 512;
			pac_fld_num = 1;
			cfg->QueryIntAttribute("block_size", &(block_size));
			cfg->QueryIntAttribute("field", &(pac_fld_num));
			seq_len = 0;
			if ( (comm_str = cfg->Attribute("start_seq") ) )
			{
				seq_len = BTool::unescape(comm_str, start_seq) ;
			}
#if defined(_WIN32)
			dwDesiredAccess = 0;
			dwShareMode = 0;
			lpSecurityAttributes = 0;
			dwCreationDisposition = 0 ;
			dwFlagsAndAttributes = 0;
#else
			oflag = 0;
			mode = 0;
#endif
			set_cfg(cfg);
		};
	};
	struct G_CFG *gCFG;
	bool has_config;

	TBuffer *rcv_buf, *snd_buf;
	TBuffer *m_rcv_buf, *m_snd_buf;
	TBuffer wk_rcv_buf, wk_snd_buf;

	void transmitto_ex();
	void recito_ex();
	void deliver(Notitia::HERE_ORDO aordo);

#include "wlog.h"
};

void Aio::a_close()
{
	WBUG("a_close(%s).....", file_name);
#if !defined(AIO_WRITE_TEST)
#if defined(_WIN32)
	if ( !CloseHandle(hdev) )
	{
		WLOG_OSERR("CloseHandle");
	}
	hdev = INVALID_HANDLE_VALUE;
#else
	if ( close(fd) != 0 )
	{
		WLOG_OSERR("close");
	}
	fd = -1;
#endif
#endif
}

bool Aio::a_open()
{
	char msg[1000];
#if defined(_WIN32)
	if ( this->hdev  != INVALID_HANDLE_VALUE ) return false;
	/* to do SecurityAttributes .... */
	this->hdev = CreateFile(this->file_name, gCFG->dwDesiredAccess, gCFG->dwShareMode, NULL, 
		gCFG->dwCreationDisposition, gCFG->dwFlagsAndAttributes, NULL);

	if (hdev == INVALID_HANDLE_VALUE)
	{
		TEXTUS_SPRINTF(msg, "CreateFile(name=%s)",this->file_name);
		WLOG_OSERR(msg);
		return false;
	}
	//ovlpR.hEvent = hEvent;
#elif defined(__sun) || defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__) || defined(__linux__)
	fd = open(file_name,  gCFG->oflag, gCFG->mode);
	if ( fd == -1 )
	{
		TEXTUS_SPRINTF(msg, "open(%s)",this->file_name);
		WLOG_OSERR(msg);
		return false;
	}
#endif
	epoll_launch();
	return true;
}

void Aio::epoll_launch()
{
	/* ����(����)��������� */
	wk_rcv_buf.reset();	
	wk_snd_buf.reset();

#if defined(_WIN32)
	memset(&ovlpW, 0, sizeof(OVERLAPPED));
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	ovlpR_offset = ovlpW_offset = 0;
	pollor.hnd.file = hdev;
#elif defined(__sun) || defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__) || defined(__linux__)
	aiocbp_R->aio_fildes = fd;
	aiocbp_W->aio_fildes = fd;
	aiocbp_R->aio_nbytes = gCFG->block_size;
        aiocbp_R->aio_offset = 0;
	//aiocbp_W->aio_nbytes = block_size;, given when transmitto_ex
        aiocbp_W->aio_offset = 0;
#endif

	gCFG->sch->sponte(&epl_set_ps);	//��tpoll
	if ( gCFG->seq_len > 0 ) {
		snd_buf->input(gCFG->start_seq, gCFG->seq_len);
		transmitto_ex();
	}
}

void Aio::ignite(TiXmlElement *cfg)
{
	const char *comm_str;
	if ( !gCFG) 
	{
		gCFG = new struct G_CFG(cfg);
		has_config = true;
	}
	comm_str = cfg->Attribute("file");
	if ( comm_str ) 
		TEXTUS_SPRINTF(file_name, "%s", comm_str);
	else
		gCFG->on_start = false;
	block_size = gCFG->block_size;
}

#define DELI(X)	\
	if ( should_spo ) {	\
		aptus->sponte(&X);	\
	} else {			\
		aptus->facio(&X); }

bool Aio::facio( Amor::Pius *pius)
{
	int get_bytes;
#if defined(_WIN32)
	OVERLAPPED_ENTRY *aget;
	DWORD dwPtr;
#else
	off_t offset;
#endif
#if defined(__linux__)
	struct io_event *io_evp;
#endif
	TBuffer **tb;
	assert(pius);

	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :
		WBUG("facio PRO_TBUF");
		transmitto_ex();
		break;

	case Notitia::PRO_EPOLL:
		WBUG("facio PRO_EPOLL");
#if defined(_WIN32)
		aget = (OVERLAPPED_ENTRY *)pius->indic;
		if ( aget->lpOverlapped == &ovlpR )
		{	//�Ѷ�����,  ��ʧ�ܲ������ݲ�������ߴ���
			if ( aget->dwNumberOfBytesTransferred ==0 ) 
			{
				WBUG("IOCP read 0 bytes, EOF");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_End;
				tmp_ps.indic = 0;
				goto ERR_END;
			} else {
				WBUG("IOCP read %d bytes", aget->dwNumberOfBytesTransferred);
				wk_rcv_buf.commit_ack(aget->dwNumberOfBytesTransferred);
				ovlpR_offset += aget->dwNumberOfBytesTransferred;
				TBuffer::pour(*rcv_buf, wk_rcv_buf);
			}
		} else if ( aget->lpOverlapped == &ovlpW ) {
			WLOG(INFO, "write completed");
			ovlpW_offset += aget->dwNumberOfBytesTransferred;
			wk_snd_buf.commit_ack(-(TEXTUS_LONG)aget->dwNumberOfBytesTransferred);	//�Ѿ�����ϵͳ
			if ( snd_buf->point != snd_buf->base )
			{
				transmitto_ex();
			}
			goto H_END;
		} else  {
			WLOG(EMERG, "not my overlap");
			goto H_END;
		}
#endif

#if defined(__linux__)
		io_evp = (struct io_event*)pius->indic;
		if ( (void*)io_evp->obj == (void*)aiocbp_R ) {
			//WBUG("io_submit(read) return " TLONG_FMT " bytes", io_evp->res);
			WBUG("io_submit(read) return %lld bytes", io_evp->res);
			switch ( io_evp->res) {
			case 0:
				WLOG(INFO, "end of file");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_End;
				tmp_ps.indic = 0;
				goto ERR_END;
				break;
			case -1:
				WLOG_OSERR("io_submit(read)");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_Err;
				tmp_ps.indic = 0;
				goto ERR_END;
				break;
			default:
				wk_rcv_buf.commit(io_evp->res);
				aiocbp_R->aio_offset += io_evp->res ;
				TBuffer::pour(*rcv_buf, wk_rcv_buf);
				break;
			}
		} else if ( (void*)io_evp->obj == (void*)aiocbp_W ) {
			WBUG("io_submit(write) return %lld bytes", io_evp->res);
			if ( io_evp->res <= 0 )	{
				WLOG_OSERR("io_submit(write)");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_Err;
				tmp_ps.indic = 0;
				goto ERR_END;
			}
			aiocbp_W->aio_offset += io_evp->res ;
			wk_snd_buf.commit_ack(-(TEXTUS_LONG)io_evp->res);	//�Ѿ�����ϵͳ
			if ( snd_buf->point != snd_buf->base )
			{
				transmitto_ex();
			}
			goto H_END;
		} else {
			WLOG(EMERG, "not my iocb");
			goto H_END;
		}
#endif

#if defined(__sun) || defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
		if (  (struct aiocb*)pius->indic == aiocbp_R) {
			get_bytes = aio_return(aiocbp_R);
			WBUG("aio_return(read) %d bytes", get_bytes);
			switch ( get_bytes) {
			case 0:
				WLOG(INFO, "end of file");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_End;
				tmp_ps.indic = 0;
				goto ERR_END;
			case -1:
				WLOG_OSERR("aio_return(read)");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_Err;
				tmp_ps.indic = 0;
				goto ERR_END;
			default:
				wk_rcv_buf.commit(get_bytes);
				aiocbp_R->aio_offset +=get_bytes ;
				TBuffer::pour(*rcv_buf, wk_rcv_buf);
				break;
			}
		} else if(  (struct aiocb*)pius->indic == aiocbp_W) {
			get_bytes = aio_return(aiocbp_W);
			WBUG("aio_return(write) %d bytes", get_bytes);
			switch ( get_bytes) {
			case -1:
				WLOG_OSERR("aio_return(write)");
				a_close();
				tmp_ps.ordo = Notitia::Pro_File_Err;
				tmp_ps.indic = 0;
				goto ERR_END;
			default:
	printf("---- wk_snd_buf point %p  base %p, len %ld\n",  wk_snd_buf.point, wk_snd_buf.base, wk_snd_buf.point -wk_snd_buf.base);
				aiocbp_W->aio_offset +=get_bytes ;
				wk_snd_buf.commit(-(TEXTUS_LONG)get_bytes);	//�Ѿ�����ϵͳ
				if ( snd_buf->point != snd_buf->base )
				{
					transmitto_ex();
				}
				goto H_END;
			}
		} else {
			WLOG(EMERG, "not my aiocb");
			goto H_END;
		}
#endif
		DELI(pro_tbuf_ps)
		if ( get_file_all )
			recito_ex();
		break;
ERR_END:
		DELI(tmp_ps)
H_END:
		break;

	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("facio SET_TBUF");
		should_spo = true;
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) 
			{	//�µ������TBuffer
				snd_buf = *tb;
			}
			tb++;
			if ( *tb) rcv_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY");
		deliver(Notitia::SET_TBUF);
		tmp_ps.ordo = Notitia::CMD_GET_SCHED;
		aptus->sponte(&tmp_ps);	//��tpoll, ȡ��sched
		gCFG->sch = (Amor*)tmp_ps.indic;
		if ( !gCFG->sch ) 
		{
			WLOG(ERR, "no sched or tpoll");
			break;
		}
		tmp_ps.ordo = Notitia::POST_EPOLL;
		tmp_ps.indic = &gCFG->lor;
		gCFG->lor.pupa = this;
		
		gCFG->sch->sponte(&tmp_ps);	//��tpoll, ȡ��TPOLL
		if ( tmp_ps.indic != gCFG->sch ) break;

#if defined(_WIN32)
		if ( hEvent == INVALID_HANDLE_VALUE )
		hEvent = CreateEvent(	NULL, TRUE, FALSE, NULL);
		if (hEvent == INVALID_HANDLE_VALUE)
		{
			WLOG_OSERR("CreateEvent");
		}
#endif
		if ( gCFG->on_start )
		{
			if ( a_open())		//��ʼ��������
				deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
		}
/*
		{
		 unsigned char snd1[30] = { 
			0x02, 0x00, 0x23 ,0x34 ,0x77 ,0x03 ,0x99 ,0x56 ,0x02 ,0x00 ,0x40 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00,
			0x14 ,0x99 ,0x77 ,0x88 ,0x77 ,0x35 ,0x40 ,0x32 ,0x03 ,0xaf
			};
		 rcv_buf->input(snd1, 28);
		aptus->sponte(&pro_tbuf_ps);
		}
*/
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY");
#if defined(_WIN32)
		if ( hEvent == INVALID_HANDLE_VALUE )
		hEvent = CreateEvent(	NULL, TRUE, FALSE, NULL);
		if (hEvent == INVALID_HANDLE_VALUE)
		{
			WLOG_OSERR("CreateEvent");
		}
#endif
		deliver(Notitia::SET_TBUF);
		if ( gCFG->on_start )
		{
			if ( a_open())		//��ʼ��������
				deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
		}
		break;

	case Notitia::ERR_EPOLL:
		WBUG("facio ERR_EPOLL");
		WLOG(WARNING, (char*)pius->indic);	
		a_close();	//ֱ�ӹرվͿ�.
		tmp_ps.ordo = Notitia::Pro_File_Err;
		tmp_ps.indic = 0;
		DELI(tmp_ps)
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		{
			PacketObj **tmp;
			if ( (tmp = (PacketObj **)(pius->indic)))
			{
				if ( tmp[0] ) 
					fname_pac = tmp[0]; 
				else {
					WLOG(WARNING, "sponte SET_UNIPAC rcv_pac null");
				}
			} else {
				WLOG(WARNING, "facio SET_UNIPAC null");
			}
		}
		break;

	case Notitia::PRO_FILE_FD :
		WBUG("facio PRO_FILE_FD");
#if defined(_WIN32)
		hdevPtr = (HANDLE*)(pius->indic);
#else
		fdPtr = (int*)(pius->indic);
#endif
		break;

	case Notitia::PRO_FILE_Pac :
		WBUG("facio PRO_FILE_Pac");
		TEXTUS_STRCPY(file_name, (char*)(fname_pac->getfld(gCFG->pac_fld_num)));
		goto A_OPEN_PRO;

	case Notitia::PRO_FILE :
		WBUG("facio PRO_FILE");
		TEXTUS_STRCPY(file_name, (char*)pius->indic);
A_OPEN_PRO:
		if ( a_open() )
		{
			tmp_ps.ordo = Notitia::Pro_File_Open;
		} else {
			tmp_ps.ordo = Notitia::Pro_File_Err_Op;
		}
		tmp_ps.indic = 0;
		DELI(tmp_ps)
		break;

	case Notitia::GET_FILE :
		WBUG("facio GET_FILE");
		get_bytes = *(int*)pius->indic;
		switch ( get_bytes) {
		case 0:
			get_file_all = false;
			recito_ex();
			break;
		case -1:	//to end
			get_file_all = true;
			recito_ex();
			break;
		default:
#if defined(_WIN32)
			block_size = get_bytes;
#else
			aiocbp_R->aio_nbytes = get_bytes;
#endif
			recito_ex();
			break;
		}
		//recito_ex();
		break;

	case Notitia::Move_File_From_Begin :
		WBUG("facio Move_File_From_Begin");
#if defined(_WIN32)
		dwPtr = SetFilePointer( hdev, *(((long **)pius->indic)[0]), (((long **)pius->indic)[1]), FILE_BEGIN ); 
		if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
		{ 
			WLOG_OSERR("SetFilePointer");
		}
#else
		offset = lseek(fd, *(((TEXTUS_LONG **)pius->indic)[0]), SEEK_SET);
		if ( offset == -1 ) {
			WLOG_OSERR("lseek");
		}
#endif
		break;

	case Notitia::Move_File_From_Current :
		WBUG("facio Move_File_From_Current");
#if defined(_WIN32)
		dwPtr = SetFilePointer( hdev, *(((long **)pius->indic)[0]), (((long **)pius->indic)[1]), FILE_CURRENT ); 
		if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
		{ 
			WLOG_OSERR("SetFilePointer");
		}

#else
		offset = lseek(fd, *(((TEXTUS_LONG **)pius->indic)[0]), SEEK_CUR);
		if ( offset == -1 ) {
			WLOG_OSERR("lseek");
		}
#endif
		break;

	case Notitia::Move_File_From_End:
		WBUG("facio Move_File_From_End");
#if defined(_WIN32)
		dwPtr = SetFilePointer( hdev, *(((long **)pius->indic)[0]), (((long **)pius->indic)[1]), FILE_END ); 
		if (dwPtr == INVALID_SET_FILE_POINTER) // Test for failure
		{ 
			WLOG_OSERR("SetFilePointer");
		}
#else
		offset = lseek(fd, *(((TEXTUS_LONG **)pius->indic)[0]), SEEK_END);
		if ( offset == -1 ) {
			WLOG_OSERR("lseek");
		}
#endif
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION");	/*����������ߵ�, ��fdҲ��������� */
#if defined(_WIN32)
		hdev = *hdevPtr;
#else
		fd = *fdPtr;
#endif
		//printf("1--- ctx %lu \n", pollor.ctx);
		epoll_launch();
		//printf("2---- ctx %lu \n", pollor.ctx);
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("facio DMD_START_SESSION");
		if ( a_open())		//��ʼ��������
			deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
		break;

	case Notitia::DMD_END_SESSION:
		WBUG("facio DMD_END_SESSION");
		a_close();
		deliver(Notitia::END_SESSION);/* �����Ҵ��ݱ���ĻỰ�ر��ź� */
		break;
#if 0
	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("facio CMD_CHANNEL_PAUSE");
		gCFG->sch->sponte(&epl_clr_ps); //��tpoll,  ע��
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		gCFG->sch->sponte(&epl_set_ps); //��tpoll,  ע��
		break;
#endif
	default:
		return false;
	}	
	return true;
}

bool Aio::sponte( Amor::Pius *pius) 
{ 
	TBuffer **tb;
	switch (pius->ordo)
	{
	case Notitia::PRO_TBUF :	//����һ֡���ݶ���
		WBUG("sponte PRO_TBUF");	
		transmitto_ex();
		break;
		
	case Notitia::SET_TBUF:	/* ȡ������TBuffer��ַ */
		WBUG("sponte SET_TBUF");
		should_spo = false;
		tb = (TBuffer **)(pius->indic);
		if (tb) 
		{	//��Ȼtb����Ϊ��
			if ( *tb) 
			{	//�µ������TBuffer
				rcv_buf = *tb;
			}
			tb++;
			if ( *tb) snd_buf = *tb;
		} else 
			WLOG(NOTICE,"facio PRO_TBUF null.");
		break;

	case Notitia::DMD_END_SESSION:	//ǿ�ƹرգ���ͬ�����رգ�Ҫ֪ͨ����
		WLOG(INFO,"DMD_END_SESSION");
		a_close();
		deliver(Notitia::END_SESSION);/* �����Ҵ��ݱ���ĻỰ�ر��ź� */
		break;

	case Notitia::DMD_START_SESSION:
		WBUG("sponte DMD_START_SESSION");
		if ( a_open() )
			deliver(Notitia::START_SESSION); //������߷���֪ͨ, ������ʼ
		break;
#if 0
	case Notitia::CMD_CHANNEL_PAUSE :
		WBUG("sponte CMD_CHANNEL_PAUSE");
		gCFG->sch->sponte(&epl_clr_ps); //��tpoll,  ע��
		break;

	case Notitia::CMD_CHANNEL_RESUME :
		WBUG("sponte CMD_CHANNEL_RESUME");
		gCFG->sch->sponte(&epl_set_ps); //��tpoll,  ע��
		break;
#endif
	default:
		return false;
	}	
	return true;
}

Aio::Aio()
{
	pollor.pupa = this;
#if defined(_WIN32)
	pollor.type = DPoll::IOCPFile;
	pollor.hnd.file =  INVALID_HANDLE_VALUE;
	pollor.pro_ps.ordo = Notitia::PRO_EPOLL;
#endif
#if defined(__linux__)
	pollor.type = DPoll::EventFD;
#endif
	epl_set_ps.ordo = Notitia::AIO_EPOLL;
	epl_set_ps.indic = &pollor;
	epl_clr_ps.ordo = Notitia::CLR_EPOLL;
	epl_clr_ps.indic = &pollor;
	pro_tbuf_ps.ordo = Notitia::PRO_TBUF;
	pro_tbuf_ps.indic = 0;

	memset(file_name, 0, sizeof(file_name));

#if defined(__sun) || defined(__APPLE__)  || defined(__FreeBSD__)  || defined(__NetBSD__)  || defined(__OpenBSD__)
	aiocbp_R = &(pollor.aiocb_R);
	aiocbp_W = &(pollor.aiocb_W);
#endif

#if defined(__linux__)
	aiocbp_R = &(pollor.aiocb_R);
	aiocbp_W = &(pollor.aiocb_W);
	iocbp_R = &(pollor.iocbpp[0]);
	iocbp_W = &(pollor.iocbpp[1]);
	//printf("==== alloc aiocbp_R %p, iocbpp[0] %p\n", aiocbp_R, pollor.iocbpp[0]);
#endif
#if defined(_WIN32)
	hEvent = INVALID_HANDLE_VALUE;
	hdev = INVALID_HANDLE_VALUE;
#else
	fd = -1;
#endif

	m_rcv_buf = new TBuffer(1024);
	m_snd_buf = new TBuffer(1024);
	rcv_buf = m_rcv_buf;
	snd_buf = m_snd_buf;
	gCFG = 0;
	has_config = false;
	should_spo = false;
	get_file_all = false;
}

Aio::~Aio()
{	
	a_close();
	if (has_config )
		delete gCFG;
	delete m_rcv_buf;
	delete m_snd_buf;
}

void Aio::transmitto_ex()
{
#ifdef AIO_WRITE_TEST
#if defined(_WIN32)
	//ULONGLONG now1, now2;
	FILETIME now1, now2;
#else
	struct timeval now1, now2;
#endif
#endif
	if ( wk_snd_buf.point != wk_snd_buf.base ) {
		WBUG("last writing is still pending");
		return ;	/* not empty, wait */
	}
	TBuffer::pour(wk_snd_buf, *snd_buf);
#if defined(_WIN32)
	
	DWORD snd_len = (DWORD)(wk_snd_buf.point - wk_snd_buf.base);	//���ͳ���
	memset(&ovlpW, 0, sizeof(OVERLAPPED));
	ovlpW.Offset = (DWORD) (ovlpW_offset & 0xFFFFFFFF);
	ovlpW.OffsetHigh = (DWORD)(ovlpW_offset >> 32);
#ifdef AIO_WRITE_TEST
	//QueryUnbiasedInterruptTime(&now1);
	GetSystemTimePreciseAsFileTime(&now1);
#endif
	if ( !WriteFile(hdev, wk_snd_buf.base, snd_len, NULL, &ovlpW) )
	{
		if ( ERROR_IO_PENDING != GetLastError() ) {
			WLOG_OSERR("WriteFile");
			a_close();
			goto ERR_RET;
		}
	}
#ifdef AIO_WRITE_TEST
	//QueryUnbiasedInterruptTime(&now2);
	GetSystemTimePreciseAsFileTime(&now2);
	//WBUG("------tme --------- %I64d (us)", (now2- now1)/10);
	WBUG("------tme --------- %I64d (us) ", ((long long)((now2.dwHighDateTime- now1.dwHighDateTime)<<32) + (long long)(now2.dwLowDateTime - now1.dwLowDateTime))/10);
#endif
#else
#ifdef AIO_WRITE_TEST
	gettimeofday(&now1,0);
#endif
#if  defined(__linux__)
	aiocbp_W->aio_reqprio = 0;
	aiocbp_W->aio_buf = (u_int64_t) wk_snd_buf.base;
	aiocbp_W->aio_nbytes = wk_snd_buf.point - wk_snd_buf.base;
	//aiocbp_W->aio_offset = 0; absolute pos?
	//	
	if (io_submit(pollor.ctx, 1, iocbp_W) <= 0) {
		WLOG_OSERR("io_submit(write)");
#else
	aiocbp_W->aio_nbytes = wk_snd_buf.point - wk_snd_buf.base;
        aiocbp_W->aio_buf = wk_snd_buf.base;
	if ( aio_write(aiocbp_W) == -1 )
	{
		WLOG_OSERR("aio_write");
#endif
		a_close();
		goto ERR_RET;
	}
#ifdef AIO_WRITE_TEST
	gettimeofday(&now2,0);
	WBUG("------tme --------- %ld ", now2.tv_sec*1000000+now2.tv_usec- now1.tv_sec*1000000- now1.tv_usec);
	WBUG("------tme --------- %ld %ld %ld %ld", now2.tv_sec, now2.tv_usec, now1.tv_sec, now1.tv_usec);
	printf("++++++ wk_snd_buf point %p  base %p, len %ld\n",  wk_snd_buf.point, wk_snd_buf.base, wk_snd_buf.point -wk_snd_buf.base);
#endif
#endif
	return;
ERR_RET:
	tmp_ps.ordo = Notitia::Pro_File_Err;
	tmp_ps.indic = 0;
	DELI(tmp_ps)
}

void Aio::recito_ex()
{
	wk_rcv_buf.grant(block_size);
#if defined(_WIN32)
	//ResetEvent(hEvent);
	memset(&ovlpR, 0, sizeof(OVERLAPPED));
	ovlpR.Offset = (DWORD) (ovlpR_offset & 0xFFFFFFFF);
	ovlpR.OffsetHigh = (DWORD)(ovlpR_offset >> 32);
	if (!ReadFile(hdev, wk_rcv_buf.point, block_size, NULL, &ovlpR) )
	{
		DWORD dwError = GetLastError();
		switch (dwError) { 
                case ERROR_HANDLE_EOF:
                    	WBUG("ReadFile returned EOF %s", file_name);
			goto EOF_RET;
                case ERROR_IO_PENDING: 	/* do nothing */
                    	WBUG("ReadFile returned IO_PENDING %s", file_name);
			break;
		default:
                	{ 
			DWORD dwBytesRead;
			if (!GetOverlappedResult(hdev,  &ovlpR, &dwBytesRead, FALSE) )
			{
				dwError = GetLastError();
				switch (dwError) { 
                		case ERROR_HANDLE_EOF:
                    			WBUG("ReadFile(GetOverlappedResult) returned EOF %s", file_name);
					goto EOF_RET;
				default:
					WLOG_OSERR("ReadFile");
					goto ERR_RET;

				}
			} else {	
				printf("getover ok\n");
			}
			}
		}
	}  else { printf("Read ok\n");}
#elif  defined(__linux__)
	aiocbp_R->aio_reqprio = 0;
	aiocbp_R->aio_buf = (u_int64_t) wk_rcv_buf.point;
	//aiocbp_R->aio_nbytes = block_size; had the value when a_open()
	//printf("block %ld bytes %d iocbp_R %p aiocbp_R %p\n",block_size,  aiocbp_R->aio_nbytes, iocbp_R, aiocbp_R);
	//aiocbp_R->aio_offset = 0; absolute pos?
	//{int *a =0; *a= 0; }
	if (io_submit(pollor.ctx, 1, iocbp_R) <= 0) {
		WLOG_OSERR("io_submit(read)");
		goto ERR_RET;
	}
#else
	aiocbp_R->aio_buf = wk_rcv_buf.point;
	if ( aio_read(aiocbp_R) == -1 )
	{
		WLOG_OSERR("aio_read");
		goto ERR_RET;
	}
#endif
	return;
ERR_RET:
	a_close();
	tmp_ps.ordo = Notitia::Pro_File_Err;
	tmp_ps.indic = 0;
	DELI(tmp_ps)
	return;

#if defined(_WIN32)
EOF_RET:
	a_close();
	tmp_ps.ordo = Notitia::Pro_File_End;
	tmp_ps.indic = 0;
	DELI(tmp_ps)
	return;
#endif
}

Amor* Aio::clone()
{
	Aio *child;
	child = new Aio();
	child->gCFG = gCFG;
	memcpy(child->file_name, file_name, strlen(file_name)+1);
	child->block_size = gCFG->block_size;
#if 0	/* �Ժ��Ƿ����������� */
#if defined(_WIN32)
	child->dwDesiredAccess = dwDesiredAccess ;
	child->dwShareMode = dwShareMode ;
	child->lpSecurityAttributes = lpSecurityAttributes ;
	child->dwCreationDisposition = dwCreationDisposition ;
	child->dwFlagsAndAttributes = dwFlagsAndAttributes ;
#else
	child->oflag = oflag;
	child->mode mode;
#endif
#endif
	return (Amor*)child;
}

/* ��������ύ */
void Aio::deliver(Notitia::HERE_ORDO aordo)
{
	TBuffer *tb[3];
	tmp_ps.ordo = aordo;
	tmp_ps.indic = 0;

	switch (aordo )
	{
	case Notitia::SET_TBUF:
		WBUG("deliver SET_TBUF");
		tb[0] = rcv_buf;
		tb[1] = snd_buf;
		tb[2] = 0;
		tmp_ps.indic = &tb[0];
		break;

	case Notitia::END_SESSION:
		WBUG("deliver END_SESSION");
		if ( should_spo ) {
			aptus->sponte(&tmp_ps);
			return;
		}
		break;

	case Notitia::START_SESSION:
		WBUG("deliver START_SESSION");
		if ( should_spo ) {
			aptus->sponte(&tmp_ps);
			return;
		}
		break;

	default:
		break;
	}
	aptus->facio(&tmp_ps);
}
#include "hook.c"
