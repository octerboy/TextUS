# Title: Makefile
# ID: Textus-sched-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I./include -I../logdata/include
all:sched.so

Sched.o:Sched.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Sched -o $@ $^

sched.so: Sched.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
