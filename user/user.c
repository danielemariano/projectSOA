#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//Implementare i frontend delle funzioni
int tag_get(int key, int command, int permission){
    return syscall(134, key, command, permission);
}

int tag_send(int tag, int level, char *buffer, size_t size){
    return syscall(174, tag, level, buffer, size);
}

int tag_receive(int tag, int level, char *buffer, size_t size){
    return syscall(182, tag, level, buffer, size);
}

int main(int argc, char *argv[]){
    int command;
    int ret;
    printf("Scegli quale servizio vuoi utilizzare: \n \n \n");
    scanf("%d",&command);
    //CHIEDERE ALL'UTENTE DI METTERE UNA CHIAVE DA 0 A 255, CON IPC PRIVATE per renderlo non piu apribile
    printf("questa è la tua scelta: %d \n", command);
    char *s = malloc(256);
    if(s == NULL){
        fprintf(stderr, "errore nella malloc\n");
        return 1;
    }
    ret = tag_get(12, 1, 0);
    printf("questo il risultato: %d\n", ret);
    ret = tag_send(12, 3,"ciao mamma ti voglio bene", 10);
    printf("questa è ret: %d", ret);
    tag_receive(11,3,s,10);
    ret = tag_get(11, 1, 0);
    tag_receive(11,3,s,10);
    ret = tag_send(11, 3,"ciao mamma ti voglio bene", 10);
    ret = tag_send(11, 4,"ciao mamma ti voglio bene", 10);
    printf("questo il messaggio: %s", s);
    return 0;


}