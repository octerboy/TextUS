# Title: Makefile for dbport
# ID: Textus-DBPort-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  += -I./include -I../logdata/include -I../baselib/include -I../unipac/include

all:dbport.so

DBPort.o: DBPort.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=DBPort -o $@ $^ 
	
dbport.so: DBPort.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
