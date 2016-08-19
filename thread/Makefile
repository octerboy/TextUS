# Title: Makefile
# ID: Textus-Thread-Makefile
# Build:
#	B01:created by octerboy 2007/02/23, Panyu

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include
AMOROBJS=Thread.o

all:thread.so

Thread.o:Thread.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNNDEBUG -DAMOR_CLS_TYPE=Thread -o $@ $^

thread.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
