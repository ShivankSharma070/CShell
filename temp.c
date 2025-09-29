#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

int main() {
    int pipefd[2];
    pid_t p1, p2;

    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(1);
    }

    if ((p1 = fork()) == 0) {
        // Child 1: "ls"
        dup2(pipefd[1], STDOUT_FILENO);  // stdout -> pipe write
        close(pipefd[0]);                // not needed
        close(pipefd[1]);                // close after dup2
        char *argv[] = {"cat", "temp.c", NULL};
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }

    waitpid(p1, NULL, 0);

    if ((p2 = fork()) == 0) {
        // Child 2: "wc -l"
        dup2(pipefd[0], STDIN_FILENO);   // stdin -> pipe read
        close(pipefd[1]);                // not needed
        close(pipefd[0]);                // close after dup2
        char *argv[] = {"grep", "main", NULL};
        execvp(argv[0], argv);
        perror("execvp");
        exit(1);
    }

    // Parent closes both ends
    close(pipefd[0]);
    close(pipefd[1]);

    waitpid(p2, NULL, 0);

    return 0;
}
