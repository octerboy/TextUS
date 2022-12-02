# Title :Makefile
# ID: Textus-journal-Makefile
# Build
#	B01:created by octerboy 2022/01/04, Guangzhou

include ../Rules.basemod
AMOROBJS=Jor.o

all:journal.so

Jor.o:Jor.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -I./include -I../baselib/include -DNDEBUG -DAMOR_CLS_TYPE=Jor -o $@ $^

journal.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
