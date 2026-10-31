# hashwatch - Makefile
CC      ?= cc
CFLAGS  ?= -O2
CFLAGS  += -std=c11 -D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE -Wall -Wextra -Wpedantic -MMD -MP -Isrc
PREFIX  ?= /usr/local

BUILD    := build
LIB_SRC  := $(filter-out src/main.c,$(wildcard src/*.c))
LIB_OBJ  := $(patsubst src/%.c,$(BUILD)/%.o,$(LIB_SRC))
TEST_SRC := $(wildcard tests/test_*.c)
TEST_BIN := $(patsubst tests/%.c,$(BUILD)/%,$(TEST_SRC))

ifneq ($(wildcard src/main.c),)
BIN := $(BUILD)/hashwatch
endif

.PHONY: all test clean install uninstall

all: $(BIN)

$(BUILD):
	mkdir -p $@

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/hashwatch: src/main.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $< $(LIB_OBJ) -o $@

$(BUILD)/test_%: tests/test_%.c $(LIB_OBJ) | $(BUILD)
	$(CC) $(CFLAGS) $< $(LIB_OBJ) -o $@

test: $(BIN) $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done
	@if [ -f tests/integration.sh ] && [ -n "$(BIN)" ]; then \
		echo "== tests/integration.sh"; sh tests/integration.sh $(BIN) || exit 1; fi

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BIN) $(DESTDIR)$(PREFIX)/bin/hashwatch

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/hashwatch

clean:
	rm -rf $(BUILD)

-include $(wildcard $(BUILD)/*.d)
