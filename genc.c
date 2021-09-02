#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <git2.h>

#define MAKE_SOURCE_FILE(str, folder_name, file_name)   \
    strcat(str, folder_name);                           \
    strcat(str, "/");                                   \
    strcat(str, file_name)

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

FILE *
gen_file(char * filepath) {
    FILE * new_file = fopen(filepath, "w");

    if (new_file == NULL) {
        fprintf(stderr, "Cannot open file");
        exit(1);
    }

    return new_file;
}

void
gen_makefile(char * project_name) {
    char makefilepath[255] = "";
    strcat(makefilepath, project_name);
    strcat(makefilepath, "/Makefile");
    
    FILE * makefile_fp = gen_file(makefilepath);
    fprintf(makefile_fp,
            "PROGRAM = %s\n"
            "CC = gcc\n"
            "CFLAGS = -Wall -Werror -Wextra -Wpedantic -Wformat=2 -Wformat-overflow=2 -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 -Wtraditional-conversion -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Wconversion -Warith-conversion -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wformat-signedness -Wshadow -Wstrict-overflow=4 -Wundef -Wstrict-prototypes -Wswitch-default -Wswitch-enum -Wstack-usage=1000000 -Wcast-align=strict -D_FORTIFY_SOURCE=2 -fstack-protector-strong -fstack-clash-protection -fPIE -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code -pipe -O2\n"
            "DEBUGFLAGS = -fsanitize=address -fsanitize=pointer-compare -fsanitize=pointer-subtract -fsanitize=leak -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=bounds-strict -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow $(shell export ASAN_OPTIONS=strict_string_checks=1:detect_stack_use_after_return=1:check_initialization_order=1:strict_init_order=1:detect_invalid_pointer_pairs=2) -fanalyzer\n"
	    "LDFLAGS = \n"
	    "\n"
	    "all: $(PROGRAM)\n"
	    "$(PROGRAM): Makefile $(PROGRAM).c ; $(CC) $(PROGRAM).c -o $(PROGRAM) $(CFLAGS) $(DEBUGFLAGS) $(LDFLAGS)\n"
	    "release: Makefile $(PROGRAM).c ; $(CC) $(PROGRAM).c -o $(PROGRAM) $(CFLAGS) $(LDFLAGS)\n"
	    "run: ; ./$(PROGRAM) $@\n"
            "install: ; $(shell ln -s ./$(PROGRAM) ~/.local/bin/)\n"
            "uninstall: ; $(shell rm -f ~/.local/bin/$(PROGRAM)\n"
	    , project_name);
    fclose(makefile_fp);
}

/*
  char * 
  generate_a_string() {
  size_t size = 4;
  char * str = (char *)malloc(sizeof(char) * size); // stored in the heap, just remember to clean up when you're done!
  str = "abc\0";
  return str;
  }
*/

void
gen_src(char * project_name, const char * source) {
    char mainfilepath[255] = "";
    MAKE_SOURCE_FILE(mainfilepath, project_name, project_name);
    strcat(mainfilepath, ".c");
    
    FILE * mainfile_fp = gen_file(mainfilepath);

    if (!strcmp(source, MAINFILE)) {
        fputs(source, mainfile_fp);
    } else {
        FILE * source_fp = fopen(source, "r");

        // probably fine
        char buffer[1000];
        while (fgets(buffer, (int)sizeof * buffer, source_fp) != NULL) {
            fputs(buffer, mainfile_fp);
        }

        fclose(source_fp);
    }

    fclose(mainfile_fp);
}

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

    char gitignore[255] = "";
    strcat(gitignore, project_name);
    strcat(gitignore, "/.gitignore");
    
    FILE * gitignore_fp = gen_file(gitignore);
    fprintf(gitignore_fp, "%s\n",project_name);
    fclose(gitignore_fp);

    // Must free values that are filled in for you
    git_repository_free(repo);
    git_libgit2_shutdown();
}

void
init_project(char * project_name, char * source_file) {
    // Generate project directory
    int exists = mkdir(project_name, 0777);

    if (exists) {
        fprintf(stderr, "Directory already exists\n");
        exit(1);
    }

    gen_makefile(project_name);

    if (source_file == NULL) {
        gen_src(project_name, MAINFILE);
    } else {
        gen_src(project_name, source_file);
    }
    
    gen_git_dir(project_name);
}

// this function is VERY inefficient
void rename_project(char * old_project_name, char * new_project_name) {
    char old_filepath[255] = "";
    MAKE_SOURCE_FILE(old_filepath, new_project_name, old_project_name);
    strcat(old_filepath, ".c");
    char new_filepath[255] = "";
    MAKE_SOURCE_FILE(new_filepath, new_project_name, new_project_name);
    strcat(new_filepath, ".c");

    // Rename folder
    int folder_fail = rename(old_project_name, new_project_name);
    
    // Rename source file
    int file_fail = rename(old_filepath, new_filepath);

    // Rename binary
    char old_binary[255] = "";
    MAKE_SOURCE_FILE(old_binary, new_project_name, old_project_name);
    char new_binary[255] = "";
    MAKE_SOURCE_FILE(new_binary, new_project_name, new_project_name);
    FILE * fp = fopen(old_binary, "r");
    if (fp != NULL) {
        rename(old_binary, new_binary);
        fclose(fp);
    }

    if (folder_fail || file_fail) {
        fprintf(stderr,
                "Could not rename file or directory\n"
                "Please ensure you are not in the project folder\n");
        exit(1);
    }

    char makefile[255] = "";
    strcat(makefile, new_project_name);
    strcat(makefile, "/Makefile");
    FILE * makefile_fp = fopen(makefile, "r");

    char new_makefile[255] = "";
    strcat(new_makefile, ".tmp");
    FILE * makefile_new_fp = fopen(new_makefile, "w");

    char buffer[255] = "";
    
    int count = 0;
    while (fgets(buffer, (int)sizeof * buffer, makefile_fp) != NULL) {
	count++;

	if (count == 1) {
	    if (strncmp(buffer, "PROGRAM = ", (size_t)10)) {
		fprintf(stderr, "Your Makefile must start with \"PROGRAM = <program_name>\"\n");
		exit(1);
	    }

	    fprintf(makefile_new_fp, "PROGRAM = %s\n", new_project_name);
	} else {
	    fputs(buffer, makefile_new_fp);
	}
    }

    fclose(makefile_new_fp);
    fclose(makefile_fp);
    remove(makefile);
    rename(new_makefile, makefile);
}

int
main(int argc, char * argv[]) {
    // Make folders for debug/release binaries, like cargo
    // The ability to insert more than one custom source file
    if (!strcmp(argv[1], "init") && argc == 3) {
        init_project(argv[2], NULL);
    } else if (!strcmp(argv[1], "init") && argc == 4) {
        init_project(argv[2], argv[3]);
    } else if (!strcmp(argv[1], "rename") && argc == 4) {
        rename_project(argv[2], argv[3]);
    } else {
        printf("%s\n", HELP_MESSAGE);
        exit(1);
    }
}
