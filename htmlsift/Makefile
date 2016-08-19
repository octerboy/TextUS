# Title: Makefile for htmlsift
# ID: Textus-Sift-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:htmlsift.so

Sift.o: Sift.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Sift -o $@ $^ 
	
htmlsift.so: Sift.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
