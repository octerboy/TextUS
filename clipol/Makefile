# Title: Makefile
# ID: Textus-clipol-Makefile
# Build
#	B01:created by octerboy 2016/05/14, Panyu
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include -I../baselib/include

all:clipol.so

clipol.o:CliPol.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=CliPol -o $@ $^

clipol.so: clipol.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release
	
clean:
	rm *.o
	rm *.so
