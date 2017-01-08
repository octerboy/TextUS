# Title: Makefile
# ID:Textus-httpurlsw-Makefile
# Build
#	B01:created by octerboy 2006/07/15
include ../Rules.basemod
INCLUDE_DIR  += -I../httpsrvhead/include -I../baselib/include -I../logdata/include
AMOROBJS=Urlsw.o

all:httpurlsw.so

Urlsw.o:Urlsw.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Urlsw -o $@ $^

httpurlsw.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
