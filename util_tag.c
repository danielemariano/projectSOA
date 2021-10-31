#include "data_structures.h"

/**
 * Struttura di utility che ci permette di capire
 * se un determinato livello può essere eliminato
 * */
int search_for_level(struct tag TAG_list){
    int i;
    int j;
    for(j = 0; j < LEVELS; j ++) {
        if (TAG_list.structlevels[j].is_empty == 0){
            continue;
        }
        else{
            printk("Alemno un livello contiene un messaggio, non possiamo cancellare il tag.\n");
            return -1;
        }
    }
    printk("Nessun livello contiene un messaggio, possiamo cancellare il tag.\n");
    return 0;
}

/**
 * Funzione che implementa l'eliminazione del tag
*/
struct tag delete_tag(struct tag TAG){
    TAG.exist = 0;
    TAG.structlevels = NULL;
    TAG.key = NULL;
    TAG.opened = NULL;
    TAG.tag_id = NULL;
    printk("Il tag è stato cancellato.\n");
    return TAG;
}