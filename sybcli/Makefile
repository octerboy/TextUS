# Title: Makefile for finora
# ID: Textus-SybCli-Makefile
# Build:
#	B01:created by octerboy 2008/08/27, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../logdata/include -I../dbport/include -I../unipac/include -I../baselib/include  -I$(SYBASE)/include

all:sybcli.so

SybCli.o: SybCli.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=SybCli -o $@ $^ 
	
sybcli.so: SybCli.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -L$(SYBASE)/lib -lct -lcs -lsybtcl -lcomn -lintl -linsck -ldl -lc
	#${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) 
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
