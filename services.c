#include <linux/module.h>
#include <linux/kernel.h>
#include "data_structures.h"
#include <linux/unistd.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include "util_tag.c"
#include <linux/sched.h>



//possibilità di fare una lista collegata per ora accantonata
struct tag *TAG_list = NULL;
struct level *level_list = NULL;
spinlock_t lock;


int tag_get(int key, int command, int permission){
    //Controllo se è gia istanziata quell'area di memoria, se è la prima volta che chiamo questa call o se è stata fatta gia la free, verrà reistanziata
    if(TAG_list == NULL) {
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
        //Qui forse ci mettiamo un bel SEMAFORO
        if (TAG_list[key].exist != 1) {
            my_tag.tag_id = key;
            if (key == IPC_PRIVATE) {
                my_tag.private = 0;
            }
            my_tag.opened = 1;
            TAG_list[key] = my_tag;
            printk("Hai creato un nuovo tag nella posizione %d", key);
            //ora creo la serie di livelli associati al tag di riferimento
            level_list = kmalloc(sizeof(struct level) * LEVELS, GFP_KERNEL);
            if (level_list == NULL) {
                printk("errore nell'assegnazione dell'area di memoria dedicata ai livelli per il tag: %d", i);
                return -1;
            }
            int lvl;
            //adesso assegno ad ogni livello il tag e il proprio livello appunto
            for (lvl = 0; lvl < LEVELS; lvl++) {
                level_list[lvl].tag = key;
                level_list[lvl].lvl = lvl;
                level_list[lvl].is_empty = 0;
            }
            TAG_list[key].structlevels = level_list;
            printk("ho inserito i livelli: %d", lvl);
        }
        else {
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
        //in questo caso devo aprire un tag esistente, che vuol dire di preciso aprire un tag esistente?
        if(TAG_list[key].exist==1) {
            if (key == IPC_PRIVATE) {
                printk("questo tag è stato creato come IPC_PRIVATE e non può essere riaperto\n");
                return -1;
            }
            if(TAG_list[key].permission == permission || TAG_list[key].permission == 0) {
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
    if(tag < 0 || tag > MAX_TAG_NUMBER){
        printk("purtroppo il valore del tag %d inserito non è valido \n", tag);
        return -1;
    }
    printk("questo è il tag: %d", tag);
    if(TAG_list[tag].exist != 1){
        printk("il tag inserito non esiste, send\n");
        return -1;
    }
    if(TAG_list[tag].opened != 1){
        printk("il servizio di tag è chiuso\n");
        return -1;
    }
    spin_lock(&lock);
    int ret;
    if(TAG_list[tag].structlevels[level].reader < 1){
        printk("nessuno aspetta il messaggio nel livello %d, del tag %d, il messaggio è stato scartato", level,tag);
        spin_unlock(&lock)
        return -1;
    }
    if(true) {
        ret = copy_from_user(TAG_list[tag].structlevels[level].bufs, buffer, size);
        if(ret < 0){
            printk("errore nella copy from user");
            return -1;
        }
        TAG_list[tag].structlevels[level].is_empty = 1;
    }
    spin_unlock(&lock);
    return 0;
}

int tag_receive(int tag, int level, char *buffer, size_t size) {
    int wait;
    //aumento il contatore dei lettori di 1 per ogni chiamata relativa a quel livello di quel tag
    TAG_list[tag].structlevels[level].reader ++;
    TAG_list[tag].structlevels[level].is_queued = 1;
    wait = wait_event_interruptible(TAG_list[tag].structlevels[level].wq, TAG_list[tag].structlevels[level].is_empty == 0 );
    if(wait < 0){
        printk("errore nella wait_event_interruptible\n");
        return -1;
    }
    if(wait == 0){
        printk("il thread %d è stato messo in attesa di messaggi\n", current);
        //Qua mettere sincro della RCU
        rcu_read_lock();
        //diminuisco il contatore dei lettori di 1 per ogni chiamata relativa a quel livello di quel tag
        TAG_list[tag].structlevels[level].reader --;
        TAG_list[tag].structlevels[level].is_queued = 0;
        int ret;
        ret = copy_to_user(buffer,TAG_list[tag].structlevels[level].bufs,min(size,MSG_MAX_SIZE));
        if(ret < 0){
            rcu_read_unlock();
            printk("errore nella copy to user");
            return -1;
        }
        //reinserisco il buf del livello come vuoto
        TAG_list[tag].structlevels[level].is_empty = 0;
        if(reader < 1){
            //cancello il messaggio dopo che tutti lo hanno letto
            kfree(TAG_list[tag].structlevels[level].bufs);
            TAG_list[tag].structlevels[level].bufs = NULL;
        }
        rcu_read_unlock();
    }
    //da capire come ipmlementare un thread in attesa di un messaggio
    //bisogna usare il device driver che ci dice quali thread devono aspettare
    return 0;
}

int tag_ctl(int tag, int command) {
    if(command == 1){
        char *s;
        s = kmalloc(sizeof(char) * 17);
        if(s == NULL){
            printk("errore nella kmalloc dell'awake_all");
            return -1;
        }
        s = "AWAKE ALL THREADS";
        int i,j;
        for(i = 0, i< MAX_TAG_NUMBER, i++){
            if(TAG_list[i].exist){
                for(j = 0; j < LEVELS; j++){
                    if(TAG_list[tag].structlevels[level].is_queued = 1){
                        TAG_list[tag].structlevels[level].bufs = s;
                    }
                }
            }
        }
        wake_up_interruptible(TAG_list[tag].structlevels[level].wq);
        //awake all, serve il device driver
    }
    else if (command == 2){
        if(tag < 0 || tag > MAX_TAG_NUMBER){
            printk("purtroppo il valore del tag %d inserito non è valido \n", tag);
            return -1;
        }
        if(TAG_list[tag].exist != 1){
            printk("il tag inserito non esiste\n");
            return -1;
        }
        //ricordiamoci di segnare che il livello è vuoto dopo la receive
        if(search_for_level(TAG_list[tag])){
            delete_tag(TAG_list[tag]);
        }
        else{
            printk("non si può chiudere il tag\n");
        }
    }
    return 0;

}