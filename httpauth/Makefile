# Title:Makefile
# ID:Textus-httpauth-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  +=  -I../baselib/include -I../httpsrvhead/include -I../logdata/include

all:httpauth.so

HttpAuth.o:HttpAuth.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=HttpAuth -o $@ $^

httpauth.so: HttpAuth.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
