# Title: Makefile
# ID:Textus-Obu-Makefile
# Build:
#	B01:created by octerboy 2011/06/6

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include -I../unipac/include
all:obu.so

Obu.o:Obu.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Obu -o $@ $^

obu.so: Obu.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
