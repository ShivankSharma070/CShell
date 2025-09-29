#include "utils.h"
#include "builtins.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#define SHELL_NAME "cshell"
#define LINE_BUFSIZE 10
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
    if (c == '\\') {
      if ((c = getchar()) == '\n') {
        printf("\t >\t");
        continue;
      }
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

typedef struct command command_t;

typedef struct command {
  char *data;
  char *sep_operator;
} command_t;

void printcommands(command_t **tokens) {
  int count = 0;
  while (tokens[count] != NULL) {
    printf("%s with operator %s\n", tokens[count]->data,
           tokens[count]->sep_operator);
    count++;
  }
  printf("\n");
}

char **parseCommandIntoTokens(char *input, char *seperator) {
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

void setReadPipe(int *readPipe) {
  if (readPipe == NULL)
    return;
  dup2(*readPipe, STDIN_FILENO);
  close(*readPipe);
  close(*(readPipe + 1));
}

void setWritePipe(int *writePipe) {
  if (writePipe == NULL)
    return;
  dup2(*writePipe, STDOUT_FILENO);
  close(*writePipe);
  close(*(writePipe - 1));
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

// Parses a char * into commands by spilitting them with ||, && ,;
command_t **parseInputToCommands(char *input) {
  command_t **commands = malloc(sizeof(command_t *) * 20);
  char *start = input;
  int depth = 0;
  char *p = input;
  int count = 0;

  while (*p) {
    // Handle parenthesis
    if (*p == '(')
      depth++;
    else if (*p == ')')
      if (depth > 0)
        depth--;

    if (depth == 0) {
      int seperator_len = 0;
      char *sep;
      if (p[0] == ';') {
        seperator_len = 1;
        sep = ";";
      } else if (p[0] == '&' && p[1] == '&') {
        seperator_len = 2;
        sep = "&&";
      } else if (p[0] == '|' && p[1] == '|') {
        seperator_len = 2;
        sep = "||";
      }

      if (seperator_len > 0) {
        size_t len = p - start;
        command_t *command = malloc(sizeof(command_t));
        command->data = strndup(start, len);
        command->sep_operator = sep;
        commands[count++] = command;
        p += seperator_len;
        start = p;
      }
    }
    p++;
  }

  if (p > start) {
    size_t len = p - start;
    command_t *command = malloc(sizeof(command_t));
    command->data = strndup(start, len);
    command->sep_operator = "";
    commands[count++] = command;
  }

  commands[count] = NULL;

  return commands;
}

int runPipedCommands(char **commands) {
    // TODO: refactor this function to use command_execute() 
  int fd_input = STDIN_FILENO;
  int status = 1;
  for (int i = 0; commands[i] != NULL; i++) {
    int pipefd[2];
    char **tokens = parseCommandIntoTokens(commands[i], TOKEN_SEP);
    resolve_env(tokens);
    if (commands[i + 1] != NULL) {
      pipe(pipefd);
    }
    if (fork() == 0) {
      // child process
      if (fd_input != STDIN_FILENO) {
        dup2(fd_input, STDIN_FILENO);
        close(fd_input);
      }
      if (commands[i + 1] != NULL) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
      }

      if (execvp(tokens[0], tokens) < 0) {
        perror("cshell");
        exit(EXIT_FAILURE);
      }
    } else {
      // parent process
      if (fd_input != STDIN_FILENO)
        close(fd_input);
      if (commands[i + 1] != NULL) {
        close(pipefd[1]);
        fd_input = pipefd[0]; // next read from here
      }
    }
  }

  for (int i = 0; commands[i] != NULL; i++) {
    wait(NULL);
  }
  return status;
}

int run_commands(command_t **commands) {
  int status = 1;
  char **tokens;
  for (int i = 0; commands[i] != NULL; i++) {
    command_t *command = commands[i];
    // If command is to be run in a subshell
    if (strchr(command->data, '(') && strchr(command->data, ')')) {
      char *first = strchr(command->data, '(');
      char *last = strchr(command->data, ')');
      char *data = strndup(first + 1, last - first - 1);
      pid_t pid = fork();
      if (pid == 0) {
        int status = run_commands(parseInputToCommands(data));
        exit(status);
      } else {
        do {
          waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      }

      status = (WEXITSTATUS(status) == 100) ? 0 : WEXITSTATUS(status);

    } else if (strchr(command->data, '|')) {
      // If command is piped command
      tokens = parseCommandIntoTokens(command->data, "|");
      status = runPipedCommands(tokens);

    } else {
      // If normal command is to be executed
      tokens = parseCommandIntoTokens(command->data, TOKEN_SEP);
      resolve_env(tokens);
      status = command_execute(tokens);
    }

    // Run next command according to what seperates those commands
    if (strcmp(command->sep_operator, "&&") == 0) {
      if (status != EXIT_SUCCESS)
        break;
    } else if (strcmp(command->sep_operator, "||") == 0) {
      if (status == EXIT_SUCCESS)
        break;
    }
  }
  return status;
}

void loop() {
  char *prompt = "";
  char *line;
  int status = 1;
  command_t **commands;

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
    if (!line) {
      return;
    }

    commands = parseInputToCommands(line);
    status = run_commands(commands);
  } while (status != 100);

  free(line);
  free(commands);
}
