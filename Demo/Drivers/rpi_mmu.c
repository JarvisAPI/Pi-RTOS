#include "Drivers/rpi_mmu.h"

#define CACHED_TLB
//#undef CACHED_TLB

static volatile __attribute__ ((aligned (0x4000))) uint32_t page_table[4096];
static volatile __attribute__ ((aligned (0x400))) uint32_t leaf_table[256];

struct page {
    uint8_t data[4096];
};

void mmu_init_page_table(void) {
    uint32_t base;
    // initialize page_table
    // 1024MB - 16MB of kernel memory (some belongs to the VC)
    for (base = 0; base < 1024 - 16; base++) {
        // section descriptor (1 MB)
#ifdef CACHED_TLB
        // outer and inner write back, write allocate, not shareable (fast
        // but unsafe)
        //page_table[base] = base << 20 | 0x0140E;
        // outer and inner write back, write allocate, shareable (fast but
        // unsafe)
        page_table[base] = base << 20 | 0x1140E;
#else
        // outer and inner write through, no write allocate, shareable
        // (safe but slower)
        page_table[base] = base << 20 | 0x1040A;
#endif
    }

    // unused up to 0x3F000000
    for (; base < 1024 - 16; base++) {
        page_table[base] = 0;
    }

    // 16 MB peripherals at 0x3F000000
    for (; base < 1024; base++) {
        // shared device, never execute
        page_table[base] = base << 20 | 0x10416;
    }

    // 1 MB mailboxes
    // shared device, never execute
    page_table[base] = base << 20 | 0x10416;
    ++base;
        
    // unused up to 0x7FFFFFFF
    for (; base < 2048; base++) {
        page_table[base] = 0;
    }

    // one second level page tabel (leaf table) at 0x80000000
    page_table[base++] = (intptr_t)leaf_table | 0x1;

    // 2047MB unused (rest of address space)
    for (; base < 4096; base++) {
        page_table[base] = 0;
    }

    // initialize leaf_table
    for (base = 0; base < 256; base++) {
        leaf_table[base] = 0;
    }
}
   
void mmu_init(void) {
    // set SMP bit in ACTLR
    uint32_t auxctrl;
    asm volatile ("mrc p15, 0, %0, c1, c0,  1" : "=r" (auxctrl));
    auxctrl |= 1 << 6;
    asm volatile ("mcr p15, 0, %0, c1, c0,  1" :: "r" (auxctrl));

    // setup domains (CP15 c3)
    // Write Domain Access Control Register
    // use access permissions from TLB entry
    asm volatile ("mcr     p15, 0, %0, c3, c0, 0" :: "r" (0x55555555));

    // set domain 0 to client
    asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (1));

    // always use TTBR0
    asm volatile ("mcr p15, 0, %0, c2, c0, 2" :: "r" (0));

#ifdef CACHED_TLB
    // set TTBR0 (page table walk inner and outer write-back,
    // write-allocate, cacheable, shareable memory)
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"
                  :: "r" (0b1001010 | (unsigned) &page_table));
    // set TTBR0 (page table walk inner and outer write-back,
    // write-allocate, cacheable, non-shareable memory)
    //asm volatile ("mcr p15, 0, %0, c2, c0, 0"
    //              :: "r" (0b1101010 | (unsigned) &page_table));
#else
    // set TTBR0 (page table walk inner and outer non-cacheable,
    // non-shareable memory)
    asm volatile ("mcr p15, 0, %0, c2, c0, 0"
                  :: "r" (0 | (unsigned) &page_table));
#endif
    asm volatile ("isb" ::: "memory");

    /* SCTLR
     * Bit 31: SBZ     reserved
     * Bit 30: TE      Thumb Exception enable (0 - take in ARM state)
     * Bit 29: AFE     Access flag enable (1 - simplified model)
     * Bit 28: TRE     TEX remap enable (0 - no TEX remapping)
     * Bit 27: NMFI    Non-Maskable FIQ (read-only)
     * Bit 26: 0       reserved
     * Bit 25: EE      Exception Endianness (0 - little-endian)
     * Bit 24: VE      Interrupt Vectors Enable (0 - use vector table)
     * Bit 23: 1       reserved
     * Bit 22: 1/U     (alignment model)
     * Bit 21: FI      Fast interrupts (probably read-only)
     * Bit 20: UWXN    (Virtualization extension)
     * Bit 19: WXN     (Virtualization extension)
     * Bit 18: 1       reserved
     * Bit 17: HA      Hardware access flag enable (0 - enable)
     * Bit 16: 1       reserved
     * Bit 15: 0       reserved
     * Bit 14: RR      Round Robin select (0 - normal replacement strategy)
     * Bit 13: V       Vectors bit (0 - remapped base address)
     * Bit 12: I       Instruction cache enable (1 - enable)
     * Bit 11: Z       Branch prediction enable (1 - enable)
     * Bit 10: SW      SWP/SWPB enable (maybe RAZ/WI)
     * Bit 09: 0       reserved
     * Bit 08: 0       reserved
     * Bit 07: 0       endian support / RAZ/SBZP
     * Bit 06: 1       reserved
     * Bit 05: CP15BEN DMB/DSB/ISB enable (1 - enable)
     * Bit 04: 1       reserved
     * Bit 03: 1       reserved
     * Bit 02: C       Cache enable (1 - data and unified caches enabled)
     * Bit 01: A       Alignment check enable (1 - fault when unaligned)
     * Bit 00: M       MMU enable (1 - enable)
     */
        
    // enable MMU, caches and branch prediction in SCTLR
    uint32_t mode;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (mode));
    // mask: 0b0111 0011 0000 0010 0111 1000 0010 0111
    // bits: 0b0010 0000 0000 0000 0001 1000 0010 0111
#ifdef CACHED_TLB
    mode &= 0x73027827;
    mode |= 0x20001827;
#else
    // no caches
    mode &= 0x73027827;
    mode |= 0x20000023;
#endif
    asm volatile ("mcr p15, 0, %0, c1, c0, 0" :: "r" (mode) : "memory");

    // instruction cache makes delay way faster, slow panic down
/* #ifdef CACHED_TLB */
/*     panic_delay = 0x2000000; */
/* #endif */
}

/**
 * Check if mmu is enabled.
 * @return 1 if mmu is enabled, 0 otherwise.
 */
uint32_t mmu_is_enabled(void) {
    uint32_t val;
    asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (val));
    return val & 0x1;
}
