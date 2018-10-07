# Title: Makefile
# ID: Textus-Ponder-Makefile
# Build:
#	B01:created by octerboy 2007/02/23, Panyu

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../sched/include
AMOROBJS=Ponder.o

all:ponder.so

Ponder.o:Ponder.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Ponder -o $@ $^

ponder.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
