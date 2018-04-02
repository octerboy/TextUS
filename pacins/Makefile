# Title: Makefile
# ID: Textus-pacins-Makefile
# Build:
#	B01:created by octerboy 2018/02/24, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include -I../tranway/include  -I../dbport/include

all:pacins.so

PacIns.o: PacIns.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=PacIns -o $@ PacIns.cpp
	
pacins.so: PacIns.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
