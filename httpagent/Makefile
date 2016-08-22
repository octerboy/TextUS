# Title: HttpAgent Makefile
# ID: Textus-HttpAgent-Makefile
# Build:
#	B01:created by octerboy 2006/09/13, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include -I../httpsrvhead

all:httpagent.so

DeHead.o:../httpsrvhead/DeHead.cpp ../httpsrvhead/tdate_parse.c
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ ../httpsrvhead/DeHead.cpp

HttpAgent.o:HttpAgent.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpAgent -o $@ $^ 
	
httpagent.so: HttpAgent.o DeHead.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
