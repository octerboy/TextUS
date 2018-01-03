# Title: Makefile
# ID: Textus-ttyop-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../sched/include -I../baselib/include

all:ttyop.so

TTY.o:TTY.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TTY -o $@ $^

ttyop.so: TTY.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
