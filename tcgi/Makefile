# Title: Makefile
# ID: Textus-Tcpcli-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../sched/include  -I../baselib/include -I../unipac/include  -I../tpoll/include

all:tcgi.so

TCgi.o:TCgi.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TCgi -o $@ $^

tcgi.so: TCgi.o
	${LD} $(LDFLAGS) -o $@ $^ $(LIBSOCKET) $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
