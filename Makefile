include ./Rules.basemod
include ./Allmodules

all:
	@for i in $(DIRS); \
	do \
	(echo "Now for $$i") ;\
	if [ -f "$$i/Makefile" ]; then \
		(cd $$i && echo "making all in $$i..." && $(MAKE) all) || exit 1; \
	fi; \
	done;
	$(MAKE) tus

clean:
	@for i in $(DIRS); \
	do \
	(echo "Now for $$i") ;\
	if [ -f "$$i/Makefile" ]; then \
		(cd $$i && echo "making clean in $$i..." && \
		$(MAKE) clean) || exit 1; \
	fi; \
	done;
	rm *.o; 
	rm tus	
	
tus:	main.c
	$(CXX) main.c -o $@ -ldl

