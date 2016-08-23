# Title: Makefile
# ID: Textus-Keep-Makefile
# Build:
#	B01:created by octerboy 2005/06/10, Guangzhou
include ../Rules.basemod
AMOROBJS=Keep.o

all:keep.so

Keep.o:Keep.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Keep -o $@ $^

keep.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
