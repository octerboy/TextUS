# Title: HttpAgent Makefile
# ID: Textus-tbufchan-Makefile
# Build:
#	B01:created by octerboy 2007/06/12, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:tbufchan.so

TBufChan.o: TBufChan.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNNDEBUG -DAMOR_CLS_TYPE=TBufChan -o $@ $^ 
	
tbufchan.so: TBufChan.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
