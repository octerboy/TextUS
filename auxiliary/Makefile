# Title:Makefile
# ID:Textus-Auxiliary-Makefile
# Build
#	B01:created by octerboy 2006/05/01, �ڹ���
include ../Rules.basemod
AMOROBJS=auxi.o

all:auxiliary.so

auxi.o:Auxi.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Auxi -o $@ $^

auxiliary.so:$(AMOROBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
