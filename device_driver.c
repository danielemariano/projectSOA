/**
 * Lo sviluppo del device driver è stato basato sullo
 * snippet di codice visto a lezione concurrency_driver
 */

#define EXPORT_SYMTAB
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/pid.h>		/* For pid types */
#include <linux/tty.h>		/* For the tty declarations */
#include <linux/version.h>	/* For LINUX_VERSION_CODE */
#include <linux/unistd.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "services.c"
#include "data_structures.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniele Mariano");

#define MODNAME  "DEVICE DRIVER"

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

#define DEVICE_NAME "my-new-dev"  \
/* Device file name in /dev/ - not mandatory  */

static int Major;
/* Major number assigned to broadcast device driver */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0)
#define get_major(session)	MAJOR(session->f_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_inode->i_rdev)
#else
#define get_major(session)	MAJOR(session->f_dentry->d_inode->i_rdev)
#define get_minor(session)	MINOR(session->f_dentry->d_inode->i_rdev)
#endif


typedef struct _object_state{
    struct mutex object_busy;
    struct mutex operation_synchronizer;
    int valid_bytes;
    char * stream_content;
    //Il nodo di I/O è un buffer
} object_state;

#define MINORS 8
object_state objects[MINORS];
// Questa è una struttura che uso per mettere i dati
// da inserire di volta in volta nel device driver
struct dev_struct *dev_lines = NULL;
#define OBJECT_MAX_SIZE  (4096)
// ovvero una pagina


/**
 * Nella funzione di apertura si prende l'oggetto del device driver
 * e si esegue una trylock sopra, se questa non fallisce allora
 * l'oggetto in questione è da considerarsi aperto.
 */
static int dev_open(struct inode *inode, struct file *file) {
    int minor;
    minor = get_minor(file);
    if(minor >= MINORS){
        return -ENODEV;
    }
    if (!mutex_trylock(&(objects[minor].object_busy))) {
        return -EBUSY;
    }
    printk("%s: device file successfully opened for object with minor %d\n",MODNAME,minor);
    return 0;
}

/**
 * Nella funzione di release del device driver si prende un oggetto
 * e su questo eseguiamo il rilascio semplicemente il mutex.
 */
static int dev_release(struct inode *inode, struct file *file) {
    int minor;
    minor = get_minor(file);
    mutex_unlock(&(objects[minor].object_busy));
    printk("%s: device file closed\n",MODNAME);
    return 0;
}


/**
 * Nella funzione di scrittura del device driver prendiamo l'oggeto del driver
 * e se i controlli su spazio e risorse associate al minor in questione non falliscono
 * posso eseguire la scrittura relativa al livello e al servizio TAG associato attraverso
 * una memcopy per poi ripulire tutte le risorse utilizzate.
 */
static ssize_t dev_write(struct file *filp, const char *buff, size_t len, loff_t *off) {
    int i,j;
    int minor = get_minor(filp);
    object_state *the_object;

    the_object = objects + minor;
    // prende l'entry dell array pari a minor
    printk("%s: somebody called a write on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

    mutex_lock(&(the_object->operation_synchronizer));
    if(*off >= OBJECT_MAX_SIZE) {
        // controllo se l'offset è troppo grande
        mutex_unlock(&(the_object->operation_synchronizer));
        return -ENOSPC;
        // in questo caso significa che non ho sufficiente spazio nel device
    }
    if(*off > the_object->valid_bytes) {
        // controllo l'offset dello stream corrente
        mutex_unlock(&(the_object->operation_synchronizer));
        return -ENOSR;
        // in questo caso sono fuori dalle risorse dello stream
    }
    if((OBJECT_MAX_SIZE - *off) < len) len = OBJECT_MAX_SIZE - *off;
    for(i = 0; i < MAX_TAG_NUMBER; i ++){
        if(TAG_list[i].exist){
            for(j = 0; j < LEVELS; j ++){
                if(TAG_list[i].structlevels[j].reader > 0){
                    // eseguo il ciclo per ogni tag, per ogni livello se esistono lettori
                    // ed eseguo la malloc per quanti reader in attesa abbiamo ovvero
                    // alloco memoria sufficiente per gli attuali TAG in attesa
                    dev_lines = kmalloc(sizeof(struct dev_struct)*total_tag,GFP_KERNEL);
                    if(dev_lines == NULL){
                        printk("errore nella kmalloc del driver");
                        return -1;
                    }
                    struct dev_struct line;
                    line.tag = TAG_list[i].key;
                    line.sleepers = TAG_list[i].structlevels[j].reader;
                    //se il tag è privato ci sarà l'ID del thread, altrimenti 0
                    line.thread = TAG_list[i].permission;
                    dev_lines[i] = line;
                }
            }
        }
    }
    //qui copio dalla struct presa da service sul driver
    memcpy(&(the_object->stream_content[*off]),dev_lines,sizeof(dev_lines));
    *off += (len);
    the_object->valid_bytes = *off;
    //libero la memoria della struttura
    kfree(dev_lines);
    mutex_unlock(&(the_object->operation_synchronizer));
    //una volta scritto nel device libero tutto

    return len;
}

static ssize_t dev_read(struct file *filp, char *buff, size_t len, loff_t *off) {
    int minor = get_minor(filp);
    int ret;
    object_state *the_object;
    the_object = objects + minor;
    printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));
    // need to lock in any case
    mutex_lock(&(the_object->operation_synchronizer));
    if(*off > the_object->valid_bytes) {
        mutex_unlock(&(the_object->operation_synchronizer));
        return 0;
    }
    if((the_object->valid_bytes - *off) < len) len = the_object->valid_bytes - *off;

    // se tutto è andato per il verso giusto posso scrivere
    ret = copy_to_user(buff,&(the_object->stream_content[*off]),len);

    // aggiorno l'offset e rilascio il mutex
    *off += (len - ret);
    mutex_unlock(&(the_object->operation_synchronizer));

    return len - ret;
    printk("%s: somebody called a read on dev with [major,minor] number [%d,%d]\n",MODNAME,get_major(filp),get_minor(filp));

    return 0;
}

static struct file_operations fops = {
        .owner = THIS_MODULE,
        //IMPORTANTE: non dimenticare
        .write = dev_write,
        .read = dev_read,
        .open =  dev_open,
        .release = dev_release,
};


/**
 * Questa funzione si occupa di inizializzare il modulo e di creare un numero
 * associato MINORS di oggetti del device driver che corrisponde all'identificativo
 * associato da MAJOR.
 * Nel ciclo for vengono inizializzati i vari campi dei vari oggetti del device driver
 * che viene poi registrato nel kernel se tutto procede senza errori.
 */
int init_module(void) {
    int i;
    // Initialize the drive internal state
    // Minor number identificativo di un oggetto all'interno del driver
    // Major number è l'identificativo del driver
    for(i=0;i<MINORS;i++){
        // l'operazione di installazione del driver è bloccante
        mutex_init(&(objects[i].object_busy));
        mutex_init(&(objects[i].operation_synchronizer));
        objects[i].valid_bytes = 0;
        objects[i].stream_content = NULL;
        objects[i].stream_content = (char*)kmalloc(sizeof(char)*256*4096,GFP_KERNEL);
        if(objects[i].stream_content == NULL) goto revert_allocation;
    }
    Major = __register_chrdev(0, 0, 256, DEVICE_NAME, &fops);
    // int __register_chrdev(unsigned int major, unsigned int baseminor, unsigned int count,
    // const char * name, const struct file_operations * fops);
        // 0: assegnazione dinamica (automatica) del dev driver
        // 0: primo tra i minor richiesti
        // 256: numero di minor richiesti
        // DEVICE_NAME: nome del device driver
        // fops: file operation associate al device driver

    if (Major < 0) {
        printk("%s: registering device failed\n",MODNAME);
        return Major;
    }
    printk(KERN_INFO "%s: new device registered, it is assigned major number %d\n",MODNAME, Major);
    return 0;

    revert_allocation:
    for(;i>=0;i--){
        kfree((unsigned long)objects[i].stream_content);
    }
    return -ENOMEM;
}


/**
 * Funzione che si occupa di smontare il modulo che ha caricato il device driver.
 */
void cleanup_module(void) {
    int i;
    // rivedi se lo fa anche Giorgia
    for(i=0;i<MINORS;i++){
        kmalloc((unsigned long)objects[i].stream_content,GFP_KERNEL);
    }
    unregister_chrdev(Major, DEVICE_NAME);
    printk(KERN_INFO "%s: new device unregistered, it was assigned major number %d\n",MODNAME, Major);
    return;
}