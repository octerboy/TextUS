# Title: Makefile
# ID: Textus-Tcpsrv-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../sched/include -I../baselib/include -I../logdata/include
all:tcpsrv.so

Tcpsrv.o:Tcpsrv.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -o $@ $^

Tcpsrvuna.o:Tcpsrvuna.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Tcpsrvuna -o $@ $^

tcpsrv.so: Tcpsrv.o Tcpsrvuna.o
	${LD} $(LDFLAGS) -o $@ $^ $(LIBSOCKET) $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
