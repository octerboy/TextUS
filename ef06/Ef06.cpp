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

	PacketObj *rcv_pac;	/* ������ڵ��PacketObj */
	PacketObj *snd_pac;

	Ef06();
	~Ef06();

	void ignite(TiXmlElement *cfg);	
	bool facio( Pius *);
	bool sponte( Pius *);
	Amor *clone();
	
	void handle_pac();	
	typedef struct _I5A {	 //��5Aָ��Ĵ�����
		const char *protect_key_index_06;	//06������Կ����
		const char *out_key_index_06;		//06������Կ����
		const char *protect_key_index_mu;	//ĸ��������Կ����, ������ܲ�����
		const char *div_ins_mu;		//ĸ����ɢ��Կ��ָ��ͷ, Lc�������
		const char *div_key_index_mu;		//ĸ����ɢ��Կ����, ��11 01 FE(ĸ����) 01 01 00(ESAM����Կͷ)
		const char *out_ins_mu;		//ĸ��������Կ��ָ��ͷ, Lc�������
	} I5A;

	typedef struct _I70 {	 //��70ָ��Ĵ�����
		const char *div_key_06;		//06��ɢ��Կ����
		const char *div_ins_mu;		//ĸ����ɢ��Կ��ָ��ͷ, Lc�������
		const char *div_key_mu;		//ĸ����ɢ��Կ����
		const char *out_ins_mu;		//ĸ�����MAC��ָ��ͷ,Lc�������
	} I70;

	struct G_CFG 	//ȫ�ֶ���
	{
		int work_id;
		int factory_id;		//ĸ�����̶���: 0:����, 1:�ݵ�, 2:����

		int i5A_num;
		I5A *i5a;
	
		int i70_num;
		I70 *i70;
	
		inline G_CFG() {
			factory_id = 1;		//ĸ������Ĭ��Ϊ�ݵ�
			work_id = 0;
			i5a = 0;
			i70 = 0;
			i5A_num = 0;
			i70_num = 0;
		};	
		inline ~G_CFG() {
		};
	};
	struct G_CFG *gCFG;     /* ȫ�ֹ������ */
	bool has_config;
	int instance_id;
	char m_error_buf[512];
	
	Ef06 *who_back;		//0:����������
	HANDLE op_mutex;	//�ж�who_back�Ĳ���������, 
	HANDLE pro_ev;		//event����, ָʾ�й���Ҫ��
	bool CanI()
	{
		bool ret;
		if (WaitForSingleObject(op_mutex, 4000) == WAIT_OBJECT_0 )
		{
			if ( who_back )		//�ѱ�ռ��
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
			who_back = (Ef06*) 0;	//����ռ��
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
	/* �ؽ���, ʵ�ʹ�����, ���Ƕ��������� */
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
					TRUE, // ��ʼ���ź� 
					NULL);
			if (pro_ev == NULL) 
			{
				WLOG(ERR, "CreateEvent error %d", GetLastError());
				break;
			}
		}

		/* ����ʵ��, ĸ��׼�� */
#define PRO(C)  ret = Command(C,res, &sw); if ( ret !=0 || sw != 0x9000 ) {goto ISTOP;} else { ret=0;}	
		PRO("00A4000C023F00")
		PRO("00200001083132333435363738")
		PRO("00A4000002A001")
#undefine PRO
		/* ����ʵ�������߳�, �ȴ��ź� */
		if ( _beginthread((my_thread_func)pac_thrd, 0, this) == -1 )
			WLOG(ERR, "_beginthread error= %08x",  GetLastError());
	ISTOP:
		break;

	case Notitia::CLONE_ALL_READY:
		WBUG("facio CLONE_ALL_READY" );
		break;

	case Notitia::PRO_UNIPAC:
		WBUG("facio PRO_UNIPAC");
		a_mu = look_idle ();	 //���������߳�
		if ( a_mu) 
		{
			a_mu->who_back = this;
			SetEvent(a_mu->pro_evt);	//�����ź�,�߳����С�
		} else {
		/* û�п��õ�ĸ��, ��ô��? */
		}
		if ( has_config || gCFG->work_id != instance_id )  /* �������ID�������趨���� */
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
	/* ���ݶ���who_back�� */
	who_back->snd_pac->reset();
	who_back->snd_pac->input(0, (unsigned char*)"99",2);
	p =who_back->rcv_pac->getfld(1, &len);	//1, ������
	if ( len == 2 && memcmp(p, "5A",2) == 0)	//5Aָ��, ��Կ����
	{
		WBUG("5A"); 
		p =rcv_pac->getfld(3, &len);	//ȫ����ָ������, 5A�Ժ��
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
	// TODO: �ڴ����ʵ�ִ���
	int ret;
	int which = 1;
	void *ps[6];
	Amor::Pius para;

	ret = -1;
	m_error_buf[0] = 0;
	ps[0] = &ret;
	ps[1] = &m_error_buf[0];
	ps[2] = (void*) 0;	//��ָ��slot
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
		sprintf(m_error_buf, "%s(\"%s\")\n���� %04x", which==0 ? "SamCommand":"ProCommand", command, *psw);
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
	op_mutex=NULL;		//�ж�who_back�Ĳ���������, 
	pro_ev = NULL;		//event����, ָʾ�й���Ҫ��
}

Ef06::~Ef06()
{
	if ( has_config && gCFG )
		delete gCFG;
}

Amor* Ef06::clone()
{
	Ef06 *child = new Ef06();
	child->instance_id = instance_id++; //��ʵ����id�����ӣ������岻ͬ��
	child->gCFG =gCFG;
	return (Amor*)child;
}
#define AMOR_CLS_TYPE Ef06
#include "hook.c"
