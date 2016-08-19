# Title:Makefile
# ID:Textus-Auxiliary-Makefile
# Build
#	B01:created by octerboy 2006/05/01, ÓÚ¹ãÖÝ
include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include
AMOROBJS=auxi.o

all:auxiliary.so

auxi.o:Auxi.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Auxi -o $@ $^

auxiliary.so:$(AMOROBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
