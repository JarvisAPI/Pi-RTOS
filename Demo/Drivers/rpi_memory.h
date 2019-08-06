//
// memorymap.h
//
// Memory addresses and sizes
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2017  R. Stange <rsta2@o2online.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef DRIVERS_RPI_MEMORY_H
#define DRIVERS_RPI_MEMORY_H

#define MMIO_READ(ADDR) ( *( volatile uint32_t* ) ( ADDR ) )
#define MMIO_WRITE(ADDR, DATA) *( volatile uint32_t* ) ( ADDR ) = ( DATA )

#define KERNEL_MAX_SIZE	(2 * MEGABYTE)

#ifndef MEGABYTE
    #define MEGABYTE    0x100000
#endif

#define MEM_SIZE                (256 * MEGABYTE)            // default size
#define GPU_MEM_SIZE            (64 * MEGABYTE)             // set in config.txt
#define ARM_MEM_SIZE            (MEM_SIZE - GPU_MEM_SIZE)   // normally overwritten


#define KERNEL_STACK_SIZE       0x20000                    // all sizes must be a multiple of 16K
#define EXCEPTION_STACK_SIZE    0x8000

#define MEM_KERNEL_START        0x8000
#define MEM_KERNEL_END          (MEM_KERNEL_START + KERNEL_MAX_SIZE)
#define MEM_KERNEL_STACK        (MEM_KERNEL_END + KERNEL_STACK_SIZE)        // expands down

#define CORES            4      // must be a power of 2
#define MEM_ABORT_STACK         (MEM_KERNEL_STACK + KERNEL_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_IRQ_STACK           (MEM_ABORT_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_FIQ_STACK           (MEM_IRQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)
#define MEM_SVC_STACK           (MEM_FIQ_STACK + EXCEPTION_STACK_SIZE * (CORES-1) + EXCEPTION_STACK_SIZE)

#define RPI_CORE_MMIO_BASE         ( 0x40000000 )

#endif
