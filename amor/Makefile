# ID:Textus-Amor-Makefile
# Title:Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
TiXML_DIR=../tinyxml
AMOROBJS=$(TiXML_DIR)/tinystr.o $(TiXML_DIR)/tinyxmlerror.o $(TiXML_DIR)/tinyxmlparser.o $(TiXML_DIR)/tinyxml.o
all:libanimus.so ok

Animus.o:Animus.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -O3 -DNDEBUG -o $@ $^

ok:$(AMOROBJS) toOK.cpp
	${CXX} -I$(TiXML_DIR) -I./include toOK.cpp $(AMOROBJS) -o $@

libanimus.so:$(AMOROBJS) Animus.o
	${LD} $(ALONE_LDFLAGS) -o $@ $^ $(ALONE_LDLIBS) 
	cp $@ ../release

clean:
	rm *.o
	rm *.so
