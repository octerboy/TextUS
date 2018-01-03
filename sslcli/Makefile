# Title: Makefile
# ID: Textus-sslcli-Makefile
# Build
#	B01:created by octerboy 2007/12/18

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:sslcli.so

SSLcli.o:SSLcli.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -o $@ $^

SSLcliuna.o:SSLcliuna.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=SSLcliuna -o $@ $^

sslcli.so: SSLcli.o SSLcliuna.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lcrypto -lssl -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
