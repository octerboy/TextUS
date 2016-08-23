# Title: Makefile
# ID: Textus-tix2pac-Makefile
# Build:
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../unipac/include -I../baselib/include
all:tix2pac.so

X2pac.o:X2pac.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=X2pac -o $@ $^

tix2pac.so:X2pac.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
