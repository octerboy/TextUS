#include "Notitia.h"
#include "Amor.h"
#include <stdio.h>

#define SCM_MODULE_ID  "$Workfile: Ef06.cpp $"
#define TEXTUS_MODTIME  "$Date$"
#define TEXTUS_BUILDNO  "$Revision$"
/* $NoKeywords: $ */

#include "PacData.h"
#include "BTool.h"
#include "casecmp.h"
#include "textus_string.h"

#include <process.h> 
#include <stdlib.h>
#include <time.h>
#include <sys/timeb.h>
#include <ctype.h>
#include <stdarg.h>

#define MINLINE inline
#define ObtainHex(s, X)   ( (s) > 9 ? (s)-10+X :(s)+'0')
#define Obtainx(s)   ObtainHex(s, 'a')
#define ObtainX(s)   ObtainHex(s, 'A')
#define Obtainc(s)   (s >= 'A' && s <='F' ? s-'A'+10 :(s >= 'a' && s <='f' ? s-'a'+10 : s-'0' ) )

static char* byte2hex(const unsigned char *byte, size_t blen, char *hex) 
{
	size_t i;
	for ( i = 0 ; i < blen ; i++ )
	{
		hex[2*i] =  ObtainX((byte[i] & 0xF0 ) >> 4 );
		hex[2*i+1] = ObtainX(byte[i] & 0x0F );
	}
//	hex[2*i] = '\0';
	return hex;
}

static unsigned char* hex2byte(unsigned char *byte, size_t blen, const char *hex)
{
	size_t i;
	const char *p ;	

	p = hex; i = 0;

	while ( i < blen )
	{
		byte[i] =  (0x0F & Obtainc( hex[2*i] ) ) << 4;
		byte[i] |=  Obtainc( hex[2*i+1] ) & 0x0f ;
		i++;
		p +=2;
	}
	return byte;
}

class Ef06: public Amor
{
public:
	Amor::Pius local_pius;

	PacketObj *rcv_pac;	/* 来自左节点的PacketObj */
	PacketObj *snd_pac;

	Ef06();
	~Ef06();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	void handle_pac();	
	typedef struct _I5A {	 //对5A指令的处理定义
		const char *protect_key_index_06;	//06保护密钥索引
		const char *out_key_index_06;		//06导出密钥索引
		const char *protect_key_index_mu;	//母卡保护密钥索引, 这项可能不存在
		const char *div_ins_mu;		//母卡分散密钥的指令头, Lc这里计算
		const char *div_key_index_mu;		//母卡分散密钥索引, 如11 01 FE(母卡中) 01 01 00(ESAM的密钥头)
		const char *out_ins_mu;		//母卡导出密钥的指令头, Lc这里计算
	} I5A;

	typedef struct _I70 {	 //对70指令的处理定义
		const char *div_key_06;		//06分散密钥索引
		const char *div_ins_mu;		//母卡分散密钥的指令头, Lc这里计算
		const char *div_key_mu;		//母卡分散密钥索引
		const char *out_ins_mu;		//母卡输出MAC的指令头,Lc这里计算
	} I70;

	struct G_CFG 	//全局定义
	{
		int work_id;
		int factory_id;		//母卡厂商定义: 0:握奇, 1:捷德, 2:天喻

		int i5A_num;
		I5A *i5a;
	
		int i70_num;
		I70 *i70;
	
		inline G_CFG() {
			factory_id = 1;		//母卡厂商默认为捷德
			work_id = 0;
			i5a = 0;
			i70 = 0;
			i5A_num = 0;
			i70_num = 0;
		};	
		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* 全局共享参数 */
	bool has_config;
	int instance_id;
	char m_error_buf[512];
	
	Ef06 *who_back;		//0:本对象闲着
	HANDLE op_mutex;	//判断who_back的操作互斥量, 
	HANDLE pro_ev;		//event变量, 指示有工作要做
	bool CanI()
	{
		bool ret;
		if (WaitForSingleObject(op_mutex, 4000) == WAIT_OBJECT_0 )
		{
			if ( who_back )		//已被占用
				ret = false;
			else
				ret = true;
		} else {
			sprintf(m_error_buf, "WaitForSingleObject != WAIT_OBJECT_0");
			return false;
		}

		ReleaseMutex(op_mutex);
		return ret;
	};

	void YouCan()
	{
		if (WaitForSingleObject(op_mutex, 4000) == WAIT_OBJECT_0 )
		{
			who_back = (Ef06*) 0;	//不再占用
		}
		ReleaseMutex(op_mutex);
	};

private:

#include "wlog.h"
};

typedef struct _IC_Pool {
	int wid;
	Ef06 *ef;
	_IC_Pool () {
		wid = -1;
		ef = 0;
	};
} IC_Pool;

#define POOL_MAX 100
static IC_Pool *g_pool=0;

Ef06 *look_idle ()
{
	int i;
	Ef06 *ret, *tmp = 0;
	for ( i = 0 ; i < POOL_MAX; i++)
	{
		tmp = g_pool[i].ef;
		if ( tmp == (Ef06*) 0 )
			break;
		
		if ( tmp->CanI() )
		{
			ret = tmp;
			break;
		}
	}
	return ret;
}

#if !defined(_WIN32)
	typedef void (*my_thread_func)(void*);
#else
	typedef void (__cdecl *my_thread_func)(void*);
#endif

static void  pac_thrd(Ef06  *arg)
{
#if !defined(_WIN32)
	pthread_detach(pthread_self());
#endif
	arg->handle_pac();
#if defined(_WIN32)
	_endthreadex(0);
#endif
}

void Ef06::ignite(TiXmlElement *prop)
{
	TiXmlElement *ele_5a, *ele_70;
	TiXmlElement *var_ele;
	const char *vn="5A";
	int many = 0, vmany = 0;
	if (!prop) return;
	if ( !gCFG ) 
	{
		gCFG = new struct G_CFG();
		has_config = true;
	}
	prop->QueryIntAttribute("work", &(gCFG->work_id));
	if ( g_pool == 0 ) 
	{
		g_pool = new IC_Pool[POOL_MAX];
	}
	/* 池建设, 实际工作的, 都是顶级对象啦 */
	for ( int i = 0 ; i < POOL_MAX ; i++ )
	{
		if (g_pool[i].ef == 0 ) 
		{
			g_pool[i].ef = this;
			break;
		}
	}

	for (var_ele = root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
		many++;

	gCFG->i5a = new I5A[many];
	for (var_ele = root->FirstChildElement(vn); var_ele; var_ele = var_ele->NextSiblingElement(vn) ) 
	{
		if ( gCFG->i5a[vmany].prepare(var_ele) )
				vmany++;
	}
	gCFG->i5A_num = vmany;
}

bool Ef06::facio( Amor::Pius *pius)
{
	PacketObj **tmp;
	int a_len;
	char req[256], res[256], sw[5];
	int sw;
	Ef06 *a_mu;

	assert(pius);
	switch(pius->ordo )
	{
	case Notitia::IGNITE_ALL_READY:
		WBUG("facio IGNITE_ALL_READY" );
		if (op_mutex == NULL)
		{
			op_mutex = CreateMutex( NULL, // default security attributes
					FALSE, // initially not owned
					NULL);
			if (op_mutex == NULL) 
			{
				WLOG(ERR, "CreateMutex error %d", GetLastError());
				break;
			}
		}
		if (pro_ev == NULL)
		{
			pro_ev = CreateEvent( NULL, // default security attributes
					FALSE, //not Manual Reset (Auto Reset)
					TRUE, // 开始无信号 
					NULL);
			if (pro_ev == NULL) 
			{
				WLOG(ERR, "CreateEvent error %d", GetLastError());
				break;
			}
		}

		/* 顶层实例, 母卡准备 */
#define PRO(C)  ret = Command(C,res, &sw); if ( ret !=0 || sw != 0x9000 ) {goto ISTOP;} else { ret=0;}	
		PRO("00A4000C023F00")
		PRO("00200001083132333435363738")
		PRO("00A4000002A001")
#undefine PRO
		/* 顶层实例的子线程, 等待信号 */
		if ( _beginthread((my_thread_func)pac_thrd, 0, this) == -1 )
			WLOG(ERR, "_beginthread error= %08x",  GetLastError());
	ISTOP:
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		a_mu = look_idle ();	 //这里是主线程
		if ( a_mu) 
		{
			a_mu->who_back = this;
			SetEvent(a_mu->pro_evt);	//触发信号,线程运行。
		} else {
		/* 没有可用的母卡, 怎么办? */
		}
		if ( has_config || gCFG->work_id != instance_id )  /* 如果工作ID不符合设定则不理 */
			break;
		break;

	case Notitia::START_SESSION:
		WBUG("facio START_SESSION");
		break;

	case Notitia::SET_UNIPAC:
		WBUG("facio SET_UNIPAC");
		if ( (tmp = (PacketObj **)(pius->indic)))
		{
			if ( *tmp) rcv_pac = *tmp; 
			else
				WLOG(WARNING, "facio SET_UNIPAC rcv_pac null");
			tmp++;
			if ( *tmp) snd_pac = *tmp;
			else
				WLOG(WARNING, "facio SET_UNIPAC snd_pac null");
		} else 
			WLOG(WARNING, "facio SET_UNIPAC null");
		
		break;

	default:
		return false;
	}
	return true;
}

void Ef06::handle_pac()
{
	unsigned char *p;
	unsigned long len;
	char req[256], res[256], sw[5];
	int ret;

WAIT_PRO:
	if (WaitForSingleObject(pro_ev, INFINITE) != WAIT_OBJECT_0 )
	{
		WLOG(ERR, "WaitForSingleObject(pro_ev) error= %08x",  GetLastError());
		return;
	}
	/* 数据都在who_back中 */
	who_back->snd_pac->reset();
	who_back->snd_pac->input(0, (unsigned char*)"99",2);
	p =who_back->rcv_pac->getfld(1, &len);	//1, 功能字
	if ( len == 2 && memcmp(p, "5A",2) == 0)	//5A指令, 密钥导出
	{
		WBUG("5A"); 
		p =rcv_pac->getfld(3, &len);	//全部的指令数据, 5A以后的
		memcpy(req, p, len); req[len] = 0;
		ret = Sam_Command(req , res);
		WBUG("REQ %s, ANS %s, sw %4X", req, res, ret);
		sprintf_s(sw, "%4X", (ret & 0x0000FFFF));
		if ( ret < 0  ) 
		{
			snd_pac->input(2, (unsigned char*)"A",1);
			snd_pac->input(3, (unsigned char*)"0", 0);
			WLOG(ERR, "%s", m_error_buf);
		} else {
			snd_pac->input(2, (unsigned char*)"0",1);
			snd_pac->input(3, (unsigned char*)res, strlen(res));
		}
		snd_pac->input(1, (unsigned char*)"CAUA",4);
		snd_pac->input(4, (unsigned char*)sw,4);
		goto BACK;
	}

BACK:
	who_back->aptus->sponte(&local_pius);
goto WAIT_PRO;
}

int Ef06::Command(const char *command, char* reply, int *psw)
{
	// TODO: 在此添加实现代码
	int ret;
	int which = 1;
	void *ps[6];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) 0;	//不指定slot
	ps[3] = command;
	ps[4] = reply;
	ps[5] = psw;
	para.indic = &ps[0];

	switch (which)
	{
	case 0:	//Sam
		para.ordo = Notitia::IC_SAM_COMMAND;
		break;
	case 1:	//Pro
		para.ordo = Notitia::IC_PRO_COMMAND;
		break;
	default:
		//para.ordo = Notitia::TEXTUS_RESERVED;
		break;
	}
	
	aptus->facio(&para);
	if ( ret == 0 && *psw != 0x9000 )
	{
		sprintf(m_error_buf, "%s(\"%s\")\n返回 %04x", which==0 ? "SamCommand":"ProCommand", command, *psw);
	}
	return ret;
}

bool Ef06::sponte( Amor::Pius *pius)
{
	return true;
}

Ef06::Ef06()
{
	local_pius.ordo = Notitia::PRO_UNIPAC;
	local_pius.indic = 0;
	who_back = (Ef06*) 0;
	op_mutex=NULL;		//判断who_back的操作互斥量, 
	pro_ev = NULL;		//event变量, 指示有工作要做
}

Ef06::~Ef06()
{
	if ( has_config && gCFG )
		delete gCFG;
}

Amor* Ef06::clone()
{
	Ef06 *child = new Ef06();
	child->instance_id = instance_id++; //顶实例的id在增加，但意义不同。
	child->gCFG =gCFG;
	return (Amor*)child;
}
#define AMOR_CLS_TYPE Ef06
#include "hook.c"
