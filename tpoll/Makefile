# Title: Makefile
# ID: Textus-tpoll-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I./include -I../logdata/include -I../sched/include
all:tpoll.so tpoll_thrd.so

TPoll.o:TPoll.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TPoll -o $@ $^

TPollThd.o:TPoll.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TPoll -DMULTI_PTHREAD -o $@ $^

tpoll.so: TPoll.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	$(_POST_RPO_SUB)

tpoll_thrd.so: TPollThd.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lpthread
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
