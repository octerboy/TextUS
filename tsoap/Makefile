# Title: Makefile
# ID: Textus-TSoap-Makefile
# Build:
#	B01:created by octerboy 2006/10/12, Guangzhou

include ../Rules.basemod
INCLUDE_DIR  +=  -I../logdata/include -I../httpsrvhead/include
TiXML_DIR=../tinyxml

all:tsoap.so

Wsdl.o: Wsdl.cpp Array.h Wsdl.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ Wsdl.cpp

Schema.o: Schema.cpp Array.h Wsdl.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ Schema.cpp

TSoap.o: TSoap.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TSoap -o $@ $^

tsoap.so: TSoap.o Wsdl.o Schema.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
