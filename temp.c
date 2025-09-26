
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *data;
  char *sep_operator;
} token_t;

void printtokens(token_t **tokens) {
  int count = 0;
  while (tokens[count] != NULL) {
    printf("%s with operator %s\n", tokens[count]->data, tokens[count]->sep_operator);
    count++;
  }
  printf("\n");
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

  printtokens(tokens);
  return tokens;
}
int main() {
  parser("echo hello && (ls; pwd) || echo fail; echo done");
  return 0;
}
