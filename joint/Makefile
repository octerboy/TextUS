# Title: Makefile
# ID: Textus-Joint-Makefile
# Build:
#	B01:created by octerboy 2006/05/10

include ../Rules.basemod
AMOROBJS=Joint.o

all:joint.so

Joint.o:Joint.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG -DAMOR_CLS_TYPE=Joint -o $@ $^

joint.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
