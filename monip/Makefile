# Title: Makefile
# ID: Textus-monip-Makefile
# Build:
#	B01:created by octerboy 2007/08/02, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:monip.so

NetIP.o: NetIP.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=NetIP -o $@ NetIP.cpp
	
monip.so: NetIP.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
