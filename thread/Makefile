# Title: Makefile
# ID: Textus-Thread-Makefile
# Build:
#	B01:created by octerboy 2007/02/23, Panyu

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include
AMOROBJS=Thread.o

all:thread.so

Thread.o:Thread.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Thread -o $@ $^

thread.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
