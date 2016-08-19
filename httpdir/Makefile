# Title: Makefile
# ID: Textus-dirlet-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../httpsrvhead/include -I../baselib/include -I../logdata/include
all:httpdir.so

DirLet.o:DirLet.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=DirLet -o $@ $^

httpdir.so: DirLet.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
