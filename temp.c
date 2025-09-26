#include<string.h>
#include<stdio.h>

int main() {    
    char i[] = "ls -la; cat main.c";
    const char *seperator = ";";
    char * token;
    token = strtok(i, seperator);
    while(token != NULL) {
        token = strtok(NULL, ";");
   t 
    return 0;
}
