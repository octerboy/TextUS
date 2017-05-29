# Title: Makefile
# ID: Textus-insway-Makefile
# Build:
#	B01:created by octerboy 2016/08/15, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include -I../dbport/include

all:pacway.so

PacWay.o: PacWay.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=PacWay -o $@ PacWay.cpp
	
pacway.so: PacWay.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lcrypto -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
