# Title: Makefile
# ID: Textus-JvmPort-Makefile
# Build:
#	B01:created by octerboy 2007/05/01, Panyu

include ../Rules.basemod
PACDIR = ./jetus/jvmport/
PACPATH = jetus.jvmport.
include ../Rules.java
INCLUDE_DIR  +=  -I../logdata/include -I../baselib/include  -I$(JDK_HOME)/include -I$(JDK_HOME)/include/linux

all:jvmport.so  $(PACDIR)/Pius.class $(PACDIR)/Amor.class $(PACDIR)/TBuffer.class $(PACDIR)/PacketData.class $(PACDIR)/TiXML.class

JvmPort.o: JvmPort.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -I../unipac/include $(DEBUG_FLAG) -DAMOR_CLS_TYPE=JvmPort -o $@ $^

jvmport.so: JvmPort.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS)
	$(_POST_RPO_SUB)

$(PACDIR)/Amor.class:Amor.java
	${JC} $^
	mv Amor.class $(PACDIR)
	${JH} $(PACPATH)Amor
	${JAR} cvf jvmport.jar jetus
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/Pius.class:Pius.java
	${JC} $^
	mv Pius.class $(PACDIR)
	${JAR} cvf jvmport.jar jetus
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/TBuffer.class:TBuffer.java
	${JC} $^
	mv TBuffer.class $(PACDIR)
	${JH} $(PACPATH)TBuffer
	${JAR} cvf jvmport.jar jetus
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/PacketData.class:PacketData.java
	${JC} $^
	mv PacketData.class $(PACDIR)
	${JH} $(PACPATH)PacketData
	${JAR} cvf jvmport.jar jetus
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/TiXML.class: TiXML.java
	${JC} $^
	mv TiXML.class $(PACDIR)
	${JH} $(PACPATH)TiXML
	${JAR} cvf jvmport.jar jetus
	cp jvmport.jar $(RELEASE_DIR)

jv:jv.c
	gcc $(INCLUDE_DIR) jv.c -o jv -L../release/ -ljvm
	
clean:
	rm *.o
	rm *.so
