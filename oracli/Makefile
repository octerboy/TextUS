# Title: Makefile for finora
# ID: Textus-OraCli-Makefile
# Build:
#	B01:created by octerboy 2006/10/30, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../logdata/include -I../dbport/include -I../unipac/include -I../baselib/include

all:oracli.so

OraCli.o: OraCli.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DUSE_DYNAMIC_ORACLE -DAMOR_CLS_TYPE=OraCli -o $@ $^ 
	
oracli.so: OraCli.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
