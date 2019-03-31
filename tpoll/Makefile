# Title: Makefile
# ID: Textus-tpoll-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I./include -I../logdata/include -I../sched/include
all:tpoll.so

TPoll.o:TPoll.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TPoll -o $@ $^

tpoll.so: TPoll.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
