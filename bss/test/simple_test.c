#include "bss.h"

#include <assert.h>

int main(void) {
    int bss = 0;
    bss_post(&bss);
    assert(bss == 1);
    bss_wait(&bss);
    assert(bss == 0);
    return 0;
}
