PROGRAM = genc
CC = gcc
CFLAGS = -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -pipe  -O2 -std=c11
DEBUGFLAGS = -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow $(shell export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2) -fanalyzer
LDFLAGS =

CFLAGS += $(shell pkg-config --cflags libgit2)
LDFLAGS += $(shell pkg-config --libs libgit2)

all: $(PROGRAM)
$(PROGRAM): Makefile $(PROGRAM).c ; $(CC) $(PROGRAM).c -o $(PROGRAM) $(CFLAGS) $(DEBUGFLAGS) $(LDFLAGS)

release: Makefile $(PROGRAM).c ; $(CC) $(PROGRAM).c -o $(PROGRAM) $(CFLAGS) $(LDFLAGS)

run: ; ./$(PROGRAM) $@

install: ; ln -s $(shell pwd)/$(PROGRAM) ~/.local/bin/
uninstall: ; rm -f ~/.local/bin/$(PROGRAM)
