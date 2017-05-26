/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/**
 Title: HTTP Download File
 Build:created by octerboy 2005/04/12
 $Id$
*/

#define SCM_MODULE_ID  "$Id$"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#if !defined(_WIN32)
#include <unistd.h>
#include <sys/mman.h>
#endif
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>

#include "Amor.h"
#include "Notitia.h"
#include "TBuffer.h"
#include "textus_string.h"
#include "casecmp.h"

#define DEFAULT_PAGE "index.htm"
#define DEFAULT_HTTP_FILE_CHARSET "iso-8859-1"

class FileLet :public Amor
{
public:
	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();

	FileLet();
	~FileLet();

private:
#include "httpsrv_obj.h"
#include "wlog.h"
	Amor::Pius local_pius;  //仅用于向mary传回数据

	TBuffer *req_body;	/* 请求体 */
	TBuffer *res_entity;	/* 响应体 */
	const char *file_name;

	struct G_CFG {
		char default_page[256];	/* 首页, 通常是index.html, default.htm 等 */
		char home_dir[256];	/* 主目录, 所有文件都在这个目录及其子目录下 */
		char charset[64];	/* 字符集, 比如gb2312等 */
		int block_size;		/* 每次向TBuffer中写多少K字节,
					 0表示所有, 即一次性把文件内容写到套接字中 */
		bool sec_check;
		bool use_default;	/* 是否使用缺省值 */
		bool use_gz_ext;	/* 是否使用.gz 扩展. 若是, 则把URL中的文件名加上".gz"扩展名再搜索 */

		inline G_CFG(TiXmlElement *cfg) {
			const char *page_str, *dir_str, *set_str, *comm_str;
			
			memset(default_page, 0, sizeof(default_page));
			memset(home_dir, 0, sizeof(home_dir));
			memset(charset, 0, sizeof(charset));
			TEXTUS_STRCPY(default_page, DEFAULT_PAGE);
			TEXTUS_STRCPY(home_dir, "/");
			TEXTUS_STRCPY(charset, DEFAULT_HTTP_FILE_CHARSET);
			sec_check = use_default = false;
			use_gz_ext = false;
			block_size = 8;

			cfg->QueryIntAttribute("block", &block_size);
			if ( block_size < 0 ) block_size = 8;
			if ( (page_str = cfg->Attribute("default")))
				TEXTUS_STRNCPY(default_page, page_str, sizeof(default_page)-1);

			if ( (dir_str = cfg->Attribute("home")))
				TEXTUS_STRNCPY(home_dir, dir_str, sizeof(home_dir)-2);

			if ( (set_str = cfg->Attribute("charset")))
				TEXTUS_STRNCPY(charset, set_str, sizeof(charset)-1);

			if ( (comm_str = cfg->Attribute("security")) && strcasecmp(comm_str, "yes") == 0 )
				sec_check = true;

			if ( (comm_str = cfg->Attribute("fixed")) && strcasecmp(comm_str, "yes") == 0 )
				use_default = true;

			if ( (comm_str = cfg->Attribute("gzip_ext")) && strcasecmp(comm_str, "yes") == 0 )
				use_gz_ext = true;

			if(strlen(home_dir) < 1 ) 
				TEXTUS_STRCPY(home_dir, "/");

			if ( home_dir[0] != '/' ) 
			{
				char tmp[512];
				TEXTUS_STRCPY(tmp,home_dir);
				TEXTUS_STRCPY(&home_dir[1], tmp);
				home_dir[0] = '/';
			}

			if( home_dir[strlen(home_dir)-1] != '/' )
				TEXTUS_STRCAT(home_dir,"/");
		};
	};

	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;


	/* false:未处理请求; true:已处理请求 */
	bool handle();
	void to_response(char *ptr, size_t total);

	char path[1024];
	char *file;
};

static const char* get_mime_type( char* name );

void FileLet::ignite(TiXmlElement *prop)
{
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG(prop);
		has_config = true;
	}
}

bool FileLet::facio( Amor::Pius *pius)
{
	TBuffer **tb = 0;
	assert(pius);

	switch ( pius->ordo )
	{
	case Notitia::PRO_HTTP_HEAD:	/* 有HTTP请求 */
		WBUG("facio PRO_HTTP_HEAD ");
		if ( !req_body || !res_entity )
		{/* 当然输入输出缓冲区得已经准备好 */
			WLOG(ERR,"req_body buf or res_entity buf is null!");
			break;
		}
		(void) handle();
		break;

	case Notitia::SET_TBUF:	/* 第一个是http请求体, 第二个是http响应体 */
		WBUG("facio SET_TBUF");
		if ( (tb = (TBuffer **)(pius->indic)))
		{	//tb应当不为NULL，*tb是rcv_buf
			if ( *tb) req_body = *tb; 
			tb++;
			if ( *tb) res_entity = *tb;
		} else 
		{	
			WLOG(WARNING, "facio SET_TBUF null");
		}
		file_name = getHead("Path");
		break;
		
	default:
		WBUG("facio %lu", pius->ordo);
		return false;
	}
	return true;
}

bool FileLet::sponte( Amor::Pius *pius) { return false; }

FileLet::FileLet()
{
	local_pius.ordo = 0;
	local_pius.indic = 0;

	gCFG = 0;
	has_config = false;
	file = &(path[1]);
}

Amor* FileLet::clone()
{
	FileLet *child = new FileLet();
	child->gCFG = gCFG;
	return (Amor*)child;
}

FileLet::~FileLet() { 
	if ( has_config  )
	{	
		if(gCFG) delete gCFG;
	}
}

void FileLet::to_response(char *ptr, size_t total)
{
	size_t wlen = total;
	int cents,last_cents=100, j=0, to_times;
	char *p = ptr;
	unsigned long block, hsize; 
	hsize = gCFG->block_size*1024;
	res_entity->grant(hsize); 
	
	to_times = total / hsize; 
	if ( to_times < 1 ) to_times = 1;
	while ( wlen > 0 )
	{	
		block = wlen > hsize ? hsize : wlen;
		memcpy(res_entity->point, p, block ); 
		res_entity->commit(block);		
		local_pius.ordo = Notitia::PRO_TBUF ;
		aptus->sponte(&local_pius);
		wlen -= block;
		p += block;
		cents = j*100/to_times;
		if ( cents % 10 == 0 && cents !=last_cents)
		{
			WLOG(INFO, "sent %d%% of the file", cents);
			last_cents = cents;
		}
		j++;
	}
}

bool FileLet::handle()
{
	char *end_c;
	bool has_slash;
	struct stat sb;	/* 文件状态 */
	bool file_found=true;
	bool is_gzip_file=false;
	const char* type;
	char fixed_type[500];

#if defined(_WIN32)
	HANDLE fd;
	HANDLE hFileMapping;
	#define ISMYDIR(x) (_S_IFDIR & x)
	#define ISMYREG(x) (_S_IFREG & x)
#else
	#define ISMYDIR(x) (S_ISDIR(x))	
	#define ISMYREG(x) (S_ISREG(x))	
	int fd;		/* 文件句柄 */
#endif
	char* ptr;

	WBUG("Request path is %s", file_name);
	if ( file_name[0] != '/')
	{
		sendError(400);
		WBUG("file_name[0] != '/', the path is %s", file_name);
		goto Error;
	} else {	
		if ( strlen(file_name) <=1 || gCFG->use_default )
			TEXTUS_SNPRINTF(path, sizeof(path)-2, "%s%s", gCFG->home_dir, gCFG->default_page);
		else
			TEXTUS_SNPRINTF(path, sizeof(path)-2, "%s%s", gCFG->home_dir, &file_name[1]);

		if ( file[0] == '\0' )
			TEXTUS_STRCPY(file,"./");
PROAGAIN:
		if ( gCFG->sec_check && ( strstr( file, "/.." ) != (char*) 0 || strstr( file, "\\.." ) != (char*) 0))
		{
			sendError(400);
			setContentSize(0);
			WBUG("file name is invalid, ths file is %s", file);
			goto Error;
		}

		/* if the URI's last character is the '\\' or '/', remove it because of WIN32 */
		end_c = &file[strlen(file)-1];	
		has_slash = false;
		if ( *end_c == '\\' || *end_c == '/' )
		{
			has_slash = true;
			*end_c = '\0';
		}

		type = get_mime_type( file );
		(void) TEXTUS_SNPRINTF( fixed_type, sizeof(fixed_type), type, gCFG->charset);

		if ( stat( file, &sb ) < 0 )
		{
			if ( gCFG->use_gz_ext )
			{
				TEXTUS_STRCAT(path, ".gz");	/* 加上".gz"扩展再找 */
				if ( stat( file, &sb ) < 0 )
					file_found = false;
				else
					is_gzip_file = true;
			} else
				file_found = false;
		}
			
		if ( !file_found )
		{
			WLOG(INFO, "Not found the file of %s", file);
			return false;	/* 被认为找不到文件，另行处理 */
		}

		if ( ISMYDIR(sb.st_mode) )
		{
			if ( !has_slash )		/* Redirect */
			{
				TEXTUS_STRNCPY(path, file_name, sizeof(path)-3);
				TEXTUS_STRCAT(path, "/");
				setStatus(303);
				setHead("Location", path);
				//output(path);
				setContentSize(strlen(path));
				res_entity->input((unsigned char*)path, strlen(path));
				goto Error;
			}

			TEXTUS_SNPRINTF(&file[strlen(file)], 512, "/%s", gCFG->default_page);
			goto PROAGAIN;
		}

		if ( !(ISMYREG(sb.st_mode)) )
		{
			WLOG(INFO, "it's not regular file of %s", file);
			return false;	/* 被认为找不到文件，另行处理 */
		}
	}

	setHead("Content-Type", fixed_type);
	if ( is_gzip_file)
		setHead("Content-Encoding", "gzip");

	if ( gCFG->use_default)
		goto SEND_FILE;

	setHeadTime("Last-Modified", sb.st_mtime);

	time_t if_modified_since ;
	if_modified_since = getHeadInt("If-Modified-Since");
	if ( if_modified_since >0 &&
		if_modified_since == sb.st_mtime )
	{
		setStatus(304);
		setContentSize(0);
		goto Error;
	}

SEND_FILE:
#if defined(_WIN32)
	fd = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (fd == INVALID_HANDLE_VALUE )
#else
    	fd = open( file, O_RDONLY );
    	if ( fd < 0 )
#endif
	{	
		sendError(403);
		local_pius.ordo = Notitia::PRO_HTTP_HEAD ;
		aptus->sponte(&local_pius);
		return true;	/* 已经发现错误,返回内容已定,故返回真 */
    	}
	WBUG("found the file of %s", file);

	setContentSize(sb.st_size);
	
	/* 响应头已经准备完毕, 将此发送出去 */
	local_pius.ordo = Notitia::PRO_HTTP_HEAD ;	
	aptus->sponte(&local_pius);

    	if ( sb.st_size > 0 )	/* avoid zero-length mmap */
	{

#if defined(_WIN32)
		hFileMapping = CreateFileMapping(fd, NULL,PAGE_READONLY, 0,0, NULL);
		ptr = (char*) MapViewOfFile(hFileMapping,FILE_MAP_READ,0, 0, sb.st_size);
		if ( ptr != NULL )
	    	{	
			to_response(ptr, sb.st_size);
			UnmapViewOfFile(ptr);
		}
		CloseHandle(hFileMapping);
#else
		ptr =(char *) mmap( 0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0 );
		if ( ptr != (char*) -1 )
	    	{	
			to_response(ptr, sb.st_size);
	    		(void) munmap( ptr, sb.st_size );
		}
#endif
	}
#if defined(_WIN32)
	CloseHandle(fd);
#else
    	(void) close( fd );
#endif

	WBUG("has sent the file of %s", file);
	goto End;
Error:
	local_pius.ordo = Notitia::PRO_HTTP_HEAD ;	
	aptus->sponte(&local_pius);
End:
	return true;
}

const char* get_mime_type( char* name )
{
    	struct 
    	{	
		const char* ext;
		const char* type;
	} table[] = 
	{ 
		{ ".html", "text/html; charset=%s" },
		{ ".htm", "text/html; charset=%s" },
		{ ".txt", "text/plain; charset=%s" },
		{ ".rtx", "text/richtext" },
		{ ".etx", "text/x-setext" },
		{ ".tsv", "text/tab-separated-values" },
		{ ".css", "text/css" },
		{ ".xml", "text/xml" },
		{ ".dtd", "text/xml" },
		{ ".gif", "image/gif" },
		{ ".jpg", "image/jpeg" },
		{ ".jpeg", "image/jpeg" },
		{ ".jpe", "image/jpeg" },
		{ ".jfif", "image/jpeg" },
		{ ".tif", "image/tiff" },
		{ ".tiff", "image/tiff" },
		{ ".pbm", "image/x-portable-bitmap" },
		{ ".pgm", "image/x-portable-graymap" },
		{ ".ppm", "image/x-portable-pixmap" },
		{ ".pnm", "image/x-portable-anymap" },
		{ ".xbm", "image/x-xbitmap" },
		{ ".xpm", "image/x-xpixmap" },
		{ ".xwd", "image/x-xwindowdump" },
		{ ".ief", "image/ief" },
		{ ".png", "image/png" },
		{ ".au", "audio/basic" },
		{ ".snd", "audio/basic" },
		{ ".aif", "audio/x-aiff" },
		{ ".aiff", "audio/x-aiff" },
		{ ".aifc", "audio/x-aiff" },
		{ ".ra", "audio/x-pn-realaudio" },
		{ ".ram", "audio/x-pn-realaudio" },
		{ ".rm", "audio/x-pn-realaudio" },
		{ ".rpm", "audio/x-pn-realaudio-plugin" },
		{ ".wav", "audio/wav" },
		{ ".mid", "audio/midi" },
		{ ".midi", "audio/midi" },
		{ ".kar", "audio/midi" },
		{ ".mpga", "audio/mpeg" },
		{ ".mp2", "audio/mpeg" },
		{ ".mp3", "audio/mpeg" },
		{ ".mpeg", "video/mpeg" },
		{ ".mpg", "video/mpeg" },
		{ ".mpe", "video/mpeg" },
		{ ".qt", "video/quicktime" },
		{ ".mov", "video/quicktime" },
		{ ".avi", "video/x-msvideo" },
		{ ".movie", "video/x-sgi-movie" },
		{ ".mv", "video/x-sgi-movie" },
		{ ".vx", "video/x-rad-screenplay" },
		{ ".a", "application/octet-stream" },
		{ ".bin", "application/octet-stream" },
		{ ".exe", "application/octet-stream" },
		{ ".dump", "application/octet-stream" },
		{ ".o", "application/octet-stream" },
		{ ".class", "application/java" },
		{ ".js", "application/x-javascript" },
		{ ".ai", "application/postscript" },
		{ ".eps", "application/postscript" },
		{ ".ps", "application/postscript" },
		{ ".dir", "application/x-director" },
		{ ".dcr", "application/x-director" },
		{ ".dxr", "application/x-director" },
		{ ".fgd", "application/x-director" },
		{ ".aam", "application/x-authorware-map" },
		{ ".aas", "application/x-authorware-seg" },
		{ ".aab", "application/x-authorware-bin" },
		{ ".fh4", "image/x-freehand" },
		{ ".fh7", "image/x-freehand" },
		{ ".fh5", "image/x-freehand" },
		{ ".fhc", "image/x-freehand" },
		{ ".fh", "image/x-freehand" },
		{ ".spl", "application/futuresplash" },
		{ ".swf", "application/x-shockwave-flash" },
		{ ".dvi", "application/x-dvi" },
		{ ".gtar", "application/x-gtar" },
		{ ".hdf", "application/x-hdf" },
		{ ".hqx", "application/mac-binhex40" },
		{ ".iv", "application/x-inventor" },
		{ ".latex", "application/x-latex" },
		{ ".man", "application/x-troff-man" },
		{ ".me", "application/x-troff-me" },
		{ ".mif", "application/x-mif" },
		{ ".ms", "application/x-troff-ms" },
		{ ".oda", "application/oda" },
		{ ".pdf", "application/pdf" },
		{ ".rtf", "application/rtf" },
		{ ".bcpio", "application/x-bcpio" },
		{ ".cpio", "application/x-cpio" },
		{ ".sv4cpio", "application/x-sv4cpio" },
		{ ".sv4crc", "application/x-sv4crc" },
		{ ".sh", "application/x-shar" },
		{ ".shar", "application/x-shar" },
		{ ".sit", "application/x-stuffit" },
		{ ".tar", "application/x-tar" },
		{ ".tex", "application/x-tex" },
		{ ".texi", "application/x-texinfo" },
		{ ".texinfo", "application/x-texinfo" },
		{ ".tr", "application/x-troff" },
		{ ".roff", "application/x-troff" },
		{ ".man", "application/x-troff-man" },
		{ ".me", "application/x-troff-me" },
		{ ".ms", "application/x-troff-ms" },
		{ ".zip", "application/x-zip-compressed" },
		{ ".tsp", "application/dsptype" },
		{ ".wsrc", "application/x-wais-source" },
		{ ".ustar", "application/x-ustar" },
		{ ".cdf", "application/x-netcdf" },
		{ ".nc", "application/x-netcdf" },
		{ ".doc", "application/msword" },
		{ ".ppt", "application/powerpoint" },
		{ ".wrl", "model/vrml" },
		{ ".vrml", "model/vrml" },
		{ ".mime", "message/rfc822" },
		{ ".pac", "application/x-ns-proxy-autoconfig" },
		{ ".wml", "text/vnd.wap.wml" },
		{ ".wmlc", "application/vnd.wap.wmlc" },
		{ ".wmls", "text/vnd.wap.wmlscript" },
		{ ".wmlsc", "application/vnd.wap.wmlscriptc" },
		{ ".wbmp", "image/vnd.wap.wbmp" },
	};
    	int fl = strlen( name );
    	unsigned int i;

    	for ( i = 0; i < sizeof(table) / sizeof(*table); ++i )
	{
		int el = strlen( table[i].ext );
		if ( strcasecmp( &(name[fl - el]), table[i].ext ) == 0 )
	    		return table[i].type;
	}
    	return "text/plain; charset=%s";
}
#include "hook.c"

