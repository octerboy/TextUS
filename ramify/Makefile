# Title: Makefile
# ID:	Textus-ramify-Makefile
# Build:
#	B01:created by octerboy 2006/10/21 Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../unipac/include -I../baselib/include
AMOROBJS=Ramify.o

all:ramify.so

Ramify.o:Ramify.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Ramify -o $@ $^

ramify.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
