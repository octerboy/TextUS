# Title: Makefile
# ID: Textus-Unifom-Makefile
# Build:
#	B01:created by octerboy 2006/10/13, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:unifom.so

Unifom.o: Unifom.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Unifom -o $@ Unifom.cpp
	
unifom.so: Unifom.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
