/*
 * base.h
 *
 *  Created on: Feb 21, 2016
 *      Author: TsungMin Huang
 */

#ifndef _RPI_BASE_H_
#define _RPI_BASE_H_

#include <stdint.h>

/* Physical base address for RPI device */
#ifdef RPI2
    #define PRI_BASE_ADDRESS     0x3F000000UL
#else
	/* For RPI1 is broken now */
    #define PRI_BASE_ADDRESS     0x20000000UL
#endif

#endif /* _RPI_BASE_H_  */
