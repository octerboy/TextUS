# Title: Makefile
# ID: Textus-Version-Makefile
# Build
#	B01:created by octerboy 2006/04/01, Wuhan

include ../Rules.basemod
AMOROBJS=Version.o

all:tversion.so

Version.o:Version.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG  -DAMOR_CLS_TYPE=Version -o $@ $^

tversion.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
