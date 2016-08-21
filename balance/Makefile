# Title: Makefile
# ID: Textus-Balance-Makefile
# Build:
#	B01:created by octerboy 2005/06/10
include ../Rules.basemod
INCLUDE_DIR += -I../logdata/include
all:balance.so

Balance.o:Balance.cpp
	$(CC) $(CFLAGS) $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Balance -o $@ $^

balance.so:Balance.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)
	
clean:
	rm *.o
	rm *.so
