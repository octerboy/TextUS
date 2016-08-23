# Title: Makefile
# ID:Textus-TclPort-Makefile
# Build:
#	B01:created by octerboy 2014/08/14, π„÷›

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include -I../unipac/include -I../../js/src/ -I../../js/src/Linux_All_DBG.OBJ
#LDFLAGS +=  ../../js/src/Linux_All_DBG.OBJ/*.o

all:jsport.so

TclPort.o:TclPort.cpp 
	${CC} ${CFLAGS} -DXP_UNIX=1 $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=TclPort -o $@ TclPort.cpp

tclport.so: TclPort.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
