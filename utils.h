// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


extern struct sigaction sa;
typedef struct command command_t;
void sigint_handler(int signal);
char *get_input();
command_t **parseInputToCommands(char *input);
char **parseCommandIntoTokens(char *str, char *seperator);
void setReadPipe(int *readPipe);
void setWritePipe(int *writePipe);
int command_execute(char **command);
void resolve_env(char **command);
int runPipedCommands(char **command);
int run_command(command_t **parser);

void loop();
