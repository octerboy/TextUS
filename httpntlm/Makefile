# Title:Makefile
# ID:Textus-httpntlm-Makefile
# Build
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR  +=  -I../baselib/include -I../httpsrvhead/include -I../logdata/include

all:httpntlm.so

HttpNTLM.o:HttpNTLM.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=HttpNTLM -o $@ $^

httpntlm.so: HttpNTLM.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
