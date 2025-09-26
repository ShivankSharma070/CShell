#include "builtins.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int execute_cd(char **args) {
  if (args[1] == NULL) {
    if (chdir(getenv("HOME")) != 0) {
      perror("chshell");
    }
  } else {
    if (chdir(args[1]) != 0) {
      perror("chshell");
    }
  }
  return EXIT_SUCCESS;
}

int execute_exit(char **args) { return 100; }

int execute_help(char **args) { return EXIT_SUCCESS; }

int execute_exec(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "exec: %s", "no arguments to exec");
    return EXIT_FAILURE;
  }
  args = args + 1;
  execvp(args[0], args);
  return EXIT_FAILURE;
}

int negate(char **args) {
    return !command_execute(args+1);
}

char *builtins[] = {"cd", "exit", "help", "exec", "!"};

int (*builtins_func[])(char **) = {&execute_cd, &execute_exit, &execute_help,
                                   &execute_exec, &negate};

size_t num_builtins() { return sizeof(builtins) / sizeof(char *); }
