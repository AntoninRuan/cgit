SRC := $(wildcard src/*.c)
OBJ := $(addsuffix .o, $(basename $(SRC)))
OBJ_DEST := $(addprefix build/, $(OBJ))
CFLAGS := -lcrypto -lm -lz

DEBUG ?= false
ifeq ($(DEBUG), true)
	DEBUG_FLAG = -DDEBUG -ggdb
endif

all: $(OBJ_DEST)
	gcc -o build/cgit $(OBJ_DEST) $(CFLAGS) $(DEBUG_FLAG)

build/%.o: %.c
	@mkdir -p $(dir $@)
	gcc -c $< -o $@ $(CFLAGS) $(DEBUG_FLAG)

clean:
	@rm -rf build/*

run: all
	build/cgit