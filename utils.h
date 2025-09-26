// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct token token_t;
void sigint_handler(int signal);
char *get_input();
char **parse_input(char *str, char *seperator);
int command_execute(char **command);
void resolve_env(char **command);
int run_command(token_t **parser);

extern struct sigaction sa;

void loop();
