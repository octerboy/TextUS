all:libsjl06.so

eng_sjl06.o:eng_sjl06.c
	gcc -O3 -g -Wall -c -fPIC -o $@ $^

libsjl06.so: eng_sjl06.o
	g++ -O3 -shared -dn -o $@ $^ $(LDLIBS)
	cp $@ /tmp/

clean:
	rm *.o
	rm *.so
