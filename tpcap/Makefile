# Title: Makefile
# ID: Textus-tpcap-Makefile
# Build:
#	B01:created by octerboy 2007/07/29, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../sched/include -I../tpoll/include

all:tpcap.so

TPCap.o: TPCap.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TPCap -o $@ TPCap.cpp
	
tpcap.so: TPCap.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
