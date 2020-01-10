# Title: HttpIns Makefile
# ID: Textus-HttpIns-Makefile
# Build:
#	B01:created by octerboy 2019/09/19, Guangzhou
include ../Rules.basemod
INCLUDE_DIR  += -I../baselib/include -I../tranway/include -I../logdata/include -I../httpsrvhead/include -I../httpsrvhead

all:httpins.so

DeHead.o:../httpsrvhead/DeHead.cpp ../httpsrvhead/tdate_parse.c
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ ../httpsrvhead/DeHead.cpp

HttpIns.o:HttpIns.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpIns -o $@ $^ 
	
httpins.so: HttpIns.o DeHead.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
