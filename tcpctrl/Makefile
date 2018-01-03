# Titile: Makefile
# ID: Textus-tcpctrl-Makefile
# Build
#	B01:created by octerboy 2006/08/02, π„÷›

include ../Rules.basemod
INCLUDE_DIR  +=  -I../logdata/include -I../unipac/include -I../baselib/include
all:tcpctrl.so

TCtrl.o:TCtrl.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=TCtrl -o $@ $^

tcpctrl.so: TCtrl.o
	${LD} $(LDFLAGS) -o $@ $^ $(LIBSOCKET) $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
