# Title :Makefile
# ID: Textus-Logdata-Makefile
# Build
#	B01:created by octerboy 2006/04/01, ”⁄Œ‰∫∫

include ../Rules.basemod
AMOROBJS=Logdata.o md5.o

all:logdata.so

Logdata.o:Logdata.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -I../baselib/include -DNDEBUG -DAMOR_CLS_TYPE=Logdata -o $@ $^

logdata.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
