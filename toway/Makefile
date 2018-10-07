# Title: Makefile for toway
# ID: Textus-ToWay-Makefile
# Build:
#	B01:created by octerboy 2016/04/09, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:toway.so

toway.o:ToWay.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=ToWay -o $@ $^ 
	
toway.so: toway.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
