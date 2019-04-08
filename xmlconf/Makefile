# Title: XmlConf Makefile
# ID: Textus-XmlConf-Makefile
# Build:
#	B01:created by octerboy 2019/04/05, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include  -I../unipac/include

all:xmlconf.so

XmlConf.o: XmlConf.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=XmlConf -o $@ $^ 
	
xmlconf.so: XmlConf.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
