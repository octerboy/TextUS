# Title: Makefile
# ID: Textus-TSoap-Makefile
# Build:
#	B01:created by octerboy 2006/10/12, Guangzhou

include ../Rules.basemod
INCLUDE_DIR  +=  -I../logdata/include -I../httpsrvhead/include
TiXML_DIR=../tinyxml

all:tsoap.so test

test.o:  test.cpp Array.h Wsdl.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@  test.cpp
	
Wsdl.o: Wsdl.cpp Array.h Wsdl.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ Wsdl.cpp

Schema.o: Schema.cpp Array.h Wsdl.h
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ Schema.cpp

TSoap.o: TSoap.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TSoap -o $@ $^

tsoap.so: TSoap.o Wsdl.o Schema.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

test: test.o  Wsdl.o Schema.o $(TiXML_DIR)/tinystr.o $(TiXML_DIR)/tinyxmlerror.o $(TiXML_DIR)/tinyxmlparser.o $(TiXML_DIR)/tinyxml.o
	$(CXX) -o $@ $^

test2: test.o
	$(CXX) -o $@ $^

clean:
	rm *.o
	rm *.so
