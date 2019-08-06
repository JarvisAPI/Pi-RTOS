#ifndef DRIVERS_RPI_MMU_H
#define DRIVERS_RPI_MMU_H

#include <stdint.h>

void mmu_init_page_table(void);
void mmu_init(void);
uint32_t mmu_is_enabled(void);

#endif // DRIVERS_RPI_MMU_H

