# Title: Makefile
# ID: Textus-HttpXML-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod

INCLUDE_DIR += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:httpxml.so

HttpXML.o:HttpXML.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=HttpXML -o $@ $^ 
	
httpxml.so: HttpXML.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so

