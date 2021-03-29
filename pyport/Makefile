# Title: Makefile
# ID: Textus-PyPort-Makefile
# Build:
#	B01:created by octerboy 2018/07/20, Guangzhou

include ../Rules.basemod
include ../Rules.python
INCLUDE_DIR  +=  -I../logdata/include -I../baselib/include  -I../unipac/include -I../dbport/include -I${PYTHON_INC}
all:pyport.so

PyPort.o: PyPort.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=PyPort -o $@ $^

pyport.so: PyPort.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase -l${PYTHON_LIB}
	$(_POST_RPO_SUB)

py:py.cpp
	${CC} -I$(INCLUDE_DIR) $^ -o py -L$(RELEASE_DIR) -l${PYTHON_LIB}

clean:
	rm *.o
	rm *.so
