# Title: Makefile
# ID: Textus-Trend-Makefile
# Build
#	B01:created by octerboy 2007/03/09, Panyu

include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include
AMOROBJS=Trend.o

all:trend.so

Trend.o:Trend.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -DNDEBUG  -DAMOR_CLS_TYPE=Trend -o $@ $^

trend.so:$(AMOROBJS)
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.o
	rm *.so
