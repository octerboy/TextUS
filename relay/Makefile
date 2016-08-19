# Title: Makefile
# ID: Textus-Relay-Makefile
# Build:
#	B01:created by octerboy 2006/04/01, Wuhan

include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include

AMOROBJS=Relay.o

all:relay.so

Relay.o:Relay.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Relay -o $@ $^

relay.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
