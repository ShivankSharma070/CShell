#pragma once
#include <stdlib.h>

int execute_cd(char **args);
int execute_exit(char **args);
int execute_help(char **args);
int execute_exec(char **args);
int negate(char **args);
extern char *builtins[];

extern int (*builtins_func[])(char **);

size_t num_builtins(); 
