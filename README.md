# Supporting Aperiodic Tasks and Multiprocessor Scheduling in FreeRTOS

Currently supports Global EDF and Partitioned EDF. Majority of the modifications is in tasks.c.

## Example build
The makefile currently builds the example in the Demo folder. There are currently two configuration header files: **PureFreeRTOSConfig.h** and **FreeRTOSConfig.h**. PureFreeRTOSConfig.h is used by the arm assembly/C files specific to the raspberry pi in *Source/portable/GCC/RaspberryPi*, therefore it can not contain any C code. FreeRTOSConfig.h includes extra macros along with the ones defined in PureFreeRTOSConfig.h, it also contains additional C code, therefore it can not be used by any assembly code.

## General Macros
* **configUSE_MULTICORE**: Set to 1 to enable multicore scheduling, 0 to disable. Note that some single core config options must be disabled in order for multicore scheduling to work correctly see the #if ( configUSE_MULTICORE ) section in FreeRTOSConfig.h.

## Aperodic Scheduling
Constant Bandwidth Server (CBS) server.

      Enabling macro: configUSE_CBS_CASH
      Example: Demo/task1_main.c, Demo/task1_main2.c, Demo/task2_cash.c
      
## Global EDF
This scheduling options designates one core as the primary core which is in charge of receiving tasks and distributing them to other secondary cores as well as to itself. The primary core is the only core that receives periodic interrupts and performs scheduler related functions.

      Enbaling macro: configUSE_GLOBAL_EDF
      Example: Demo/task3_global.c
      
## Partitioned EDF
This scheduling options places tasks on different cores on startup and each core runs their own set of tasks with no communcation between cores induced by the scheduler.

      Enabling macro: configUSE_PARTITIONED_EDF
      Example: Demo/task3_part.c
      
## Demo
The Demo folder contains examples on how to run this version of FreeRTOS on a raspberry pi (mainly tested on a raspberry pi 2). The Demo also includes some driver code (in Demo/Drivers) that is specific to the raspberry pi, some of the code such as the code in rpi_synch.h are necessary for synchronization in a multicore setting. Other than the Demo/Drivers folder the other places where platform dependent code is defined is in Source/portable/GCC/RaspberryPi.

## Bugs
Currently with Global EDF floating point context is not properly stored and restored, therefore floating point calculations is not supported by Global EDF, but it can be modified by saving and restoring it in porrtASM.S.
