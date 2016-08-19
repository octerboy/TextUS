# Title:Makefile
# ID:Textus-httpdigest-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  +=  -I../baselib/include -I../httpsrvhead/include -I../logdata/include -I../unipac/include

all:httpdigest.so

Digest.o:Digest.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Digest -o $@ $^

md5.o:md5.c
	${CC} ${CFLAGS} -o $@ $^

tools.o:tools.c
	${CC} ${CFLAGS} -o $@ $^

digcalc.o:digcalc.c
	${CC} ${CFLAGS} $(INCLUDE_DIR) -o $@ $^

httpdigest.so: Digest.o digcalc.o tools.o md5.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
