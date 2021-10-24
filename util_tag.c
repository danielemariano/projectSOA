#include "data_structures.h"

int search_for_level(struct tag TAG_list){
    int i;
    int j;
    for(j = 0; j < LEVELS; j ++) {
        if (TAG_list.structlevels[j].is_empty){
            continue;
        }
        else{
            printk("un livello contiene un messaggio\n");
            return -1;
        }
    }
    return 0;
}


void delete_tag(struct tag TAG){
    TAG.exist = 0;
    TAG.structlevels = NULL;
    TAG.key = NULL;
    TAG.opened = NULL;
    TAG.tag_id = NULL;
}