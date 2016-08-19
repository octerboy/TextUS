# Title: Makefile
# ID: Textus-Unipac-Makefile
# Build:
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR  +=-I./include -I../baselib/include -I../logdata/include

all:unipac.so

Unipac.o: Unipac.cpp cupsDef.h const_para.h iso8583-87.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Unipac -o $@ Unipac.cpp
	
unipac.so: Unipac.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
