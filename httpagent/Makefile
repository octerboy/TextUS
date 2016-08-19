# Title: HttpAgent Makefile
# ID: Textus-HttpAgent-Makefile
# Build:
#	B01:created by octerboy 2006/09/13, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../logdata/include -I../httpsrvhead/include

all:httpagent.so

DeHead.o:DeHead.cpp tdate_parse.c
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ DeHead.cpp

HttpAgent.o:HttpAgent.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpAgent -o $@ $^ 
	
httpagent.so: HttpAgent.o DeHead.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
