# Title: Makefile
# ID: Textus-sysadm-Makefile
# Buidl:
#	B01:created by octerboy 2006/12/23, Panyu

include ../Rules.basemod
INCLUDE_DIR  += -I../logdata/include -I../baselib/include

all:getversions.so restart.so sysinfo.so sysroute.so systime.so

GetVer.o: GetVer.cpp
	${CC} ${CFLAGS} -I../httpsrvhead/include $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=GetVer -o $@ $^

SysStat.o: SysStat.cpp
	${CC} ${CFLAGS} -I../httpsrvhead/include $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=SysStat -o $@ $^

Route.o: Route.cpp
	${CC} ${CFLAGS} -I../httpsrvhead/include $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Route -o $@ $^

Systime.o: Systime.cpp
	${CC} ${CFLAGS} -I../httpsrvhead/include $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Systime -o $@ $^

Restart.o: Restart.cpp
	${CC} ${CFLAGS} -I../httpsrvhead/include $(INCLUDE_DIR) $(DEBUG_FLAG) -DAMOR_CLS_TYPE=Restart -o $@ $^

netrs.o:netrs.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -o $@ $^

getversions.so: GetVer.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

sysinfo.so: SysStat.o netrs.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

sysroute.so: Route.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

systime.so: Systime.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

restart.so: Restart.o
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	cp $@ ../release

clean:
	rm *.o
	rm *.so
