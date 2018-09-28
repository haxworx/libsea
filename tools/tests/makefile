PKGS = openssl
PKGS_SDL = sdl2 SDL2_mixer
CFLAGS = -std=gnu11 -Wall -Wl,-rpath -Wl,.. -Wno-format -g -ggdb3 -O0 -pthread -I../src -L../
LDFLAGS += -lsea

EXES = test thread server notify net ipc urltest kiss sound proc strings

default: $(EXES)

test: test.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs --cflags $(PKGS)) test.c -o test

urltest: urltest.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs --cflags $(PKGS)) urltest.c -o urltest

server: server.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs --cflags $(PKGS)) server.c -o server

sound: sound.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(shell pkg-config --libs --cflags $(PKGS_SDL)) sound.c -o sound

proc: proc.c
	$(CC) $(CFLAGS) $(LDFLAGS) proc.c -o proc

strings: strings.c
	$(CC) $(CFLAGS) $(LDFLAGS) strings.c -o strings

sdl:
	$(MAKE) -C sdl
clean:
	rm -f $(EXES)
