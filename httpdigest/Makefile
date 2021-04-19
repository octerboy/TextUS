# Title:Makefile
# ID:Textus-httpdigest-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  +=  -I../baselib/include -I../httpsrvhead/include -I../logdata/include -I../unipac/include

all:httpdigest.so

Digest.o:Digest.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Digest -o $@ $^

Tools.o:tools.cpp
	${CC} ${CFLAGS} -o $@ $^

Digcalc.o:digcalc.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -o $@ $^

httpdigest.so: Digest.o Digcalc.o Tools.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
