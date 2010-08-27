GCC = g++
CFLAGS = -m32 -shared
LDFLAGS = libv8.so -lpthread -ldl

OUTFILE = libkwl.so
OBJ = Bot.o GlobalModule.o IrcSocket.o ScriptEventManager.o Core.o IrcChannel.o IrcUser.o ScriptFunctions.o IrcSettings.o Script.o ScriptModule.o SendQueue.o TimerManager.o IrcMessage.o

kwlbot: $(OBJ)
	$(GCC) $(CFLAGS) -o $(OUTFILE) $(OBJ) $(LDFLAGS)
	strip --strip-unneeded $(OUTFILE)

%.o: %.c
	$(GCC) $(CFLAGS) -Wall -fPIC -c $<

clean:
	rm -f $(OBJ)
