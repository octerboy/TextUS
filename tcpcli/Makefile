# Title: Makefile
# ID: Textus-Tcpcli-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../sched/include -I../baselib/include

all:tcpcli.so

Tcpcli.o:Tcpcli.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -o $@ $^

Tcpcliuna.o:Tcpcliuna.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Tcpcliuna -o $@ $^

tcpcli.so: Tcpcli.o Tcpcliuna.o
	${LD} $(LDFLAGS) -o $@ $^ $(LIBSOCKET) $(LDLIBS) -lbase
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
