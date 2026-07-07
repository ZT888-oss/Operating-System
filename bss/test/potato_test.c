#include "bss.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int potato_baked = 0, potato_delivered = 0, potato_packaged = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct arg_str {
    int potatos2deliver;
    int* delivered_potato_bss;
    int* baked_potato_bss;
} Arguments;

void* potato_delivery(void* arg){
    Arguments* input = (Arguments*) arg;

    int delivery_count = 0;
    while(delivery_count < input->potatos2deliver){
        // sleep(1);
        pthread_mutex_lock(&mutex);
        potato_delivered++;
        pthread_mutex_unlock(&mutex);
        bss_post(input->delivered_potato_bss);
        printf("Delivered a potato. delivered_potato_bss value %d\n", *(input->delivered_potato_bss));
        delivery_count++;
    }
    printf("All potatos delivered\n");

    return NULL;
}

void* baking_machine(void* arg){
    Arguments* input = (Arguments*) arg;
    int cnt = 0;
    while(cnt < input->potatos2deliver){
        bss_wait(input->delivered_potato_bss);
        // sleep(2);
        cnt++;
        pthread_mutex_lock(&mutex);
        potato_baked++;
        assert(potato_baked <= potato_delivered && "More potatos baked than delivered");
        pthread_mutex_unlock(&mutex);
        bss_post(input->baked_potato_bss);
        printf("Baked %d potatos so far. delivered_potato_bss value %d; baked_potato_bss %d\n", 
                cnt, 
                *(input->delivered_potato_bss), 
                *(input->baked_potato_bss));
    }

    return NULL;
}

void* packaging_machine(void* arg){
    Arguments* input = (Arguments*) arg;

    while (potato_packaged < input->potatos2deliver) {
        bss_wait(input->baked_potato_bss);
        printf("Packaging potatos so far. baked_potato_bss %d\n", *(input->baked_potato_bss));

        pthread_mutex_lock(&mutex);
        potato_packaged++;
        assert(potato_baked <= potato_delivered && "More potatos baked than delivered");
        printf("Packaged %d potatos so far\n", potato_packaged);
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

    pthread_t t1, t2, t3;
    pthread_create(&t3, NULL, packaging_machine, (void*) &a);
    pthread_create(&t2, NULL, baking_machine, (void*) &a);
    pthread_create(&t1, NULL, potato_delivery, (void*) &a);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("Delivered %d potatoes, received %d baked and packaged potatos.\n", potatos2deliver, potato_packaged);

    assert(potatos2deliver == potato_packaged);
}

int main(void) {
    baked_potato_factory(500);
    return 0;
}
