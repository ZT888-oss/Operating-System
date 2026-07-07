#include "bss.h"

void bss_wait(int* bss) {
    while (1) {
        int old = __atomic_load_n(bss, __ATOMIC_ACQUIRE);

        if (old > 0) {
            if (__atomic_compare_exchange_n(
                    bss,        // memory location
                    &old,       // expected value
                    old - 1,    // new value
                    0,          // strong CAS
                    __ATOMIC_ACQ_REL,
                    __ATOMIC_ACQUIRE)) {
                return;  // success
            }
        }
        // else: spin and retry
    }
}

void bss_post(int* bss) {
     __atomic_fetch_add(bss, 1, __ATOMIC_RELEASE);
}
