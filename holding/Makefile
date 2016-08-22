# Title: Makefile
# ID: Textus-Holding-Makefile
# Build:
#	B01:created by octerboy 2006/11/1, Hebi(Henan)
include ../Rules.basemod
INCLUDE_DIR  += -I./include -I../tinyxml -I../amor/include -I../logdata/include -I../baselib/include -I../httpcookie/include

all:holding.so

Hold.o: Hold.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Hold -o $@ Hold.cpp
	
holding.so: Hold.o
	${LD} $(LDFLAGS)  -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
