# Title: HttpAgent Makefile
# ID: Textus-tbufchan-Makefile
# Build:
#	B01:created by octerboy 2007/06/12, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:grate.so

Grate.o: Grate.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNNDEBUG -DAMOR_CLS_TYPE=Grate -o $@ $^ 
	
grate.so: Grate.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
