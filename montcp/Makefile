# Title: Makefile
# ID: Textus-montcp-Makefile
# Build:
#	B01:created by octerboy 2007/08/06, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:montcp.so

NetTcp.o: NetTcp.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=NetTcp -o $@ NetTcp.cpp
	
montcp.so: NetTcp.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
