# Title: Makefile
# ID: Textus-Signal-Makefile
# Build:
#	B01:created by octerboy 2006/12/26

include ../Rules.basemod
all:signal.so

Signal.o:Signal.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Signal -o $@ $^

signal.so:Signal.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release
	
clean:
	rm *.o
	rm *.so
