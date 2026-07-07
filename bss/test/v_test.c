#include "bss.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void* increment_worker(void* arg) {
    int* bss = (int*) arg;

    for(int i = 0; i < 100; i++)
        bss_post(bss);

    return NULL;
}

void increase_sem_test(void) {
    int bss = 0;

    pthread_t t1, t2, t3, t4;
    pthread_create( &t1, NULL, increment_worker, (void*)(&bss));
    pthread_create( &t2, NULL, increment_worker, (void*)(&bss));
    pthread_create( &t3, NULL, increment_worker, (void*)(&bss));
    pthread_create( &t4, NULL, increment_worker, (void*)(&bss));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    assert(bss == 400);
}

int main(void) {
    increase_sem_test();
    return 0;
}
