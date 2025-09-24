// Copyright (c) 2025 ShivankSharma. All Rights Reserved.
#pragma once
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

char * get_input();
char **parse_input(char *str);
int command_execute(char **comand);

void loop();
