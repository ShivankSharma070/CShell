#include<stdio.h>
#include <string.h>

int main() {
    char string[]= " first (Hello world) second";
    char * token;
    while(token != NULL) {
        printf("%s\n", token);
    token = strtok(NULL, "()");
    }
    
    return 0;
}
