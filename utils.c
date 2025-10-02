#include "utils.h"
#include "builtins.h"
#include <ctype.h>
#include <fcntl.h>
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

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

static int jmp_set;
static sigjmp_buf env;
void sigint_handler(int signal) {
  if (jmp_set) {
    siglongjmp(env, 42);
  }
}

typedef struct command command_t;
typedef struct file file_t;
typedef struct fdPair fdpair_t;

typedef struct command {
  char *data;
  char *sep_operator;
} command_t;

typedef struct file {
  char *filename;
  int f_opts;
} file_t;

typedef struct fdPair {
  int (*data)[2];
  int size;
  int capacity;
} fdpair_t;

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

int command_execute(char **command, fdpair_t *fdpair) {
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
    if (fdpair != NULL) {
      for (int i = 0; i < fdpair->size; i++) {
        if (fdpair->data[i][0] != fdpair->data[i][1]) {
          dup2(fdpair->data[i][0], fdpair->data[i][1]);
          close(fdpair->data[i][0]);
        }
      }
    }
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
  int fd_input = STDIN_FILENO;
  int status = 1;
  for (int i = 0; commands[i] != NULL; i++) {
    int pipefd[2];
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

      command_t **wrapper = malloc(sizeof(command_t *));
      wrapper[0] = malloc(sizeof(command_t));
      wrapper[0]->data = commands[i];
      wrapper[0]->sep_operator = " ";
      wrapper[1] = NULL;
      status = run_commands(wrapper);
            free(wrapper);
      exit(status);
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

int handleSubShell(command_t *command) {
  int status;
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

  return (WEXITSTATUS(status) == 100) ? 0 : WEXITSTATUS(status);
}

char *extractOneWord(char *str) {
  if (str == NULL)
    return NULL;
  // eat all space
  while (isspace(*str)) {
    str++;
  };
  char *word = strndup(str, strchr(str, ' ') - str);
  return word;
}

fdpair_t *newfdpair(int initialCapacity) {
  fdpair_t *newfdpair = malloc(sizeof(fdpair_t));
  if (newfdpair == NULL)
    return NULL;
  newfdpair->data = malloc(initialCapacity * sizeof(int[2]));
  newfdpair->size = 0;
  newfdpair->capacity = initialCapacity;
  return newfdpair;
}

void pushfdpair(fdpair_t *arr, int a, int b) {
  if (arr->size >= arr->capacity) {
    arr->capacity += arr->capacity;
    arr->data = realloc(arr->data, arr->capacity * sizeof(int[2]));
  }
  arr->data[arr->size][0] = a;
  arr->data[arr->size][1] = b;
  arr->size++;
}

void printfdpair(fdpair_t *arr) {
  for (int i = 0; i < arr->size; i++)
    printf("%d %d\n", arr->data[i][0], arr->data[i][1]);
}

int handleRedirection(command_t *command) {
  int status = EXIT_SUCCESS;
  int count = 0;
  fdpair_t *fdpair = newfdpair(2);
  char *data = command->data;
  char *cmdToExecute = NULL;
  int readfd = STDIN_FILENO;
  int oldreadfd = 0;
  int writefd = STDOUT_FILENO;
  int oldwritefd = 1;
  file_t *readFile = NULL;
  file_t *writeFile = NULL;
  while (data[count] != '\0') {
    if (cmdToExecute == NULL &&
        (data[count + 1] == '<' || data[count + 1] == '>')) {
      if (isdigit(data[count]))
<<<<<<< HEAD
        cmdToExecute = strndup(data, count);
      else
        cmdToExecute = strndup(data, count + 1);
    }
=======
        cmdToExecute = strndup(data, count); // exclude that integer from actual command
      else
        cmdToExecute = strndup(data, count + 1);
    }

>>>>>>> dev
    if (data[count] == '<') {
      readFile = malloc(sizeof(file_t));
      if (data[count - 1] != ' ' && isdigit(data[count - 1])) {

        oldreadfd = data[count - 1] - '0';
      }
      if (data[count + 1] == '>') {
        count++;
        readFile->f_opts = O_CREAT | O_RDWR;
      } else {
        readFile->f_opts = O_RDONLY;
      }
      readFile->filename = extractOneWord(data + count + 1);
    } else if (data[count] == '>') {
      writeFile = malloc(sizeof(file_t));
      if (data[count - 1] != ' ' && isdigit(data[count - 1])) {
        oldwritefd = data[count - 1] - '0';
      }
      if (data[count + 1] == '>') {
        count++;
        writeFile->f_opts = O_CREAT | O_APPEND | O_WRONLY;
      } else {
        writeFile->f_opts = O_CREAT | O_TRUNC | O_WRONLY;
      }
      writeFile->filename = extractOneWord(data + count + 1);
    }
    count++;
  }

  if (readFile) {
    readfd = open(readFile->filename, readFile->f_opts);
    if (readfd < 0)
      return EXIT_FAILURE;
    pushfdpair(fdpair, readfd, oldreadfd);
  }
  if (writeFile) {
    writefd = open(writeFile->filename, writeFile->f_opts, 0644);
    pushfdpair(fdpair, writefd, oldwritefd);
  }

  char **tokens = parseCommandIntoTokens(cmdToExecute, TOKEN_SEP);
  resolve_env(tokens);
  status = command_execute(tokens, fdpair);

  if (readFile)
    close(readfd);
  if (writeFile)
    close(writefd);

  free(readFile);
  free(writeFile);
  free(cmdToExecute);
  /* free(tokens); */
  return status;
}

int run_commands(command_t **commands) {
  int status = 1;
  char **tokens;
  for (int i = 0; commands[i] != NULL; i++) {
    command_t *command = commands[i];
    if (strchr(command->data, '(') && strchr(command->data, ')')) {
      // Subshell Handling
      status = handleSubShell(command);
    } else if (strchr(command->data, '|')) {
      // Piped Commands
      tokens = parseCommandIntoTokens(command->data, "|");
      status = runPipedCommands(tokens);
    } else if (strchr(command->data, '<') || strchr(command->data, '>')) {
      // Redirection Commands
      status = handleRedirection(command);
    } else {
      // Normal Command
      tokens = parseCommandIntoTokens(command->data, TOKEN_SEP);
      resolve_env(tokens);
      status = command_execute(tokens, NULL);
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
    // Set jump point for SIGINT to jump to gnal_handler
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
