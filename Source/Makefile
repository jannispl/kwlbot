all:
	make -C libkwl
	make -C kwlbot

install:
	cp libkwl/libkwl.so /usr/lib/
	chmod 644 /usr/lib/libkwl.so
	cp ../../v8/libv8.so /usr/lib/
	chmod 644 /usr/lib/libv8.so
