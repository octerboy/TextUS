# Title: Makefile
# ID: Textus-pmuto-Makefile
# Build:
#	B01:created by octerboy 2006/10/19, Guangzhou
include ../Rules.basemod
AMOROBJS=Muto.o

all:pmuto.so

Muto.o:Muto.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Muto -o $@ $^

pmuto.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
