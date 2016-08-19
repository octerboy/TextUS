# Title: Makefile
# ID:Textus-Slice-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:slice.so

Slice.o: Slice.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Slice -o $@ $^

slice.so: Slice.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release
	
clean:
	rm *.o
	rm *.so
