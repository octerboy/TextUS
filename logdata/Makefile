# Title :Makefile
# ID: Textus-Logdata-Makefile
# Build
#	B01:created by octerboy 2006/04/01, ”⁄Œ‰∫∫

include ../Rules.basemod
AMOROBJS=Logdata.o md5.o

all:logdata.so

Logdata.o:Logdata.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Logdata -o $@ $^

md5.o:md5.c
	${CC} ${CFLAGS} -o $@ $^

logdata.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
