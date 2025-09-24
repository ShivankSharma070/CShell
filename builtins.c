#include "builtins.h"
#include <stdio.h>
#include <unistd.h>

int execute_cd(char **args) {
    if(args[1] == NULL) {
        fprintf(stderr, "error: expected arguments for \"cd\"\n");
    } else {
        if(chdir(args[1]) != 0 ) {
            perror("chshell");
        }
    }
    return 1;
}

int execute_exit(char **args) {
    return 0;
}

int execute_help(char **args) {
    return 1;
}

char *builtins[] = {"cd", "exit", "help"};

int (*builtins_func[])(char **) = {&execute_cd, &execute_exit, &execute_help};

size_t num_builtins() {
    return sizeof(builtins)/sizeof(char*);
}
