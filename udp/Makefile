# Title: Makefile
# ID: Textus-Udp-Makefile
# Build
#	B01:created by octerboy 2006/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../sched/include -I../baselib/include -I../logdata/include
all:udp.so

Udp.o:Udp.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ $^

Udpuna.o:Udpuna.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Udpuna -o $@ $^

udp.so: Udp.o Udpuna.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBSOCKET) $(LDLIBS)
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
