#include <stdio.h>
#include "digcalc.h"
#define xisspace(x) isspace((unsigned char)(x))
int strListGetItem(const char * str, char del, const char **item, int *ilen, const char **pos);

int main(int argc, char ** argv) {

/*
      char * pszNonce = "DCD98b7102dd2f0e8b11d0f600bfb0c093";
      char * pszCNonce = "0a4f113b";
      char * pszUser = "Mufasa";
      char * pszRealm = "testrealm@host.com";
      char * pszPass = "Circle Of Life";
      char * pszAlg = "md5";
      char szNonceCount[9] = "00000001";
      char * pszMethod = "GET";
      char * pszQop = "auth";
      char * pszURI = "/dir/index.html";
*/
      char * pszNonce = "84E0a095cfd25153b2e4014ea87a0980";
      char * pszCNonce = "a1d03d49d7f90e399f403be0ef94be7c";
      char * pszUser = "you";
      char * pszRealm = "Control ,\"Panel";
      char * pszPass = "1234";
      char * pszAlg = "md5";
      char szNonceCount[9] = "00000001";
      char * pszMethod = "GET";
      char * pszQop = "auth";
      char * pszURI = "/";
      HASHHEX HA1;
      HASHHEX HA2 = "";
      HASHHEX Response;
	
const	char *temp ="username=\"you\",realm=\"Control ,Panel\",nonce=\"84E0a095cfd25153b2e4014ea87a0980\",uri=\"/\",cnonce=\"a1d03d49d7f90e399f403be0ef94be7c\",nc=00000001,response=\"afc98d4460bf3ab27a19b138423dbb18\",algorithm=MD5, qop=\"auth\"";
	const char *item;
 const char *p;
 const char *pos = 0;
	int ilen;
	
	#define VAL_MAX 128
	struct DigestRequest {
		char username[VAL_MAX];
		char realm[VAL_MAX];
		char qop[VAL_MAX];
		char algorithm[VAL_MAX];
		char uri[VAL_MAX];
		char nonce[VAL_MAX];
		char nc[VAL_MAX];
		char cnonce[VAL_MAX];
		char response[VAL_MAX];
	};

	struct DigestRequest _request; 
	struct DigestRequest *digest_request;
	digest_request = &_request;
	
	memset(digest_request, 0, sizeof(struct DigestRequest));
	while (strListGetItem(temp, ',', &item, &ilen, &pos)) 
	{
	if ((p = strchr(item, '=')) && (p - item < ilen))
	    ilen = p++ - item;
	printf("%s\n", item);
	if (!strncmp(item, "username", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	    //digest_request->username = xstrndup(p, strchr(p, '"') + 1 - p);
		memcpy(digest_request->username, p, strchr(p, '"')  - p);
	} else if (!strncmp(item, "realm", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	   // digest_request->realm = xstrndup(p, strchr(p, '"') + 1 - p);
	   // debug(29, 9) ("authDigestDecodeAuth: Found realm '%s'\n", digest_request->realm);
		memcpy(digest_request->realm, p, strchr(p, '"')  - p);
	} else if (!strncmp(item, "qop", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    if (*p == '\"')
		/* quote mark */
		p++;
	    //digest_request->qop = xstrndup(p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") + 1);
	    //debug(29, 9) ("authDigestDecodeAuth: Found qop '%s'\n", digest_request->qop);
		memcpy(digest_request->qop, p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") );
	} else if (!strncmp(item, "algorithm", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    if (*p == '\"')
		/* quote mark */
		p++;
	   // digest_request->algorithm = xstrndup(p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") + 1);
	   // debug(29, 9) ("authDigestDecodeAuth: Found algorithm '%s'\n", digest_request->algorithm);
		memcpy(digest_request->algorithm, p, strcspn(p, "\" \t\r\n()<>@,;:\\/[]?={}") );
	} else if (!strncmp(item, "uri", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	    //digest_request->uri = xstrndup(p, strchr(p, '"') + 1 - p);
	    //debug(29, 9) ("authDigestDecodeAuth: Found uri '%s'\n", digest_request->uri);
		memcpy(digest_request->uri, p, strchr(p, '"') - p);
	} else if (!strncmp(item, "nonce", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	    //digest_request->nonceb64 = xstrndup(p, strchr(p, '"') + 1 - p);
	    //debug(29, 9) ("authDigestDecodeAuth: Found nonce '%s'\n", digest_request->nonceb64);
		memcpy(digest_request->nonce, p, strchr(p, '"')  - p);
	} else if (!strncmp(item, "nc", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    //xstrncpy(digest_request->nc, p, 9);
	    //debug(29, 9) ("authDigestDecodeAuth: Found noncecount '%s'\n", digest_request->nc);
		memcpy(digest_request->nc, p, 8);

	} else if (!strncmp(item, "cnonce", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	    //digest_request->cnonce = xstrndup(p, strchr(p, '"') + 1 - p);
	    //debug(29, 9) ("authDigestDecodeAuth: Found cnonce '%s'\n", digest_request->cnonce);
		memcpy(digest_request->cnonce, p, strchr(p, '"')  - p);

	} else if (!strncmp(item, "response", ilen)) {
	    /* white space */
	    while (xisspace(*p))
		p++;
	    /* quote mark */
	    p++;
	    //digest_request->response = xstrndup(p, strchr(p, '"') + 1 - p);
	    //debug(29, 9) ("authDigestDecodeAuth: Found response '%s'\n", digest_request->response);
		memcpy(digest_request->response, p, strchr(p, '"') - p);
	}
	}
	p = 0; *p = 0;	

      DigestCalcHA1(pszAlg, pszUser, pszRealm, pszPass, pszNonce,
pszCNonce, HA1);
      DigestCalcResponse(HA1, pszNonce, szNonceCount, pszCNonce, pszQop,
       pszMethod, pszURI, HA2, Response);
      printf("Response = %s\n", Response);
	
	return 0;
};
