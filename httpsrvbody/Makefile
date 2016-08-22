# Title: Makefile for httpsrvbody
# ID: Textus-HttpSrvBody-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:httpsrvbody.so

HttpSrvBody.o:HttpSrvBody.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpSrvBody -o $@ $^ 
	
httpsrvbody.so: HttpSrvBody.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
