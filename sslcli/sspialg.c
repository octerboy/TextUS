#pragma comment(lib,"Crypt32.lib")
#include "casecmp.h"

static int alg_id(char *name)
{
#define GetAlgID(X)                         \
  if(strcasecmp(name, #X) == 0)                      \
	return CALG_##X

	GetAlgID(3DES);
	GetAlgID(3DES_112);
	GetAlgID(AES);
	GetAlgID(AES_128);
	GetAlgID(AES_192);
	GetAlgID(AES_256);
	GetAlgID(AGREEDKEY_ANY);
	GetAlgID(CYLINK_MEK);

	GetAlgID(DES);
	GetAlgID(DESX);
	GetAlgID(DH_EPHEM);
	GetAlgID(DH_SF);
	GetAlgID(DSS_SIGN);

	GetAlgID(ECDH);
	GetAlgID(ECDH_EPHEM);
	GetAlgID(ECDSA);
	GetAlgID(ECMQV);

	GetAlgID(HASH_REPLACE_OWF);
	GetAlgID(HUGHES_MD5);
	GetAlgID(HMAC);

	GetAlgID(KEA_KEYX);
	GetAlgID(MAC);
	GetAlgID(MD2);
	GetAlgID(MD4);
	GetAlgID(MD5);

	GetAlgID(NO_SIGN);
	GetAlgID(OID_INFO_CNG_ONLY);
	GetAlgID(OID_INFO_PARAMETERS);
	GetAlgID(PCT1_MASTER);

	GetAlgID(RC2);
	GetAlgID(RC4);
	GetAlgID(RC5);

	GetAlgID(RSA_KEYX);
	GetAlgID(RSA_SIGN);

	GetAlgID(SCHANNEL_ENC_KEY);
	GetAlgID(SCHANNEL_MAC_KEY);
	GetAlgID(SCHANNEL_MASTER_HASH);

	GetAlgID(SEAL);
	GetAlgID(SHA);
	GetAlgID(SHA1);

	GetAlgID(SHA_256);
	GetAlgID(SHA_384);
	GetAlgID(SHA_512);

	GetAlgID(SKIPJACK);
	GetAlgID(SSL2_MASTER);
	GetAlgID(SSL3_MASTER);
	GetAlgID(SSL3_SHAMD5);

	GetAlgID(TEK);
	GetAlgID(TLS1_MASTER);
	GetAlgID(TLS1PRF);

	return 0;
}

void set_proto(SCHANNEL_CRED *scred, char *str)
{
	char pro_str[512], *p, *q, *t;
	int prot;
	TEXTUS_STRCPY(pro_str,  str);
	p = &pro_str[strlen(pro_str)];
	*p++ = ' ';
	*p = '\0';	//add a space
	p = &pro_str[0];
	while ( 1) {
		q = strpbrk( p, " |\t" );
		if ( q == (char*) 0 ) break;
		*q++ = '\0';
		for ( t =p ; t < q; t++) {
			if ( *t == '.' ) *t = '_';
		}
		//printf("proto %s\n", p);
      		prot = proto_id(p);
		if ( prot ) {
			scred->grbitEnabledProtocols |= prot;
		}
		p = ( q += strspn( q, " |\t" ));
	}
}

void set_algs(SCHANNEL_CRED *scred, char *str,  ALG_ID *all_algs)
{
	char alg_str[512], *p, *q;
	int alg, count;
	TEXTUS_STRCPY(alg_str,  str);
	p = &alg_str[strlen(alg_str)];
	*p++ = ' ';
	*p = '\0';	//add a space
	count = 0 ; p = &alg_str[0];
	while ( 1) {
		q = strpbrk( p, " |\t" );
		if ( q == (char*) 0 ) break;
		*q++ = '\0';
      		alg = alg_id(p);
		if ( alg ) {
      			all_algs[count++] = alg;
		}
		p = ( q += strspn( q, " |\t" ));
	}
	scred->palgSupportedAlgs = all_algs;
	scred->cSupportedAlgs = count;
}
