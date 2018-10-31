#include <printk.h>

void vASSERT(void) {
    // TODO: print stack here somehow...
    printk("Assertion failed! Panic...\n\r");
    
    while (1) {
        // Trap here...
    }
}
