#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void pushfd(int (*arr)[2], int data[2]) {
    int i;
    for(i = 0; arr[i]!= NULL; i++) ;
    arr[i][0] = data[0];
    arr[i][1] = data[1];
}

void print(int (*arr)[2]) {
    for(int i = 0; arr[i]!= NULL; i++) {
        printf("%d %d", arr[i][0], arr[i][1]);
    }
}
int main() {
    int (*arr)[2] = malloc(20 * sizeof(int[2]));

    print(arr);
    pushfd(arr, (int[]){1,2});
    pushfd(arr, (int []){1,2});
    pushfd(arr, (int []){1,2});

    return 0;
}
