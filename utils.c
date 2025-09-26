#include "utils.h"
#include "builtins.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#define SHELL_NAME "cshell"
#define LINE_BUFSIZE 1024
#define TOKEN_BUFSIZE 64
#define TOKEN_SEP " \t\r\n\a"

static int jmp_set;
static sigjmp_buf env;
void sigint_handler(int signal) {
  if (jmp_set) {
    siglongjmp(env, 42);
  }
}

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
    if (c == EOF)
      return NULL;

    if (c == '\n') {
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
  return NULL;
}

void printtokens(char **tokens) {
  int count = 0;
  while (tokens[count] != NULL) {
    printf("%s-", tokens[count++]);
  }
  printf("\n");
}

char **parse_input(char *input, char *seperator) {
  char *token;
  int capacity = TOKEN_BUFSIZE;
  char **tokens = malloc(sizeof(char *) * capacity);
  if (tokens == NULL) {
    fprintf(stderr, "cshell: allocation erorr\n");
    return NULL;
  }

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
    return EXIT_SUCCESS;
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
    return 100; // stop the shell
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

  return WEXITSTATUS(status); // by default 0 -> success , 1 -> failure, but we
                              // are treating 0 -> failure 1->success
}

// replaces ENV variable in a command with their actuall value
void resolve_env(char **command) {
  for (int i = 0; command[i] != NULL; i++) {
    if (command[i][0] == '$') {
      char *env_var = malloc(sizeof(char) * strlen(command[i]));
      strncpy(env_var, command[i] + 1, strlen(command[i]) - 1);
      env_var[strlen(command[i]) - 1] = '\0';
      char *env_value = getenv(env_var);
      if (env_value) {
        command[i] = env_value;
      } else {
        command[i] = "";
      }
      free(env_var);
    }
  }
}

int run_command(char *line) {
  int semicolon = ';';
  char *and = "&&";
  char *or = "||";
  char **commands;
  int status = EXIT_SUCCESS;
  if (strchr(line, semicolon)) {
    commands = parse_input(line, ";");
    for (int i = 0; commands[i] != NULL; i++) {
      status = run_command(commands[i]);
    }
    return status;
  }
  if (strstr(line, and)) {
    commands = parse_input(line, and);
    for (int i = 0; commands[i] != NULL && status == EXIT_SUCCESS; i++) {
      status = run_command(commands[i]);
    }
    return status;
  }

  if (strstr(line, or)) {
    commands = parse_input(line, or);
    status = EXIT_FAILURE;
    for (int i = 0; commands[i] != NULL && status == EXIT_FAILURE; i++) {
      status = run_command(commands[i]);
    }
    return status;
  }

  commands = parse_input(line, " ");
  resolve_env(commands);
  status = command_execute(commands);
  return status;
}

void loop() {
  char *prompt = "";
  char *line;
  int status = 1;

  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGINT, &sa, NULL);
  do {
    // Set jump point for SIGINT signal_handler to jump to
    if (sigsetjmp(env, 1) == 42) {
      printf("\n");
    }
    jmp_set = 1;

    // read input
    line = get_input();
    if (!line)
      return;

    status = run_command(line);

  } while (status != 100);

  free(line);
}
