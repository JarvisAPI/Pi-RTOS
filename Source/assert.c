#include <printk.h>

void vASSERT(void) {
    // TODO: print stack here somehow...
    printk("Assertion failed! Panic...: 0x%x\n\r", __builtin_return_address(0));
    
    while (1) {
        // Trap here...
    }
}
