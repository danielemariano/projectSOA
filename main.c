#include <stdio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniele Mariano");
MODULE_DESCRIPTION("Simple first Linux module");
MODULE_VERSION("1.0.0");

/*
 * Funzione per montare il modulo ed eseguire una stampa kernel,
 * possiamo vedere il risultato della stampa eseguendo dmesg
 */
static int __init initHelloWorld(void){
    printk(KERN_INFO "Hello, this is my first kernel module \n");
    return 0;
}

/*
 * Funzione per smontare il modulo ed eseguire una stampa kernel,
 * possiamo vedere il risultato della stampa eseguendo dmesg
 */
static void __exit exitHelloWorld(void){
    printk(KERN_INFO "Exit Hello world module\n");
}

/*
 * Main
 */
int main() {
    printf("Hello, World!\n");
    module_init(initHelloWorld);
    module_exit(exitHelloWorld);
    return 0;
}
