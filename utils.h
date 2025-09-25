// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void sigint_handler(int signal);
char *get_input();
char **parse_input(char *str);
int command_execute(char **comand);

extern struct sigaction sa;
extern sigjmp_buf env;
extern int jmp_set;

void loop();
