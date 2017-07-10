# Title: Makefile
# ID: Textus-jdbcli-Makefile
# Build:
#	B01:created by octerboy 2017/07/06, guangzhou

include ../Rules.basemod
include ../Rules.java

all:JdbCli.class

JdbCli.class: JdbCli.java
	${JC} ${JCFLAGS} -cp $(RELEASE_DIR)/jvmport.jar $^
	cp JdbCli.class $(RELEASE_DIR)

clean:
	rm *.class
