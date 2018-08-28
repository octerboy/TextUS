# Title: Makefile
# ID: Textus-tcrypt-Makefile
# Build:
#	B01:created by octerboy 2018/06/26, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include

all:tcrypt.so

TCrypt.o: TCrypt.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TCrypt -o $@ TCrypt.cpp
	
tcrypt.so: TCrypt.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lcrypto -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
