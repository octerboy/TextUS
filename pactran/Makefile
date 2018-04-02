# Title: Makefile
# ID: Textus-pactran-Makefile
# Build:
#	B01:created by octerboy 2018/03/18, Panyu
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../unipac/include  -I../tranway/include

all:pactran.so

PacTran.o: PacTran.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=PacTran -o $@ PacTran.cpp
	
pactran.so: PacTran.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
