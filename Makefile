include ./Rules.basemod
include ./Allmodules

all:
	@for i in $(DIRS); \
	do \
	if [ -d "$$i" ]; then \
		(cd $$i && $(MAKE) all) || exit 1; \
	else \
		$(MAKE) tus; \
	fi; \
	done;

clean:
	@for i in $(DIRS); \
	do \
	if [ -d "$$i" ]; then \
		(cd $$i && echo "making clean in $$i..." && \
		$(MAKE) clean) ; \
	else \
		rm *.o; \
		rm tus; \
	fi; \
	done;
	
tus:	main.c
	$(CXX) main.c -o $@ -ldl

