# Title: HttpAgent Makefile
# ID: Textus-tbufchan-Makefile
# Build:
#	B01:created by octerboy 2007/06/12, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:grate.so

Grate.o: Grate.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Grate -o $@ $^ 
	
grate.so: Grate.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
