#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <git2.h>

const char HELP_MESSAGE[] =
    "C project generator\n"
    "\n"
    "USAGE: genc ...\n"
    "\n"
    "COMMANDS:\n"
    "    init     <project_name> Create a new c project\n"
    "    makefile                Prints the template makefile to stdout\n"
    "\n"
    "EXAMPLES:\n"
    "    The following example creates a new c project in the current directory.\n"
    "    $ genc init my-new-project\n";
    
#define MAKEFILE_C                                                      \
    "SRC = $(wildcard src/*.c)\n"                                       \
    "CC = gcc\n"                                                        \
    "FLAGS = -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -pipe -O2\n" \
    "\n"                                                                \
    "debug: OUTPUT = $(DEBUG)\n"                                        \
    "# Non production ready flags (as of 2021-09-01), https://github.com/google/sanitizers/issues/1324: -fsanitize=pointer-compare -fsanitize=pointer-subtract\n" \
    "debug: FLAGS += -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow $(shell export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2) -fanalyzer\n" \
    "debug: executable\n"                                               \
    "\n"
    
#define MAKEFILE_CPP                                            \
    "SRC = $(wildcard src/*.cpp)\n"                             \
    "CC = g++\n"                                                \
    "FLAGS = -std=c++20 -Wall -Werror -Wextra -Wpedantic -O2\n" \
    "\n"                                                        \
    "debug: OUTPUT = $(DEBUG)\n"                                \
    "debug: FLAGS += -g\n"                                      \
    "debug: executable\n"                                       \
    "\n"

// x is replaced with c/cpp specific stuff
#define MAKEFILE(x)                                                     \
    "PROJECT = $(notdir $(CURDIR))\n"                                   \
    "DEBUG = target/debug/$(PROJECT)\n"                                 \
    "RELEASE = target/release/$(PROJECT)\n"                             \
    "\n"                                                                \
    "all: mkdir debug\n"                                                \
    "\n"                                                                \
    "mkdir:\n"                                                          \
    "	mkdir -p ./target/debug\n"                                      \
    "	mkdir -p ./target/release\n"                                    \
    "\n"                                                                \
    x                                                                   \
    "release: OUTPUT = $(RELEASE)\n"                                    \
    "release: executable\n"                                             \
    "\n"                                                                \
    "executable: $(SRC)\n"                                              \
    "	$(CC) $(SRC) -o $(OUTPUT) $(FLAGS) \n"                          \
    "\n"                                                                \
    ".PHONY: run clean install uninstall\n"                             \
    "\n"                                                                \
    "# write \"make run a=\"...\" for commandline arguments\"\n"        \
    "run:\n"                                                            \
    "	./$(DEBUG) $(a)\n"                                              \
    "\n"                                                                \
    "clean:\n"                                                          \
    "	rm -f $(DEBUG) $(RELEASE)\n"                                    \
    "\n"                                                                \
    "# installs from release folder only\n"                             \
    "install:\n"                                                        \
    "	ln -s $(CURDIR)/$(RELEASE) ~/.local/bin/\n"                     \
    "\n"                                                                \
    "uninstall:\n"                                                      \
    "	rm -f ~/.local/bin/$(PROJECT)\n"

const char SRC_MAKEFILE[] =
    "all:\n"
    "	$(MAKE) -C .. $@\n"
    "%:\n"
    "	$(MAKE) -C .. $@\n";

void
gen_dir(char filepath[]) {
    if (filepath == NULL) {
        fprintf(stderr, "the filepath is null idk what happened\n");
        exit(1);
    }

    int exists = mkdir(filepath, 0777);

    if (exists) {
        fprintf(stderr, "File or directory could not be created: %s\n", filepath);
        exit(1);
    }
}

void
gen_file(char filepath[], const char contents[]) {
    FILE * fp = fopen(filepath, "w");
    
    if (fp == NULL) {
        fprintf(stderr, "Cannot open filepath: %s\n", filepath);
        exit(1);
    }

    fputs(contents, fp);
    fclose(fp);
}

void gen_makefile_c(char filepath[]) { gen_file(filepath, MAKEFILE(MAKEFILE_C)); }
void gen_makefile_cpp(char filepath[]) { gen_file(filepath, MAKEFILE(MAKEFILE_CPP)); }
void gen_gitignore(char filepath[]) { gen_file(filepath, "/target"); }
void gen_srcmakefile(char filepath[]) { gen_file(filepath, SRC_MAKEFILE); }

void
gen_git_dir(char project_name[]) {
    // With help from https://libgit2.org/docs/guides/101-samples/
    git_libgit2_init(); 
    git_repository * repo = NULL;

    int error = git_repository_init(&repo, project_name, 0);

    if (error < 0) {
        fprintf(stderr, "Could not initialize repository\n");
        exit(1);
    }

    // Must free values that are filled in for you
    git_repository_free(repo);
    git_libgit2_shutdown();
}


// Calling this function "write" causes weird function shadowing issues caused by the libgit2 library
void
_write(char to[], const char from[], int offset, int length, void (*action)(char [])) {
    for (int i = 0; i < length; i++)
        to[i + offset] = from[i];

    action(to);
}

void
init_project(char project_name[], int cprojp) {
    int i_len = (int)strlen(project_name);
    int f_len = 13; // make sure this matches with the longest static string
    int len = i_len + f_len + 1;

    char * project_folder = (char *)malloc(sizeof(char) * (size_t)len);
    
    // To ensure a buffer overflow will not (maybe) happen
    for (int i = i_len; i < len; i++)
        project_folder[i] = '\0';

    _write(project_folder, project_name, 0, i_len, &gen_dir);

    // try to keep these in order from smallest to largest to maybe avoid buffer overflow bugs
    _write(project_folder, "/src",          i_len, 4,     &gen_dir);
    _write(project_folder, "/Makefile",     i_len, 9,     cprojp ? &gen_makefile_c : &gen_makefile_cpp);
    _write(project_folder, "/.gitignore",   i_len, 11,    &gen_gitignore);
    _write(project_folder, "/src/Makefile", i_len, f_len, &gen_srcmakefile);
    
    gen_git_dir(project_name);

    free(project_folder);
}

int
main(int argc, char * argv[]) {
    int i = 1;
    int cprojp = 1;

    if (argc >= 2 && !strncmp(argv[i], "cpp", (size_t)3)) {
        i++;
        cprojp = 0;
    }
     
    if (argc >= 3 && !strncmp(argv[i], "init", (size_t)4)) {
        init_project(argv[i+1], cprojp);
    } else if (argc >= 2 && !strncmp(argv[i], "makefile", (size_t)8)) {
        if (cprojp)
            puts(MAKEFILE(MAKEFILE_C));
        else
            puts(MAKEFILE(MAKEFILE_CPP));
    } else {
        puts(HELP_MESSAGE);
        return 1;
    }
}
