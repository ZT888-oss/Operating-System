#include <pthread.h>
#include <stdio.h>
#include <stdatomic.h>
#define NUM_THREADS 8

static int counter = 0;
// atomic_int counter = 0;
// void* run(__attribute__((unused)) void* arg) {
//     int i;
//     for (i = 0; i < 8000; ++i) {
//         atomic_fetch_add(&counter, 1);
//     }
//     return NULL;
// }
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* run(__attribute__((unused)) void* arg) {
    for(int i = 0; i<8000; ++i){
        pthread_mutex_lock(&lock);
        ++counter;
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t thread[NUM_THREADS];
    int i;
    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&thread[i], NULL, &run, NULL);
    }

    for (i = 0; i < NUM_THREADS; ++i) {
        pthread_join(thread[i], NULL); // wait untill thread finish
    }

    pthread_mutex_destroy( &lock);
    printf("counter = %i\n", counter);
    return 0;
}
