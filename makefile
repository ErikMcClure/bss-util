.PHONY: all clean distclean

all:
	make -f bss-util.mk
	make -f test.mk

clean:
	make clean -f bss-util.mk
	make clean -f test.mk

dist: all distclean
	tar -czf bss-util-posix.tar.gz *

distclean:
	make distclean -f bss-util.mk
	make distclean -f test.mk

debug:
	make debug -f bss-util.mk
	make debug -f test.mk

install: all
	make install -f bss-util.mk
  
uninstall:
	make uninstall -f bss-util.mk