# Titile: Makefile
# ID: Textus-Sequence-Makefile
# Build: 
#	B01: created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -O3 -I../logdata/include -I../baselib/include
all:sequence.so

Seque.o: Seque.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Seque -o $@ $^

sequence.so: Seque.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
