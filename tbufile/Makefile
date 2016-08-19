include ../Rules.basemod
INCLUDE_DIR += -I../baselib/include -I../logdata/include
all:tbufile.so

Bu2File.o: Bu2File.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -o $@ $^

tbufile.so: Bu2File.o
	$(LD) $(LDFLAGS) -o $@ $^  $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.so
	rm *.o
