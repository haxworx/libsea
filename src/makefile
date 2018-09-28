TARGET = ../libsea.so

PKGS=openssl sdl2 SDL2_mixer

OBJECTS = errors.o btree.o buf.o strings.o list.o hash.o url.o system.o file.o exe.o server.o notify.o thread.o ipc.o \
          net.o sound.o proc.o websocket.o

default: $(TARGET)

$(TARGET) : $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) $(shell pkg-config --libs $(PKGS)) -shared -o $@

errors.o: errors.c
	$(CC) -c $(CFLAGS) errors.c -o $@

buf.o: buf.c
	$(CC) -c $(CFLAGS) buf.c -o $@

btree.o: btree.c
	$(CC) -c $(CFLAGS) btree.c -o $@

strings.o: strings.c
	$(CC) -c $(CFLAGS) strings.c -o $@

list.o: list.c
	$(CC) -c $(CFLAGS) list.c -o $@

hash.o: hash.c
	$(CC) -c $(CFLAGS) hash.c -o $@

url.o: url.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags $(PKGS)) url.c -o $@

system.o: system.c
	$(CC) -c $(CFLAGS) system.c -o $@

file.o: file.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags $(PKGS)) file.c -o $@

exe.o: exe.c
	$(CC) -c $(CFLAGS) exe.c -o $@

server.o: server.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags openssl) server.c -o $@

notify.o: notify.c
	$(CC) -c $(CFLAGS) notify.c -o $@

thread.o: thread.c
	$(CC) -c $(CFLAGS) thread.c -o $@

ipc.o: ipc.c
	$(CC) -c $(CFLAGS) ipc.c -o $@

net.o: net.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags openssl) net.c -o $@

proc.o: proc.c
	$(CC) -c $(CFLAGS) proc.c -o $@

sound.o: sound.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags sdl2 SDL2_mixer) sound.c -o $@

websocket.o: websocket.c
	$(CC) -c $(CFLAGS) $(shell pkg-config --cflags openssl) websocket.c -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
