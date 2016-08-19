# Title: Makefile
# ID: Textus-HttpUpFile-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:httpupfile.so

HttpUpFile.o:HttpUpFile.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpUpFile -o $@ $^ 
	
httpupfile.so: HttpUpFile.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
