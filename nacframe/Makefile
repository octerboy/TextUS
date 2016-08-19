# Title: Makefile
# ID:Textus-NacFrame-Makefile
# Build
#	B01:created by octerboy 2007/10/24

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:nacframe.so

NacFrame.o: NacFrame.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=NacFrame -o $@ $^

nacframe.so: NacFrame.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
