# Title: Makefile
# ID: Textus-tranway-Makefile
# Build:
#	B01:created by octerboy 2018/02/24, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I./include -I../baselib/include -I../logdata/include -I../unipac/include

all:tranway.so

TranWay.o: TranWay.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TranWay -o $@ TranWay.cpp
	
tranway.so: TranWay.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lcrypto -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
