# Title: Makefile
# ID:Textus-Dummy-Makefile
# Build:
#	B01:created by octerboy 2005/09/4

include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include
all:dummy.so

Dummy.o:Dummy.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=Dummy -o $@ $^

dummy.so: Dummy.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
