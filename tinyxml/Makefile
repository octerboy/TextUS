
include ../Rules.basemod

all: tinystr.o  tinyxml.o  tinyxmlerror.o  tinyxmlparser.o  xmltest.o xmltest

tinystr.o: tinystr.cpp  
	${CC} ${CFLAGS} -o $@ $^

tinyxml.o: tinyxml.cpp 
	${CC} ${CFLAGS} -o $@ $^

tinyxmlerror.o: tinyxmlerror.cpp
	${CC} ${CFLAGS} -o $@ $^

tinyxmlparser.o: tinyxmlparser.cpp 
	${CC} ${CFLAGS} -o $@ $^

xmltest.o: xmltest.cpp
	${CC} ${CFLAGS} -o $@ $^

xmltest:tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o xmltest.o
	$(CXX) -o  $@ $^

clean:
	rm *.o
