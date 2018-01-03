# Title:Makefile
# ID: Textus-Pipe-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:pipe.so

Pipe.o:Pipe.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Pipe -o $@ $^

pipe.so:Pipe.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
