# Title :Makefile
# ID: Textus-Locker-Makefile
# Build:
#	B01:created by octerboy 2006/05/10
include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include
AMOROBJS=Locker.o

all:locker.so

Locker.o:Locker.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG  -DAMOR_CLS_TYPE=Locker -o $@ $^

locker.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lpthread -lbase
	cp $@ ../release
	cp $@ ../release/locker_2.so
	cp $@ ../release/locker_3.so
	cp $@ ../release/locker_4.so
	cp $@ ../release/locker_5.so
	cp $@ ../release/locker_6.so
	cp $@ ../release/locker_7.so

clean:
	rm *.o
	rm *.so
