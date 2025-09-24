#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#define SHELL_NAME "cshell"
#define LINE_BUFSIZE 1024
#define TOKEN_BUFSIZE 64

char *get_input() {
  int capacity = LINE_BUFSIZE;
  int c;
  int count = 0;
  // buffer to hold user input
  char *buffer = malloc(sizeof(char) * capacity);
  printf("%s $ ", SHELL_NAME);
  while (1) {
    c = getchar();
    if (c == '\n' || c == EOF) {
      // append \0 at end of input
      buffer[count] = '\0';
      return buffer;
    }

    buffer[count++] = c;
    // If input size exceds double the capacity
    if (count >= capacity) {
      capacity *= 2;
      buffer = realloc(buffer, sizeof(char) * capacity);
    }
  }
}

void printtokens(char **tokens) {
  int count = 0;
  while (tokens[count] != NULL) {
    printf("%s-", tokens[count++]);
  }
  printf("\n");
}

char **parse_input(char *input) {
  char *token;
  int bufsize = TOKEN_BUFSIZE;
  char **tokens = malloc(sizeof(char *) * bufsize);
  char *delimeter = " ";
  int count = 0;

  token = strtok(input, delimeter);
  while (token != NULL) {
    tokens[count++] = token;
    if (count >= bufsize) {
      bufsize *= 2;
      tokens = realloc(tokens, sizeof(char *) * bufsize);
    }

    token = strtok(NULL, delimeter);
  }

  tokens[count] = NULL;
  return tokens;
}

int command_execute(char **command) {
  pid_t pid = fork();
  int status;
  if (pid < 0) {
    perror("fork failed");
  } else if (pid == 0) {
    // child process
    if (execvp(command[0], command) < 0) {
      perror("cshell");
      exit(EXIT_FAILURE);
    }
  } else {
    // parent process
    waitpid(pid, &status, WUNTRACED);
  }
  return 1;
}

void loop() {
  char *prompt = "";
  char **command;
  char *line;
  while (1) {
    // read input
    line = get_input();

    // parse input
    command = parse_input(line);

    // exectue command
    command_execute(command);
  }

  free(line);
}
