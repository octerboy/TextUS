#include <stdio.h>
#include <string.h>
#include <time.h>
#include "textus_string.h"
#include "pinkey.h"
#include "tinyxml.h"
#include "textus_load_mod.h"
#define LINE_MAXLEN 1024
/* 返回值为0: 表示OK 
	   1: 过期
	   2: 校验错
	   3: 内容错误
	   4: 文件错误
 */
#define DEBUG

static char ld_lib_path[2048] = {0};
void env_sub(const char *path_source, char *path_dest)
{
	char *ps, *q,*qw, *qr;
	const char *right;
	char env_n[128], pt[2048];
	char *env;
	ps = (char*)path_source;
	*pt = 0;
	if ( path_dest ) *path_dest = 0;
	if ( !path_source ) return;
	while ( *ps )
	{
		q = 0;
		right= 0;
		env = 0;
		q = strstr(ps, "$(");
		qw = strstr(ps, "${");
		if ( q && qw  )
		{
			if ( q > qw )
				q = qw;
		} else {	//����һ��Ϊ��
			if ( !q )	//�����ĸ�Ϊnull, ���һ�� 
				q = qw;
		}
		if (!q)
		{	//û��$( ${��ͷ, ��������
			TEXTUS_STRCAT(pt, ps);
			break;
		} else {	//��
			TEXTUS_STRNCAT(pt, ps, q-ps);
			q++;
			if ( *q == '(' )
				right =")";
			else
				right= "}";

			q++;	//q��������
			qr = strpbrk(q, right);
			if ( qr && ((size_t) (qr - q+1)) < sizeof(env_n) )
			{
				memcpy(env_n, q, qr-q);
				env_n [qr-q] = 0;
		#if defined(_MSC_VER) && (_MSC_VER >= 1400 )
				char *env_o;
				size_t env_l;
				if (!_dupenv_s(&env_o, &env_l, env_n))
				{
					TEXTUS_STRCAT(pt, env_o);
					free(env_o);
				}
		#else
				env = getenv(env_n);
				if (env )
					TEXTUS_STRCAT(pt, env);
		#endif
				ps = (qr+1);	//�������޻�������, ��������
			} else {
				//û��������
				ps = q;
			}
		}
	}
	memcpy(path_dest, pt, strlen(pt));
	path_dest[strlen(pt)] = 0;
}
char* fileIsOK (char *filename, char *matter, int row, int keyIndex) 
{
  static char ctlstr[33];
  char *p;
  int i,t,check,encrypt;
  char aline[LINE_MAXLEN];
  char offset[16];
  time_t mtime, expired_time;
  unsigned char block[8],lrcblock[8];
  int lrc;

  unsigned char runtimes = 0;
	
  int pinkeyA[64];
  int Chiper_text[64],Clear_text[64];


  if ( strchr("5678",matter[0]) ) 
	runtimes = 1;

  for ( i = 0 ; i < 64 ; i ++ ) 
	pinkeyA[i] = pinkeys[keyIndex][i];

  if ( strlen( matter) <10 ) 
	return (char*)NULL;


  for ( i = 0 ; i < ( runtimes ? 13 : 10) ; i ++ )
		offset[i] = matter[i]; 

  srand( time(&mtime)%604800);
  for ( i =  ( runtimes ? 13 : 10); i < 15 ; i++ )
	offset[i]= ObtainX((rand()%16));

  lrc = 0;
  for( i = 0 ; i < 15 ; i ++ ) {
	lrc ^= (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
  }

  offset[15] = ObtainX(lrc);

    /* 将明文的8个字节作为校验块的初始块 */
  for ( i = 0 ; i < 8 ; i ++ ) {
        lrcblock [i] =
          16*(offset[2*i] > '9' ? offset[2*i]-'A'+10: offset[2*i] -'0')
           + (offset[2*i+1] > '9' ? offset[2*i+1]-'A'+10: offset[2*i+1] -'0');
  }
  for ( i=0; i<16; i++ )
   {
                t = (offset[i] > '9' ? offset[i]-'A'+10: offset[i] -'0');
		Clear_text[i*4] = t/8;
                t %= 8; Clear_text[i*4+1] = t/4;
                t %= 4; Clear_text[i*4+2] = t/2;
                t %= 2; Clear_text[i*4+3] = t;
    }
  /* 加密 */
  xdes(Clear_text, pinkeyA, Chiper_text, 1, 0);
  /* 先得到前面加密部分 */
  for ( i=0; i<16; i++ )
   {
            t = Chiper_text[i*4]*8 + Chiper_text[i*4+1]*4 +
                Chiper_text[i*4+2]*2 + Chiper_text[i*4+3];
            ctlstr[i] = ObtainX(t);
   }
   ctlstr[16] = '\0';
   
   if ( strchr("2468",matter[0]) ) 
	{
   /* 需要全文校验 */ 
	sumfile (filename, row, keyIndex, lrcblock,1) ;
   }  else return ctlstr;
#ifdef DEBUG
   printf ("\nLRCBLOCK:\n");
   for ( i = 0 ; i < 8 ; i ++ ) {
	printf ( "%02X",lrcblock[i]);
   }
   printf ("\n");
#endif
  /* 得到后面加密部分 */
  for ( i=0; i < 8; i++ )
  {
            ctlstr[i*2+16] = ObtainX((lrcblock[i] & 0xF0) >> 4  );
            ctlstr[i*2+1+16] = ObtainX((lrcblock[i] & 0x0F));
  }
  ctlstr[32] = '\0';
  return ctlstr;
}
char* r_share(const char *so_file)
{
	static char r_file[1024];
	int l = 0, n = 0;
	memset(r_file, 0, sizeof(r_file));
	if ( ld_lib_path)
	{
		l = strlen(ld_lib_path);
		if (l > 512 ) l = 512;
	 	memcpy(r_file, ld_lib_path, l);
		n = l;
	}
	l = strlen(so_file);
	if (l > 512 ) l = 512;
	memcpy(&r_file[n], so_file, l);
	TEXTUS_STRCAT(r_file, TEXTUS_MOD_SUFFIX);
	return r_file;
}

void sumso (const char *tag, TiXmlElement *amod )
{
  TiXmlElement *mod;
  const char *so;
  unsigned char block[8];
  int i;
	
  memcpy(block,  sum_file_vector,8);

  for ( mod = amod->FirstChildElement(tag);
	mod;
	mod = mod->NextSiblingElement(tag))
  {
	const char *sum;
	char *nsm;
	so = mod->Attribute("name");	

	sumso(tag, mod);
	if ( (sum = mod->Attribute("sum")) )
	{
		if (strcmp(sum, "no") == 0)
		{
			continue;
		}
	}
	sum = sumfile(r_share(so), 0, 1, block, 0);
	nsm = (char*)sum;
	nsm[4] = '\0';
	mod->SetAttribute("sum", sum);
  }
}

void sumclear (const char *tag, TiXmlElement *amod )
{
  TiXmlElement *mod;
  const char *so;
  unsigned char block[8];
  int i;
	
  memcpy(block,  sum_file_vector,8);

  for ( mod = amod->FirstChildElement(tag);
	mod;
	mod = mod->NextSiblingElement(tag))
  {
	const char *sum;
	char *nsm;
	so = mod->Attribute("name");	

	sumclear(tag, mod);
	if ( (sum = mod->Attribute("sum")) )
	{
		if (strcmp(sum, "no") == 0)
		{
			continue;
		}
	}
	mod->SetAttribute("sum", "no");
  }
}

int main (int argc, char *argv[] )
{
  TiXmlDocument doc;
  TiXmlElement *root;
  int row;
  time_t mtime;
  const char *tag;

  char matter[17];
  int runtimes = 10;
  char *sum;


  matter[16] = '\0';
  memset ( matter,'0',16);
  if ( (argc < 3 || argc > 5) || (argc == 3 && argv[2][0] != '9' && argv[2][0] != 'a') ) {
	printf ( "usage: %s filename method days ?runtimes?\n",argv[0]);
	printf ( "method '1':   受时间限制, 无次数限制, 不作全文校验\n");
	printf ( "method '2':   受时间限制, 无次数限制,   作全文校验\n");
	printf ( "method '3': 不受时间限制, 无次数限制, 不作全文校验\n");
	printf ( "method '4': 不受时间限制, 无次数限制,   作全文校验\n");
	printf ( "method '5':   受时间限制,   次数限制, 不作全文校验\n");

	printf ( "method '6':   受时间限制,   次数限制,   作全文校验\n");
	printf ( "method '7': 不受时间限制,   次数限制, 不作全文校验\n");
	printf ( "method '8': 不受时间限制,   次数限制,   作全文校验\n");
	printf ( "method '9': 对各模块签名\n");
	printf ( "method 'a': 取消各模块签名\n");
	exit(0);
  }

  if ( !doc.LoadFile (argv[1]) || doc.Error())
{
	printf ("Loading config %s document failed in row %d and column %d: %s\n", argv[1],doc.ErrorRow(), doc.ErrorCol(), doc.ErrorDesc());
	exit(1);
}

  root = doc.RootElement();
  row = root->Row();
  tag = root->Attribute("tag");
	env_sub(root->Attribute("path"), ld_lib_path);
  if ( !tag ) 
  {
	tag = new char [16];
	strcpy((char*) tag, "tag");
  }

  if ( argv[2][0] == '9' ) 
  {
 	sumso (tag, root);
	sumso ("Attachment", root);
	doc.SaveFile();
	goto End;
  }
	
  if ( argv[2][0] == 'a') 
  {
 	sumclear (tag, root);
	sumclear ("Attachment", root);
	doc.SaveFile();
	goto End;
  }
	
  printf("row %d\n", row);

  matter[0] = matter[1] = argv[2][0];
  sprintf(&matter[2],"%08X",(unsigned int)(atol(argv[3])*3600*24 + time(&mtime)));

  if ( argc == 5) {
	runtimes = atol(argv[4]);
	if ( runtimes < 10 ) runtimes = 10;
  }

  sprintf(&matter[10],"%03X",runtimes);
  printf ( "matter is %s\n",matter);
  sum = fileIsOK ( argv[1],matter, row, 0);
  if ( sum ) 
  {
	root->SetAttribute("sum", sum);
	if ( !  doc.SaveFile() )
	{
		printf ("Save config %s document failed: %s\n", argv[1], doc.ErrorDesc());
		exit(1);
	}
  	printf ( "%s\n", sum);
  }
End:
  return 0;
 }



