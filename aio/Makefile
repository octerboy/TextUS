# Title: Makefile
# ID: Textus-aio-Makefile
# Build
#	B01:created by octerboy 2019/02/13
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../tpoll/include -I../baselib/include -I../unipac/include

all:aio.so aiotest.so

aio.o:Aio.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Aio -o $@ $^

aiotest.o:Aio.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Aio -DAIO_WRITE_TEST -o $@ $^

aio.so: aio.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)  -lbase
	$(_POST_RPO_SUB)
	
aiotest.so: aiotest.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)  -lbase
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
