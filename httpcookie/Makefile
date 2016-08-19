# Title: Makefile
# ID: Textus-httpcookie-Makefile
# Build:
#	B01:created by octerboy 2006/11/2, Hebi(Henan)
include ../Rules.basemod
INCLUDE_DIR  += -I../logdata/include -I../httpsrvhead/include -Iinclude

all:httpcookie.so

HttpCookie.o: HttpCookie.cpp 
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=HttpCookie -o $@ HttpCookie.cpp
	
httpcookie.so: HttpCookie.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
