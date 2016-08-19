# Title: Ajpfwd Makefile
# ID: Textus-Ajpfwd-Makefile
# Build:
#	B01:created by octerboy 2016/03/09, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include -I../unipac/include

all:ajpfwd.so

Ajpfwd.o:Ajpfwd.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Ajpfwd -o $@ $^ 
	
ajpfwd.so: Ajpfwd.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
