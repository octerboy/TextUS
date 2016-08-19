# Title :Makefile
# ID: Textus-HttpForm-Makefile
# Build:
#	B01:created by octerboy 2005/09/15, Guangzhou

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:httpform.so

HttpForm.o:HttpForm.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpForm -o $@ $^ 
	
httpform.so: HttpForm.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
