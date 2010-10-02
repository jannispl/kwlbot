GCC = g++
CFLAGS = -m32
LDFLAGS = -L../libkwl/v8 -lv8 -L../libkwl -lkwl

OBJ = Main.o

kwlbot: $(OBJ)
	$(GCC) $(CFLAGS) -o kwlbot $(OBJ) $(LDFLAGS)
	strip --strip-unneeded kwlbot

%.o: %.c
	$(GCC) -Wall -fPIC $(CFLAGS) -c $<

clean:
	rm -f $(OBJ)
