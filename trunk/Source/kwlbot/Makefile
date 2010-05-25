GCC = g++
CFLAGS = -m32
LDFLAGS = libkwl.so ../../../v8/libv8.so

OBJ = Main.o

kwlbot: $(OBJ)
	$(GCC) $(CFLAGS) -o kwlbot $(OBJ) $(LDFLAGS)
	strip --strip-unneeded kwlbot

%.o: %.c
	$(GCC) -Wall -fPIC $(CFLAGS) -c $<

clean:
	rm -f $(OBJ)
