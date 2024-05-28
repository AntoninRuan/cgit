SRC := $(wildcard src/*.c)
CFLAGS := -lcrypto -lm -lz

DEBUG = false
ifeq ($(DEBUG), true)
	DEBUG_FLAG = -DDEBUG -ggdb
endif

all: $(SRC)
	gcc -o build/cgit $(SRC) $(CFLAGS) $(DEBUG_FLAG)

run: all
	build/cgit