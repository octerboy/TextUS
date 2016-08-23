# Title: Makefile for monway
# ID: Textus-MonWay-Makefile
# Build:
#	B01:created by octerboy 2016/04/22, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:monway.so

md5.o:md5.c
	${CC} ${CFLAGS} -o $@ $^ 

monway.o:MonWay.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=MonWay -o $@ $^ 

monway.so: monway.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
