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

typedef struct token token_t;

typedef struct token {
  char *data;
  char *sep_operator;
} token_t;

void printtokens(token_t **tokens) {
  int count = 0;
  while (tokens[count] != NULL) {
    printf("%s with operator %s\n", tokens[count]->data,
           tokens[count]->sep_operator);
    count++;
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

token_t **parser(char *input) {
  token_t **tokens = malloc(sizeof(token_t *) * 20);
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
        token_t *token = malloc(sizeof(token_t));
        token->data = strndup(start, len);
        token->sep_operator = sep;
        tokens[count++] = token;
        p += seperator_len;
        start = p;
      }
    }
    p++;
  }

  if (p > start) {
    size_t len = p - start;
    token_t *token = malloc(sizeof(token_t));
    token->data = strndup(start, len);
    token->sep_operator = "";
    tokens[count++] = token;
  }

  tokens[count] = NULL;

  return tokens;
}

int run_command(token_t **tokens) {
  int status = 1;
  char **command;
  for (int i = 0; tokens[i] != NULL; i++) {
    token_t *token = tokens[i];
    if (strchr(token->data, '(') && strchr(token->data, ')')) {
      char *first = strchr(token->data, '(');
      char *last = strchr(token->data, ')');
      char *data = strndup(first + 1, last - first - 1);
      pid_t pid = fork();
      if (pid == 0) {
        int status = run_command(parser(data));
        exit(status);
      } else {
        do {
          waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
      }

      status = (WEXITSTATUS(status) == 100)? 0 : WEXITSTATUS(status);
      if (strcmp(token->sep_operator, "&&") == 0) {
        if (status != EXIT_SUCCESS)
          break;
      } else if (strcmp(token->sep_operator, "||") == 0) {
        if (status == EXIT_SUCCESS)
          break;
      }
      continue;
    }

    command = parse_input(token->data, " ");
    status = command_execute(command);
    if (strcmp(token->sep_operator, "&&") == 0) {
      if (status != EXIT_SUCCESS)
        break;
    } else if (strcmp(token->sep_operator, "||") == 0) {
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
  token_t **tokens;

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

    tokens = parser(line);
    status = run_command(tokens);

  } while (status != 100);

  free(line);
}
