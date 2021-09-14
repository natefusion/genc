#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <git2.h>

const char * HELP_MESSAGE =
    "C project generator\n"
    "\n"
    "USAGE: genc ...\n"
    "\n"
    "COMMANDS:\n"
    "    init <project_name>                         Create a new c project\n"
    "    init <project_name> <custom_source_file>    Create a new c project with your own source file\n"
    "\n"
    "EXAMPLES:\n"
    "    The following example creates a new c project in the current directory.\n"
    "    $ genc init my-new-project\n";

const char * MAINFILE =
    "#include <stdio.h>\n"
    "\n"
    "int\n"
    "main(void) {\n"
    "    printf(\"Hello world!\\n\");\n"
    "}\n";

const char * MAKEFILE =
    "PROJECT = $(notdir $(CURDIR))\n"
    "SRC = $(wildcard *.c)\n"
    "DEBUG = debug/$(PROJECT)\n"
    "RELEASE = release/$(PROJECT)\n"
    "CC = gcc\n"
    "CFLAGS = -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -pipe -O2\n"
    "LDFLAGS = \n"
    "\n"
    "all: debug\n"
    "\n"
    "debug: OUTPUT = $(DEBUG)\n"
    "# Non production ready flags (as of 2021-09-01), https://github.com/google/sanitizers/issues/1324: -fsanitize=pointer-compare -fsanitize=pointer-subtract\n"
    "debug: CFLAGS += -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow $(shell export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2) -fanalyzer\n"
    "debug: executable\n"
    "\n"
    "release: OUTPUT = $(RELEASE)\n"
    "release: executable\n"
    "\n"
    "executable: $(SRC)\n"
    "	$(CC) $(SRC) -o $(OUTPUT) $(CFLAGS) $(LDFLAGS)\n"
    "\n"
    ".PHONY: run clean install uninstall\n"
    "\n"
    "run:\n"
    "	./$(DEBUG)\n"
    "\n"
    "clean:\n"
    "	rm -f $(DEBUG) $(RELEASE)\n"
    "\n"
    "# installs from release folder only\n"
    "install:\n"
    "	ln -s $(CURDIR)/$(RELEASE) ~/.local/bin/\n"
    "\n"
    "uninstall:\n"
    "	rm -f ~/.local/bin/$(PROJECT)\n";

const char * GITIGNORE =
    "debug\n"
    "release\n";

void
gen_dir(char * filepath) {
    int exists = mkdir(filepath, 0777);

    if (exists) {
        fprintf(stderr, "File or directory could not be created: %s\n", filepath);
        exit(1);
    }
}

void
gen_file(char * filepath, const char * contents) {
    FILE * fp = fopen(filepath, "w");
    
    if (fp == NULL) {
        fprintf(stderr, "Cannot open filepath: %s\n", filepath);
        exit(1);
    }

    fputs(contents, fp);
    fclose(fp);
}

void gen_mainfile(char * filepath)  { gen_file(filepath, MAINFILE);  }
void gen_makefile(char * filepath)  { gen_file(filepath, MAKEFILE);  }
void gen_gitignore(char * filepath) { gen_file(filepath, GITIGNORE); }

void
gen_git_dir(char * project_name) {
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
_write(char * to, const char * from, int offset, int length, void (*action)(char *)) {
    for (int i = 0; i < length; i++)
        to[i + offset] = from[i];

    action(to);
}

void
init_project(char * project_name)  {
    int i_len = (int)strlen(project_name);
    int f_len = 11; // make sure this matches with the longest static string
    int len = i_len + f_len + 1;

    char * project_folder = (char *)malloc(sizeof(char) * (size_t)len);
    
    // To ensure a buffer overflow will not (maybe) happen
    for (int i = i_len; i < len; i++)
        project_folder[i] = '\0';

    _write(project_folder, project_name, 0, i_len, &gen_dir);
    gen_git_dir(project_name);    
    // try to keep these in order from smallest to largest to maybe avoid buffer overflow bugs
    _write(project_folder, "/debug",      i_len, 6,     &gen_dir);
    _write(project_folder, "/main.c",     i_len, 7,     &gen_mainfile);
    _write(project_folder, "/release",    i_len, 8,     &gen_dir);
    _write(project_folder, "/Makefile",   i_len, 9,     &gen_makefile);
    _write(project_folder, "/.gitignore", i_len, f_len, &gen_gitignore);


    free(project_folder);
}

void
rename_project(char * old_project_name, char * new_project_name) {
    int folder_fail = rename(old_project_name, new_project_name);

    if (folder_fail) {
        fprintf(stderr, "Could not rename project folder\n");
        exit(1);
    }
}

int
main(int argc, char ** argv) {
    if (argc == 3 && !strncmp(argv[1], "init", (size_t)4)) {
        init_project(argv[2]);
    } else if (argc == 4 && !strncmp(argv[1], "rename", (size_t)6)) {
        rename_project(argv[2], argv[3]);
    } else {
        puts(HELP_MESSAGE);
        exit(1);
    }
}
