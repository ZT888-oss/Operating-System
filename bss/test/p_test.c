#include "bss.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

void* decrement_worker(void* arg){
    int* bss = (int*) arg;

    for(int i = 0; i < 100; i++) {
        bss_wait(bss);
    }

    return NULL;
}

void decrease_sem_test(void){
    int bss = 500;

    pthread_t t1, t2, t3, t4;
    pthread_create(&t1, NULL, decrement_worker, (void*)(&bss));
    pthread_create(&t2, NULL, decrement_worker, (void*)(&bss));
    pthread_create(&t3, NULL, decrement_worker, (void*)(&bss));
    pthread_create(&t4, NULL, decrement_worker, (void*)(&bss));

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);

    assert(bss == 100);
}

int main(void) {
    decrease_sem_test();
    return 0;
}
