#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int command;
    printf("Scegli quale servizio vuoi utilizzare: \n \n \n");
    scanf("%d",&command);
    printf("Questa è la tua scelta: %d \n", command);
    syscall(command,134,136);
    return 0;
}
