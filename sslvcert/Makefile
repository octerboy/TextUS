# Title: Makefile
# ID: Textus-sslvcert-Makefile
# Build
#	B01:created by octerboy 2006/07/26, ����

include ../Rules.basemod
INCLUDE_DIR  += -I../logdata/include
all:sslvcert.so

VCert.o:VCert.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=VCert -o $@ $^

sslvcert.so: VCert.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase  -lcrypto -lssl
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
