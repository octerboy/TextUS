include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include
all:log2buff.so

L2Buffer.o:L2Buffer.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ $^

log2buff.so: L2Buffer.o
	$(LD) $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	$(_POST_RPO_SUB)

clean:
	rm *.so
	rm *.o
