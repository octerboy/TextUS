# 标题:Makefile
# 标识:Textus-miniDB-Makefile
# 版本:B01
#	B001:created by octerboy 2005/06/10

CFLAGS  = -c -fPIC
CFLAGS  += -I./include -I../seclog/include -I../tinyxml -I../amor/include -I../fastdb
CC=g++
all:miniDB.so

MiniDB.o:MiniDB.cpp
	${CC} ${CFLAGS} -DNDEBUG -o $@ $^

miniDB.so: MiniDB.o 
	g++ -shared -o $@ $^  -lfastdb_r -lpthread
	cp $@ ../release

clean:
	rm *.o
	rm *.so
