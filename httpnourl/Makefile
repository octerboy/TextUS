# Title: Makefile
# ID:Textus-NourlLet-Makefile
# Build:
#	B01:created by octerboy 2005/09/4

include ../Rules.basemod
INCLUDE_DIR += -I../httpsrvhead/include -I../logdata/include
all:httpnourl.so

NourlLet.o:NourlLet.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=NourlLet -o $@ $^

httpnourl.so: NourlLet.o 
	${LD} $(LDFLAGS) -o $@ $^  $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
