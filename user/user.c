#include <user.h>

/**
 * Frontend della tag_get
 * */
int tag_get(int key, int command, int permission){
    return syscall(134, key, command, permission);
}

/**
 * Frontend della tag_send
 * */
int tag_send(int tag, int level, char *buffer, size_t size){
    int ret;
    ret = syscall(174, tag, level, buffer, size);
    if(ret < 0){
        printf("Errore nella syscall: %d\n", ret);
        return ret;
    }
    return ret;
}

/**
 * Frontend della tag_receive
 * */
int tag_receive(int tag, int level, char *buffer, size_t size){
    return syscall(182, tag, level, buffer, size);
}

/**
 * Frontend della tag_ctl
 * */
int tag_ctl(int tag, int command){
    return syscall(183,tag,command);
}

/**
 * Prima suite di test
 * */
void test_create_open_remove_tag(){
    /*
     * Testa la creazione un numero di tag (1)
     * Testa la logica IPC_PRIVATE (2)
     * Testa la rimozione di un numero di tag (3)
     * Testa l'apertura di un tag rimosso e quindi rimuovilo nuovamente (4)
     */

    int ret;

    // crea n tag (1)
    for (int i=0; i<256; i++){
        ret = tag_get(i, 1, 0);
        printf("Creato correttamente il tag con key: %d.\n", ret);
    }

    // apri il tag IPC_PRIVATE [0] (2)
    ret = tag_get(0, 1, 0);
    if (ret < 0)
        printf("Il tag con key: %d è IPC_PRIVATE.\n", 0);

    // rimuovi n tag (3)
    for (int i=0; i<256; i++){
        ret = tag_ctl(i, REMOVE);
        if (ret == 0)
            printf("Rimosso correttamente il tag con key: %d.\n", i);
    }

    // apri un tag rimosso e successivamente rimuovilo (4)
    ret = tag_get(100, 2, 0);
    printf("Riaperto correttamente il tag con key: %d.\n", ret);
    ret = tag_ctl(100, REMOVE);
    if (ret == 0)
        printf("Rimosso correttamente il tag con key 100.\n");
}

/**
 * Routine eseguita da ogni thread nella suite del test per il device driver
 * */
void * the_thread(void* path){
    char* device;
    int fd, ret;
    char mess[MSG_MAX_SIZE];
    device = (char*) path;
    fd = open(device, O_RDWR);
    if(fd < 0) {
        printf("Error nell'apertura del device %s.\n",device);
        pthread_exit(EXIT_SUCCESS);
    }
    fprintf(stdout, "Device %s aperto.\n",device);
    size_t len = MSG_MAX_SIZE;
    ret = read(fd,mess,len);
    if (ret < 0)
        printf("Errore nella read del device driver.\n");
    else
        printf(stdout, "Questo è presente nel tag attualmente \n%s\n.", mess);
    close(fd);
    pthread_exit(EXIT_SUCCESS);
}

/**
 * Suite di test per il device driver
 * */
void test_device_driver(){
    char *path = "/dev/driver";
    char buff[MSG_MAX_SIZE];
    int i;
    int ret;
    int tag_array[MINOR];
    pthread_t tid[MINOR];
    for(i = 0; i < MINOR; i ++){
        tag_array[i] = tag_get(i,CREATE,0);
        if(tag_array[i] < 0){
            printf("Errore nella tag_get di test_device_driver.\n");
            return -1;
        }
    }
    printf("Sto creando i thread.\n");
    for(i = 0; i< MINOR; i++){
        fflush(buff);

        snprintf(buff,MSG_MAX_SIZE, "mknod %s%d c %d %d\n", path, i, MAJOR, i);
        printf("Sto per fare una system.\n");
        system(buff);
        fflush(buff);
        sprintf(buff,"%s%d",path, i);
        printf("buff %s\n", buff);
        pthread_create(&tid[i], NULL, the_thread, *buff);
        fflush(buff);
    }
    printf("Aspetto che terminino l'esecuzione tutti i thread");
    for(i = 0; i< MINOR; i++){
        pthread_join(&tid[i], NULL);
    }
    printf("Rimuovo i tag");
    for(i = 0; i< MINOR; i++){
        ret = tag_ctl(i, REMOVE);
    }
    sprintf(buff,"rm  %s*\n",path);
    system(buff);
    printf("Esecuzione terminata\n");
}

/**
 * Funzione di supporto per lo sviluppo della suite di test per lo scambio dei messaggi.
 * Questa funzione ritorna il valore del livello.
 * */
int test_waiting_for_message(int key){
    int level=1;
    char *return_buff;
    return_buff = malloc(MSG_MAX_SIZE);
    sleep(2);
    if(return_buff == NULL){
        printf("Errore nella malloc della test_waiting_for_message.\n");
        return -1;
    }
    clock_t start, end;
    double cpu_time_used;
    start = clock();
    int ret;
    sleep(2);
    ret = tag_receive(key, level, return_buff, MSG_MAX_SIZE);
    if(ret<0){
        printf("Errore nella tag receive, ret:%d\n", ret);
        return -1;
    }
    end = clock();
    cpu_time_used = ((double) (end - start));
    printf("Ho ricevuto il messaggio, ma ho dovuto aspettare %f\n", cpu_time_used);
    printf("Il messaggio è: %s\n", return_buff);
    free(return_buff);
    return level;
}

/**
 * Questa suite testa la cancellazione di un tag e proviamo a
 * riaprirlo per verificare l'effettiva riuscita dell'azione
 * */
int test_delete_and_open(int tag, int tid){
    int ret;
    int ctl;
    ret = tag_get(tag,OPEN,tid);
    if(ret < 0){
        printf("Il tag che stai cercando di aprire per il test delete and open non esiste\n");
        return -1;
    }
    ctl = tag_ctl(ret, REMOVE);
    if(ctl <0){
        printf("Non sono riuscito a cancellare il tag %d", ret);
        return -1;
    }
    ret = tag_get(tag,OPEN,tid);
    if(ret < 0){
        printf("Il risultato è atteso, per verificare meglio usare il comando seguente:\n sudo dmesg\n");
        return 0;
    }
    return 0;
}

/**
 * Funzione di supporto per lo sviluppo della suite di test per lo scambio dei messaggi.
 * */
int test_sending_message(int tag, int level, int tid){
    char *send_buf;
    int ret;
    send_buf = malloc(MSG_MAX_SIZE);
    if(send_buf == NULL){
        printf("errore nella malloc della test_sending_message\n");
        return -1;
    }
    ret = sprintf(send_buf,"messaggio di prova del thread %d al livello %d del tag %d\n", tid, level, tag);
    if(ret < 0){
        printf("errore nella sprintf della sendbuf\n");
        return -1;
    }
    printf("ho copiato il messaggio, ora invio %d bytes il messaggio è: %s\n", sizeof(send_buf), send_buf);
    ret = tag_send(tag,level,send_buf,sizeof(send_buf));
    if(ret < 0){
        printf("errore nella tag_send: %d\n", ret);
        return -1;
    }
    tag_ctl(0,1);
    return 0;
}

/**
 * Routine eseguita da ogni thread nella suite del test per
 * il funzionamento multithread del sottosistema
 * */
int test_multithread(void *i){
    pthread_t tid;
    tid = pthread_self();
    int int_tid = (int) tid;
    int ret;
    //apertura del tag
    printf("sto per aprire il tag: %d\n", i);
    sleep(1);
    ret = tag_get(i, 1, int_tid);
    if(ret < 0){
        printf("errore nella creazione del tag %d\n", i);
        return -1;
    }
    if(i <3){
        printf("creato il tag con key = %d\n", ret);
        ret = test_waiting_for_message(ret);
        if(ret< 0){
            printf("errore nella test_waiting_for_message\n");
            return -1;
        }
    }
    else{
        sleep(i+1);
        test_sending_message(1,1,0);
        printf("terminata l'esecuzione di test_sending_message\n");
        sleep(1);
    }
}

/**
 * Test suite per il testing delle funzionalità che
 * implementano lo scambio multithread dei messaggi
 * */
void test_create_multithread(){
    int i;
    int x;
    int rc;
    int status;
    pthread_t threads[NUM_THREADS];
    printf("sto per creare i thread\n");
    for( i = 0; i < NUM_THREADS; i++ ) {
        rc = pthread_create(&threads[i], NULL, test_multithread, (void *)i);
        if (rc) {
            printf("Error:unable to create thread, %d\n", rc);
            return -1;
        }
    }
    for(i=0; i < NUM_THREADS; i++){
        printf("sono il threads %d la posizione è: %d\n", pthread_self, i);
        rc=pthread_join(threads[i], (void**)&status);
        printf("Completed join with thread%d status= %d\n",i, status);
    }
    return 0;
}

/**
 * Ulteriore suite di test per la rimozione dei tag
 * */
int remove_for_test(){
    int i;
    int ret;
    for(i = 0; i < 5; i++){
        ret = tag_ctl(i, REMOVE);
        if(ret < 0){
            printf("Errore nella ctl per rimuovere tutti i tag.\n");
        }
    }
    return 0;
}

/**
 * Main da cui scegliamo quali e come test eseguire
 * */
int main(int argc, char *argv[]){
    printf("Eseguo i test di creazione e rimozione dei tag.\n");
    test_create_open_remove_tag();
    remove_for_test();
    printf("Eseguo il test del device driver.\n");
    test_device_driver();
    printf("Eseguo il test multithread.\n");
    test_create_multithread();
    return 0;
}