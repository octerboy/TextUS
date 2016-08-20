# Tile: Makefile
# ID: Textus-baselib-Makefile
# Build	
#	B01: created by octerboy 2005/06/10
include ../Rules.basemod
CFLAGS += -I./include -I../amor/include
AMOROBJS=TBuffer.o BTool.o 

all:libbase.so

BTool.o:BTool.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DDEBUG -o $@ $^

TBuffer.o:TBuffer.cpp
	${CC} ${CFLAGS} -O3 -o $@ $^

libbase.so:$(AMOROBJS)
	$(LD) $(ALONE_LDFLAGS) -o $@ $^ $(ALONE_LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
