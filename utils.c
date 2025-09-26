#include "utils.h"
#include "builtins.h"
#include <signal.h>
#include <stdio.h>
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
    if (c == EOF)  return NULL; 

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

void loop() {
  char *prompt = "";
  char **command;
  char *line;
  int status;

  struct sigaction sa;
  sa.sa_handler = sigint_handler;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);

  sigaction(SIGINT, &sa, NULL);
  do {
    if (sigsetjmp(env, 1) == 42) {
      printf("\n");
    }
    jmp_set = 1;

    // read input
    line = get_input();
    if (!line) return; 

    // parse input
    command = parse_input(line);
        if (!command) return;
    resolve_env(command);

    // exectue command
    status = command_execute(command);
  } while (status);

  free(line);
  free(command);
}
