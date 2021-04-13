# Title: BufTmr Makefile
# ID: Textus-BufTmr-Makefile
# Build:
#	B01:created by octerboy 2019/03/19, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include  -I../unipac/include

all:buftmr.so

BufTmr.o: BufTmr.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=BufTmr -o $@ $^ 
	
buftmr.so: BufTmr.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
