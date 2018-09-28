PREFIX=/usr/local
TARGET = libsea.so
SRC_DIR=src

LIBS=

ifeq ($(OS),Windows_NT)
	OS := WINDOWS=1
else
	OS := UNIX=1
endif

OSNAME := $(shell uname -s)

ifeq ($(OSNAME), Linux)
	LIBS += -lrt
endif

ifeq ($(OSNAME), Darwin)
	LDFLAGS += $(shell pkg-config --cflags openssl)
endif

ifeq ($(OSNAME), OpenBSD)
	LIBS += -lkvm
	LDFLAGS += -I/usr/local/include -L/usr/local/lib
endif

export OSNAME

export LIBS

export LDFLAGS

export CFLAGS = -std=gnu11 -Wall -Wextra -g -ggdb -O0 -fPIC -D$(OS) -D_FILE_OFFSET_BITS=64

default:
	$(MAKE) -C src
	$(MAKE) -C tests

clean:
	rm -f $(OBJECTS) $(TARGET)
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

install:
	$(MAKE) -C src
	cp $(TARGET) "$(PREFIX)/lib"
	-mkdir "$(PREFIX)/include/sea"
	cp $(SRC_DIR)/*.h "$(PREFIX)/include/sea"
