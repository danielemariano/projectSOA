#include <linux/module.h>
#include <linux/kernel.h>
#include "data_structures.h"
#include <linux/unistd.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include "util_tag.c"


struct tag *TAG_list = NULL;
struct level *level_list = NULL;
spinlock_t lock;
spinlock_t level_lock;
spinlock_t tag_lock;
spinlock_t receive_lock;
int signal_on = 0;
int total_tag = 0;


int tag_get(int key, int command, int permission){
    //Controllo se è gia istanziata quell'area di memoria, se è la prima volta
    //che chiamo questa call o se è stata fatta gia la free, verrà reistanziata
    if(TAG_list == NULL) {
        //libero questa memoria nella syscall filler allo smontaggio del modulo
        TAG_list = kmalloc(sizeof(struct tag) * MAX_TAG_NUMBER, GFP_KERNEL);
        if (TAG_list == NULL) {
            printk("errore nell'assegnazione dell'area di memoria dedicata ai tag");
            return -1;
        }
    }
    int i;
    if(command == 1) {
        //in questo caso devo creare un nuovo tag
        struct tag my_tag;
        my_tag.exist = 1;
        my_tag.permission = permission;

        if (TAG_list[key].exist != 1) {
            my_tag.tag_id = key;
            if (key == IPC_PRIVATE) {
                my_tag.private = 0;
            }
            my_tag.opened = 1;
            //sincronizzo la struttura dei tag
            spin_lock(&tag_lock);
            TAG_list[key] = my_tag;
            total_tag = total_tag + 1;
            printk("Hai creato un nuovo tag nella posizione %d", key);
            //ora creo la serie di livelli associati al tag di riferimento
            //libero questa memoria nella syscall filler allo smontaggio del modulo
            level_list = kmalloc(sizeof(struct level) * LEVELS, GFP_KERNEL);
            if (level_list == NULL) {
                printk("errore nell'assegnazione dell'area di memoria dedicata ai livelli per il tag: %d", i);
                spin_unlock(&tag_lock);
                return -1;
            }
            int lvl;
            //adesso assegno ad ogni livello il tag e il proprio livello appunto
            for (lvl = 0; lvl < LEVELS; lvl++) {
                level_list[lvl].tag = key;
                level_list[lvl].lvl = lvl;
                level_list[lvl].is_empty = 0;
                wait_queue_head_t wq;
                init_waitqueue_head(&wq);
                level_list[lvl].wq = wq;
                /*if(level_list[lvl].wq == NULL) {
                    printk(KERN_ERR "Unable to allocate new wait queue for replacement\n");
                    kfree(level_list[lvl].wq);
                    return -ENOMEM;
                }*/

            }
            TAG_list[key].structlevels = level_list;
            spin_unlock(&tag_lock);
            printk("ho inserito i livelli: %d", lvl);
        }
        else {
            printk("%d", TAG_list[key].exist);
            printk("il tag selezionato gia esiste\n");
            return -1;
        }
        if (key >= MAX_TAG_NUMBER) {
            printk("Inserire una key tra 0 e 255");
            return -1;
        }
        printk("key: %d",key);
        return key;
    }
    else if (command == 2){
        //in questo caso devo aprire un tag esistente
        if(TAG_list[key].exist==1) {
            if (key == IPC_PRIVATE) {
                printk("questo tag è stato creato come IPC_PRIVATE e non può essere riaperto\n");
                return -1;
            }
            else if(TAG_list[key].permission == permission || TAG_list[key].permission == 0) {
                TAG_list[key].opened = 1;
                return key;
            }
            else{
                printk("non hai i permessi per aprire questo tag\n");
                return -1;
            }
        }
        else{
            printk("questo tag non esiste\n");
            return -1;
        }
    }
    return 0;
}


int tag_send(int tag, int level, char *buffer, size_t size){
    int ret;
    printk("questo è il tag: %d", tag);
    if(tag < 0 || tag > MAX_TAG_NUMBER){
        printk("purtroppo il valore del tag %d inserito non è valido \n", tag);
        return -1;
    }
    if(TAG_list[tag].exist != 1){
        printk("il tag inserito non esiste, send\n");
        return -1;
    }
    if(TAG_list[tag].opened != 1){
        printk("il servizio di tag è chiuso\n");
        return -1;
    }
    if(TAG_list[tag].structlevels[level].reader < 1){
        printk("nessuno aspetta il messaggio nel livello %d, del tag %d, il messaggio è stato scartato", level,tag);
        return -1;
    }
    spin_lock(&lock);
    printk("SEND: questo è il buffer prima della copy from user: %s", buffer);
    ret = copy_from_user(TAG_list[tag].structlevels[level].bufs,buffer, 10*sizeof(char));
    printk("SEND: dopo la copy from user il buffer del tag %d è: %s",tag, TAG_list[tag].structlevels[level].bufs);
    if(ret < 0){
        printk("errore nella copy from user");
        return -1;
    }
    printk("SEND: tolgo il lucchetto, ho finito di scrivere");
    spin_unlock(&lock);
    printk("SEND: questo il buffer dopo la copia: %s", TAG_list[tag].structlevels[level].bufs);
    __sync_fetch_and_add(&TAG_list[tag].structlevels[level].is_empty,1);
    wake_up_interruptible(&(TAG_list[tag].structlevels[level].wq));
    return 0;
}


int tag_receive(int tag, int level, char *buffer, size_t size) {
    int wait;
    __sync_fetch_and_add(&TAG_list[tag].structlevels[level].reader,1);
    spin_lock(&receive_lock);
    printk("RECEIVE, questo il buff per controllare se è NULL: %s",TAG_list[tag].structlevels[level].bufs);
    if(TAG_list[tag].structlevels[level].bufs == NULL){
        TAG_list[tag].structlevels[level].bufs = kmalloc(MSG_MAX_SIZE,GFP_KERNEL);
        if(TAG_list[tag].structlevels[level].bufs== NULL){
            printk("errore nella kmalloc");
            spin_unlock(&receive_lock);
            return -1;
        }
    }
    spin_unlock(&receive_lock);
    //aumento il contatore dei lettori di 1 per ogni chiamata relativa a quel livello di quel tag
    printk("RECEIVE: is_empty: %d, reader: %d", TAG_list[tag].structlevels[level].is_empty, TAG_list[tag].structlevels[level].reader);
    wait = wait_event_interruptible(TAG_list[tag].structlevels[level].wq,TAG_list[tag].structlevels[level].is_empty != 0);
    if(wait < 0){
        printk("errore nella wait_event_interruptible\n");
        return -1;
    }
    if(wait == -ERESTARTSYS){
        printk("Ricevuto un segnale, fallito nell'attesa del messaggio\n");
        TAG_list[tag].structlevels[level].reader --;
        return -1;
    }
    signal_on = 0;
    wait = 0;
    if(wait == 0){
        //sincro della RCU
        rcu_read_lock();
        //diminuisco il contatore dei lettori di 1 per ogni chiamata relativa a quel livello di quel tag
        __sync_fetch_and_add(&TAG_list[tag].structlevels[level].reader,-1);
        int ret;
        if(buffer,TAG_list[tag].structlevels[level].bufs == NULL){
            printk("il buffer di lettura è vuoto");
            goto here;
        }
        printk("prima della copy to user il buffer di tag: %d level: %d: %s",tag, level, TAG_list[tag].structlevels[level].bufs);
        ret = copy_to_user(buffer,TAG_list[tag].structlevels[level].bufs,min(size,MSG_MAX_SIZE));
        printk("questo il buffer dopo la copy_to_user: %s", buffer);
        if(ret < 0){
            rcu_read_unlock();
            printk("errore nella copy to user");
            return -1;
        }
        here:
        //decremento il contatore che sblocca le wait
        __sync_fetch_and_add(&TAG_list[tag].structlevels[level].is_empty,-1);
        if(TAG_list[tag].structlevels[level].reader < 1){
            //cancello il messaggio dopo che tutti lo hanno letto
            kfree(TAG_list[tag].structlevels[level].bufs);
            printk("eseguita la kfree del buffer appartente al tag: %d del livello: %d", tag,level);
            *TAG_list[tag].structlevels[level].bufs = NULL;
            printk("Metto il puntatore del buffer a NULL");
        }
        rcu_read_unlock();
    }
    return wait;
}


int tag_ctl(int tag, int command) {
    if(command == 1){
        char *s;
        //libero a riga 243
        s = kmalloc(sizeof(char) * 17,GFP_KERNEL);
        if(s == NULL){
            printk("errore nella kmalloc dell'awake_all");
            return -1;
        }
        s = "AWAKE ALL THREADS";
        int i,j;
        for(i = 0; i< MAX_TAG_NUMBER; i++){
            if(TAG_list[i].exist){
                for(j = 0; j < LEVELS; j++){
                    if(TAG_list[i].structlevels[j].is_queued = 1){
                        *TAG_list[i].structlevels[j].bufs = s;
                        //awake all
                        wake_up_interruptible(&TAG_list[i].structlevels[j].wq);
                    }
                }
            }
        }
        kfree(s);
    }
    else if (command == 2){
        printk("sto entrando nella remove dei tag");
        if(TAG_list==NULL){
            printk("non esiste ancora la lista di tag");
            return -1;
        }
        if(tag < 0 || tag > MAX_TAG_NUMBER){
            printk("purtroppo il valore del tag %d inserito non è valido \n", tag);
            return -1;
        }
        if(TAG_list[tag].exist != 1){
            printk("il tag inserito non esiste\n");
            return -1;
        }
        if(search_for_level(TAG_list[tag])){
            TAG_list[tag] = delete_tag(TAG_list[tag]);
            printk("questo l'exist del tag %d dopo la remove: %d",tag, TAG_list[tag].exist);
        }
        else{
            printk("non si può chiudere il tag\n");
        }
    }
    return 0;
}