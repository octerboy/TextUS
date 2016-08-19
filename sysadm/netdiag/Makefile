# 标题:Makefile
# 标识:Textus-Netdiag-Makefile
# 版本:B01
#	B01:created by octerboy 2006/07/13

SCMID="\"Network Diag Ver 1.0\""
CFLAGS  = -c -g -Wall -fPIC
CFLAGS  += -I../../tinyxml -I../../amor/include -I../../logdata/include -I../../baselib/include

CC=g++
LD=g++
all:netdiag.so

Netdiag.o: Netdiag.cpp
	${CC} ${CFLAGS} -DNDEBUG -DSCM_MODULE_ID=${SCMID} -DAMOR_CLS_TYPE=Netdiag -o $@ $^

diag.o:diag.cpp
	${CC} ${CFLAGS}  -o $@ $^

netdiag.so: Netdiag.o diag.o
	${LD} -shared -o $@ $^
	cp $@ ../../release

clean:
	rm *.o
	rm *.so
