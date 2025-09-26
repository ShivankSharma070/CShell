// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sigint_handler(int signal);
char *get_input();
char **parse_input(char *str, char *seperator);
int command_execute(char **command);
void resolve_env(char **command);

extern struct sigaction sa;

void loop();
