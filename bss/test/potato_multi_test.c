#include "bss.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define NUM_THREADS 10

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int potato_packaged = 0, potato_baked = 0, potato_delivered = 0;

typedef struct arg_str {
    int potatos2deliver;
    int* delivered_potato_bss;
    int* baked_potato_bss;
} Arguments;

void* potato_delivery(void* arg){
    Arguments* input = (Arguments*) arg;

    int delivery_count = 0;
    while(delivery_count < input->potatos2deliver){
        delivery_count++;

        pthread_mutex_lock(&mutex);
        potato_delivered++;
        pthread_mutex_unlock(&mutex);
        bss_post(input->delivered_potato_bss);
        printf("Delivered a potato. delivered_potato_bss value %d\n", *(input->delivered_potato_bss));
    }
    printf("All potatos delivered\n");

    return NULL;
}

void* baking_machine(void* arg){
    Arguments* input = (Arguments*) arg;
    int local_potato_baked = 0;
    while(local_potato_baked < input->potatos2deliver){
        bss_wait(input->delivered_potato_bss);
        local_potato_baked++;
        pthread_mutex_lock(&mutex);
        potato_baked++;
        assert(potato_baked <= potato_delivered && "More potatos baked than delivered");
        pthread_mutex_unlock(&mutex);
        bss_post(input->baked_potato_bss);
        printf("Baked %d potatos so far. delivered_potato_bss value %d; baked_potato_bss %d\n", 
                local_potato_baked, 
                *(input->delivered_potato_bss), 
                *(input->baked_potato_bss));
    }

    return NULL;
}

void* packaging_machine(void* arg){
    Arguments* input = (Arguments*) arg;

    int local_potato_packaged = 0;

    while (local_potato_packaged < input->potatos2deliver) {
        bss_wait(input->baked_potato_bss);
        printf("Packaging potatos so far. baked_potato_bss %d\n", *(input->baked_potato_bss));

        local_potato_packaged++;
        pthread_mutex_lock(&mutex);
        potato_packaged++;
        printf("Packaged %d potatos so far\n", potato_packaged);
        assert(potato_packaged <= potato_baked && "More potatos packaged than baked");
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void baked_potato_factory(int potatos2deliver){
    int delivered_potato_bss = 0;
    int baked_potato_bss = 0;

    Arguments a = {
        potatos2deliver,
        &delivered_potato_bss,
        &baked_potato_bss,
    };

    pthread_t t1[NUM_THREADS], t2[NUM_THREADS], t3[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_create( &t3[i], NULL, packaging_machine, (void*)(&a));
        pthread_create( &t2[i], NULL, baking_machine, (void*)(&a));
        pthread_create( &t1[i], NULL, potato_delivery, (void*)(&a));
    }

    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(t1[i], NULL);
        pthread_join(t2[i], NULL);
        pthread_join(t3[i], NULL);
    }

    printf("Delivered %d potatoes, received %d baked and packaged potatos.\n", potato_delivered, potato_packaged);
    assert(potatos2deliver * NUM_THREADS == potato_packaged);
}

int main(void) {
    baked_potato_factory(500);
    return 0;
}
