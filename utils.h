// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern struct sigaction sa;
typedef struct command command_t;
typedef struct fdPair fdpair_t;
typedef struct file file_t;

void sigint_handler(int signal);
char *get_input();
void printcommand(command_t **tokens);
command_t **parseInputToCommands(char *input);
char **parseCommandIntoTokens(char *str, char *seperator);
void setReadPipe(int *readPipe);
void setWritePipe(int *writePipe);
int command_execute(char **command, fdpair_t *fdpair);
void resolve_env(char **command);
int run_commands(command_t **commands);
int runPipedCommands(char **command);
int handleSubShell(command_t *command);
char *extractOneWord(char *str);
void pushfdpair(fdpair_t *arr, int a, int b);
void printfdpair(fdpair_t *arr);

fdpair_t *newfdpair(int initialCapacity);

int handleRedirection(command_t *command);

void loop();
