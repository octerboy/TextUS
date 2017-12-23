# ID: Textus-induco-Makefile
# Title: Makefile
# Build:B01:created by octerboy 2006/08/04, Guangzhou

include ../Rules.basemod
AMOROBJS=induco.o

all:induco.so

induco.o:Induco.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Induco -o $@ $^

induco.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
