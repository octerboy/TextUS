# Title: Makefile
# ID: Textus-sslsrv-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:sslsrv.so

SSLsrv.o:SSLsrv.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -o $@ $^

SSLsrvuna.o:SSLsrvuna.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=SSLsrvuna -o $@ $^

sslsrv.so: SSLsrv.o SSLsrvuna.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase -lcrypto -lssl
	cp $@ ../release

clean:
	rm *.o
	rm *.so
