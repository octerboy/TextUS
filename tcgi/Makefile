# Title: Makefile
# ID: Textus-Tcpcli-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../sched/include -I../baselib/include -I../unipac/include

all:tcgi.so

TCgi.o:TCgi.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TCgi -o $@ $^

tcgi.so: TCgi.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release
	
clean:
	rm *.o
	rm *.so
