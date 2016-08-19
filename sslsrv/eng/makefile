all:libsjl07.so

eng_sjl07.o:eng_sjl07.c
	gcc -O3 -g -Wall -c -fPIC -o $@ $^

libsjl07.so: eng_sjl07.o
	g++ -O3 -shared -dn -o $@ $^ -L/home/octerboy/openssl-0.9.7a -lssl -lcrypto
	cp $@ /tmp/

clean:
	rm *.o
	rm *.so
