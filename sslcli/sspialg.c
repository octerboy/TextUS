#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Cryptui.lib")
#include "casecmp.h"
#include <Cryptuiapi.h>

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

// Display a UI with the certificate info and also write it to the debug output
HRESULT ShowCertInfo(PCCERT_CONTEXT pCertContext, const char* Title)
{
	WCHAR pszNameString[256] {};
	void*            pvData;
	DWORD            cbData {};
	DWORD            dwPropId = 0;


	//  Display the certificate.
	if (!CryptUIDlgViewContext(
		CERT_STORE_CERTIFICATE_CONTEXT,
		pCertContext,
		nullptr,
		(LPCWSTR)Title,
		0,
		pszNameString // Dummy parameter just to avoid a warning
	))
	{
		printf("UI failed.\n");
	}

	if (CertGetNameString(
		pCertContext,
		CERT_NAME_SIMPLE_DISPLAY_TYPE,
		0,
		nullptr,
		(LPSTR)pszNameString,
		128))
	{
		printf("Certificate for %s\n", (char*)pszNameString);
	}
	else
		printf("CertGetName failed.\n");


	int Extensions = pCertContext->pCertInfo->cExtension;

	auto *p = pCertContext->pCertInfo->rgExtension;
	for (int i = 0; i < Extensions; i++)
	{
		printf("Extension %s\n", (p++)->pszObjId);
	}

	//-------------------------------------------------------------------
	// Loop to find all of the property identifiers for the specified  
	// certificate. The loop continues until 
	// CertEnumCertificateContextProperties returns zero.
	while (0 != (dwPropId = CertEnumCertificateContextProperties(
		pCertContext, // The context whose properties are to be listed.
		dwPropId)))    // Number of the last property found.  
		// This must be zero to find the first 
		// property identifier.
	{
		//-------------------------------------------------------------------
		// When the loop is executed, a property identifier has been found.
		// Print the property number.

		printf("Property # %d found->\n", dwPropId);

		//-------------------------------------------------------------------
		// Indicate the kind of property found.

		switch (dwPropId)
		{
		case CERT_FRIENDLY_NAME_PROP_ID:
		{
			printf("Friendly name: ");
			break;
		}
		case CERT_SIGNATURE_HASH_PROP_ID:
		{
			printf("Signature hash identifier ");
			break;
		}
		case CERT_KEY_PROV_HANDLE_PROP_ID:
		{
			printf("KEY PROVE HANDLE");
			break;
		}
		case CERT_KEY_PROV_INFO_PROP_ID:
		{
			printf("KEY PROV INFO PROP ID ");
			break;
		}
		case CERT_SHA1_HASH_PROP_ID:
		{
			printf("SHA1 HASH identifier");
			break;
		}
		case CERT_MD5_HASH_PROP_ID:
		{
			printf("md5 hash identifier ");
			break;
		}
		case CERT_KEY_CONTEXT_PROP_ID:
		{
			printf("KEY CONTEXT PROP identifier");
			break;
		}
		case CERT_KEY_SPEC_PROP_ID:
		{
			printf("KEY SPEC PROP identifier");
			break;
		}
		case CERT_ENHKEY_USAGE_PROP_ID:
		{
			printf("ENHKEY USAGE PROP identifier");
			break;
		}
		case CERT_NEXT_UPDATE_LOCATION_PROP_ID:
		{
			printf("NEXT UPDATE LOCATION PROP identifier");
			break;
		}
		case CERT_PVK_FILE_PROP_ID:
		{
			printf("PVK FILE PROP identifier ");
			break;
		}
		case CERT_DESCRIPTION_PROP_ID:
		{
			printf("DESCRIPTION PROP identifier ");
			break;
		}
		case CERT_ACCESS_STATE_PROP_ID:
		{
			printf("ACCESS STATE PROP identifier ");
			break;
		}
		case CERT_SMART_CARD_DATA_PROP_ID:
		{
			printf("SMART_CARD DATA PROP identifier ");
			break;
		}
		case CERT_EFS_PROP_ID:
		{
			printf("EFS PROP identifier ");
			break;
		}
		case CERT_FORTEZZA_DATA_PROP_ID:
		{
			printf("FORTEZZA DATA PROP identifier ");
			break;
		}
		case CERT_ARCHIVED_PROP_ID:
		{
			printf("ARCHIVED PROP identifier ");
			break;
		}
		case CERT_KEY_IDENTIFIER_PROP_ID:
		{
			printf("KEY IDENTIFIER PROP identifier ");
			break;
		}
		case CERT_AUTO_ENROLL_PROP_ID:
		{
			printf("AUTO ENROLL identifier. ");
			break;
		}
		case CERT_ISSUER_PUBLIC_KEY_MD5_HASH_PROP_ID:
		{
			printf("ISSUER PUBLIC KEY MD5 HASH identifier. ");
			break;
		}
		} // End switch.

		//-------------------------------------------------------------------
		// Retrieve information on the property by first getting the 
		// property size. 
		// For more information, see CertGetCertificateContextProperty.

		if (CertGetCertificateContextProperty(
			pCertContext,
			dwPropId,
			nullptr,
			&cbData))
		{
			//  Continue.
		}
		else
		{
			// If the first call to the function failed,
			// exit to an error routine.
			printf("Call #1 to CertGetCertificateContextProperty failed.");
			return E_FAIL;
		}
		//-------------------------------------------------------------------
		// The call succeeded. Use the size to allocate memory 
	}
	return S_OK;
}
