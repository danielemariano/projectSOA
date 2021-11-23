#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>

#define AWAKE_ALL 1
#define REMOVE 2
#define CREATE 1
#define OPEN 2
#define NUM_THREADS 5
#define RAND_MAX 32
#define MAJOR 0
#define MINOR 4
#define LEVELS 32
#define MAX_TAG_NUMBER 256
#define MSG_MAX_SIZE 4096
