# Title: Makefile
# ID:Textus-Process-Makefile
# Build:
#	B01:created by octerboy 2005/09/4

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include
all:process.so

Process.o: Process.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Process -o $@ $^

process.so: Process.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
