#include <printk.h>

void vASSERT( uint32_t x ) {
    if ( !x ) {
        // TODO: print stack here somehow...
        printk("Assertion failed! Panic...\n\r");
        
        while (1) {
            
        }
    }
}
