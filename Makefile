PROJECT = $(notdir $(CURDIR))
SRC = $(wildcard src/*.c)
DEBUG = target/debug/$(PROJECT)
RELEASE = target/release/$(PROJECT)
CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -pipe -O2
LDFLAGS = $(shell pkg-config --libs libgit2)

all: debug

debug: OUTPUT = $(DEBUG)
# Non production ready flags (as of 2021-09-01), https://github.com/google/sanitizers/issues/1324: -fsanitize=pointer-compare -fsanitize=pointer-subtract
debug: CFLAGS += -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow $(shell export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2) -fanalyzer
debug: executable

release: OUTPUT = $(RELEASE)
release: executable

executable: $(SRC)
	$(CC) $(SRC) -o $(OUTPUT) $(CFLAGS) $(LDFLAGS)

.PHONY: run clean install uninstall

# write "make run a="..." for commandline arguments"
run:
	./$(DEBUG) $(a)

clean:
	rm -f $(DEBUG) $(RELEASE)

# installs from release folder only
install:
	ln -s $(CURDIR)/$(RELEASE) ~/.local/bin/

uninstall:
	rm -f ~/.local/bin/$(PROJECT)
