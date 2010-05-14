GCC = g++
CFLAGS = -m32
LDFLAGS = ./v8/libv8.a -lpthread

OBJ = Main.o Core.o Bot.o IrcUser.o IrcChannel.o IrcSettings.o IrcSocket.o EventManager.o Script.o ScriptFunctions.o StdInc.o

kwlbot: $(OBJ)
	$(GCC) $(CFLAGS) -o kwlbot $(OBJ) $(LDFLAGS)

%.o: %.c
	$(GCC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ)
