# Title: Makefile
# ID: Textus-HttpSrvHead-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  += -I./include -I../baselib/include -I../logdata/include

all:httpsrvhead.so

DeHead.o:DeHead.cpp tdate_parse.c
	${CC} ${CFLAGS}  $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ DeHead.cpp

HttpSrvHead.o:HttpSrvHead.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpSrvHead -o $@ $^ 
	
httpsrvhead.so: DeHead.o HttpSrvHead.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
