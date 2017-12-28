# Title: Makefile
# ID: Textus-JvmPort-Makefile
# Build:
#	B01:created by octerboy 2007/05/01, Panyu
#gcc $(INCLUDE_DIR) $(JavaVM) $^ -o jv -L$(RELEASE_DIR) -lverify -ljava -ljvm -ljava

include ../Rules.basemod
PACDIR = ./textor/jvmport/
PACPATH = textor.jvmport.
include ../Rules.java
INCLUDE_DIR  +=  -I../logdata/include -I../baselib/include  -I$(JDK_HOME)/include -I$(JDK_INC_JNI) -I../dbport/include
#${CC} $(INCLUDE_DIR) $^ -o jv -L$(RELEASE_DIR) -lverify -ljava -ljvm -ljava
#gcc -I/System/Library/Frameworks/JavaVM.framework/Versions/A/Headers /System/Library/Frameworks/JavaVM.framework/Versions/A/JavaVM $^ -o jv 

all:jvmport.so
all_class:$(PACDIR)/Pius.class $(PACDIR)/Amor.class $(PACDIR)/TBuffer.class $(PACDIR)/PacketData.class $(PACDIR)/TiXML.class $(PACDIR)/DBFace.class

JvmPort.o: JvmPort.cpp
	${CC} ${CFLAGS} $(INCLUDE_DIR) -I../unipac/include $(DEBUG_FLAG) -DAMOR_CLS_TYPE=JvmPort -o $@ $^

jvmport.so: JvmPort.o 
	${LD} $(LDFLAGS) -o $@ $^ $(LDLIBS) -lbase -L$(JDK_HOME)/jre/lib/server -ljvm
	$(_POST_RPO_SUB)

$(PACDIR)/Amor.class:Amor.java
	${JC} ${JCFLAGS} $^
	mv Amor.class $(PACDIR)
	${JH} $(PACPATH)Amor
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/Pius.class:Pius.java
	${JC} ${JCFLAGS} $^
	mv Pius.class $(PACDIR)
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/TBuffer.class:TBuffer.java
	${JC} ${JCFLAGS} $^
	mv TBuffer.class $(PACDIR)
	${JH} $(PACPATH)TBuffer
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/PacketData.class:PacketData.java
	${JC} ${JCFLAGS} $^
	mv PacketData.class $(PACDIR)
	${JH} $(PACPATH)PacketData
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/TiXML.class: TiXML.java
	${JC} ${JCFLAGS} $^
	mv TiXML.class $(PACDIR)
	${JH} $(PACPATH)TiXML
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

$(PACDIR)/DBFace.class: DBFace.java
	${JC} ${JCFLAGS} $^
	mv DBFace*.class $(PACDIR)
	${JH} $(PACPATH)DBFace
	${JAR} cvf jvmport.jar textor
	cp jvmport.jar $(RELEASE_DIR)

jv:jv.c
	gcc -I$(INCLUDE_DIR) $^ -o jv -L$(RELEASE_DIR) -ljvm

clean:
	rm *.o
	rm *.so
