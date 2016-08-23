# Title: Makefile
# ID:Textus-JSPort-Makefile
# Build:
#	B01:created by octerboy 2005/09/4

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include -I../unipac/include -I../../js/src/ -I../../js/src/Linux_All_DBG.OBJ
#LDFLAGS +=  ../../js/src/Linux_All_DBG.OBJ/*.o

all:jsport.so

JSPort.o:JSPort.cpp function.c
	${CC} ${CFLAGS} -DXP_UNIX=1 $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=JSPort -o $@ JSPort.cpp

jsport.so: JSPort.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
