GCC = g++
CFLAGS = 
LDFLAGS = ./v8/libv8.a -lpthread

OBJ = Main.o Core.o Bot.o IrcUser.o IrcChannel.o IrcSettings.o IrcSocket.o EventManager.o Script.o ScriptFunctions.o StdInc.o

kwlbot: $(OBJ)
	$(GCC) -o kwlbot $(OBJ) $(LDFLAGS)

%.o: %.c
	$(GCC) -c $<

clean:
	rm -f $(OBJ)
