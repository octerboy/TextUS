# Title :Makefile
# ID: Textus-Air-Makefile
# Build:
#	B01:created by octerboy 2006/05/10
include ../Rules.basemod
AMOROBJS=Air.o

all:airline.so

Air.o:Air.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG  -DAMOR_CLS_TYPE=Air -o $@ $^

airline.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
