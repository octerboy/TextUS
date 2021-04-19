#include <string.h>
#include "casecmp.h"
#include "digcalc.h"
#include "BTool.h"

void CvtHex(
    IN HASH Bin,
    OUT HASHHEX Hex
    )
{
    unsigned short i;
    unsigned char j;

    for (i = 0; i < HASHLEN; i++) {
        j = (Bin[i] >> 4) & 0xf;
        if (j <= 9)
            Hex[i*2] = (j + '0');
         else
            Hex[i*2] = (j + 'a' - 10);
        j = Bin[i] & 0xf;
        if (j <= 9)
            Hex[i*2+1] = (j + '0');
         else
            Hex[i*2+1] = (j + 'a' - 10);
    };
    Hex[HASHHEXLEN] = '\0';
};

/* calculate H(A1) as per spec */
void DigestCalcHA1(
    IN const char * pszAlg,
    IN const char * pszUserName,
    IN const char * pszRealm,
    IN const char * pszPassword,
    IN const char * pszNonce,
    IN const char * pszCNonce,
    OUT HASHHEX SessionKey
    )
{
      BTool::MD5_CTX Md5Ctx;
      HASH HA1;

      BTool::MD5Init(&Md5Ctx);
      BTool::MD5Update(&Md5Ctx, pszUserName, (unsigned int)strlen(pszUserName));
      BTool::MD5Update(&Md5Ctx, ":", 1);
      BTool::MD5Update(&Md5Ctx, pszRealm,  (unsigned int)strlen(pszRealm));
      BTool::MD5Update(&Md5Ctx, ":", 1);
      BTool::MD5Update(&Md5Ctx, pszPassword, (unsigned int)strlen(pszPassword));
      BTool::MD5Final(HA1, &Md5Ctx);
      if (strcasecmp(pszAlg, "md5-sess") == 0) {

            BTool::MD5Init(&Md5Ctx);
            BTool::MD5Update(&Md5Ctx, HA1, HASHLEN);
            BTool::MD5Update(&Md5Ctx, ":", 1);
            BTool::MD5Update(&Md5Ctx, pszNonce, (unsigned int)strlen(pszNonce));
            BTool::MD5Update(&Md5Ctx, ":", 1);
            BTool::MD5Update(&Md5Ctx, pszCNonce, (unsigned int)strlen(pszCNonce));
            BTool::MD5Final(HA1, &Md5Ctx);
      };
      CvtHex(HA1, SessionKey);
};

/* calculate request-digest/response-digest as per HTTP Digest spec */
void DigestCalcResponse(
    IN HASHHEX HA1,           /* H(A1) */
    IN char * pszNonce,       /* nonce from server */
    IN char * pszNonceCount,  /* 8 hex digits */
    IN char * pszCNonce,      /* client nonce */
    IN char * pszQop,         /* qop-value: "", "auth", "auth-int" */
    IN const char * pszMethod,      /* method from the request */
    IN const char * pszDigestUri,   /* requested URL */
    IN HASHHEX HEntity,       /* H(entity body) if qop="auth-int" */
    OUT HASHHEX Response      /* request-digest or response-digest */
    )
{
      BTool::MD5_CTX Md5Ctx;
      HASH HA2;
      HASH RespHash;
       HASHHEX HA2Hex;

      // calculate H(A2)
      BTool::MD5Init(&Md5Ctx);
      BTool::MD5Update(&Md5Ctx, pszMethod,  (unsigned int)strlen(pszMethod));
      BTool::MD5Update(&Md5Ctx, ":", 1);
      BTool::MD5Update(&Md5Ctx, pszDigestUri, (unsigned int)strlen(pszDigestUri));
      if (strcasecmp(pszQop, "auth-int") == 0) {
            BTool::MD5Update(&Md5Ctx, ":", 1);
            BTool::MD5Update(&Md5Ctx, HEntity, HASHHEXLEN);
      };
      BTool::MD5Final(HA2, &Md5Ctx);
       CvtHex(HA2, HA2Hex);

      // calculate response
      BTool::MD5Init(&Md5Ctx);
      BTool::MD5Update(&Md5Ctx, HA1, HASHHEXLEN);
      BTool::MD5Update(&Md5Ctx, ":", 1);
      BTool::MD5Update(&Md5Ctx, pszNonce, (unsigned int)strlen(pszNonce));
      BTool::MD5Update(&Md5Ctx, ":", 1);
      if (*pszQop) {

          BTool::MD5Update(&Md5Ctx, pszNonceCount, (unsigned int)strlen(pszNonceCount));
          BTool::MD5Update(&Md5Ctx, ":", 1);
          BTool::MD5Update(&Md5Ctx, pszCNonce, (unsigned int)strlen(pszCNonce));
          BTool::MD5Update(&Md5Ctx, ":", 1);
          BTool::MD5Update(&Md5Ctx, pszQop, (unsigned int)strlen(pszQop));
          BTool::MD5Update(&Md5Ctx, ":", 1);
      };
      BTool::MD5Update(&Md5Ctx, HA2Hex, HASHHEXLEN);
      BTool::MD5Final(RespHash, &Md5Ctx);
      CvtHex(RespHash, Response);
};
