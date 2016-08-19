# Title: Makefile
# ID:Textus-ForkPro-Makefile
# Build:
#	B01:created by octerboy 2005/09/4

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:forkpro.so

ForkPro.o:ForkPro.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=ForkPro -o $@ $^

forkpro.so: ForkPro.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
