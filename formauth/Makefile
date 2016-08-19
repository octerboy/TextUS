# Title:Makefile
# ID:Textus-formauth-Makefile
# Build
#	B01:created by octerboy 2007/09/24
include ../Rules.basemod
INCLUDE_DIR  +=  -I../baselib/include -I../httpsrvhead/include -I../logdata/include -I../unipac/include

all:formauth.so

FormAuth.o:FormAuth.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG)  -DAMOR_CLS_TYPE=FormAuth -o $@ $^

formauth.so: FormAuth.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase
	cp $@ ../release

clean:
	rm *.o
	rm *.so
