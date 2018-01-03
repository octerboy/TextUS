# Title: Makefile
# ID: Textus-Nostra-Makefile
# Build
#	B01:created by octerboy 2006/04/24, ÓÚ¹ãÖÝ

include ../Rules.basemod
AMOROBJS=Nostra.o

all:nostra.so

Nostra.o:Nostra.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Nostra -o $@ $^

nostra.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
