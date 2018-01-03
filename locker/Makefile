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
	$(_POST_RPO_SUB)
	$(_POST_RPO_SUB)/locker_2.so
	$(_POST_RPO_SUB)/locker_3.so
	$(_POST_RPO_SUB)/locker_4.so
	$(_POST_RPO_SUB)/locker_5.so
	$(_POST_RPO_SUB)/locker_6.so
	$(_POST_RPO_SUB)/locker_7.so

clean:
	rm *.o
	rm *.so
