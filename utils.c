#include "utils.h"
#include "builtins.h"
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#define SHELL_NAME "cshell"
#define LINE_BUFSIZE 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_SEP " \t\r\n\a"

char *get_input() {
  int capacity = LINE_BUFSIZE;
  int c;
  int count = 0;
  // buffer to hold user input
  char *buffer = malloc(sizeof(char) * capacity);
  if (buffer == NULL) {
    fprintf(stderr, "cshell: allocation erorr\n");
    return NULL;
  }

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
      capacity += LINE_BUFSIZE;
      buffer = realloc(buffer, sizeof(char) * capacity);
      if (buffer == NULL) {
        fprintf(stderr, "cshell: allocation error");
        return NULL;
      }
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
  int capacity = TOKEN_BUFSIZE;
  char **tokens = malloc(sizeof(char *) * capacity);
  if (tokens == NULL) {
    fprintf(stderr, "cshell: allocation erorr\n");
    return NULL;
  }

  char *seperator = TOKEN_SEP;
  int count = 0;

  token = strtok(input, seperator);
  while (token != NULL) {
    tokens[count++] = token;
    if (count >= capacity) {
      capacity *= TOKEN_BUFSIZE;
      tokens = realloc(tokens, sizeof(char *) * capacity);
    }

    token = strtok(NULL, seperator);
  }

  tokens[count] = NULL;
  return tokens;
}

int command_execute(char **command) {
  if (command[0] == NULL) {
    // Empty command recieved.
    return 1;
  }
  for (int i = 0; i < num_builtins(); i++) {
    if (strcmp(command[0], builtins[i]) == 0) {
      return (*builtins_func[i])(command);
    }
  }

  pid_t pid = fork();
  int status;
  if (pid < 0) {
    perror("fork failed");
    return 0; // stop the shell
  } else if (pid == 0) {
    // child process
    if (execvp(command[0], command) < 0) {
      perror("cshell");
      exit(EXIT_FAILURE);
    }
  } else {
    // parent process
    // run the loop till the child terminates
    do {
      waitpid(pid, &status, WUNTRACED);
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    // WIFEXITED(status) --> true if child terminated normally
    // WIFSIGNALED(status) --> true if child was killed by a signal
  }

  return 1;
}

void loop() {
  char *prompt = "";
  char **command;
  char *line;
  int status;

  do {
    // read input
    line = get_input();

    // parse input
    command = parse_input(line);

    // exectue command
    status = command_execute(command);
  } while (status);

  free(line);
  free(command);
}
