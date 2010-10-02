all:
	make -C libkwl
	cp libkwl/libkwl.so ./kwlbot/
	make -C kwlbot

clean:
	make -C libkwl clean
	make -C kwlbot clean
	rm -f libkwl/libkwl.so
	rm -f kwlbot/kwlbot

install:
	cp libkwl/libkwl.so /usr/local/lib/
	chmod 644 /usr/local/lib/libkwl.so
	cp ../../v8/libv8.so /usr/local/lib/
	chmod 644 /usr/local/lib/libv8.so

nonroot:
	cp libkwl/libkwl.so ./kwlbot/
	cp libkwl/v8/libv8.so ./kwlbot/
