typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
typedef struct MD5Context {
    uint32_t buf[4];
    uint32_t bytes[2];
    uint32_t in[16];
} MD5_CTX;

void MD5Init(struct MD5Context *context);
void MD5Update(struct MD5Context *context, const char *buf, unsigned len);
void MD5Final(char digest[16], struct MD5Context *context);
void MD5Transform(uint32_t buf[4], uint32_t const in[16]);

#define MD5_DIGEST_CHARS         16
