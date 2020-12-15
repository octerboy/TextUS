# Title: Makefile
# ID: Textus-fileLet-Makefile
# Build
#	B01:created by octerboy 2005/06/10

include ../Rules.basemod
INCLUDE_DIR += -I../httpsrvhead/include -I../baselib/include -I../logdata/include
all:httpfile.so httpfileaio.so

FileLet.o:FileLet.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=FileLet -o $@ $^

FileLetAio.o:FileLet.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DUSE_TEXTUS_AIO -DAMOR_CLS_TYPE=FileLet -o $@ $^

httpfile.so: FileLet.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

httpfileaio.so: FileLetAio.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
