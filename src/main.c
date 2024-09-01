#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <git2.h>

const char* HELP_MESSAGE =
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
    "CC := gcc\n"                                                       \
    "SRCS := $(wildcard $(SRC_DIR)/*.c)\n"
    
#define MAKEFILE_CPP                                            \
    "CC = g++\n"                                                \
    "SRCS := $(wildcard $(SRC_DIR)/*.cpp)\n"

// x is replaced with c/cpp specific stuff
#define MAKEFILE(x)                                                     \
    "# https://github.com/clemedon/makefile_tutor\n"                    \
    "# https://makefiletutorial.com\n"                                  \
    "\n"                                                                \
    "DEBUG := ./build/debug\n"                                          \
    "RELEASE := ./build/release\n"                                      \
    "\n"                                                                \
    "mode ?= debug\n"                                                   \
    "OBJ_DIR ?= $(DEBUG)\n"                                             \
    "SRC_DIR := src\n"                                                  \
    "\n"                                                                \
    x                                                                   \
    "\n"                                                                \
    "CFLAGS := -Wall\n"                                                 \
    "CPPFLAGS := -MMD -MP\n"                                            \
    "\n"                                                                \
    "ifeq ($(mode), debug)\n"                                           \
    "	CFLAGS += -g\n"                                                 \
    "else\n"                                                            \
    "	ifeq ($(mode), release)\n"                                      \
    "		CFLAGS += -O2\n"                                            \
    "		OBJ_DIR = $(RELEASE)\n"                                     \
    "	endif\n"                                                        \
    "endif\n"                                                           \
    "\n"                                                                \
    "OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)\n"                   \
    "DEPS := $(OBJS:.o=.d)\n"                                           \
    "\n"                                                                \
    "NAME := $(notdir $(CURDIR))\n"                                     \
    "EXECUTABLE := $(OBJ_DIR)/$(NAME)\n"                                \
    "\n"                                                                \
    "DIR_DUP = mkdir -p $(@D)\n"                                        \
    "\n"                                                                \
    "all: $(EXECUTABLE)\n"                                              \
    "\n"                                                                \
    "$(EXECUTABLE): $(OBJS)\n"                                          \
    "	$(CC) $^ -o $@\n"                                               \
    "\n"                                                                \
    "$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c\n"                                  \
    "	$(DIR_DUP)\n"                                                   \
    "	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<\n"                      \
    "\n"                                                                \
    "-include $(DEPS)\n"                                                \
    "\n"                                                                \
    ".PHONY: clean install uninstall run test\n"                        \
    "\n"                                                                \
    "clean:\n"                                                          \
    "	rm -f $(EXECUTABLE) $(OBJS) $(DEPS)\n"                          \
    "\n"                                                                \
    "install:\n"                                                        \
    "	cp $(RELEASE)/$(NAME) ~/.local/bin/\n"                          \
    "\n"                                                                \
    "uninstall:\n"                                                      \
    "	rm -f ~/.local/bin/$(NAME)\n"                                   \
    "\n"                                                                \
    "run:\n"                                                            \
    "	-$(EXECUTABLE)\n"

const char *GITIGNORE =
    "/build\n"
    "*~\n"
    "compile_commands.json\n"
    "/.cache\n";

const char *SRC_MAKEFILE =
    "all:\n"
    "	$(MAKE) -C .. $@\n"
    "%:\n"
    "	$(MAKE) -C .. $@\n";

void
gen_dir(const char *filepath) {
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
gen_file(const char *filepath, const char *contents) {
    FILE * fp = fopen(filepath, "w");
    
    if (fp == NULL) {
        fprintf(stderr, "Cannot open filepath: %s\n", filepath);
        exit(1);
    }

    fputs(contents, fp);
    fclose(fp);
}

void gen_makefile_c(const char *filepath) { gen_file(filepath, MAKEFILE(MAKEFILE_C)); }
void gen_makefile_cpp(const char *filepath) { gen_file(filepath, MAKEFILE(MAKEFILE_CPP)); }
void gen_gitignore(const char *filepath) { gen_file(filepath, GITIGNORE); }
void gen_srcmakefile(const char *filepath) { gen_file(filepath, SRC_MAKEFILE); }

void
gen_git_dir(const char *project_name) {
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
_write(char to[], const char from[], int offset, int length, void (*action)(const char *)) {
    for (int i = 0; i < length; i++)
        to[i + offset] = from[i];

    action(to);
}

void
init_project(const char *project_name, int cprojp) {
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
