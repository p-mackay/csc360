#include <stdio.h>
#include <unistd.h>
int main() {

    printf("one\n");

    fork();

    printf("two\n");

    return 0;
}
