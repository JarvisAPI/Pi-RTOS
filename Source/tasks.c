/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#if( configUSE_TIMER == 1 )
#include "timers.h"
#endif

#include "StackMacros.h"
#include "printk.h"


/* Drivers include. */
#include <Drivers/rpi_memory.h>

#if( configUSE_SRP == 1 || configUSE_CBS_SERVER )
#include <semphr.h>
#endif


/* Lint e961 and e750 are suppressed as a MISRA exception justified because the
MPU ports require MPU_WRAPPERS_INCLUDED_FROM_API_FILE to be defined for the
header files above, but not in this file, in order to generate the correct
privileged Vs unprivileged linkage and placement. */
#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE /*lint !e961 !e750. */

/* Set configUSE_STATS_FORMATTING_FUNCTIONS to 2 to include the stats formatting
functions but without including stdio.h here. */
#if ( configUSE_STATS_FORMATTING_FUNCTIONS == 1 )
	/* At the bottom of this file are two optional functions that can be used
	to generate human readable text from the raw data generated by the
	uxTaskGetSystemState() function.  Note the formatting functions are provided
	for convenience only, and are NOT considered part of the kernel. */
	#include <stdio.h>
#endif /* configUSE_STATS_FORMATTING_FUNCTIONS == 1 ) */

#if( configUSE_PREEMPTION == 0 )
	/* If the cooperative scheduler is being used then a yield should not be
	performed just because a higher priority task has been woken. */
	#define taskYIELD_IF_USING_PREEMPTION()
#else
	#define taskYIELD_IF_USING_PREEMPTION() portYIELD_WITHIN_API()
#endif

/* Values that can be assigned to the ucNotifyState member of the TCB. */
#define taskNOT_WAITING_NOTIFICATION	( ( uint8_t ) 0 )
#define taskWAITING_NOTIFICATION		( ( uint8_t ) 1 )
#define taskNOTIFICATION_RECEIVED		( ( uint8_t ) 2 )

/*
 * The value used to fill the stack of a task when the task is created.  This
 * is used purely for checking the high water mark for tasks.
 */
#define tskSTACK_FILL_BYTE	( 0xa5U )

/* Sometimes the FreeRTOSConfig.h settings only allow a task to be created using
dynamically allocated RAM, in which case when any task is deleted it is known
that both the task's stack and TCB need to be freed.  Sometimes the
FreeRTOSConfig.h settings only allow a task to be created using statically
allocated RAM, in which case when any task is deleted it is known that neither
the task's stack or TCB should be freed.  Sometimes the FreeRTOSConfig.h
settings allow a task to be created using either statically or dynamically
allocated RAM, in which case a member of the TCB is used to record whether the
stack and/or TCB were allocated statically or dynamically, so when a task is
deleted the RAM that was allocated dynamically is freed again and no attempt is
made to free the RAM that was allocated statically.
tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE is only true if it is possible for a
task to be created using either statically or dynamically allocated RAM.  Note
that if portUSING_MPU_WRAPPERS is 1 then a protected task can be created with
a statically allocated stack and a dynamically allocated TCB. */
#define tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE ( ( ( configSUPPORT_STATIC_ALLOCATION == 1 ) && ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) ) || ( portUSING_MPU_WRAPPERS == 1 ) )
#define tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB 		( ( uint8_t ) 0 )
#define tskSTATICALLY_ALLOCATED_STACK_ONLY 			( ( uint8_t ) 1 )
#define tskSTATICALLY_ALLOCATED_STACK_AND_TCB		( ( uint8_t ) 2 )

/*
 * Macros used by vListTask to indicate which state a task is in.
 */
#define tskBLOCKED_CHAR		( 'B' )
#define tskREADY_CHAR		( 'R' )
#define tskDELETED_CHAR		( 'D' )
#define tskSUSPENDED_CHAR	( 'S' )

/*
 * Some kernel aware debuggers require the data the debugger needs access to be
 * global, rather than file scope.
 */
#ifdef portREMOVE_STATIC_QUALIFIER
	#define static
#endif

#if ( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )

	/* If configUSE_PORT_OPTIMISED_TASK_SELECTION is 0 then task selection is
	performed in a generic way that is not optimised to any particular
	microcontroller architecture. */

	/* uxTopReadyPriority holds the priority of the highest priority ready
	state task. */
	#define taskRECORD_READY_PRIORITY( uxPriority )														\
	{																									\
		if( ( uxPriority ) > uxTopReadyPriority[portCPUID()] )														\
		{																								\
			uxTopReadyPriority[portCPUID()] = ( uxPriority );														\
		}																								\
	} /* taskRECORD_READY_PRIORITY */

	/*-----------------------------------------------------------*/
#if( configUSE_SCHEDULER_EDF == 1 )
    #define taskSELECT_HIGHEST_PRIORITY_TASK()                      \
	{																									\
	UBaseType_t uxTopPriority = uxTopReadyPriority[portCPUID()];														\
																										\
		/* Find the highest priority queue that contains ready tasks. */								\
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) ) )							\
		{																								\
			configASSERT( uxTopPriority );																\
			--uxTopPriority;																			\
		}																								\
        ListItem_t const *endMarker = listGET_END_MARKER( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) );    \
        pxCurrentTCB[portCPUID()] = listGET_LIST_ITEM_OWNER( endMarker->pxNext );                                            \
		uxTopReadyPriority[portCPUID()] = uxTopPriority;																\
    } /* taskSELECT_HIGHEST_PRIORITY_TASK */
#else
	#define taskSELECT_HIGHEST_PRIORITY_TASK()															\
	{																									\
	UBaseType_t uxTopPriority = uxTopReadyPriority[portCPUID()];														\
																										\
		/* Find the highest priority queue that contains ready tasks. */								\
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) ) )							\
		{																								\
			configASSERT( uxTopPriority );																\
			--uxTopPriority;																			\
		}																								\
																										\
		/* listGET_OWNER_OF_NEXT_ENTRY indexes through the list, so the tasks of						\
		the	same priority get an equal share of the processor time. */									\
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB[portCPUID()], &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) );			\
		uxTopReadyPriority[portCPUID()] = uxTopPriority;																\
	} /* taskSELECT_HIGHEST_PRIORITY_TASK */
#endif /* configSET_SCHEDULER_EDF */

	/*-----------------------------------------------------------*/

	/* Define away taskRESET_READY_PRIORITY() and portRESET_READY_PRIORITY() as
	they are only required when a port optimised method of task selection is
	being used. */
	#define taskRESET_READY_PRIORITY( uxPriority )
	#define portRESET_READY_PRIORITY( uxPriority, uxTopReadyPriority )

#else /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

	/* If configUSE_PORT_OPTIMISED_TASK_SELECTION is 1 then task selection is
	performed in a way that is tailored to the particular microcontroller
	architecture being used. */

	/* A port optimised version is provided.  Call the port defined macros. */
	#define taskRECORD_READY_PRIORITY( uxPriority )	portRECORD_READY_PRIORITY( uxPriority, uxTopReadyPriority[portCPUID()] )

	/*-----------------------------------------------------------*/

	#define taskSELECT_HIGHEST_PRIORITY_TASK()														\
	{																								\
	UBaseType_t uxTopPriority;																		\
																									\
		/* Find the highest priority list that contains ready tasks. */								\
		portGET_HIGHEST_PRIORITY( uxTopPriority, uxTopReadyPriority[portCPUID()] );								\
		configASSERT( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) ) > 0 );		\
		listGET_OWNER_OF_NEXT_ENTRY( pxCurrentTCB[portCPUID()], &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) );		\
	} /* taskSELECT_HIGHEST_PRIORITY_TASK() */

	/*-----------------------------------------------------------*/

	/* A port optimised version is provided, call it only if the TCB being reset
	is being referenced from a ready list.  If it is referenced from a delayed
	or suspended list then it won't be in a ready list. */
	#define taskRESET_READY_PRIORITY( uxPriority )														\
	{																									\
		if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ ( uxPriority ) ] ) ) == ( UBaseType_t ) 0 )	\
		{																								\
			portRESET_READY_PRIORITY( ( uxPriority ), ( uxTopReadyPriority[portCPUID()] ) );							\
		}																								\
	}

#endif /* configUSE_PORT_OPTIMISED_TASK_SELECTION */

/*-----------------------------------------------------------*/

/* pxDelayedTaskList and pxOverflowDelayedTaskList are switched when the tick
count overflows. */
#define taskSWITCH_DELAYED_LISTS()																	\
{																									\
	List_t *pxTemp;																					\
																									\
	/* The delayed tasks list should be empty when the lists are switched. */						\
	configASSERT( ( listLIST_IS_EMPTY( pxDelayedTaskList[portCPUID()] ) ) );										\
																									\
	pxTemp = pxDelayedTaskList[portCPUID()];																		\
	pxDelayedTaskList[portCPUID()] = pxOverflowDelayedTaskList[portCPUID()];													\
	pxOverflowDelayedTaskList[portCPUID()] = pxTemp;																\
	xNumOfOverflows[portCPUID()]++;																				\
	prvResetNextTaskUnblockTime();																	\
}

/*-----------------------------------------------------------*/

/*
 * Place the task represented by pxTCB into the appropriate ready list for
 * the task.  It is inserted at the end of the list.
 */
#define prvAddTaskToReadyList( pxTCB )																\
	traceMOVED_TASK_TO_READY_STATE( pxTCB );														\
	taskRECORD_READY_PRIORITY( ( pxTCB )->uxPriority );												\
        ( pxTCB )->xStateListItem.xItemValue = pxTCB->xRelativeDeadline;\
	vListInsert( &( pxReadyTasksLists[portCPUID()][ ( pxTCB )->uxPriority ] ), &( ( pxTCB )->xStateListItem ) ); \
	tracePOST_MOVED_TASK_TO_READY_STATE( pxTCB )
/*-----------------------------------------------------------*/

/*
 * Several functions take an TaskHandle_t parameter that can optionally be NULL,
 * where NULL is used to indicate that the handle of the currently executing
 * task should be used in place of the parameter.  This macro simply checks to
 * see if the parameter is NULL and returns a pointer to the appropriate TCB.
 */
#define prvGetTCBFromHandle( pxHandle ) ( ( ( pxHandle ) == NULL ) ? ( TCB_t * ) pxCurrentTCB[portCPUID()] : ( TCB_t * ) ( pxHandle ) )

/* The item value of the event list item is normally used to hold the priority
of the task to which it belongs (coded to allow it to be held in reverse
priority order).  However, it is occasionally borrowed for other purposes.  It
is important its value is not updated due to a task priority change while it is
being used for another purpose.  The following bit definition is used to inform
the scheduler that the value should not be changed - in which case it is the
responsibility of whichever module is using the value to ensure it gets set back
to its original value when it is released. */
#if( configUSE_16_BIT_TICKS == 1 )
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x8000U
#else
	#define taskEVENT_LIST_ITEM_VALUE_IN_USE	0x80000000UL
#endif

/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct tskTaskControlBlock
{
	volatile StackType_t	*pxTopOfStack;	/*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

	#if ( portUSING_MPU_WRAPPERS == 1 )
		xMPU_SETTINGS	xMPUSettings;		/*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
	#endif

	ListItem_t			xStateListItem;	/*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
	ListItem_t			xEventListItem;		/*< Used to reference a task from an event list. */
	UBaseType_t			uxPriority;			/*< The priority of the task.  0 is the lowest priority. */
	StackType_t			*pxStack;			/*< Points to the start of the stack. */
	char				pcTaskName[ configMAX_TASK_NAME_LEN ];/*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

	#if ( portSTACK_GROWTH > 0 )
		StackType_t		*pxEndOfStack;		/*< Points to the end of the stack on architectures where the stack grows up from low memory. */
	#endif

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
		UBaseType_t		uxCriticalNesting;	/*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
	#endif

		UBaseType_t		uxTCBNumber;		/*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
		UBaseType_t		uxTaskNumber;		/*< Stores a number specifically for use by third party trace code. */

	#if ( configUSE_MUTEXES == 1 )
		UBaseType_t		uxBasePriority;		/*< The priority last assigned to the task - used by the priority inheritance mechanism. */
		UBaseType_t		uxMutexesHeld;
	#endif

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
		TaskHookFunction_t pxTaskTag;
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0 )
		void *pvThreadLocalStoragePointers[ configNUM_THREAD_LOCAL_STORAGE_POINTERS ];
	#endif

	#if( configGENERATE_RUN_TIME_STATS == 1 )
		uint32_t		ulRunTimeCounter;	/*< Stores the amount of time the task has spent in the Running state. */
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
		/* Allocate a Newlib reent structure that is specific to this task.
		Note Newlib support has been included by popular demand, but is not
		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
		responsible for resulting newlib operation.  User must be familiar with
		newlib and must provide system-wide implementations of the necessary
		stubs. Be warned that (at the time of writing) the current newlib design
		implements a system-wide malloc() that must be provided with locks. */
		struct	_reent xNewLib_reent;
	#endif

	#if( configUSE_TASK_NOTIFICATIONS == 1 )
		volatile uint32_t ulNotifiedValue;
		volatile uint8_t ucNotifyState;
	#endif

	/* See the comments above the definition of
	tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
	#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
		uint8_t	ucStaticallyAllocated; 		/*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
		uint8_t ucDelayAborted;
	#endif

        #if( configUSE_SCHEDULER_EDF == 1 )
            TickType_t xRelativeDeadline;
            TickType_t xCurrentRunTime;
            TickType_t xWCET;
            TickType_t xPeriod;
            TickType_t xLastWakeTime;
            TaskFunction_t pxTaskCode;
            uint32_t usStackDepth;
            void*  pvParameters;
            BaseType_t xNoPreserve;
            float fUtilization;
            #if( configUSE_CBS_SERVER == 1 )
                BaseType_t xIsServer;
                BaseType_t pxJobQueue;
            #endif
        #endif

   #if( configUSE_SRP == 1 )
    StackType_t xPreemptionLevel;
    // Longest time that task can be blocked by a lower priority task, used for admission control
    TickType_t xBlockTime; 
    BaseType_t xBlocked;
    #endif

#if( configUSE_GLOBAL_EDF == 1 )
#define TASK_WAITING 0
#define TASK_EXECUTING 1
#define TASK_COMPLETED 2 
#define TASK_SET_PHASE 3
    uint32_t xTaskState;

    TickType_t xTaskPhase;
#endif /* configUSE_GLOBAL_EDF */

} tskTCB;

/* The old tskTCB name is maintained above then typedefed to the new TCB_t name
below to enable the use of older kernel aware debuggers. */
typedef tskTCB TCB_t;

/*lint -e956 A manual analysis and inspection has been used to determine which
static variables must be declared volatile. */

PRIVILEGED_DATA TCB_t * volatile pxCurrentTCB[CORES] = {NULL};

/* Lists for ready and blocked tasks. --------------------*/
PRIVILEGED_DATA static List_t pxReadyTasksLists[CORES][ configMAX_PRIORITIES ];/*< Prioritised ready tasks. */
PRIVILEGED_DATA static List_t xDelayedTaskList1[CORES];						/*< Delayed tasks. */
PRIVILEGED_DATA static List_t xDelayedTaskList2[CORES];						/*< Delayed tasks (two lists are used - one for delays that have overflowed the current tick count. */
PRIVILEGED_DATA static List_t * volatile pxDelayedTaskList[CORES];				/*< Points to the delayed task list currently being used. */
PRIVILEGED_DATA static List_t * volatile pxOverflowDelayedTaskList[CORES];		/*< Points to the delayed task list currently being used to hold tasks that have overflowed the current tick count. */
PRIVILEGED_DATA static List_t xPendingReadyList[CORES];						/*< Tasks that have been readied while the scheduler was suspended.  They will be moved to the ready list when the scheduler is resumed. */

#if ( configUSE_SRP == 1 )
    PRIVILEGED_DATA static List_t pxResourceList;/*< Prioritised ready tasks. */
#endif /* configUSE_SRP */

#if ( configUSE_CBS_CASH == 1 )
typedef struct CASHItem_s {
    TickType_t xBudget;
    TickType_t xDeadline;
} CASHItem_t;

typedef struct CASHQueue_s {
    CASHItem_t *xCASHItems;
    size_t sHead;
    size_t sSize;
    size_t sMaxSize;    
} CASHQueue_t;

#define CASH_QUEUE_SIZE 16

/* xCASHQueue is sorted in ascending order in terms of xDeadline. */
PRIVILEGED_DATA static CASHQueue_t xCASHQueue;


#define vServerCBSIsEmptyCASHQueue() ((BaseType_t) (xCASHQueue.sSize == 0))

#define vServerCBSPeakCASHQueue() (xCASHQueue.sSize == 0 ? NULL : &xCASHQueue.xCASHItems[xCASHQueue.sHead])

#define SWAP(a, b) do { typeof(a) t; t = a; a = b; b = t; } while(0)

void vServerCBSDecrementCASHQueue(void);
void vServerCBSQueueCASH(TickType_t xLeftover, TickType_t xDeadline);

#endif /* configCBS_CASH */

#if( INCLUDE_vTaskDelete == 1 )

	PRIVILEGED_DATA static List_t xTasksWaitingTermination;				/*< Tasks that have been deleted - but their memory not yet freed. */
	PRIVILEGED_DATA static volatile UBaseType_t uxDeletedTasksWaitingCleanUp = ( UBaseType_t ) 0U;

#endif

#if ( INCLUDE_vTaskSuspend == 1 )

	PRIVILEGED_DATA static List_t xSuspendedTaskList[CORES];					/*< Tasks that are currently suspended. */

#endif

/* Other file private variables. --------------------------------*/
PRIVILEGED_DATA static volatile UBaseType_t uxCurrentNumberOfTasks[CORES]   = {( UBaseType_t ) 0U};
PRIVILEGED_DATA static volatile TickType_t xTickCount[CORES]                = {( TickType_t ) 0U};
// TODO POSSIBLE BUG WITH THIS ONE!
PRIVILEGED_DATA static volatile UBaseType_t uxTopReadyPriority[CORES]       = {tskIDLE_PRIORITY};
PRIVILEGED_DATA static volatile BaseType_t xSchedulerRunning[CORES]         = {pdFALSE};
PRIVILEGED_DATA static volatile UBaseType_t uxPendedTicks[CORES]            = {( UBaseType_t ) 0U};
PRIVILEGED_DATA static volatile BaseType_t xYieldPending[CORES]             = {pdFALSE};
PRIVILEGED_DATA static volatile BaseType_t xNumOfOverflows[CORES]           = {( BaseType_t ) 0};
PRIVILEGED_DATA static UBaseType_t uxTaskNumber[CORES]                      = {(UBaseType_t ) 0U};
PRIVILEGED_DATA static volatile TickType_t xNextTaskUnblockTime[CORES]      = {( TickType_t ) 0U}; /* Initialised to portMAX_DELAY before the scheduler starts. */
PRIVILEGED_DATA static TaskHandle_t xIdleTaskHandle[CORES]                  = {NULL};			/*< Holds the handle of the idle task.  The idle task is created automatically when the scheduler is started. */

/* Context switches are held pending while the scheduler is suspended.  Also,
interrupts must not manipulate the xStateListItem of a TCB, or any of the
lists the xStateListItem can be referenced from, if the scheduler is suspended.
If an interrupt needs to unblock a task while the scheduler is suspended then it
moves the task's event list item into the xPendingReadyList, ready for the
kernel to move the task from the pending ready list into the real ready list
when the scheduler is unsuspended.  The pending ready list itself can only be
accessed from a critical section. */
PRIVILEGED_DATA static volatile UBaseType_t uxSchedulerSuspended[CORES]	= {( UBaseType_t ) pdFALSE};

#if ( configGENERATE_RUN_TIME_STATS == 1 )

	PRIVILEGED_DATA static uint32_t ulTaskSwitchedInTime = 0UL;	/*< Holds the value of a timer/counter the last time a task was switched in. */
	PRIVILEGED_DATA static uint32_t ulTotalRunTime = 0UL;		/*< Holds the total amount of execution time as defined by the run time counter clock. */

#endif

/*lint +e956 */

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

/**
 * @brief Simulate task execution by waiting for a certain number of ticks to elapse while this task
 *        being.executed.
 * @param ticks The number of ticks to keep the processor busy for.
 **/
void vBusyWait(TickType_t ticks)
{
    while(pxCurrentTCB[portCPUID()]->xCurrentRunTime < ticks);
    return;
}

void vBusyWait2(TickType_t ticks)
{
    TickType_t xInitTick = pxCurrentTCB[portCPUID()]->xCurrentRunTime;
    TickType_t xEndTick = xInitTick + ticks;
    printk("xInitTick: %u, xEndTick: %u\r\n", xInitTick, xEndTick);
    while(pxCurrentTCB[portCPUID()]->xCurrentRunTime < xEndTick);
    return;
}

/**
 * @brief Prints the status of the currently scheduled tasks
 **/
void vPrintSchedule(void)
{
    List_t* pxReadyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    ListItem_t* pxCurrentItem = listGET_HEAD_ENTRY(pxReadyList);
    ListItem_t const* pxEndMarker = listGET_END_MARKER(pxReadyList);
    while(pxCurrentItem != pxEndMarker)
    {
        TCB_t* pxTCB = listGET_LIST_ITEM_OWNER(pxCurrentItem);
        #if ( configUSE_SRP == 1 )
            printk("Task %s, relative deadline %u, remaining %u, wcet %u, preemption level: %u\r\n",
                   pxTCB->pcTaskName,
                   pxTCB->xRelativeDeadline,
                   pxCurrentItem->xItemValue,
                   pxTCB->xWCET,
                   pxTCB->xPreemptionLevel);
        #else
            printk("Task %s, deadline %u, remaining %u\r\n",
                   pxTCB->pcTaskName,
                   (uint32_t) pxTCB->xRelativeDeadline,
                   (uint32_t) pxCurrentItem->xItemValue);
        #endif
        pxCurrentItem = listGET_NEXT(pxCurrentItem);
    }
}

/*-----------------------------------------------------------*/

/* Callback function prototypes. --------------------------*/
#if(  configCHECK_FOR_STACK_OVERFLOW > 0 )
	extern void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName );
#endif

#if( configUSE_TICK_HOOK > 0 )
	extern void vApplicationTickHook( void );
#endif

#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	extern void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );
#endif

/* File private functions. --------------------------------*/

/**
 * Utility task that simply returns pdTRUE if the task referenced by xTask is
 * currently in the Suspended state, or pdFALSE if the task referenced by xTask
 * is in any other state.
 */
#if ( INCLUDE_vTaskSuspend == 1 )
	static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask ) PRIVILEGED_FUNCTION;
#endif /* INCLUDE_vTaskSuspend */

/*
 * Utility to ready all the lists used by the scheduler.  This is called
 * automatically upon the creation of the first task.
 */
static void prvInitialiseTaskLists( void ) PRIVILEGED_FUNCTION;

/*
 * The idle task, which as all tasks is implemented as a never ending loop.
 * The idle task is automatically created and added to the ready lists upon
 * creation of the first user task.
 *
 * The portTASK_FUNCTION_PROTO() macro is used to allow port/compiler specific
 * language extensions.  The equivalent prototype for this function is:
 *
 * void prvIdleTask( void *pvParameters );
 *
 */
static portTASK_FUNCTION_PROTO( prvIdleTask, pvParameters );

/*
 * Utility to free all memory allocated by the scheduler to hold a TCB,
 * including the stack pointed to by the TCB.
 *
 * This does not free memory allocated by the task itself (i.e. memory
 * allocated by calls to pvPortMalloc from within the tasks application code).
 */
#if ( INCLUDE_vTaskDelete == 1 )

	static void prvDeleteTCB( TCB_t *pxTCB ) PRIVILEGED_FUNCTION;

#endif

/*
 * Used only by the idle task.  This checks to see if anything has been placed
 * in the list of tasks waiting to be deleted.  If so the task is cleaned up
 * and its TCB deleted.
 */
static void prvCheckTasksWaitingTermination( void ) PRIVILEGED_FUNCTION;

/*
 * The currently executing task is entering the Blocked state.  Add the task to
 * either the current or the overflow delayed task list.
 */
static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait, const BaseType_t xCanBlockIndefinitely ) PRIVILEGED_FUNCTION;

/*
 * Fills an TaskStatus_t structure with information on each task that is
 * referenced from the pxList list (which may be a ready list, a delayed list,
 * a suspended list, etc.).
 *
 * THIS FUNCTION IS INTENDED FOR DEBUGGING ONLY, AND SHOULD NOT BE CALLED FROM
 * NORMAL APPLICATION CODE.
 */
#if ( configUSE_TRACE_FACILITY == 1 )

	static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t *pxTaskStatusArray, List_t *pxList, eTaskState eState ) PRIVILEGED_FUNCTION;

#endif

/*
 * Searches pxList for a task with name pcNameToQuery - returning a handle to
 * the task if it is found, or NULL if the task is not found.
 */
#if ( INCLUDE_xTaskGetHandle == 1 )

	static TCB_t *prvSearchForNameWithinSingleList( List_t *pxList, const char pcNameToQuery[] ) PRIVILEGED_FUNCTION;

#endif

/*
 * When a task is created, the stack of the task is filled with a known value.
 * This function determines the 'high water mark' of the task stack by
 * determining how much of the stack remains at the original preset value.
 */
#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static uint16_t prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte ) PRIVILEGED_FUNCTION;

#endif

/*
 * Return the amount of time, in ticks, that will pass before the kernel will
 * next move a task from the Blocked state to the Running state.
 *
 * This conditional compilation should use inequality to 0, not equality to 1.
 * This is to ensure portSUPPRESS_TICKS_AND_SLEEP() can be called when user
 * defined low power mode implementations require configUSE_TICKLESS_IDLE to be
 * set to a value other than 1.
 */
#if ( configUSE_TICKLESS_IDLE != 0 )

	static TickType_t prvGetExpectedIdleTime( void ) PRIVILEGED_FUNCTION;

#endif

/*
 * Set xNextTaskUnblockTime to the time at which the next Blocked state task
 * will exit the Blocked state.
 */
static void prvResetNextTaskUnblockTime( void );

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	/*
	 * Helper function used to pad task names with spaces when printing out
	 * human readable tables of task information.
	 */
	static char *prvWriteNameToBuffer( char *pcBuffer, const char *pcTaskName ) PRIVILEGED_FUNCTION;

#endif

/*
 * Called after a Task_t structure has been allocated either statically or
 * dynamically to fill in the structure's members.
 */
static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									TaskHandle_t * const pxCreatedTask,
									TCB_t *pxNewTCB,
									const MemoryRegion_t * const xRegions ) PRIVILEGED_FUNCTION; /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

/*
 * Called after a new task has been created and initialised to place the task
 * under the control of the scheduler.
 */
static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB ) PRIVILEGED_FUNCTION;


#if( configUSE_SCHEDULER_EDF == 1  && configUSE_CBS_SERVER == 1)
    
typedef struct JobItem_s {
    TaskFunction_t pxJobCode;
    void * pvParameters;
} JobItem_t;

#define SERVER_JOB_QUEUE_SIZE 16
typedef struct JobQueue_s {
    JobItem_t *pxJobItems;
    size_t sHead;
    size_t sSize;
    size_t sMaxSize;
} JobQueue_t;


/**
 * Place a job onto the server's job queue
 * @param pxServer handle to the server task
 * @param pxJobCode address to the code for the job
 * @param pvParameters parameters to pass in when executing the job
 * @return pdTRUE if job is successfully queued, pdFALSE otherwise
 */
static BaseType_t vServerCBSQueueJob( TCB_t *pxServerTCB, TaskFunction_t pxJobCode, void * pvParameters );
void vServerCBSReplenish( TCB_t* pxServerTCB );
void vServerCBSRefresh( TCB_t* pxServerTCB );
#endif


#if( configUSE_SRP == 1 )
typedef struct Resource_s
{
    ListItem_t xResourceItem;
    TCB_t* pxOwner;
    StackType_t *xCeilPtr; // Pointer to the resource ceiling value of a task
    SemaphoreHandle_t xSemaphore;
} Resource_t;

typedef struct Stack_s
{
    StackType_t *xStackTop; // Last item on the stack
    StackType_t *xStackTopLimit; // The highest memory address that stack can have    
    StackType_t *xStackBottomLimit; // The lowest memory address that stack can grow
} Stack_t;

/* Invariant: The size of (number of values in) mSysCeilStack should always be at least 1. */
static Stack_t mSysCeilStack;

static StackType_t srpSysCeilStackPeak(void);

static void vSRPSysCeilStackPop(void);
static void vSRPTCBSemaphoreGive(TCB_t* tcb);

static void srpSysCeilStackPush(StackType_t vStackVar);

static void srpStackPush(Stack_t *vStackT, StackType_t vStackVar);

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
static Stack_t mRuntimeStack;
static Stack_t mTempStack;
PRIVILEGED_DATA Stack_t * volatile pxTempStack = &mTempStack;

/* Pop the current task's run stack off of the runtime stack. */
static void srpRuntimeStackPopTask(void);
static void srpInitTaskRuntimeStack(TCB_t *vTCB);

#endif /* configUSE_SHARED_RUNTIME_STACK */

#endif /* configUSE_SRP */

#if( configUSE_GLOBAL_EDF == 1 )

#include <syncqueue.h>

#define NUM_CORES 4
typedef struct CoreInfoBlock_s {
    TCB_t * volatile pxCurrentTask;
    TCB_t * volatile pxPendingTask; // Pending task that is going to preempt current task
    // pxIdleTask should only be set once by the kernel core during scheudler startup
    // after which it is safe to read this field without locking.
    TCB_t * pxIdleTask;
#define MSG_NONE 0
#define MSG_SWITCH_TASK 1
#define MSG_FINISHED_TASK 2
#define MSG_SET_TASK_PHASE 3
    SpinLock_t xLock;
} CoreInfoBlock_t;
static uint32_t ulNumCoresUsed = 0;
// NUM_CORES number of core info struct for symmetry, kernel core entry is not used.
PRIVILEGED_DATA CoreInfoBlock_t mCoreInfo[NUM_CORES];

#define MAX_FINISH_QUEUE_SIZE 32
PRIVILEGED_DATA SyncQueue_t xFinishedTaskQueue;

PRIVILEGED_DATA SpinLock_t xNumIdleCoresLock = 0;
PRIVILEGED_DATA uint32_t ulNumIdleCores = 0;

uint32_t xGetNumIdleCores(void) {
    uint32_t val;
    portSPIN_LOCK_ACQUIRE( &xNumIdleCoresLock );
    val = ulNumIdleCores;
    portSPIN_LOCK_RELEASE( &xNumIdleCoresLock );
    return val;
}    

#define DEC_NUM_IDLE_CORES() \
    portSPIN_LOCK_ACQUIRE( &xNumIdleCoresLock ); \
    ulNumIdleCores--; \
    portSPIN_LOCK_RELEASE( &xNumIdleCoresLock )

#define INC_NUM_IDLE_CORES() \
    portSPIN_LOCK_ACQUIRE( &xNumIdleCoresLock ); \
    ulNumIdleCores++; \
    portSPIN_LOCK_RELEASE( &xNumIdleCoresLock )

TCB_t *xGetSecondaryCoreCurrentTask(uint32_t cpuid);
TCB_t *xGetSecondaryCorePendingTask(uint32_t cpuid);
void vSetSecondaryCorePendingTask(uint32_t cpuid, TCB_t *xTask);
void vSetSecondaryCorePendingTask(uint32_t cpuid, TCB_t *xTCB);
void vCoreSignalTaskEnd(void);
void vCoreSignalTaskEndPhase(TickType_t ticks);

void prvSecondaryIdleTask(void * param);

#endif

/*-----------------------------------------------------------*/

#if( configSUPPORT_STATIC_ALLOCATION == 1 )

	TaskHandle_t xTaskCreateStatic(	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									StackType_t * const puxStackBuffer,
									StaticTask_t * const pxTaskBuffer ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
	{
	TCB_t *pxNewTCB;
	TaskHandle_t xReturn;

		configASSERT( puxStackBuffer != NULL );
		configASSERT( pxTaskBuffer != NULL );

		if( ( pxTaskBuffer != NULL ) && ( puxStackBuffer != NULL ) )
		{
			/* The memory used for the task's TCB and stack are passed into this
			function - use them. */
			pxNewTCB = ( TCB_t * ) pxTaskBuffer; /*lint !e740 Unusual cast is ok as the structures are designed to have the same alignment, and the size is checked by an assert. */
			pxNewTCB->pxStack = ( StackType_t * ) puxStackBuffer;

			#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
			{
				/* Tasks can be created statically or dynamically, so note this
				task was created statically in case the task is later deleted. */
				pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_AND_TCB;
			}
			#endif /* configSUPPORT_DYNAMIC_ALLOCATION */

			prvInitialiseNewTask( pxTaskCode, pcName, ulStackDepth, pvParameters, uxPriority, &xReturn, pxNewTCB, NULL );
			prvAddNewTaskToReadyList( pxNewTCB );
		}
		else
		{
			xReturn = NULL;
		}

		return xReturn;
	}

#endif /* SUPPORT_STATIC_ALLOCATION */
/*-----------------------------------------------------------*/

#if( portUSING_MPU_WRAPPERS == 1 )

	BaseType_t xTaskCreateRestricted( const TaskParameters_t * const pxTaskDefinition, TaskHandle_t *pxCreatedTask )
	{
	TCB_t *pxNewTCB;
	BaseType_t xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;

		configASSERT( pxTaskDefinition->puxStackBuffer );

		if( pxTaskDefinition->puxStackBuffer != NULL )
		{
			/* Allocate space for the TCB.  Where the memory comes from depends
			on the implementation of the port malloc function and whether or
			not static allocation is being used. */
			pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

			if( pxNewTCB != NULL )
			{
				/* Store the stack location in the TCB. */
				pxNewTCB->pxStack = pxTaskDefinition->puxStackBuffer;

				/* Tasks can be created statically or dynamically, so note
				this task had a statically allocated stack in case it is
				later deleted.  The TCB was allocated dynamically. */
				pxNewTCB->ucStaticallyAllocated = tskSTATICALLY_ALLOCATED_STACK_ONLY;

				prvInitialiseNewTask(	pxTaskDefinition->pvTaskCode,
										pxTaskDefinition->pcName,
										( uint32_t ) pxTaskDefinition->usStackDepth,
										pxTaskDefinition->pvParameters,
										pxTaskDefinition->uxPriority,
										pxCreatedTask, pxNewTCB,
										pxTaskDefinition->xRegions );

				prvAddNewTaskToReadyList( pxNewTCB );
				xReturn = pdPASS;
			}
		}

		return xReturn;
	}

#endif /* portUSING_MPU_WRAPPERS */
/*-----------------------------------------------------------*/

#if (configUSE_SCHEDULER_EDF == 1)
/**
 *  @brief Retrieves the total utilization of all created tasks.
 *  @return The current task set utilization.
 **/
float getTotalUtilization(void)
{
    float fCurrentUtilization = 0;

    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    while(currentItem != endMarker)
    {
        TCB_t* tcb = listGET_LIST_ITEM_OWNER(currentItem);
        fCurrentUtilization += (float) tcb->xWCET / tcb->xPeriod;
        currentItem = listGET_NEXT( currentItem );
    }

    return fCurrentUtilization;
}

#if ( configUSE_MULTICORE == 1 )
float fGetCoreTotalUtilization(uint32_t core)
{
    float fCurrentUtilization = 0;

    List_t* readyList = &pxReadyTasksLists[core][PRIORITY_EDF];
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    while(currentItem != endMarker)
    {
        TCB_t* tcb = listGET_LIST_ITEM_OWNER(currentItem);
        fCurrentUtilization += (float) tcb->xWCET / tcb->xPeriod;
        currentItem = listGET_NEXT( currentItem );
    }

    return fCurrentUtilization;
}
#endif

/**
 *  @brief Retrieves L* for the currently specified task set
 *  @return L* for all scheduled tasks
 **/
float getEDFLStart(void)
{
    float fTotalUtilization = getTotalUtilization();
    float fLStar = 0;

    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    while(currentItem != endMarker)
    {
        TCB_t* tcb = listGET_LIST_ITEM_OWNER(currentItem);
        fLStar += (tcb->xPeriod - tcb->xRelativeDeadline) * (((float) tcb->xWCET) / tcb->xPeriod);;
        currentItem = listGET_NEXT(currentItem);
    }

    return fLStar / ( 1- fTotalUtilization );
}

/**
 * @brief Generate the LL bound for the scheduled task set, compare the utilization and report the
 *        result.
 **/
void vVerifyLLBound(void)
{
    // TODO: Ensure that the scheduler is not running
    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    uint32_t ulNumTasks = listCURRENT_LIST_LENGTH(readyList);
    float fLLBound = (2 * (powf(2, 1 / (float) ulNumTasks) - 1));
    float fTotalUtilization = getTotalUtilization();

    if (fTotalUtilization > fLLBound)
        printk("Failed to meet LL bound requirements!\r\n");
    else
        printk("LL bound requirements met!\r\n");
}


/**
 * @brief Verify that the scheduled task set is schedulable by carrying our the exact EDF demand
 *        analysis.
 * @note This function will assert should the task set be unschedulable.
 **/
void vVerifyEDFExactBound(void)
{
    // TODO: Ensure that the scheduler is not running
    float fTotalUtilization = getTotalUtilization();
    printk("Total U is: %d/100\r\n", (int32_t) (fTotalUtilization * 100));
    if( fTotalUtilization > 1 ) {
        printk("Task Set Utilization Over 100%!\r\n");
        configASSERT( pdFALSE );
    }

    float fLStar = getEDFLStart();
    printk("L* is: %d\r\n", ((int32_t) fLStar));

    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    //check all absolute deadlines by iterating through all periods of each task
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    while( currentItem != endMarker )
    {
        //get task pointer
        TCB_t* tcb = listGET_LIST_ITEM_OWNER( currentItem );

        //verify that demand at time L is less than L, where L is equal to
        //the task's absolute deadlines up to lStar
        for( TickType_t L = tcb->xRelativeDeadline; L <= fLStar; L += tcb->xPeriod )
        {
            float totalDemand = 0;
            //find total demand at time L by summing the demand of each task
            ListItem_t* currentItem2 = listGET_HEAD_ENTRY(readyList);
            ListItem_t const* endMarker2 = listGET_END_MARKER(readyList);
            while( currentItem2 != endMarker2 )
            {
                TCB_t* tcb2 = listGET_LIST_ITEM_OWNER( currentItem2 );
                int temp = ( L + tcb2->xPeriod - tcb2->xRelativeDeadline ) / tcb2->xPeriod;
                totalDemand += temp * tcb2->xWCET;
                currentItem2 = listGET_NEXT( currentItem2 );
            }
            if( totalDemand > L ) {
                printk( "Fail at L = %d, total demand = %d\r\n", (int32_t) L, (int32_t) totalDemand);
                while(1);
                return ;
            }
            printk( "Success at L = %d, total demand = %d\r\n", (int32_t) L, (int32_t) totalDemand);
        }

        currentItem = listGET_NEXT( currentItem );
    }
    printk("Task set has passed exact deadline analysis!\r\n");
    return;

}

#if ( configUSE_SRP == 1 )
/**
 * @brief Verify that the scheduled task set is schedulable by carrying out exact EDF demand
 *        analysis.
 * @note This function will assert should the task set be schedulable.
 **/
void verifyEDFExactBound2(void)
{
    // TODO: Ensure that the scheduler is not running
    float fTotalUtilization = getTotalUtilization();
    printk("Total U is: %d\r\n", (int32_t) (fTotalUtilization * 100));
    if( fTotalUtilization > 1 ) {
        return;
    }
    
    float fLStar = getEDFLStart();
    printk("L* is: %d\r\n", ((int32_t) fLStar));
    
    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    //check all absolute deadlines by iterating through all periods of each task
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    while( currentItem != endMarker )
    {
        //get task pointer
        TCB_t* tcb = listGET_LIST_ITEM_OWNER( currentItem );
        
        //verify that demand at time L is less than L, where L is equal to
        //the task's absolute deadlines up to lStar
        for( TickType_t L = tcb->xRelativeDeadline; L <= fLStar; L += tcb->xPeriod )
        {
            float totalDemand = 0;
            //find total demand at time L by summing the demand of each task
            ListItem_t* currentItem2 = listGET_HEAD_ENTRY(readyList);
            ListItem_t const* endMarker2 = listGET_END_MARKER(readyList);
            while( currentItem2 != endMarker2 )
            {
                TCB_t* tcb2 = listGET_LIST_ITEM_OWNER( currentItem2 );
                int temp = ( L + tcb2->xPeriod - tcb2->xRelativeDeadline ) / tcb2->xPeriod;
                totalDemand += temp * tcb2->xWCET;
                currentItem2 = listGET_NEXT( currentItem2 );
            }
            //ADDED BLOCKING TIME TO DEMAND
            if(( totalDemand + tcb->xBlockTime ) > L ) {
                printk( "Fail at L = %d, total demand = %d\r\n", (int32_t) L, (int32_t) totalDemand);
                while(1);
                return ;
            }
            printk( "Success at L = %d, total demand = %d\r\n", (int32_t) L, (int32_t) totalDemand);
        }
        
        currentItem = listGET_NEXT( currentItem );
    }
    printk("Task set has passed exact deadline analysis!\r\n");
    return;
    
}


/**
 * @brief Verify that the scheduled task set is schedulable by carrying out blocking time analysis for SRP.
 * @note This function will assert should the task set be schedulable.
 **/
void verifyEDFExactBound3(void)
{
    //EQUATION FROM PAGE 50, DOESNT SEEM TO TAKE INTO ACCOUNT WHEN DEADLINE DIFFERENT FROM PERIOD
    List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
    //check all absolute deadlines by iterating through all periods of each task
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    while( currentItem != endMarker )
    {
        //get task pointer
        TCB_t* tcb = listGET_LIST_ITEM_OWNER( currentItem );
        float sum = ( tcb->xBlockTime / tcb->xPeriod );
        // ^^^^^^^^^^^^^^^NEEDS CHANGED PROB
        
        ListItem_t* currentItem2 = listGET_HEAD_ENTRY(readyList);
        while( currentItem2 != currentItem )
        {
            //get task pointer
            TCB_t* tcb2 = listGET_LIST_ITEM_OWNER( currentItem2 );
            sum += tcb2->xWCET / tcb2->xPeriod;
            currentItem2 = listGET_NEXT( currentItem2 );
        }
        if ( sum > 1 ) {
            printk("fail\r\n");
            while(1);
        } else {
            printk("pass\r\n");
            while(1);
        }
        currentItem = listGET_NEXT( currentItem );
    }
}
#endif /*  configUSE_SRP == 1 */

/**
 *  @brief Signals the completion of the current task's execution during this period and places it
 *         on the wait queue for the next period.
 **/
void vEndTaskPeriod(void)
{
#if( configUSE_GLOBAL_EDF == 1 )
    if (portCPUID() == portKERNEL_CORE) {
        vTaskDelayUntil( &(pxCurrentTCB[portCPUID()]->xLastWakeTime), pxCurrentTCB[portCPUID()]->xPeriod );
    }
    else {
        vCoreSignalTaskEnd();
        // Yield to save context and switch to idle. This forces the
        // core to go into supervisor mode where context switch occurs.
        portSECONDARY_CORE_YIELD();
        //portDISABLE_INTERRUPTS();        
        //printk("Stopping Core: %u\r\n", portCPUID());
        //while(1);
    }
#else /* configUSE_GLOBAL_EDF */
    vTaskDelayUntil( &(pxCurrentTCB[portCPUID()]->xLastWakeTime), pxCurrentTCB[portCPUID()]->xPeriod );    
#endif /* configUSE_GLOBAL_EDF */    
}

void vEndTask(TickType_t ticks) {
#if ( configUSE_GLOBAL_EDF == 1 )
    if (portCPUID() == portKERNEL_CORE) {
        vTaskDelayUntil( &(pxCurrentTCB[portCPUID()]->xLastWakeTime), ticks );
    }
    else {
        vCoreSignalTaskEndPhase( ticks );
        portSECONDARY_CORE_YIELD();
    }
#else /* configUSE_GLOBAL_EDF */
    vTaskDelayUntil( &(pxCurrentTCB[portCPUID()]->xLastWakeTime), ticks );    
#endif /* configUSE_GLOBAL_EDF */
}

#endif /* configUSE_SCHEDULER_EDF */

#if( configSUPPORT_DYNAMIC_ALLOCATION == 1 )
    #if( configUSE_SCHEDULER_EDF == 1 )
	BaseType_t xTaskCreate(	TaskFunction_t pxTaskCode,
							const char * const pcName,
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
                            uint16_t usStackDepth,
#else
							const uint16_t usStackDepth,
#endif /* configUSE_SHARED_RUNTIME_STACK */
							void * const pvParameters,
							UBaseType_t uxPriority,
                            TickType_t xWCET,
                            TickType_t xRelativeDeadline,
                            TickType_t xPeriod,                            
							TaskHandle_t * const pxCreatedTask ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
    #else
	BaseType_t xTaskCreate(	TaskFunction_t pxTaskCode,
							const char * const pcName,
							const uint16_t usStackDepth,
							void * const pvParameters,
							UBaseType_t uxPriority,
							TaskHandle_t * const pxCreatedTask ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
    #endif /* configUSE_SCHEDULER_EDF */
	{
	TCB_t *pxNewTCB;
	BaseType_t xReturn;

#if( configUSE_SHARED_RUNTIME_STACK == 1)
    if ( pxTaskCode != prvIdleTask ) {
        pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );        
        usStackDepth = 0;
    }
    else {
#endif /* configUSE_SHARED_RUNTIME_STACK */
        
		/* If the stack grows down then allocate the stack then the TCB so the stack
		does not grow into the TCB.  Likewise if the stack grows up then allocate
		the TCB then the stack. */
		#if( portSTACK_GROWTH > 0 )
		{
			/* Allocate space for the TCB.  Where the memory comes from depends on
			the implementation of the port malloc function and whether or not static
			allocation is being used. */
			pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) );

			if( pxNewTCB != NULL )
			{
				/* Allocate space for the stack used by the task being created.
				The base of the stack memory stored in the TCB so the task can
				be deleted later if required. */
				pxNewTCB->pxStack = ( StackType_t * ) pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

				if( pxNewTCB->pxStack == NULL )
				{
					/* Could not allocate the stack.  Delete the allocated TCB. */
					vPortFree( pxNewTCB );
					pxNewTCB = NULL;
				}
			}
		}
		#else /* portSTACK_GROWTH */
		{
		StackType_t *pxStack;

			/* Allocate space for the stack used by the task being created. */
			pxStack = ( StackType_t * ) pvPortMalloc( ( ( ( size_t ) usStackDepth ) * sizeof( StackType_t ) ) ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

			if( pxStack != NULL )
			{
				/* Allocate space for the TCB. */
				pxNewTCB = ( TCB_t * ) pvPortMalloc( sizeof( TCB_t ) ); /*lint !e961 MISRA exception as the casts are only redundant for some paths. */

				if( pxNewTCB != NULL )
				{
					/* Store the stack location in the TCB. */
					pxNewTCB->pxStack = pxStack;
				}
				else
				{
					/* The stack cannot be used as the TCB was not created.  Free
					it again. */
					vPortFree( pxStack );
				}
			}
			else
			{
				pxNewTCB = NULL;
			}
		}
		#endif /* portSTACK_GROWTH */
        
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    }
#endif /* configUSE_SHARED_RUNTIME_STACK */
        
		if( pxNewTCB != NULL )
		{
			#if( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0 )
			{
				/* Tasks can be created statically or dynamically, so note this
				task was created dynamically in case it is later deleted. */
				pxNewTCB->ucStaticallyAllocated = tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB;
			}
			#endif /* configSUPPORT_STATIC_ALLOCATION */

                        // Record all the information used by the EDF scheduler to handle
                        // rescheduling and priority assignment.
                        #if( configUSE_SCHEDULER_EDF == 1 )
                        {
                            pxNewTCB->xStateListItem.xItemValue = xRelativeDeadline;
                            pxNewTCB->xPeriod = xPeriod;
                            pxNewTCB->xRelativeDeadline = xRelativeDeadline;
                            pxNewTCB->xWCET = xWCET;
                            pxNewTCB->xLastWakeTime = xTaskGetTickCount();
                            pxNewTCB->pxTaskCode = pxTaskCode;
                            pxNewTCB->usStackDepth = usStackDepth;
                            pxNewTCB->xNoPreserve = pdFALSE;
                            pxNewTCB->pvParameters = pvParameters;
                            #if ( configUSE_SRP == 1 )
                                pxNewTCB->xBlockTime = 0;
                            #endif /* configUSE_SRP */
                            #if ( configUSE_CBS_SERVER == 1 )
                                pxNewTCB->xIsServer = pdFALSE;
                                pxNewTCB->pxJobQueue = (BaseType_t) NULL;
                            #endif /* configUSE_CBS_SERVER */
                            #if ( configUSE_GLOBAL_EDF == 1)
                                pxNewTCB->xTaskState = TASK_WAITING;
                                pxNewTCB->xTaskPhase = 0;
                            #endif /* configUSE_GLOBAL_EDF */
                            pxNewTCB->fUtilization = ( xWCET / ( MIN( xPeriod, xRelativeDeadline ) ) );
                        }
                        #endif /* configUSE_SCHEDULER_EDF */
                        #if( configUSE_SRP == 1 )
                        pxNewTCB->xBlocked = pdFALSE;
                        #endif /* configUSE_SRP */

			prvInitialiseNewTask( pxTaskCode, pcName, ( uint32_t ) usStackDepth, pvParameters, uxPriority, pxCreatedTask, pxNewTCB, NULL );
			prvAddNewTaskToReadyList( pxNewTCB );
			xReturn = pdPASS;
		}
		else
		{
			xReturn = errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY;
		}

		return xReturn;
	}

#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
/*-----------------------------------------------------------*/

static void prvInitialiseNewTask( 	TaskFunction_t pxTaskCode,
									const char * const pcName,
									const uint32_t ulStackDepth,
									void * const pvParameters,
									UBaseType_t uxPriority,
									TaskHandle_t * const pxCreatedTask,
									TCB_t *pxNewTCB,
									const MemoryRegion_t * const xRegions ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
{
StackType_t *pxTopOfStack;
UBaseType_t x;

 
	#if( portUSING_MPU_WRAPPERS == 1 )
		/* Should the task be created in privileged mode? */
		BaseType_t xRunPrivileged;
		if( ( uxPriority & portPRIVILEGE_BIT ) != 0U )
		{
			xRunPrivileged = pdTRUE;
		}
		else
		{
			xRunPrivileged = pdFALSE;
		}
		uxPriority &= ~portPRIVILEGE_BIT;
	#endif /* portUSING_MPU_WRAPPERS == 1 */

#if( configUSE_SHARED_RUNTIME_STACK == 1 )        
    if ( pxTaskCode == prvIdleTask ) {
#endif
	/* Avoid dependency on memset() if it is not required. */
	#if( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )
	{
		/* Fill the stack with a known value to assist debugging. */
		( void ) memset( pxNewTCB->pxStack, ( int ) tskSTACK_FILL_BYTE, ( size_t ) ulStackDepth * sizeof( StackType_t ) );
	}
	#endif /* ( ( configCHECK_FOR_STACK_OVERFLOW > 1 ) || ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) ) ) */

	/* Calculate the top of stack address.  This depends on whether the stack
	grows from high memory to low (as per the 80x86) or vice versa.
	portSTACK_GROWTH is used to make the result positive or negative as required
	by the port. */
	#if( portSTACK_GROWTH < 0 )
	{
		pxTopOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
        
		pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) ); /*lint !e923 MISRA exception.  Avoiding casts between pointers and integers is not practical.  Size differences accounted for using portPOINTER_SIZE_TYPE type. */

		/* Check the alignment of the calculated top of stack is correct. */
		configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
	}
	#else /* portSTACK_GROWTH */
	{
		pxTopOfStack = pxNewTCB->pxStack;

		/* Check the alignment of the stack buffer is correct. */
		configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxNewTCB->pxStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

		/* The other extreme of the stack space is required if stack checking is
		performed. */
		pxNewTCB->pxEndOfStack = pxNewTCB->pxStack + ( ulStackDepth - ( uint32_t ) 1 );
	}
	#endif /* portSTACK_GROWTH */

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
}
#endif /* configUSE_SHARED_RUNTIME_STACK */

	/* Store the task name in the TCB. */
	for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
	{
		pxNewTCB->pcTaskName[ x ] = pcName[ x ];

		/* Don't copy all configMAX_TASK_NAME_LEN if the string is shorter than
		configMAX_TASK_NAME_LEN characters just in case the memory after the
		string is not accessible (extremely unlikely). */
		if( pcName[ x ] == 0x00 )
		{
			break;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

	/* Ensure the name string is terminated in the case that the string length
	was greater or equal to configMAX_TASK_NAME_LEN. */
	pxNewTCB->pcTaskName[ configMAX_TASK_NAME_LEN - 1 ] = '\0';

	/* This is used as an array index so must ensure it's not too large.  First
	remove the privilege bit if one is present. */
	if( uxPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
	{
		uxPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	pxNewTCB->uxPriority = uxPriority;
	#if ( configUSE_MUTEXES == 1 )
	{
		pxNewTCB->uxBasePriority = uxPriority;
		pxNewTCB->uxMutexesHeld = 0;
	}
	#endif /* configUSE_MUTEXES */

	vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
	vListInitialiseItem( &( pxNewTCB->xEventListItem ) );

	/* Set the pxNewTCB as a link back from the ListItem_t.  This is so we can get
	back to	the containing TCB from a generic item in a list. */
	listSET_LIST_ITEM_OWNER( &( pxNewTCB->xStateListItem ), pxNewTCB );

	/* Event lists are always in priority order. */
	listSET_LIST_ITEM_VALUE( &( pxNewTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxPriority ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
	listSET_LIST_ITEM_OWNER( &( pxNewTCB->xEventListItem ), pxNewTCB );

	#if ( portCRITICAL_NESTING_IN_TCB == 1 )
	{
		pxNewTCB->uxCriticalNesting = ( UBaseType_t ) 0U;
	}
	#endif /* portCRITICAL_NESTING_IN_TCB */

	#if ( configUSE_APPLICATION_TASK_TAG == 1 )
	{
		pxNewTCB->pxTaskTag = NULL;
	}
	#endif /* configUSE_APPLICATION_TASK_TAG */

	#if ( configGENERATE_RUN_TIME_STATS == 1 )
	{
		pxNewTCB->ulRunTimeCounter = 0UL;
	}
        #if( configUSE_SCHEDULER_EDF == 1 )
        {
                pxNewTCB->xCurrentRunTime = 0;
        }
        #endif /* configUSE_SCHEDULER_EDF */

	#endif /* configGENERATE_RUN_TIME_STATS */

	#if ( portUSING_MPU_WRAPPERS == 1 )
	{
		vPortStoreTaskMPUSettings( &( pxNewTCB->xMPUSettings ), xRegions, pxNewTCB->pxStack, ulStackDepth );
	}
	#else
	{
		/* Avoid compiler warning about unreferenced parameter. */
		( void ) xRegions;
	}
	#endif

	#if( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )
	{
		for( x = 0; x < ( UBaseType_t ) configNUM_THREAD_LOCAL_STORAGE_POINTERS; x++ )
		{
			pxNewTCB->pvThreadLocalStoragePointers[ x ] = NULL;
		}
	}
	#endif

	#if ( configUSE_TASK_NOTIFICATIONS == 1 )
	{
		pxNewTCB->ulNotifiedValue = 0;
		pxNewTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
	}
	#endif

	#if ( configUSE_NEWLIB_REENTRANT == 1 )
	{
		/* Initialise this task's Newlib reent structure. */
		_REENT_INIT_PTR( ( &( pxNewTCB->xNewLib_reent ) ) );
	}
	#endif

	#if( INCLUDE_xTaskAbortDelay == 1 )
	{
		pxNewTCB->ucDelayAborted = pdFALSE;
	}
	#endif

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    configASSERT( mRuntimeStack.xStackTop != NULL );
    
    if( pxTaskCode != prvIdleTask ) {
        pxNewTCB->pxTopOfStack = NULL;
        pxNewTCB->pxStack = NULL;
    
        /* If scheduler has yet to start and priority of task is the highest, then
           initialize it on the runtime stack. */
        /* TODO: We assume that the scheduler can never be stopped, or else we might be
           overwriting another task's stack variables. */
        if( xSchedulerRunning[portCPUID()] == pdFALSE) {
            /* Scheduler is not running and no task has their execution halted. */
            if( pxCurrentTCB[portCPUID()] == NULL || (pxCurrentTCB[portCPUID()]->uxPriority < pxNewTCB->uxPriority) ||
                ( pxNewTCB->xStateListItem.xItemValue < pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue)) {

                // Decrement mRuntimeStack.xStackTop before passing in as argument since it is expected
                // by pxPortInitialiseStack
                pxTopOfStack = pxPortInitialiseStack( mRuntimeStack.xStackTopLimit - 1, pxTaskCode, pvParameters );
                
                pxNewTCB->pxTopOfStack = pxTopOfStack;
                pxNewTCB->pxStack = mRuntimeStack.xStackTopLimit + 1;

                /* Clear the current task's stack pointer values so that they can be
                   properly reinitialized later. */
                pxCurrentTCB[portCPUID()]->pxTopOfStack = NULL;
                pxCurrentTCB[portCPUID()]->pxStack = NULL;
            }
        }
    }
    else {
#endif /* configUSE_SHARED_RUNTIME-STACK */
        
	/* Initialize the TCB stack to look as if the task was already running,
	but had been interrupted by the scheduler.  The return address is set
	to the start of the task function. Once the stack has been initialised
	the	top of stack variable is updated. */
	#if( portUSING_MPU_WRAPPERS == 1 )
	{
		pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters, xRunPrivileged );
	}
	#else /* portUSING_MPU_WRAPPERS */
	{
		pxNewTCB->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, pxTaskCode, pvParameters );
	}
	#endif /* portUSING_MPU_WRAPPERS */
    
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    }
#endif /* configUSE_SHARED_RUNTIME_STACK */
    
	if( ( void * ) pxCreatedTask != NULL )
	{
		/* Pass the handle out in an anonymous way.  The handle can be used to
		change the created task's priority, delete the created task, etc.*/
		*pxCreatedTask = ( TaskHandle_t ) pxNewTCB;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

static void prvAddNewTaskToReadyList( TCB_t *pxNewTCB )
{
	/* Ensure interrupts don't access the task lists while the lists are being
	updated. */
	taskENTER_CRITICAL();
	{
		uxCurrentNumberOfTasks[portCPUID()]++;
		if( pxCurrentTCB[portCPUID()] == NULL )
		{
			/* There are no other tasks, or all the other tasks are in
			the suspended state - make this the current task. */
			pxCurrentTCB[portCPUID()] = pxNewTCB;

			if( uxCurrentNumberOfTasks[portCPUID()] == ( UBaseType_t ) 1 )
			{
				/* This is the first task to be created so do the preliminary
				initialisation required.  We will not recover if this call
				fails, but we will report the failure. */
				prvInitialiseTaskLists();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			/* If the scheduler is not already running, make this task the
			current task if it is the highest priority task to be created
			so far. */
			if( xSchedulerRunning[portCPUID()] == pdFALSE )
			{
                #if( configUSE_SCHEDULER_EDF == 1 )
                if( (pxCurrentTCB[portCPUID()]->uxPriority < pxNewTCB->uxPriority) ||
                    ( pxNewTCB->xStateListItem.xItemValue < pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue))
                #else
				if( pxCurrentTCB[portCPUID()]->uxPriority <= pxNewTCB->uxPriority )
                #endif
				{
					pxCurrentTCB[portCPUID()] = pxNewTCB;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

		uxTaskNumber[portCPUID()]++;

			/* Add a counter into the TCB for tracing only. */
		pxNewTCB->uxTCBNumber = uxTaskNumber[CPUID()];
		traceTASK_CREATE( pxNewTCB );

        #if ( configUSE_SRP == 1 )
        {
            if (pxNewTCB->pxTaskCode != prvIdleTask) {
                // TODO: By scanning through only ready tasks we are not allowing for
                // online scheduling.
                List_t *vReadyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];

                ListItem_t const *vEndMarker = listGET_END_MARKER( vReadyList );
                ListItem_t *vCurrentItem = listGET_HEAD_ENTRY( vReadyList );
                BaseType_t vSamePreemptLevel = pdFALSE;
                // TODO: Check that head entry going down the list results in increasing
                // relative deadlines.
                while( vCurrentItem != vEndMarker ) {
                    TCB_t *vTCB = listGET_LIST_ITEM_OWNER( vCurrentItem );
                    if (vTCB->xRelativeDeadline < pxNewTCB->xRelativeDeadline) {
                        // New task has lower preemption level so increment and keep going down.
                        vTCB->xPreemptionLevel++;
                    }
                    else if (vTCB->xRelativeDeadline == pxNewTCB->xRelativeDeadline) {
                        pxNewTCB->xPreemptionLevel = vTCB->xPreemptionLevel;
                        vSamePreemptLevel = pdTRUE;
                        break;
                    }
                    else {
                        // New task preemtpion level should be greater than this task in the list.
                        pxNewTCB->xPreemptionLevel = vTCB->xPreemptionLevel + 1;
                        break;
                    }
                    vCurrentItem = vCurrentItem->pxNext;
                }
            
                if ( vCurrentItem == vEndMarker ) {
                    pxNewTCB->xPreemptionLevel = 1;
                }
                else if ( vSamePreemptLevel == pdTRUE ) {
                    ListItem_t *vMatchedItem = vCurrentItem;
                    vCurrentItem = listGET_HEAD_ENTRY( vReadyList );
                    while( vCurrentItem != vMatchedItem ) {
                        TCB_t *vTCB = listGET_LIST_ITEM_OWNER( vCurrentItem );
                        vTCB->xPreemptionLevel--;
                        vCurrentItem = vCurrentItem->pxNext;
                    }
                }
            }
        }
        #endif

		prvAddTaskToReadyList( pxNewTCB );

		portSETUP_TCB( pxNewTCB );
	}
	taskEXIT_CRITICAL();

	if( xSchedulerRunning[portCPUID()] != pdFALSE )
	{
		/* If the created task is of a higher priority than the current task
		then it should run now. */
		if( pxCurrentTCB[portCPUID()]->uxPriority < pxNewTCB->uxPriority )
		{
            // TODO: If we are using SRP then we might need to change this
			taskYIELD_IF_USING_PREEMPTION();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )

	void vTaskDelete( TaskHandle_t xTaskToDelete )
	{
	TCB_t *pxTCB;

		taskENTER_CRITICAL();
		{
			/* If null is passed in here then it is the calling task that is
			being deleted. */
			pxTCB = prvGetTCBFromHandle( xTaskToDelete );

			/* Remove task from the ready list. */
			if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
			{
				taskRESET_READY_PRIORITY( pxTCB->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			/* Is the task waiting on an event also? */
			if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
			{
				( void ) uxListRemove( &( pxTCB->xEventListItem ) );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			/* Increment the uxTaskNumber also so kernel aware debuggers can
			detect that the task lists need re-generating.  This is done before
			portPRE_TASK_DELETE_HOOK() as in the Windows port that macro will
			not return. */
			uxTaskNumber[portCPUID()]++;

			if( pxTCB == pxCurrentTCB[portCPUID()] )
			{
				/* A task is deleting itself.  This cannot complete within the
				task itself, as a context switch to another task is required.
				Place the task in the termination list.  The idle task will
				check the termination list and free up any memory allocated by
				the scheduler for the TCB and stack of the deleted task. */
				vListInsertEnd( &xTasksWaitingTermination, &( pxTCB->xStateListItem ) );

				/* Increment the ucTasksDeleted variable so the idle task knows
				there is a task that has been deleted and that it should therefore
				check the xTasksWaitingTermination list. */
				++uxDeletedTasksWaitingCleanUp;

				/* The pre-delete hook is primarily for the Windows simulator,
				in which Windows specific clean up operations are performed,
				after which it is not possible to yield away from this task -
				hence xYieldPending[portCPUID()] is used to latch that a context switch is
				required. */
				portPRE_TASK_DELETE_HOOK( pxTCB, &xYieldPending[portCPUID()] );
			}
			else
			{
				--uxCurrentNumberOfTasks[portCPUID()];
				prvDeleteTCB( pxTCB );

				/* Reset the next expected unblock time in case it referred to
				the task that has just been deleted. */
				prvResetNextTaskUnblockTime();
			}

			traceTASK_DELETE( pxTCB );
		}
		taskEXIT_CRITICAL();

		/* Force a reschedule if it is the currently running task that has just
		been deleted. */
		if( xSchedulerRunning[portCPUID()] != pdFALSE )
		{
			if( pxTCB == pxCurrentTCB[portCPUID()] )
			{
				configASSERT( uxSchedulerSuspended[portCPUID()] == 0 );
				portYIELD_WITHIN_API();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
	}

#endif /* INCLUDE_vTaskDelete */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelayUntil == 1 )
	void vTaskDelayUntil( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement )
	{
	TickType_t xTimeToWake;
	BaseType_t xAlreadyYielded, xShouldDelay = pdFALSE;

		configASSERT( pxPreviousWakeTime );
		configASSERT( ( xTimeIncrement > 0U ) );
		configASSERT( uxSchedulerSuspended[portCPUID()] == 0 );

		vTaskSuspendAll();
		{
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
            srpRuntimeStackPopTask();
#endif
            
			/* Minor optimisation.  The tick count cannot change in this
			block. */
			const TickType_t xConstTickCount = xTickCount[portCPUID()];

			/* Generate the tick time at which the task wants to wake. */
			xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

			if( xConstTickCount < *pxPreviousWakeTime )
			{
				/* The tick count has overflowed since this function was
				lasted called.  In this case the only time we should ever
				actually delay is if the wake time has also	overflowed,
				and the wake time is greater than the tick time.  When this
				is the case it is as if neither time had overflowed. */
				if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				/* The tick time has not overflowed.  In this case we will
				delay if either the wake time has overflowed, and/or the
				tick time is less than the wake time. */
				if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}

			/* Update the wake time ready for the next call. */
			*pxPreviousWakeTime = xTimeToWake;

			if( xShouldDelay != pdFALSE )
			{
				traceTASK_DELAY_UNTIL( xTimeToWake );

				/* prvAddCurrentTaskToDelayedList() needs the block time, not
				the time to wake, so subtract the current tick count. */
				prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
			}
			else
			{
			mtCOVERAGE_TEST_MARKER();
			}
		}
		xAlreadyYielded = xTaskResumeAll();

		/* Force a reschedule if xTaskResumeAll has not already done so, we may
		have put ourselves to sleep. */
		if( xAlreadyYielded == pdFALSE )
		{
			portYIELD_WITHIN_API();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#if ( configUSE_GLOBAL_EDF == 1 )
	void vTaskDelayUntil2( TickType_t * const pxPreviousWakeTime, const TickType_t xTimeIncrement )
	{
	TickType_t xTimeToWake;
	BaseType_t xShouldDelay = pdFALSE;

		configASSERT( pxPreviousWakeTime );
		configASSERT( ( xTimeIncrement > 0U ) );
		configASSERT( uxSchedulerSuspended[portCPUID()] == 0 );
		{
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
            srpRuntimeStackPopTask();
#endif
            
			/* Minor optimisation.  The tick count cannot change in this
			block. */
			const TickType_t xConstTickCount = xTickCount[portCPUID()];

			/* Generate the tick time at which the task wants to wake. */
			xTimeToWake = *pxPreviousWakeTime + xTimeIncrement;

			if( xConstTickCount < *pxPreviousWakeTime )
			{
				/* The tick count has overflowed since this function was
				lasted called.  In this case the only time we should ever
				actually delay is if the wake time has also	overflowed,
				and the wake time is greater than the tick time.  When this
				is the case it is as if neither time had overflowed. */
				if( ( xTimeToWake < *pxPreviousWakeTime ) && ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				/* The tick time has not overflowed.  In this case we will
				delay if either the wake time has overflowed, and/or the
				tick time is less than the wake time. */
				if( ( xTimeToWake < *pxPreviousWakeTime ) || ( xTimeToWake > xConstTickCount ) )
				{
					xShouldDelay = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}

			/* Update the wake time ready for the next call. */
			*pxPreviousWakeTime = xTimeToWake;

			if( xShouldDelay != pdFALSE )
			{
				traceTASK_DELAY_UNTIL( xTimeToWake );

				/* prvAddCurrentTaskToDelayedList() needs the block time, not
				the time to wake, so subtract the current tick count. */
				prvAddCurrentTaskToDelayedList( xTimeToWake - xConstTickCount, pdFALSE );
			}
			else
			{
                mtCOVERAGE_TEST_MARKER();
			}

		}
	}
#endif /* configUSE_GLOBAL_EDF */

#endif /* INCLUDE_vTaskDelayUntil */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelay == 1 )

	void vTaskDelay( const TickType_t xTicksToDelay )
	{
	BaseType_t xAlreadyYielded = pdFALSE;

		/* A delay time of zero just forces a reschedule. */
		if( xTicksToDelay > ( TickType_t ) 0U )
		{
			configASSERT( uxSchedulerSuspended[portCPUID()] == 0 );
			vTaskSuspendAll();
			{
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
                srpRuntimeStackPopTask();
#endif
				traceTASK_DELAY();

				/* A task that is removed from the event list while the
				scheduler is suspended will not get placed in the ready
				list or removed from the blocked list until the scheduler
				is resumed.

				This task cannot be in an event list as it is the currently
				executing task. */
				prvAddCurrentTaskToDelayedList( xTicksToDelay, pdFALSE );
			}
			xAlreadyYielded = xTaskResumeAll();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Force a reschedule if xTaskResumeAll has not already done so, we may
		have put ourselves to sleep. */
		if( xAlreadyYielded == pdFALSE )
		{
			portYIELD_WITHIN_API();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* INCLUDE_vTaskDelay */
/*-----------------------------------------------------------*/

#if( ( INCLUDE_eTaskGetState == 1 ) || ( configUSE_TRACE_FACILITY == 1 ) )

	eTaskState eTaskGetState( TaskHandle_t xTask )
	{
	eTaskState eReturn;
	List_t *pxStateList;
	const TCB_t * const pxTCB = ( TCB_t * ) xTask;

		configASSERT( pxTCB );

		if( pxTCB == pxCurrentTCB[portCPUID()] )
		{
			/* The task calling this function is querying its own state. */
			eReturn = eRunning;
		}
		else
		{
			taskENTER_CRITICAL();
			{
				pxStateList = ( List_t * ) listLIST_ITEM_CONTAINER( &( pxTCB->xStateListItem ) );
			}
			taskEXIT_CRITICAL();

			if( ( pxStateList == pxDelayedTaskList[portCPUID()] ) || ( pxStateList == pxOverflowDelayedTaskList[portCPUID()] ) )
			{
				/* The task being queried is referenced from one of the Blocked
				lists. */
				eReturn = eBlocked;
			}

			#if ( INCLUDE_vTaskSuspend == 1 )
				else if( pxStateList == &xSuspendedTaskList[portCPUID()] )
				{
					/* The task being queried is referenced from the suspended
					list.  Is it genuinely suspended or is it block
					indefinitely? */
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL )
					{
						eReturn = eSuspended;
					}
					else
					{
						eReturn = eBlocked;
					}
				}
			#endif

			#if ( INCLUDE_vTaskDelete == 1 )
				else if( ( pxStateList == &xTasksWaitingTermination ) || ( pxStateList == NULL ) )
				{
					/* The task being queried is referenced from the deleted
					tasks list, or it is not referenced from any lists at
					all. */
					eReturn = eDeleted;
				}
			#endif

			else /*lint !e525 Negative indentation is intended to make use of pre-processor clearer. */
			{
				/* If the task is not in any other state, it must be in the
				Ready (including pending ready) state. */
				eReturn = eReady;
			}
		}

		return eReturn;
	} /*lint !e818 xTask cannot be a pointer to const because it is a typedef. */

#endif /* INCLUDE_eTaskGetState */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

	UBaseType_t uxTaskPriorityGet( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	UBaseType_t uxReturn;

		taskENTER_CRITICAL();
		{
			/* If null is passed in here then it is the priority of the that
			called uxTaskPriorityGet() that is being queried. */
			pxTCB = prvGetTCBFromHandle( xTask );
			uxReturn = pxTCB->uxPriority;
		}
		taskEXIT_CRITICAL();

		return uxReturn;
	}

#endif /* INCLUDE_uxTaskPriorityGet */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskPriorityGet == 1 )

	UBaseType_t uxTaskPriorityGetFromISR( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	UBaseType_t uxReturn, uxSavedInterruptState;

		/* RTOS ports that support interrupt nesting have the concept of a
		maximum	system call (or maximum API call) interrupt priority.
		Interrupts that are	above the maximum system call priority are keep
		permanently enabled, even when the RTOS kernel is in a critical section,
		but cannot make any calls to FreeRTOS API functions.  If configASSERT()
		is defined in FreeRTOSConfig.h then
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
		failure if a FreeRTOS API function is called from an interrupt that has
		been assigned a priority above the configured maximum system call
		priority.  Only FreeRTOS functions that end in FromISR can be called
		from interrupts	that have been assigned a priority at or (logically)
		below the maximum system call interrupt priority.  FreeRTOS maintains a
		separate interrupt safe API to ensure interrupt entry is as fast and as
		simple as possible.  More information (albeit Cortex-M specific) is
		provided on the following link:
		http://www.freertos.org/RTOS-Cortex-M3-M4.html */
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		uxSavedInterruptState = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			/* If null is passed in here then it is the priority of the calling
			task that is being queried. */
			pxTCB = prvGetTCBFromHandle( xTask );
			uxReturn = pxTCB->uxPriority;
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptState );

		return uxReturn;
	}

#endif /* INCLUDE_uxTaskPriorityGet */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskPrioritySet == 1 )

	void vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority )
	{
	TCB_t *pxTCB;
	UBaseType_t uxCurrentBasePriority, uxPriorityUsedOnEntry;
	BaseType_t xYieldRequired = pdFALSE;

		configASSERT( ( uxNewPriority < configMAX_PRIORITIES ) );

		/* Ensure the new priority is valid. */
		if( uxNewPriority >= ( UBaseType_t ) configMAX_PRIORITIES )
		{
			uxNewPriority = ( UBaseType_t ) configMAX_PRIORITIES - ( UBaseType_t ) 1U;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		taskENTER_CRITICAL();
		{
			/* If null is passed in here then it is the priority of the calling
			task that is being changed. */
			pxTCB = prvGetTCBFromHandle( xTask );

			traceTASK_PRIORITY_SET( pxTCB, uxNewPriority );

			#if ( configUSE_MUTEXES == 1 )
			{
				uxCurrentBasePriority = pxTCB->uxBasePriority;
			}
			#else
			{
				uxCurrentBasePriority = pxTCB->uxPriority;
			}
			#endif

			if( uxCurrentBasePriority != uxNewPriority )
			{
				/* The priority change may have readied a task of higher
				priority than the calling task. */
				if( uxNewPriority > uxCurrentBasePriority )
				{
					if( pxTCB != pxCurrentTCB[portCPUID()] )
					{
						/* The priority of a task other than the currently
						running task is being raised.  Is the priority being
						raised above that of the running task? */
						if( uxNewPriority >= pxCurrentTCB[portCPUID()]->uxPriority )
						{
							xYieldRequired = pdTRUE;
						}
						else
						{
							mtCOVERAGE_TEST_MARKER();
						}
					}
					else
					{
						/* The priority of the running task is being raised,
						but the running task must already be the highest
						priority task able to run so no yield is required. */
					}
				}
				else if( pxTCB == pxCurrentTCB[portCPUID()] )
				{
					/* Setting the priority of the running task down means
					there may now be another task of higher priority that
					is ready to execute. */
					xYieldRequired = pdTRUE;
				}
				else
				{
					/* Setting the priority of any other task down does not
					require a yield as the running task must be above the
					new priority of the task being modified. */
				}

				/* Remember the ready list the task might be referenced from
				before its uxPriority member is changed so the
				taskRESET_READY_PRIORITY() macro can function correctly. */
				uxPriorityUsedOnEntry = pxTCB->uxPriority;

				#if ( configUSE_MUTEXES == 1 )
				{
					/* Only change the priority being used if the task is not
					currently using an inherited priority. */
					if( pxTCB->uxBasePriority == pxTCB->uxPriority )
					{
						pxTCB->uxPriority = uxNewPriority;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The base priority gets set whatever. */
					pxTCB->uxBasePriority = uxNewPriority;
				}
				#else
				{
					pxTCB->uxPriority = uxNewPriority;
				}
				#endif

				/* Only reset the event list item value if the value is not
				being used for anything else. */
				if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
				{
					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) uxNewPriority ) ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				/* If the task is in the blocked or suspended list we need do
				nothing more than change it's priority variable. However, if
				the task is in a ready list it needs to be removed and placed
				in the list appropriate to its new priority. */
				if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[portCPUID()][ uxPriorityUsedOnEntry ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
				{
					/* The task is currently in its ready list - remove before adding
					it to it's new ready list.  As we are in a critical section we
					can do this even if the scheduler is suspended. */
					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{
						/* It is known that the task is in its ready list so
						there is no need to check again and the port level
						reset macro can be called directly. */
						portRESET_READY_PRIORITY( uxPriorityUsedOnEntry, uxTopReadyPriority[portCPUID()] );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				if( xYieldRequired != pdFALSE )
				{
					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				/* Remove compiler warning about unused variables when the port
				optimised task selection is not being used. */
				( void ) uxPriorityUsedOnEntry;
			}
		}
		taskEXIT_CRITICAL();
	}

#endif /* INCLUDE_vTaskPrioritySet */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskSuspend( TaskHandle_t xTaskToSuspend )
	{
	TCB_t *pxTCB;

		taskENTER_CRITICAL();
		{
			/* If null is passed in here then it is the running task that is
			being suspended. */
			pxTCB = prvGetTCBFromHandle( xTaskToSuspend );

			traceTASK_SUSPEND( pxTCB );

			/* Remove task from the ready/delayed list and place in the
			suspended list. */
			if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
			{
				taskRESET_READY_PRIORITY( pxTCB->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			/* Is the task waiting on an event also? */
			if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
			{
				( void ) uxListRemove( &( pxTCB->xEventListItem ) );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			vListInsertEnd( &xSuspendedTaskList[portCPUID()], &( pxTCB->xStateListItem ) );
		}
		taskEXIT_CRITICAL();

		if( xSchedulerRunning[portCPUID()] != pdFALSE )
		{
			/* Reset the next expected unblock time in case it referred to the
			task that is now in the Suspended state. */
			taskENTER_CRITICAL();
			{
				prvResetNextTaskUnblockTime();
			}
			taskEXIT_CRITICAL();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		if( pxTCB == pxCurrentTCB[portCPUID()] )
		{
			if( xSchedulerRunning[portCPUID()] != pdFALSE )
			{
				/* The current task has just been suspended. */
				configASSERT( uxSchedulerSuspended[portCPUID()] == 0 );
				portYIELD_WITHIN_API();
			}
			else
			{
				/* The scheduler is not running, but the task that was pointed
				to by pxCurrentTCB has just been suspended and pxCurrentTCB
				must be adjusted to point to a different task. */
				if( listCURRENT_LIST_LENGTH( &xSuspendedTaskList[portCPUID()] ) == uxCurrentNumberOfTasks[portCPUID()] )
				{
					/* No other tasks are ready, so set pxCurrentTCB back to
					NULL so when the next task is created pxCurrentTCB will
					be set to point to it no matter what its relative priority
					is. */
					pxCurrentTCB[portCPUID()] = NULL;
				}
				else
				{
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
                    printk("Task cannot be suspended with runtime stack sharing enabled, need to fix this...\r\n");
                    configASSERT( pdFALSE );
#endif
					vTaskSwitchContext();
				}
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* INCLUDE_vTaskSuspend */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

	static BaseType_t prvTaskIsTaskSuspended( const TaskHandle_t xTask )
	{
	BaseType_t xReturn = pdFALSE;
	const TCB_t * const pxTCB = ( TCB_t * ) xTask;

		/* Accesses xPendingReadyList so must be called from a critical
		section. */

		/* It does not make sense to check if the calling task is suspended. */
		configASSERT( xTask );

		/* Is the task being resumed actually in the suspended list? */
		if( listIS_CONTAINED_WITHIN( &xSuspendedTaskList[portCPUID()], &( pxTCB->xStateListItem ) ) != pdFALSE )
		{
			/* Has the task already been resumed from within an ISR? */
			if( listIS_CONTAINED_WITHIN( &xPendingReadyList[portCPUID()], &( pxTCB->xEventListItem ) ) == pdFALSE )
			{
				/* Is it in the suspended list because it is in the	Suspended
				state, or because is is blocked with no timeout? */
				if( listIS_CONTAINED_WITHIN( NULL, &( pxTCB->xEventListItem ) ) != pdFALSE )
				{
					xReturn = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return xReturn;
	} /*lint !e818 xTask cannot be a pointer to const because it is a typedef. */

#endif /* INCLUDE_vTaskSuspend */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskSuspend == 1 )

	void vTaskResume( TaskHandle_t xTaskToResume )
	{
	TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;

		/* It does not make sense to resume the calling task. */
		configASSERT( xTaskToResume );

		/* The parameter cannot be NULL as it is impossible to resume the
		currently executing task. */
		if( ( pxTCB != NULL ) && ( pxTCB != pxCurrentTCB[portCPUID()] ) )
		{
			taskENTER_CRITICAL();
			{
				if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
				{
					traceTASK_RESUME( pxTCB );

					/* As we are in a critical section we can access the ready
					lists even if the scheduler is suspended. */
					( void ) uxListRemove(  &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );

					/* We may have just resumed a higher priority task. */
					if( pxTCB->uxPriority >= pxCurrentTCB[portCPUID()]->uxPriority )
					{
						/* This yield may not cause the task just resumed to run,
						but will leave the lists in the correct state for the
						next yield. */
						taskYIELD_IF_USING_PREEMPTION();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			taskEXIT_CRITICAL();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* INCLUDE_vTaskSuspend */

/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) )

	BaseType_t xTaskResumeFromISR( TaskHandle_t xTaskToResume )
	{
	BaseType_t xYieldRequired = pdFALSE;
	TCB_t * const pxTCB = ( TCB_t * ) xTaskToResume;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToResume );

		/* RTOS ports that support interrupt nesting have the concept of a
		maximum	system call (or maximum API call) interrupt priority.
		Interrupts that are	above the maximum system call priority are keep
		permanently enabled, even when the RTOS kernel is in a critical section,
		but cannot make any calls to FreeRTOS API functions.  If configASSERT()
		is defined in FreeRTOSConfig.h then
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
		failure if a FreeRTOS API function is called from an interrupt that has
		been assigned a priority above the configured maximum system call
		priority.  Only FreeRTOS functions that end in FromISR can be called
		from interrupts	that have been assigned a priority at or (logically)
		below the maximum system call interrupt priority.  FreeRTOS maintains a
		separate interrupt safe API to ensure interrupt entry is as fast and as
		simple as possible.  More information (albeit Cortex-M specific) is
		provided on the following link:
		http://www.freertos.org/RTOS-Cortex-M3-M4.html */
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			if( prvTaskIsTaskSuspended( pxTCB ) != pdFALSE )
			{
				traceTASK_RESUME_FROM_ISR( pxTCB );

				/* Check the ready lists can be accessed. */
				if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
				{
					/* Ready lists can be accessed so move the task from the
					suspended list to the ready list directly. */
					if( pxTCB->uxPriority >= pxCurrentTCB[portCPUID()]->uxPriority )
					{
						xYieldRequired = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					/* The delayed or ready lists cannot be accessed so the task
					is held in the pending ready list until the scheduler is
					unsuspended. */
					vListInsertEnd( &( xPendingReadyList[portCPUID()] ), &( pxTCB->xEventListItem ) );
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

		return xYieldRequired;
	}

#endif /* ( ( INCLUDE_xTaskResumeFromISR == 1 ) && ( INCLUDE_vTaskSuspend == 1 ) ) */
/*-----------------------------------------------------------*/

void vTaskStartScheduler( void )
{
BaseType_t xReturn;

	/* Add the idle task at the lowest priority. */
	#if( configSUPPORT_STATIC_ALLOCATION == 1 )
	{
		StaticTask_t *pxIdleTaskTCBBuffer = NULL;
		StackType_t *pxIdleTaskStackBuffer = NULL;
		uint32_t ulIdleTaskStackSize;

		/* The Idle task is created using user provided RAM - obtain the
		address of the RAM then create the idle task. */
		vApplicationGetIdleTaskMemory( &pxIdleTaskTCBBuffer, &pxIdleTaskStackBuffer, &ulIdleTaskStackSize );
		xIdleTaskHandle[portCPUID()] = xTaskCreateStatic(	prvIdleTask,
												"IDLE",
												ulIdleTaskStackSize,
												( void * ) NULL,
												( tskIDLE_PRIORITY | portPRIVILEGE_BIT ),
												pxIdleTaskStackBuffer,
												pxIdleTaskTCBBuffer ); /*lint !e961 MISRA exception, justified as it is not a redundant explicit cast to all supported compilers. */

		if( xIdleTaskHandle[portCPUID()] != NULL )
		{
			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
		}
	}
	#else
	{

#if( configUSE_CBS_CASH == 1 )
        printk("Allocating CASHQueue Items\r\n");
        xCASHQueue.xCASHItems = (CASHItem_t *) pvPortMalloc( sizeof( CASHItem_t ) * CASH_QUEUE_SIZE );
        if (xCASHQueue.xCASHItems == NULL) {
            printk("Not enough RAM to create CASH Queue! Panic...\r\n");
            configASSERT(pdFALSE);
        }
        xCASHQueue.sHead = 0;
        xCASHQueue.sSize = 0;
        xCASHQueue.sMaxSize = CASH_QUEUE_SIZE;
#endif
        
		/* The Idle task is being created using dynamically allocated RAM. */
		xReturn = xTaskCreate(	prvIdleTask,
								"IDLE", configMINIMAL_STACK_SIZE,
								( void * ) NULL,
								( tskIDLE_PRIORITY | portPRIVILEGE_BIT ),
                                0xFFFFFFFF,
                                0xFFFFFFFF,
                                0xFFFFFFFF,
								&xIdleTaskHandle[portCPUID()] ); /*lint !e961 MISRA exception, justified as it is not a redundant explicit cast to all supported compilers. */
#if( configUSE_GLOBAL_EDF == 1 )
        TCB_t *tsk;
        tsk = (TCB_t *) xIdleTaskHandle[portCPUID()];
        // Idle Task is always executing
        tsk->xTaskState = TASK_EXECUTING;
#endif /* configUSE_GLOBAL_EDF */
	}
	#endif /* configSUPPORT_STATIC_ALLOCATION */

	#if ( configUSE_TIMERS == 1 )
	{
		if( xReturn == pdPASS )
		{
			xReturn = xTimerCreateTimerTask();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif /* configUSE_TIMERS */

	if( xReturn == pdPASS )
	{
		/* Interrupts are turned off here, to ensure a tick does not occur
		before or during the call to xPortStartScheduler().  The stacks of
		the created tasks contain a status word with interrupts switched on
		so interrupts will automatically get re-enabled when the first task
		starts to run. */
		portDISABLE_INTERRUPTS();

		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{
			/* Switch Newlib's _impure_ptr variable to point to the _reent
			structure specific to the task that will run first. */
			_impure_ptr = &( pxCurrentTCB[portCPUID()]->xNewLib_reent );
		}
		#endif /* configUSE_NEWLIB_REENTRANT */

		xNextTaskUnblockTime[portCPUID()] = portMAX_DELAY;
		xSchedulerRunning[portCPUID()] = pdTRUE;
		xTickCount[portCPUID()] = ( TickType_t ) 0U;

		/* If configGENERATE_RUN_TIME_STATS is defined then the following
		macro must be defined to configure the timer/counter used to generate
		the run time counter time base. */
		portCONFIGURE_TIMER_FOR_RUN_TIME_STATS();

#if( configUSE_GLOBAL_EDF == 1 )
        printk("List length: %u\r\n", listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ PRIORITY_EDF ] ) ));
        // Kernel core should start with higest priority task.
        pxCurrentTCB[portCPUID()]->xTaskState = TASK_EXECUTING;        
        if ( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ PRIORITY_EDF ] ) ) > 1 ) {
            // More than one task available to schedule.            
            // Core 0, has highest priority task, so select ulNumCoresUsed-1 number
            // of highest priority tasks immediately below the highest priority task.
            ListItem_t const * endMarker = listGET_END_MARKER( &( pxReadyTasksLists[portCPUID()][ PRIORITY_EDF ] ) );
            ListItem_t *xItem = endMarker->pxNext; // xItem contains highest priority task.
            uint32_t i;
            for (i=1; i<ulNumCoresUsed; i++) {
                xItem = xItem->pxNext;
                if (xItem == endMarker) {
                    break;
                }
                TCB_t *xTCB = listGET_LIST_ITEM_OWNER( xItem );
                xTCB->xTaskState = TASK_EXECUTING;
                //printk("Setting pending task for core %u\r\n", i);
                vSetSecondaryCorePendingTask(i, xTCB);
                portSEND_CORE_INTERRUPT(i, portSECONDARY_CORE_INT_CHANNEL, MSG_SWITCH_TASK);
                DEC_NUM_IDLE_CORES();                
            }
        }
#endif  

		/* Setting up the timer tick is hardware specific and thus in the
		portable interface. */
		if( xPortStartScheduler() != pdFALSE )
		{
			/* Should not reach here as if the scheduler is running the
			function will not return. */
		}
		else
		{
			/* Should only reach here if a task calls xTaskEndScheduler(). */
		}
	}
	else
	{
		/* This line will only be reached if the kernel could not be started,
		because there was not enough FreeRTOS heap to create the idle task
		or the timer task. */
        printk("Out of memory\r\n");
		configASSERT( xReturn != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY );
	}

	/* Prevent compiler warnings if INCLUDE_xTaskGetIdleTaskHandle is set to 0,
	meaning xIdleTaskHandle is not used anywhere else. */
	( void ) xIdleTaskHandle[portCPUID()];
}
/*-----------------------------------------------------------*/

void vTaskEndScheduler( void )
{
	/* Stop the scheduler interrupts and call the portable scheduler end
	routine so the original ISRs can be restored if necessary.  The port
	layer must ensure interrupts enable	bit is left in the correct state. */
	portDISABLE_INTERRUPTS();
	xSchedulerRunning[portCPUID()] = pdFALSE;
	vPortEndScheduler();
}
/*----------------------------------------------------------*/

void vTaskSuspendAll( void )
{
	/* A critical section is not required as the variable is of type
	BaseType_t.  Please read Richard Barry's reply in the following link to a
	post in the FreeRTOS support forum before reporting this as a bug! -
	http://goo.gl/wu4acr */
	++uxSchedulerSuspended[portCPUID()];
}
/*----------------------------------------------------------*/

#if ( configUSE_TICKLESS_IDLE != 0 )

	static TickType_t prvGetExpectedIdleTime( void )
	{
	TickType_t xReturn;
	UBaseType_t uxHigherPriorityReadyTasks = pdFALSE;

		/* uxHigherPriorityReadyTasks takes care of the case where
		configUSE_PREEMPTION is 0, so there may be tasks above the idle priority
		task that are in the Ready state, even though the idle task is
		running. */
		#if( configUSE_PORT_OPTIMISED_TASK_SELECTION == 0 )
		{
			if( uxTopReadyPriority[portCPUID()] > tskIDLE_PRIORITY )
			{
				uxHigherPriorityReadyTasks = pdTRUE;
			}
		}
		#else
		{
			const UBaseType_t uxLeastSignificantBit = ( UBaseType_t ) 0x01;

			/* When port optimised task selection is used the uxTopReadyPriority
			variable is used as a bit map.  If bits other than the least
			significant bit are set then there are tasks that have a priority
			above the idle priority that are in the Ready state.  This takes
			care of the case where the co-operative scheduler is in use. */
			if( uxTopReadyPriority[portCPUID()] > uxLeastSignificantBit )
			{
				uxHigherPriorityReadyTasks = pdTRUE;
			}
		}
		#endif

		if( pxCurrentTCB[portCPUID()]->uxPriority > tskIDLE_PRIORITY )
		{
			xReturn = 0;
		}
		else if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ tskIDLE_PRIORITY ] ) ) > 1 )
		{
			/* There are other idle priority tasks in the ready state.  If
			time slicing is used then the very next tick interrupt must be
			processed. */
			xReturn = 0;
		}
		else if( uxHigherPriorityReadyTasks != pdFALSE )
		{
			/* There are tasks in the Ready state that have a priority above the
			idle priority.  This path can only be reached if
			configUSE_PREEMPTION is 0. */
			xReturn = 0;
		}
		else
		{
			xReturn = xNextTaskUnblockTime[portCPUID()] - xTickCount[portCPUID()];
		}

		return xReturn;
	}

#endif /* configUSE_TICKLESS_IDLE */
/*----------------------------------------------------------*/

BaseType_t xTaskResumeAll( void )
{
TCB_t *pxTCB = NULL;
BaseType_t xAlreadyYielded = pdFALSE;

	/* If uxSchedulerSuspended is zero then this function does not match a
	previous call to vTaskSuspendAll(). */
	configASSERT( uxSchedulerSuspended[portCPUID()] );

	/* It is possible that an ISR caused a task to be removed from an event
	list while the scheduler was suspended.  If this was the case then the
	removed task will have been added to the xPendingReadyList.  Once the
	scheduler has been resumed it is safe to move all the pending ready
	tasks from this list into their appropriate ready list. */
	taskENTER_CRITICAL();
	{
		--uxSchedulerSuspended[portCPUID()];

		if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
		{
			if( uxCurrentNumberOfTasks[portCPUID()] > ( UBaseType_t ) 0U )
			{
				/* Move any readied tasks from the pending list into the
				appropriate ready list. */
				while( listLIST_IS_EMPTY( &xPendingReadyList[portCPUID()] ) == pdFALSE )
				{
					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &xPendingReadyList[portCPUID()] ) );
					( void ) uxListRemove( &( pxTCB->xEventListItem ) );
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
                    
					/* If the moved task has a priority higher than the current
					task then a yield must be performed. */
					if( pxTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
					{
						xYieldPending[portCPUID()] = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( pxTCB != NULL )
				{
					/* A task was unblocked while the scheduler was suspended,
					which may have prevented the next unblock time from being
					re-calculated, in which case re-calculate it now.  Mainly
					important for low power tickless implementations, where
					this can prevent an unnecessary exit from low power
					state. */
					prvResetNextTaskUnblockTime();
				}

				/* If any ticks occurred while the scheduler was suspended then
				they should be processed now.  This ensures the tick count does
				not	slip, and that any delayed tasks are resumed at the correct
				time. */
				{
					UBaseType_t uxPendedCounts = uxPendedTicks[portCPUID()]; /* Non-volatile copy. */

					if( uxPendedCounts > ( UBaseType_t ) 0U )
					{
						do
						{
							if( xTaskIncrementTick() != pdFALSE )
							{
								xYieldPending[portCPUID()] = pdTRUE;
							}
							else
							{
								mtCOVERAGE_TEST_MARKER();
							}
							--uxPendedCounts;
						} while( uxPendedCounts > ( UBaseType_t ) 0U );

						uxPendedTicks[portCPUID()] = 0;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( xYieldPending[portCPUID()] != pdFALSE )
				{
					#if( configUSE_PREEMPTION != 0 )
					{
						xAlreadyYielded = pdTRUE;
					}
					#endif
					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	taskEXIT_CRITICAL();

	return xAlreadyYielded;
}
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCount( void )
{
TickType_t xTicks;

#if( configUSE_GLOBAL_EDF == 1 )
    xTicks = xTickCount[portKERNEL_CORE];
    return xTicks;
#endif /* configUSE_GLOBAL_EDF */
	/* Critical section required if running on a 16 bit processor. */
	portTICK_TYPE_ENTER_CRITICAL();
	{
		xTicks = xTickCount[portCPUID()];        
	}
	portTICK_TYPE_EXIT_CRITICAL();

	return xTicks;
}
/*-----------------------------------------------------------*/

TickType_t xTaskGetTickCountFromISR( void )
{
TickType_t xReturn;
UBaseType_t uxSavedInterruptStatus;

	/* RTOS ports that support interrupt nesting have the concept of a maximum
	system call (or maximum API call) interrupt priority.  Interrupts that are
	above the maximum system call priority are kept permanently enabled, even
	when the RTOS kernel is in a critical section, but cannot make any calls to
	FreeRTOS API functions.  If configASSERT() is defined in FreeRTOSConfig.h
	then portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
	failure if a FreeRTOS API function is called from an interrupt that has been
	assigned a priority above the configured maximum system call priority.
	Only FreeRTOS functions that end in FromISR can be called from interrupts
	that have been assigned a priority at or (logically) below the maximum
	system call	interrupt priority.  FreeRTOS maintains a separate interrupt
	safe API to ensure interrupt entry is as fast and as simple as possible.
	More information (albeit Cortex-M specific) is provided on the following
	link: http://www.freertos.org/RTOS-Cortex-M3-M4.html */
	portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

	uxSavedInterruptStatus = portTICK_TYPE_SET_INTERRUPT_MASK_FROM_ISR();
	{
		xReturn = xTickCount[portCPUID()];
	}
	portTICK_TYPE_CLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

	return xReturn;
}
/*-----------------------------------------------------------*/

UBaseType_t uxTaskGetNumberOfTasks( void )
{
	/* A critical section is not required because the variables are of type
	BaseType_t. */
	return uxCurrentNumberOfTasks[portCPUID()];
}
/*-----------------------------------------------------------*/

char *pcTaskGetName( TaskHandle_t xTaskToQuery ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
{
TCB_t *pxTCB;

	/* If null is passed in here then the name of the calling task is being
	queried. */
	pxTCB = prvGetTCBFromHandle( xTaskToQuery );
	configASSERT( pxTCB );
	return &( pxTCB->pcTaskName[ 0 ] );
}
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )

	static TCB_t *prvSearchForNameWithinSingleList( List_t *pxList, const char pcNameToQuery[] )
	{
	TCB_t *pxNextTCB, *pxFirstTCB, *pxReturn = NULL;
	UBaseType_t x;
	char cNextChar;

		/* This function is called with the scheduler suspended. */

		if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );

			do
			{
				listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );

				/* Check each character in the name looking for a match or
				mismatch. */
				for( x = ( UBaseType_t ) 0; x < ( UBaseType_t ) configMAX_TASK_NAME_LEN; x++ )
				{
					cNextChar = pxNextTCB->pcTaskName[ x ];

					if( cNextChar != pcNameToQuery[ x ] )
					{
						/* Characters didn't match. */
						break;
					}
					else if( cNextChar == 0x00 )
					{
						/* Both strings terminated, a match must have been
						found. */
						pxReturn = pxNextTCB;
						break;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}

				if( pxReturn != NULL )
				{
					/* The handle has been found. */
					break;
				}

			} while( pxNextTCB != pxFirstTCB );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return pxReturn;
	}

#endif /* INCLUDE_xTaskGetHandle */
/*-----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetHandle == 1 )

	TaskHandle_t xTaskGetHandle( const char *pcNameToQuery ) /*lint !e971 Unqualified char types are allowed for strings and single characters only. */
	{
	UBaseType_t uxQueue = configMAX_PRIORITIES;
	TCB_t* pxTCB;

		/* Task names will be truncated to configMAX_TASK_NAME_LEN - 1 bytes. */
		configASSERT( strlen( pcNameToQuery ) < configMAX_TASK_NAME_LEN );

		vTaskSuspendAll();
		{
			/* Search the ready lists. */
			do
			{
				uxQueue--;
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) &( pxReadyTasksLists[portCPUID()][ uxQueue ] ), pcNameToQuery );

				if( pxTCB != NULL )
				{
					/* Found the handle. */
					break;
				}

			} while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

			/* Search the delayed lists. */
			if( pxTCB == NULL )
			{
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxDelayedTaskList[portCPUID()], pcNameToQuery );
			}

			if( pxTCB == NULL )
			{
				pxTCB = prvSearchForNameWithinSingleList( ( List_t * ) pxOverflowDelayedTaskList[portCPUID()], pcNameToQuery );
			}

			#if ( INCLUDE_vTaskSuspend == 1 )
			{
				if( pxTCB == NULL )
				{
					/* Search the suspended list. */
					pxTCB = prvSearchForNameWithinSingleList( &xSuspendedTaskList[portCPUID()], pcNameToQuery );
				}
			}
			#endif

			#if( INCLUDE_vTaskDelete == 1 )
			{
				if( pxTCB == NULL )
				{
					/* Search the deleted list. */
					pxTCB = prvSearchForNameWithinSingleList( &xTasksWaitingTermination, pcNameToQuery );
				}
			}
			#endif
		}
		( void ) xTaskResumeAll();

		return ( TaskHandle_t ) pxTCB;
	}

#endif /* INCLUDE_xTaskGetHandle */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

	UBaseType_t uxTaskGetSystemState( TaskStatus_t * const pxTaskStatusArray, const UBaseType_t uxArraySize, uint32_t * const pulTotalRunTime )
	{
	UBaseType_t uxTask = 0, uxQueue = configMAX_PRIORITIES;

		vTaskSuspendAll();
		{
			/* Is there a space in the array for each task in the system? */
			if( uxArraySize >= uxCurrentNumberOfTasks[portCPUID()] )
			{
				/* Fill in an TaskStatus_t structure with information on each
				task in the Ready state. */
				do
				{
					uxQueue--;
					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &( pxReadyTasksLists[portCPUID()][ uxQueue ] ), eReady );

				} while( uxQueue > ( UBaseType_t ) tskIDLE_PRIORITY ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

				/* Fill in an TaskStatus_t structure with information on each
				task in the Blocked state. */
				uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxDelayedTaskList[portCPUID()], eBlocked );
				uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), ( List_t * ) pxOverflowDelayedTaskList[portCPUID()], eBlocked );

				#if( INCLUDE_vTaskDelete == 1 )
				{
					/* Fill in an TaskStatus_t structure with information on
					each task that has been deleted but not yet cleaned up. */
					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xTasksWaitingTermination, eDeleted );
				}
				#endif

				#if ( INCLUDE_vTaskSuspend == 1 )
				{
					/* Fill in an TaskStatus_t structure with information on
					each task in the Suspended state. */
					uxTask += prvListTasksWithinSingleList( &( pxTaskStatusArray[ uxTask ] ), &xSuspendedTaskList[portCPUID()], eSuspended );
				}
				#endif

				#if ( configGENERATE_RUN_TIME_STATS == 1)
				{
					if( pulTotalRunTime != NULL )
					{
						#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
							portALT_GET_RUN_TIME_COUNTER_VALUE( ( *pulTotalRunTime ) );
						#else
							*pulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
						#endif
					}
				}
				#else
				{
					if( pulTotalRunTime != NULL )
					{
						*pulTotalRunTime = 0;
					}
				}
				#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		( void ) xTaskResumeAll();

		return uxTask;
	}

#endif /* configUSE_TRACE_FACILITY */
/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskGetIdleTaskHandle == 1 )

	TaskHandle_t xTaskGetIdleTaskHandle( void )
	{
		/* If xTaskGetIdleTaskHandle() is called before the scheduler has been
		started, then xIdleTaskHandle will be NULL. */
		configASSERT( ( xIdleTaskHandle[portCPUID()] != NULL ) );
		return xIdleTaskHandle[portCPUID()];
	}

#endif /* INCLUDE_xTaskGetIdleTaskHandle */
/*----------------------------------------------------------*/

/* This conditional compilation should use inequality to 0, not equality to 1.
This is to ensure vTaskStepTick() is available when user defined low power mode
implementations require configUSE_TICKLESS_IDLE to be set to a value other than
1. */
#if ( configUSE_TICKLESS_IDLE != 0 )

	void vTaskStepTick( const TickType_t xTicksToJump )
	{
		/* Correct the tick count value after a period during which the tick
		was suppressed.  Note this does *not* call the tick hook function for
		each stepped tick. */
		configASSERT( ( xTickCount[portCPUID()] + xTicksToJump ) <= xNextTaskUnblockTime[portCPUID()] );
		xTickCount[portCPUID()] += xTicksToJump;
		traceINCREASE_TICK_COUNT( xTicksToJump );
	}

#endif /* configUSE_TICKLESS_IDLE */
/*----------------------------------------------------------*/

#if ( INCLUDE_xTaskAbortDelay == 1 )

	BaseType_t xTaskAbortDelay( TaskHandle_t xTask )
	{
	TCB_t *pxTCB = ( TCB_t * ) xTask;
	BaseType_t xReturn = pdFALSE;

		configASSERT( pxTCB );

		vTaskSuspendAll();
		{
			/* A task can only be prematurely removed from the Blocked state if
			it is actually in the Blocked state. */
			if( eTaskGetState( xTask ) == eBlocked )
			{
				/* Remove the reference to the task from the blocked list.  An
				interrupt won't touch the xStateListItem because the
				scheduler is suspended. */
				( void ) uxListRemove( &( pxTCB->xStateListItem ) );

				/* Is the task waiting on an event also?  If so remove it from
				the event list too.  Interrupts can touch the event list item,
				even though the scheduler is suspended, so a critical section
				is used. */
				taskENTER_CRITICAL();
				{
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						( void ) uxListRemove( &( pxTCB->xEventListItem ) );
						pxTCB->ucDelayAborted = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				taskEXIT_CRITICAL();

				/* Place the unblocked task into the appropriate ready list. */
				prvAddTaskToReadyList( pxTCB );

				/* A task being unblocked cannot cause an immediate context
				switch if preemption is turned off. */
				#if (  configUSE_PREEMPTION == 1 )
				{
					/* Preemption is on, but a context switch should only be
					performed if the unblocked task has a priority that is
					equal to or higher than the currently executing task. */
					if( pxTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
					{
						/* Pend the yield to be performed when the scheduler
						is unsuspended. */
						xYieldPending[portCPUID()] = pdTRUE;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				#endif /* configUSE_PREEMPTION */
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		xTaskResumeAll();

		return xReturn;
	}

#endif /* INCLUDE_xTaskAbortDelay */
/*----------------------------------------------------------*/

#if ( configUSE_SCHEDULER_EDF == 1 )

void RestartMissedTask(TCB_t* pxMissedTCB)
{
    taskENTER_CRITICAL_FROM_ISR();

    // Calculate by how long we have to wait to reschedule
    TickType_t xTimeToSleep = (pxMissedTCB->xPeriod - pxMissedTCB->xRelativeDeadline) + pxMissedTCB->xPeriod;
    // Update the woken time so that the vEndTaskPeriod calls function correctly
    pxMissedTCB->xLastWakeTime = xTimeToSleep + xTickCount[portCPUID()];

    // Delay the missed task. Note that the delay function only operates on the current TCB, thus we
    // have to perform a temporary swap.
    TCB_t* pxOldCurrentTCB = pxCurrentTCB[portCPUID()];
    pxCurrentTCB[portCPUID()] = pxMissedTCB;
    prvAddCurrentTaskToDelayedList( xTimeToSleep, pdFALSE );
    pxCurrentTCB[portCPUID()] = pxOldCurrentTCB;

    // The missed task should be restarted, thus we signal to the context switcher to not preserve
    // its stack when swaping it out/in
    pxMissedTCB->xNoPreserve = pdTRUE;

    // Loop through every resource and release the resource
    #if ( configUSE_SRP == 1)
        vSRPTCBSemaphoreGive(pxMissedTCB);
    #endif
    taskEXIT_CRITICAL_FROM_ISR(0);
}
#endif /* configUSE_SCHEDULER_EDF */

BaseType_t xTaskIncrementTick( void )
{
        TCB_t * pxTCB;
        TickType_t xItemValue;
        BaseType_t xSwitchRequired = pdFALSE;
        TickType_t xSmallestTick = 0xFFFFFFFF;
        #if( configUSE_SRP == 1 )
            uint32_t *vSysCeilPtr;
        #endif /* configUSE_SRP */
            
        #if( configUSE_SCHEDULER_EDF == 1 )
        {
            List_t* readyList = &pxReadyTasksLists[portCPUID()][PRIORITY_EDF];
            ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
            ListItem_t const* endMarker = listGET_END_MARKER(readyList);
#if( configUSE_GLOBAL_EDF == 1 )
            uint32_t numIdleCores = xGetNumIdleCores();
            uint32_t tasksWaiting = 0;
#endif /* configUSE_GLOBAL_EDF */
            while(currentItem != endMarker)
            {
                TCB_t* currentTCB = listGET_LIST_ITEM_OWNER(currentItem);
                if (currentTCB->xStateListItem.xItemValue > 0)
                {
                    currentTCB->xStateListItem.xItemValue--;
                    xSmallestTick = MIN(xSmallestTick, currentTCB->xStateListItem.xItemValue);
#if( configUSE_GLOBAL_EDF == 1 )
                    if (currentTCB->xTaskState == TASK_WAITING) {
                        tasksWaiting++;
                        if (numIdleCores > 0) {                            
                            uint32_t cpuid;
                            for (cpuid=1; cpuid<ulNumCoresUsed; cpuid++) {
                                TCB_t *xCurrentTask = xGetSecondaryCoreCurrentTask(cpuid);
                                if (xCurrentTask == NULL) {
                                    TCB_t *xPendingTask = xGetSecondaryCorePendingTask(cpuid);
                                    if (xPendingTask == NULL) {
                                        break;
                                    }
                                }
                            }
                            if (cpuid >= ulNumCoresUsed) {
                                /* Although cores are seemingly idle, they could be switching
                                   right now to their idle task, so we best wait till that is
                                   done. */
                                //continue;
                                numIdleCores = 0;
                            }
                            else {
                                currentTCB->xTaskState = TASK_EXECUTING;
                                tasksWaiting--;                                
                                vSetSecondaryCorePendingTask(cpuid, currentTCB);
                                portSEND_CORE_INTERRUPT(cpuid, portSECONDARY_CORE_INT_CHANNEL, MSG_SWITCH_TASK);
                                DEC_NUM_IDLE_CORES();
                                numIdleCores = xGetNumIdleCores();
                            }
                        }
                    }
#endif /* configUSE_GLOBAL_EDF */
                }
                else
                {
#if( configUSE_GLOBAL_EDF == 1 )
                    // TODO: Add functionality to handle overruns.
                    printk("Overruned TaskName: %s\r\n", currentTCB->pcTaskName);
                    printk("No overrun allowed currently for global EDF, so panic...\r\n");
                    while(1);
#endif /* configUSE_GLOBAL_EDF */
                    printk("Missed Deadline, will reset processor!\r\n");
                    RestartMissedTask(currentTCB);
                    return pdTRUE;
                }
                currentItem = listGET_NEXT(currentItem);
            }
#if( configUSE_GLOBAL_EDF == 1 )
            if ( tasksWaiting > 0 ) {
                //printk("tasksWaiting: %u\r\n", tasksWaiting);
                if ( pxCurrentTCB[portCPUID()] == NULL ||
                     pxCurrentTCB[portCPUID()] == xIdleTaskHandle[portCPUID()] ) {
                    //printk("Making Switch!\r\n");
                    xSwitchRequired = pdTRUE;
                }
            }   
#endif /* configUSE_GLOBAL_EDF */
#if( configUSE_SRP == 1 )
            currentItem = listGET_HEAD_ENTRY(readyList);

            while(currentItem != endMarker) {
                TCB_t* currentTCB = listGET_LIST_ITEM_OWNER(currentItem);
                // If highest priority task is blocked then we need to unblock it
                if (currentTCB->xBlocked == pdTRUE) {
                    if(currentTCB->xStateListItem.xItemValue == xSmallestTick) {
                        vSysCeilPtr = (StackType_t *) srpSysCeilStackPeak();

                        if ( vSysCeilPtr == NULL ||
                             currentTCB->xPreemptionLevel > *vSysCeilPtr ) {
                            currentTCB->xBlocked = pdFALSE;
                            return pdTRUE;        
                        }
                    }
                    else {
                        break;
                    }
                }
                currentItem = listGET_NEXT(currentItem);                
            }
#endif /* configUSE_SRP */
        }
        #endif /* configUSE_SCHEDULER_EDF  */

#if( configUSE_CBS_CASH == 1 )
        size_t sCursor, sTail;
        CASHItem_t *xCASHItems = xCASHQueue.xCASHItems;

        // Try to find the correct position starting from head. This is because mod
        // in C is not the mathematical mod, it is the remainder and will return negative
        // results so we want to avoid decrementing.
        sCursor = xCASHQueue.sHead;
        sTail = ( xCASHQueue.sHead + xCASHQueue.sSize ) % xCASHQueue.sMaxSize;
        int deadline;
        while (sCursor != sTail) {
            deadline = xCASHItems[sCursor].xDeadline - 1;
            if (deadline <= 0) {
                xCASHQueue.sHead = (xCASHQueue.sHead + 1) % xCASHQueue.sMaxSize;
                xCASHQueue.sSize--;
                printk("Tick: Removed item\r\n");
            }
            sCursor = (sCursor + 1) % xCASHQueue.sMaxSize;
        }
#endif /* configUSE_CBS_CASH */

	/* Called by the portable layer each time a tick interrupt occurs.
	Increments the tick then checks to see if the new tick value will cause any
	tasks to be unblocked. */
	traceTASK_INCREMENT_TICK( xTickCount[portCPUID()] );
	if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
	{
		/* Minor optimisation.  The tick count cannot change in this
		block. */
		const TickType_t xConstTickCount = xTickCount[portCPUID()] + 1;

                #if( configUSE_SCHEDULER_EDF == 1 )
#if( configUSE_CBS_CASH == 1 )
        if ( pxCurrentTCB[portCPUID()]->xIsServer == pdTRUE ) {
            CASHItem_t *xHeadItem = vServerCBSPeakCASHQueue();
            if ( xHeadItem != NULL &&
                 xHeadItem->xDeadline <= pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue) {
                //printk("Some leftover!\r\n");
                //printk("Name: %s\r\n", pxCurrentTCB[portCPUID()]->pcTaskName);                
                vServerCBSDecrementCASHQueue();
            }
            else {
                pxCurrentTCB[portCPUID()]->xCurrentRunTime++;
            }
        }
        else {
            pxCurrentTCB[portCPUID()]->xCurrentRunTime++;
        }
#else /* configUSE_CBS_CASH */
        pxCurrentTCB[portCPUID()]->xCurrentRunTime++;        
#endif /* configUSE_CBS_CASH */
                    #if( configUSE_CBS_SERVER == 1 )
                    if ( ( pxCurrentTCB[portCPUID()]->xIsServer == pdTRUE ) &&
                         ( pxCurrentTCB[portCPUID()]->xCurrentRunTime == pxCurrentTCB[portCPUID()]->xWCET ) )
                    {
                        printk("[%s] Used up budget at %u\r\n", pxCurrentTCB[portCPUID()]->pcTaskName, xTaskGetTickCount());
                        vServerCBSReplenish( pxCurrentTCB[portCPUID()] );
                        vServerCBSRefresh( pxCurrentTCB[portCPUID()] );
                        
                        TCB_t *pxNewCurrentTCB;
                        ListItem_t const *endMarker = listGET_END_MARKER( &( pxReadyTasksLists[portCPUID()][PRIORITY_EDF] ) );
                        pxNewCurrentTCB = listGET_LIST_ITEM_OWNER( endMarker->pxNext );
                        //printk("New Current TCB: %s\r\n", pxNewCurrentTCB->pcTaskName);

                        //ListItem_t *currentItem = endMarker->pxNext;
                        /* while(currentItem != endMarker) { */
                        /*     TCB_t *tsk = listGET_LIST_ITEM_OWNER( currentItem ); */
                        /*     printk("tsk->pcTaskName: %s\r\n", tsk->pcTaskName); */
                        /*     printk("tsk->xStateListItem.xItemValue: %u\r\n", tsk->xStateListItem.xItemValue); */
                        /*     currentItem = currentItem->pxNext; */
                        /* } */
                        
                        if (pxNewCurrentTCB != pxCurrentTCB[portCPUID()]) {
                            // Server task is not the highest priority task, so we need to switch
                            printk("pxNewCurrentTCB->pcTaskName: %s\r\n", pxNewCurrentTCB->pcTaskName);
                            xSwitchRequired = pdTRUE;
                        }
                    }
                    #endif /* configUSE_CBS_SERVER */
                #endif /* configUSE_SCHEDULER_EDF  */

		/* Increment the RTOS tick, switching the delayed and overflowed
		delayed lists if it wraps to 0. */
		xTickCount[portCPUID()] = xConstTickCount;

		if( xConstTickCount == ( TickType_t ) 0U )
		{
			taskSWITCH_DELAYED_LISTS();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* See if this tick has made a timeout expire.  Tasks are stored in
		the	queue in the order of their wake time - meaning once one task
		has been found whose block time has not expired there is no need to
		look any further down the list. */
		if( xConstTickCount >= xNextTaskUnblockTime[portCPUID()] )
		{
			for( ;; )
			{
				if( listLIST_IS_EMPTY( pxDelayedTaskList[portCPUID()] ) != pdFALSE )
				{
					/* The delayed list is empty.  Set xNextTaskUnblockTime[portCPUID()]
					to the maximum possible value so it is extremely
					unlikely that the
					if( xTickCount[portCPUID()] >= xNextTaskUnblockTime ) test will pass
					next time through. */
					xNextTaskUnblockTime[portCPUID()] = portMAX_DELAY; /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
					break;
				}
				else
				{                    
					/* The delayed list is not empty, get the value of the
					item at the head of the delayed list.  This is the time
					at which the task at the head of the delayed list must
					be removed from the Blocked state. */
					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList[portCPUID()] );
					xItemValue = listGET_LIST_ITEM_VALUE( &( pxTCB->xStateListItem ) );

					if( xConstTickCount < xItemValue )
					{
						/* It is not time to unblock this item yet, but the
						item value is the time at which the task at the head
						of the blocked list must be removed from the Blocked
						state -	so record the item value in
						xNextTaskUnblockTime. */
						xNextTaskUnblockTime[portCPUID()] = xItemValue;
						break;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* It is time to remove the item from the Blocked state. */
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );

					/* Is the task waiting on an event also?  If so remove
					it from the event list. */
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						( void ) uxListRemove( &( pxTCB->xEventListItem ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* Place the unblocked task into the appropriate ready
					list. */
                    #if( configUSE_SCHEDULER_EDF == 1 )
                    pxTCB->xStateListItem.xItemValue = pxTCB->xRelativeDeadline;
                    pxTCB->xCurrentRunTime = 0;
                    #endif
					prvAddTaskToReadyList( pxTCB );

					/* A task being unblocked cannot cause an immediate
					context switch if preemption is turned off. */
					#if (  configUSE_PREEMPTION == 1 )
					{
						/* Preemption is on, but a context switch should
						only be performed if the unblocked task has a
						priority that is equal to or higher than the
						currently executing task. */
                        #if( configUSE_SCHEDULER_EDF == 1 )
                            if(pxTCB->xRelativeDeadline < xSmallestTick)
                        #else
						if( pxTCB->uxPriority >= pxCurrentTCB[portCPUID()]->uxPriority )
                        #endif
						{
#if( configUSE_GLOBAL_EDF == 1 )
                            //printk("pxTCB->pcTaskName: %s\r\n", pxTCB->pcTaskName);
                            TCB_t *xPendingTask;
                            TCB_t *xCurrentTask = pxCurrentTCB[portCPUID()];
                            TCB_t *xTask = NULL;
                            uint32_t cpuid;
                            uint32_t lowCPUid = 0; // CPU id running the lowest priority task
                            // Find lowest priority task out of all currently running tasks
                            for (cpuid=1; cpuid<ulNumCoresUsed; cpuid++) {
                                xCurrentTask = xGetSecondaryCoreCurrentTask( cpuid );
                                xPendingTask = xGetSecondaryCorePendingTask( cpuid );
                                if (xPendingTask == NULL) {
                                    if (xCurrentTask == NULL) {
                                        lowCPUid = cpuid;
                                        break;
                                    }
                                    if (xTask == NULL ||
                                        xTask->xStateListItem.xItemValue < xCurrentTask->xStateListItem.xItemValue) {
                                        xTask = xCurrentTask;
                                        lowCPUid = cpuid;
                                    }
                                }
                            }
                            if (lowCPUid != 0) {
                                //printk("Task for other core!\r\n");
                                pxTCB->xTaskState = TASK_EXECUTING;
                                vSetSecondaryCorePendingTask(lowCPUid, pxTCB);
                                portSEND_CORE_INTERRUPT(lowCPUid, portSECONDARY_CORE_INT_CHANNEL, MSG_SWITCH_TASK);
                            }
                            else {
                                //printk("Other Area!\r\n");
                                // All tasks running on secondary cores are higher priority so
                                // kernel core should be the one running the new task
                                xSwitchRequired = pdTRUE;
                            }
#else /* configUSE_GLOBAL_EDF */
                        #if( configUSE_SRP == 1 )
                            {
                                vSysCeilPtr = (StackType_t *) srpSysCeilStackPeak();
                                if ( vSysCeilPtr == NULL ||
                                    pxTCB->xPreemptionLevel > *vSysCeilPtr ) {
                                    xSwitchRequired = pdTRUE;
                                }
                                else {
                                    pxTCB->xBlocked = pdTRUE;
                                }
                            }    
                        #else /* configUSE_SRP */
#if( configUSE_CBS_SERVER == 1 )
                            if (pxCurrentTCB[portCPUID()]->xIsServer) {
                                printk("Smallest tick: %u\r\n", xSmallestTick);
                                printk("Preempted by %s, Deadline: %u\r\n", pxTCB->pcTaskName, pxTCB->xRelativeDeadline);
                                printk("Server Deadline: %u\r\n", pxCurrentTCB[portCPUID()]->xRelativeDeadline);
                                printk("Server xItemValue: %u\r\n", pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue);                                
                            }
                          #endif /* configUSE_CBS_SERVER */
                            xSwitchRequired = pdTRUE;
                        #endif /* configUSE_SRP */
						}
						else
						{
#if( configUSE_CBS_SERVER == 1 )
                            if (pxCurrentTCB[portCPUID()]->xIsServer) {
                                printk("Smallest tick: %u\r\n", xSmallestTick);
                                printk("NOT Preempted by %s, Deadline: %u\r\n", pxTCB->pcTaskName, pxTCB->xRelativeDeadline);
                                printk("Server Deadline: %u\r\n", pxCurrentTCB[portCPUID()]->xRelativeDeadline);
                                printk("Server xItemValue: %u\r\n", pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue);
                            }
                        #endif /* configUSE_CBS_SERVER */
#endif /* configUSE_GLOBAL_EDF */                            
							mtCOVERAGE_TEST_MARKER();
						}
					}
					#endif /* configUSE_PREEMPTION */
				}
			}
		}

		/* Tasks of equal priority to the currently running task will share
		processing time (time slice) if preemption is on, and the application
		writer has not explicitly turned time slicing off. */
		#if ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) )
		{
			if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ pxCurrentTCB[portCPUID()]->uxPriority ] ) ) > ( UBaseType_t ) 1 )
			{
                #if( configUSE_SRP == 1 )
                {
                    vSysCeilPtr = (StackType_t *) srpSysCeilStackPeak();

                    if ( vSysCeilPtr == NULL ||
                         pxTCB->xPreemptionLevel > *vSysCeilPtr ) {
                        xSwichRequired = pdTRUE;
                    }
                    else {
                        pxTCB->xBlocked = pdTRUE;
                    }
                }
                #else
                    xSwitchRequired = pdTRUE;
                #endif /* configUSE_SRP */
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configUSE_TIME_SLICING == 1 ) ) */

		#if ( configUSE_TICK_HOOK == 1 )
		{
			/* Guard against the tick hook being called when the pended tick
			count is being unwound (when the scheduler is being unlocked). */
			if( uxPendedTicks[portCPUID()] == ( UBaseType_t ) 0U )
			{
				vApplicationTickHook();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif /* configUSE_TICK_HOOK */
	}
	else
	{
		++uxPendedTicks[portCPUID()];

		/* The tick hook gets called at regular intervals, even if the
		scheduler is locked. */
		#if ( configUSE_TICK_HOOK == 1 )
		{
			vApplicationTickHook();
		}
		#endif
	}

	#if ( configUSE_PREEMPTION == 1 )
	{
		if( xYieldPending[portCPUID()] != pdFALSE )
		{
			xSwitchRequired = pdTRUE;
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif /* configUSE_PREEMPTION */

#if( configUSE_CBS_CASH == 1 )
    // Processor is idle, so decrement capacity in CASH queue
    if (pxCurrentTCB[portCPUID()] == NULL ||
        pxCurrentTCB[portCPUID()] == xIdleTaskHandle[portCPUID()]) {
        vServerCBSDecrementCASHQueue();
    }
#endif
	return xSwitchRequired;
}
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	void vTaskSetApplicationTaskTag( TaskHandle_t xTask, TaskHookFunction_t pxHookFunction )
	{
	TCB_t *xTCB;

		/* If xTask is NULL then it is the task hook of the calling task that is
		getting set. */
		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB[portCPUID()];
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}

		/* Save the hook function in the TCB.  A critical section is required as
		the value can be accessed from an interrupt. */
		taskENTER_CRITICAL();
			xTCB->pxTaskTag = pxHookFunction;
		taskEXIT_CRITICAL();
	}

#endif /* configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	TaskHookFunction_t xTaskGetApplicationTaskTag( TaskHandle_t xTask )
	{
	TCB_t *xTCB;
	TaskHookFunction_t xReturn;

		/* If xTask is NULL then we are setting our own task hook. */
		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB[portCPUID()];
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}

		/* Save the hook function in the TCB.  A critical section is required as
		the value can be accessed from an interrupt. */
		taskENTER_CRITICAL();
		{
			xReturn = xTCB->pxTaskTag;
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif /* configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if ( configUSE_APPLICATION_TASK_TAG == 1 )

	BaseType_t xTaskCallApplicationTaskHook( TaskHandle_t xTask, void *pvParameter )
	{
	TCB_t *xTCB;
	BaseType_t xReturn;

		/* If xTask is NULL then we are calling our own task hook. */
		if( xTask == NULL )
		{
			xTCB = ( TCB_t * ) pxCurrentTCB[portCPUID()];
		}
		else
		{
			xTCB = ( TCB_t * ) xTask;
		}

		if( xTCB->pxTaskTag != NULL )
		{
			xReturn = xTCB->pxTaskTag( pvParameter );
		}
		else
		{
			xReturn = pdFAIL;
		}

		return xReturn;
	}

#endif /* configUSE_APPLICATION_TASK_TAG */
/*-----------------------------------------------------------*/

#if( configUSE_SCHEDULER_EDF == 1 )
/**
 * @brief Restarts the execution of a given task.
 **/
void vRestartTask(TCB_t* tcb)
{
    printk("Restarting Task!\r\n");
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    srpInitTaskRuntimeStack(tcb);
#else /* configUSE_SHARED_RUNTIME_STACK */
    StackType_t *pxTopOfStack;
    pxTopOfStack = tcb->pxStack + ( tcb->usStackDepth - ( uint32_t ) 1 );
    pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );
    configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
    tcb->pxTopOfStack = pxPortInitialiseStack( pxTopOfStack, tcb->pxTaskCode, tcb->pvParameters );
#endif /* configUSE_SHARED_RUNTIME_STACK */
}
#endif  /* configUSE_SCHEDULER_EDF  */

void vTaskSwitchContext( void )
{
	if( uxSchedulerSuspended[portCPUID()] != ( UBaseType_t ) pdFALSE )
	{
		/* The scheduler is currently suspended - do not allow a context
		switch. */
		xYieldPending[portCPUID()] = pdTRUE;
	}
	else
	{
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
        if ( pxCurrentTCB[portCPUID()]->pxTaskCode != prvIdleTask && pxCurrentTCB[portCPUID()]->pxStack != NULL) {
            // Previous task is not completed yet, so use it's stack top
            mRuntimeStack.xStackTop = (StackType_t *) pxCurrentTCB[portCPUID()]->pxTopOfStack;            
        }
        // Otherwise previous task is completed and mRuntimeStack.xStackTop would already be
        // the correct value, as set in srpRuntimeStackPopTask()
#endif
		xYieldPending[portCPUID()] = pdFALSE;
		traceTASK_SWITCHED_OUT();

		#if ( configGENERATE_RUN_TIME_STATS == 1 )
		{
				#ifdef portALT_GET_RUN_TIME_COUNTER_VALUE
					portALT_GET_RUN_TIME_COUNTER_VALUE( ulTotalRunTime );
				#else
					ulTotalRunTime = portGET_RUN_TIME_COUNTER_VALUE();
				#endif

				/* Add the amount of time the task has been running to the
				accumulated time so far.  The time the task started running was
				stored in ulTaskSwitchedInTime.  Note that there is no overflow
				protection here so count values are only valid until the timer
				overflows.  The guard against negative values is to protect
				against suspect run time stat counter implementations - which
				are provided by the application, not the kernel. */
				if( ulTotalRunTime > ulTaskSwitchedInTime )
				{
					pxCurrentTCB[portCPUID()]->ulRunTimeCounter += ( ulTotalRunTime - ulTaskSwitchedInTime );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
				ulTaskSwitchedInTime = ulTotalRunTime;
		}
		#endif /* configGENERATE_RUN_TIME_STATS */

                #if( configUSE_SCHEDULER_EDF == 1 )
                    // Re Initialize the task
                    if( pxCurrentTCB[portCPUID()]->xNoPreserve == pdTRUE)
                    {
                        vRestartTask(pxCurrentTCB[portCPUID()]);
                        pxCurrentTCB[portCPUID()]->xNoPreserve = pdFALSE;
                    }
                #endif  /* configUSE_SCHEDULER_EDF  */
		/* Check for stack overflow, if configured. */
		taskCHECK_FOR_STACK_OVERFLOW();

		/* Select a new task to run using either the generic C or port
		optimised asm code. */
#if( configUSE_GLOBAL_EDF == 1 )
        UBaseType_t uxTopPriority = uxTopReadyPriority[portCPUID()];
        //printk("uxTopPriority: %u\r\n", uxTopPriority);
		/* Find the highest priority queue that contains ready tasks. */				  
		while( listLIST_IS_EMPTY( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) ) )
		{
			configASSERT( uxTopPriority );
			--uxTopPriority;
		}
        ListItem_t const *endMarker = listGET_END_MARKER( &( pxReadyTasksLists[portCPUID()][ uxTopPriority ] ) );
		uxTopReadyPriority[portCPUID()] = uxTopPriority;

        TCB_t *xCurrentTask = NULL;
        if (uxTopReadyPriority[portCPUID()] == PRIORITY_EDF) {
            ListItem_t *xItem = endMarker->pxNext;
            TCB_t *xTask;
            // Find highest priority task not taken by other core.
            while (xItem != endMarker) {
                xTask = listGET_LIST_ITEM_OWNER( xItem );
                if (xTask->xTaskState == TASK_WAITING) {
                    xCurrentTask = xTask;
                    //printk("xCurrentTask->pcTaskName: %s\r\n", xCurrentTask->pcTaskName);
                    xCurrentTask->xTaskState = TASK_EXECUTING;
                    break;
                }
                xItem = xItem->pxNext;
            }
        }
        if (xCurrentTask == NULL) {
            xCurrentTask = (TCB_t *) xIdleTaskHandle[portCPUID()];
        }
        if (pxCurrentTCB[portCPUID()] != NULL &&
            pxCurrentTCB[portCPUID()] != xIdleTaskHandle[portCPUID()]) {
            pxCurrentTCB[portCPUID()]->xTaskState = TASK_WAITING;
        }
        pxCurrentTCB[portCPUID()] = xCurrentTask;
#else /* configUSE_GLOBAL_EDF */
		taskSELECT_HIGHEST_PRIORITY_TASK();
#endif /* configUSE_GLOBAL_EDF */
		traceTASK_SWITCHED_IN();

		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{
			/* Switch Newlib's _impure_ptr variable to point to the _reent
			structure specific to this task. */
			_impure_ptr = &( pxCurrentTCB[portCPUID()]->xNewLib_reent );
		}
		#endif /* configUSE_NEWLIB_REENTRANT */

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
        srpInitTaskRuntimeStack( pxCurrentTCB[portCPUID()] );
#endif /* configUSE_SHARED_RUNTIME_STACK */
	}
}
/*-----------------------------------------------------------*/

void vTaskPlaceOnEventList( List_t * const pxEventList, const TickType_t xTicksToWait )
{
	configASSERT( pxEventList );

	/* THIS FUNCTION MUST BE CALLED WITH EITHER INTERRUPTS DISABLED OR THE
	SCHEDULER SUSPENDED AND THE QUEUE BEING ACCESSED LOCKED. */

	/* Place the event list item of the TCB in the appropriate event list.
	This is placed in the list in priority order so the highest priority task
	is the first to be woken by the event.  The queue that contains the event
	list is locked, preventing simultaneous access from interrupts. */
	vListInsert( pxEventList, &( pxCurrentTCB[portCPUID()]->xEventListItem ) );

	prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}
/*-----------------------------------------------------------*/

void vTaskPlaceOnUnorderedEventList( List_t * pxEventList, const TickType_t xItemValue, const TickType_t xTicksToWait )
{
	configASSERT( pxEventList );

	/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.  It is used by
	the event groups implementation. */
	configASSERT( uxSchedulerSuspended[portCPUID()] != 0 );

	/* Store the item value in the event list item.  It is safe to access the
	event list item here as interrupts won't access the event list item of a
	task that is not in the Blocked state. */
	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB[portCPUID()]->xEventListItem ), xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );

	/* Place the event list item of the TCB at the end of the appropriate event
	list.  It is safe to access the event list here because it is part of an
	event group implementation - and interrupts don't access event groups
	directly (instead they access them indirectly by pending function calls to
	the task level). */
	vListInsertEnd( pxEventList, &( pxCurrentTCB[portCPUID()]->xEventListItem ) );

	prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
}
/*-----------------------------------------------------------*/

#if( configUSE_TIMERS == 1 )

	void vTaskPlaceOnEventListRestricted( List_t * const pxEventList, TickType_t xTicksToWait, const BaseType_t xWaitIndefinitely )
	{
		configASSERT( pxEventList );

		/* This function should not be called by application code hence the
		'Restricted' in its name.  It is not part of the public API.  It is
		designed for use by kernel code, and has special calling requirements -
		it should be called with the scheduler suspended. */


		/* Place the event list item of the TCB in the appropriate event list.
		In this case it is assume that this is the only task that is going to
		be waiting on this event list, so the faster vListInsertEnd() function
		can be used in place of vListInsert. */
		vListInsertEnd( pxEventList, &( pxCurrentTCB[portCPUID()]->xEventListItem ) );

		/* If the task should block indefinitely then set the block time to a
		value that will be recognised as an indefinite delay inside the
		prvAddCurrentTaskToDelayedList() function. */
		if( xWaitIndefinitely != pdFALSE )
		{
			xTicksToWait = portMAX_DELAY;
		}

		traceTASK_DELAY_UNTIL( ( xTickCount[portCPUID()] + xTicksToWait ) );
		prvAddCurrentTaskToDelayedList( xTicksToWait, xWaitIndefinitely );
	}

#endif /* configUSE_TIMERS */
/*-----------------------------------------------------------*/

BaseType_t xTaskRemoveFromEventList( const List_t * const pxEventList )
{
TCB_t *pxUnblockedTCB;
BaseType_t xReturn;

	/* THIS FUNCTION MUST BE CALLED FROM A CRITICAL SECTION.  It can also be
	called from a critical section within an ISR. */

	/* The event list is sorted in priority order, so the first in the list can
	be removed as it is known to be the highest priority.  Remove the TCB from
	the delayed list, and add it to the ready list.

	If an event is for a queue that is locked then this function will never
	get called - the lock count on the queue will get modified instead.  This
	means exclusive access to the event list is guaranteed here.

	This function assumes that a check has already been made to ensure that
	pxEventList is not empty. */
	pxUnblockedTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxEventList );
	configASSERT( pxUnblockedTCB );
	( void ) uxListRemove( &( pxUnblockedTCB->xEventListItem ) );

	if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
	{
		( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
		prvAddTaskToReadyList( pxUnblockedTCB );
	}
	else
	{
		/* The delayed and ready lists cannot be accessed, so hold this task
		pending until the scheduler is resumed. */
		vListInsertEnd( &( xPendingReadyList[portCPUID()] ), &( pxUnblockedTCB->xEventListItem ) );
	}

	if( pxUnblockedTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
	{
		/* Return true if the task removed from the event list has a higher
		priority than the calling task.  This allows the calling task to know if
		it should force a context switch now. */
		xReturn = pdTRUE;

		/* Mark that a yield is pending in case the user is not using the
		"xHigherPriorityTaskWoken" parameter to an ISR safe FreeRTOS function. */
		xYieldPending[portCPUID()] = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	#if( configUSE_TICKLESS_IDLE != 0 )
	{
		/* If a task is blocked on a kernel object then xNextTaskUnblockTime
		might be set to the blocked task's time out time.  If the task is
		unblocked for a reason other than a timeout xNextTaskUnblockTime is
		normally left unchanged, because it is automatically reset to a new
		value when the tick count equals xNextTaskUnblockTime.  However if
		tickless idling is used it might be more important to enter sleep mode
		at the earliest possible time - so reset xNextTaskUnblockTime here to
		ensure it is updated at the earliest possible time. */
		prvResetNextTaskUnblockTime();
	}
	#endif

	return xReturn;
}
/*-----------------------------------------------------------*/

BaseType_t xTaskRemoveFromUnorderedEventList( ListItem_t * pxEventListItem, const TickType_t xItemValue )
{
TCB_t *pxUnblockedTCB;
BaseType_t xReturn;

	/* THIS FUNCTION MUST BE CALLED WITH THE SCHEDULER SUSPENDED.  It is used by
	the event flags implementation. */
	configASSERT( uxSchedulerSuspended[portCPUID()] != pdFALSE );

	/* Store the new item value in the event list. */
	listSET_LIST_ITEM_VALUE( pxEventListItem, xItemValue | taskEVENT_LIST_ITEM_VALUE_IN_USE );

	/* Remove the event list form the event flag.  Interrupts do not access
	event flags. */
	pxUnblockedTCB = ( TCB_t * ) listGET_LIST_ITEM_OWNER( pxEventListItem );
	configASSERT( pxUnblockedTCB );
	( void ) uxListRemove( pxEventListItem );

	/* Remove the task from the delayed list and add it to the ready list.  The
	scheduler is suspended so interrupts will not be accessing the ready
	lists. */
	( void ) uxListRemove( &( pxUnblockedTCB->xStateListItem ) );
	prvAddTaskToReadyList( pxUnblockedTCB );

	if( pxUnblockedTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
	{
		/* Return true if the task removed from the event list has
		a higher priority than the calling task.  This allows
		the calling task to know if it should force a context
		switch now. */
		xReturn = pdTRUE;

		/* Mark that a yield is pending in case the user is not using the
		"xHigherPriorityTaskWoken" parameter to an ISR safe FreeRTOS function. */
		xYieldPending[portCPUID()] = pdTRUE;
	}
	else
	{
		xReturn = pdFALSE;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

void vTaskSetTimeOutState( TimeOut_t * const pxTimeOut )
{
	configASSERT( pxTimeOut );
	pxTimeOut->xOverflowCount = xNumOfOverflows[portCPUID()];
	pxTimeOut->xTimeOnEntering = xTickCount[portCPUID()];
}
/*-----------------------------------------------------------*/

BaseType_t xTaskCheckForTimeOut( TimeOut_t * const pxTimeOut, TickType_t * const pxTicksToWait )
{
BaseType_t xReturn;

	configASSERT( pxTimeOut );
	configASSERT( pxTicksToWait );

	taskENTER_CRITICAL();
	{
		/* Minor optimisation.  The tick count cannot change in this block. */
		const TickType_t xConstTickCount = xTickCount[portCPUID()];

		#if( INCLUDE_xTaskAbortDelay == 1 )
			if( pxCurrentTCB[portCPUID()]->ucDelayAborted != pdFALSE )
			{
				/* The delay was aborted, which is not the same as a time out,
				but has the same result. */
				pxCurrentTCB[portCPUID()]->ucDelayAborted = pdFALSE;
				xReturn = pdTRUE;
			}
			else
		#endif

		#if ( INCLUDE_vTaskSuspend == 1 )
			if( *pxTicksToWait == portMAX_DELAY )
			{
				/* If INCLUDE_vTaskSuspend is set to 1 and the block time
				specified is the maximum block time then the task should block
				indefinitely, and therefore never time out. */
				xReturn = pdFALSE;
			}
			else
		#endif

		if( ( xNumOfOverflows[portCPUID()] != pxTimeOut->xOverflowCount ) && ( xConstTickCount >= pxTimeOut->xTimeOnEntering ) ) /*lint !e525 Indentation preferred as is to make code within pre-processor directives clearer. */
		{
			/* The tick count is greater than the time at which
			vTaskSetTimeout() was called, but has also overflowed since
			vTaskSetTimeOut() was called.  It must have wrapped all the way
			around and gone past again. This passed since vTaskSetTimeout()
			was called. */
			xReturn = pdTRUE;
		}
		else if( ( ( TickType_t ) ( xConstTickCount - pxTimeOut->xTimeOnEntering ) ) < *pxTicksToWait ) /*lint !e961 Explicit casting is only redundant with some compilers, whereas others require it to prevent integer conversion errors. */
		{
			/* Not a genuine timeout. Adjust parameters for time remaining. */
			*pxTicksToWait -= ( xConstTickCount - pxTimeOut->xTimeOnEntering );
			vTaskSetTimeOutState( pxTimeOut );
			xReturn = pdFALSE;
		}
		else
		{
			xReturn = pdTRUE;
		}
	}
	taskEXIT_CRITICAL();

	return xReturn;
}
/*-----------------------------------------------------------*/

void vTaskMissedYield( void )
{
	xYieldPending[portCPUID()] = pdTRUE;
}
/*-----------------------------------------------------------*/


	UBaseType_t uxTaskGetTaskNumber( TaskHandle_t xTask )
	{
	UBaseType_t uxReturn;
	TCB_t *pxTCB;

		if( xTask != NULL )
		{
			pxTCB = ( TCB_t * ) xTask;
			uxReturn = pxTCB->uxTaskNumber;
		}
		else
		{
			uxReturn = 0U;
		}

		return uxReturn;
	}

/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

	void vTaskSetTaskNumber( TaskHandle_t xTask, const UBaseType_t uxHandle )
	{
	TCB_t *pxTCB;

		if( xTask != NULL )
		{
			pxTCB = ( TCB_t * ) xTask;
			pxTCB->uxTaskNumber = uxHandle;
		}
	}

#endif /* configUSE_TRACE_FACILITY */

/*
 * -----------------------------------------------------------
 * The Idle task.
 * ----------------------------------------------------------
 *
 * The portTASK_FUNCTION() macro is used to allow port/compiler specific
 * language extensions.  The equivalent prototype for this function is:
 *
 * void prvIdleTask( void *pvParameters );
 *
 */
static portTASK_FUNCTION( prvIdleTask, pvParameters )
{
	/* Stop warnings. */
	( void ) pvParameters;

    while(1) {
    }
	/** THIS IS THE RTOS IDLE TASK - WHICH IS CREATED AUTOMATICALLY WHEN THE
	SCHEDULER IS STARTED. **/

	for( ;; )
	{
		/* See if any tasks have deleted themselves - if so then the idle task
		is responsible for freeing the deleted task's TCB and stack. */
		prvCheckTasksWaitingTermination();

		#if ( configUSE_PREEMPTION == 0 )
		{
			/* If we are not using preemption we keep forcing a task switch to
			see if any other task has become available.  If we are using
			preemption we don't need to do this as any task becoming available
			will automatically get the processor anyway. */
			taskYIELD();
		}
		#endif /* configUSE_PREEMPTION */

		#if ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) )
		{
			/* When using preemption tasks of equal priority will be
			timesliced.  If a task that is sharing the idle priority is ready
			to run then the idle task should yield before the end of the
			timeslice.

			A critical region is not required here as we are just reading from
			the list, and an occasional incorrect value will not matter.  If
			the ready list at the idle priority contains more than one task
			then a task other than the idle task is ready to execute. */
			if( listCURRENT_LIST_LENGTH( &( pxReadyTasksLists[portCPUID()][ tskIDLE_PRIORITY ] ) ) > ( UBaseType_t ) 1 )
			{
#if( configUSE_GLOBAL_EDF == 0 )
                // Only do this if not using multicore global EDF, since global EDF has
                // Idle task for each core so unless we do a check to see if there are tasks
                // that can be time sliced on a single core there is no point yielding
				taskYIELD();
#endif /* configUSE_GLOBAL_EDF */
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif /* ( ( configUSE_PREEMPTION == 1 ) && ( configIDLE_SHOULD_YIELD == 1 ) ) */

		#if ( configUSE_IDLE_HOOK == 1 )
		{
			extern void vApplicationIdleHook( void );

			/* Call the user defined function from within the idle task.  This
			allows the application designer to add background functionality
			without the overhead of a separate task.
			NOTE: vApplicationIdleHook() MUST NOT, UNDER ANY CIRCUMSTANCES,
			CALL A FUNCTION THAT MIGHT BLOCK. */
			vApplicationIdleHook();
		}
		#endif /* configUSE_IDLE_HOOK */

		/* This conditional compilation should use inequality to 0, not equality
		to 1.  This is to ensure portSUPPRESS_TICKS_AND_SLEEP() is called when
		user defined low power mode	implementations require
		configUSE_TICKLESS_IDLE to be set to a value other than 1. */
		#if ( configUSE_TICKLESS_IDLE != 0 )
		{
		TickType_t xExpectedIdleTime;

			/* It is not desirable to suspend then resume the scheduler on
			each iteration of the idle task.  Therefore, a preliminary
			test of the expected idle time is performed without the
			scheduler suspended.  The result here is not necessarily
			valid. */
			xExpectedIdleTime = prvGetExpectedIdleTime();

			if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
			{
				vTaskSuspendAll();
				{
					/* Now the scheduler is suspended, the expected idle
					time can be sampled again, and this time its value can
					be used. */
					configASSERT( xNextTaskUnblockTime[portCPUID()] >= xTickCount[portCPUID()] );
					xExpectedIdleTime = prvGetExpectedIdleTime();

					if( xExpectedIdleTime >= configEXPECTED_IDLE_TIME_BEFORE_SLEEP )
					{
						traceLOW_POWER_IDLE_BEGIN();
						portSUPPRESS_TICKS_AND_SLEEP( xExpectedIdleTime );
						traceLOW_POWER_IDLE_END();
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}
				}
				( void ) xTaskResumeAll();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif /* configUSE_TICKLESS_IDLE */
	}
}
/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

	void vTaskSetThreadLocalStoragePointer( TaskHandle_t xTaskToSet, BaseType_t xIndex, void *pvValue )
	{
	TCB_t *pxTCB;

		if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
		{
			pxTCB = prvGetTCBFromHandle( xTaskToSet );
			pxTCB->pvThreadLocalStoragePointers[ xIndex ] = pvValue;
		}
	}

#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS */
/*-----------------------------------------------------------*/

#if ( configNUM_THREAD_LOCAL_STORAGE_POINTERS != 0 )

	void *pvTaskGetThreadLocalStoragePointer( TaskHandle_t xTaskToQuery, BaseType_t xIndex )
	{
	void *pvReturn = NULL;
	TCB_t *pxTCB;

		if( xIndex < configNUM_THREAD_LOCAL_STORAGE_POINTERS )
		{
			pxTCB = prvGetTCBFromHandle( xTaskToQuery );
			pvReturn = pxTCB->pvThreadLocalStoragePointers[ xIndex ];
		}
		else
		{
			pvReturn = NULL;
		}

		return pvReturn;
	}

#endif /* configNUM_THREAD_LOCAL_STORAGE_POINTERS */
/*-----------------------------------------------------------*/

#if ( portUSING_MPU_WRAPPERS == 1 )

	void vTaskAllocateMPURegions( TaskHandle_t xTaskToModify, const MemoryRegion_t * const xRegions )
	{
	TCB_t *pxTCB;

		/* If null is passed in here then we are modifying the MPU settings of
		the calling task. */
		pxTCB = prvGetTCBFromHandle( xTaskToModify );

		vPortStoreTaskMPUSettings( &( pxTCB->xMPUSettings ), xRegions, NULL, 0 );
	}

#endif /* portUSING_MPU_WRAPPERS */
/*-----------------------------------------------------------*/

static void prvInitialiseTaskLists( void )
{
UBaseType_t uxPriority;

	for( uxPriority = ( UBaseType_t ) 0U; uxPriority < ( UBaseType_t ) configMAX_PRIORITIES; uxPriority++ )
	{
		vListInitialise( &( pxReadyTasksLists[portCPUID()][ uxPriority ] ) );
	}

	vListInitialise( &xDelayedTaskList1[portCPUID()] );
	vListInitialise( &xDelayedTaskList2[portCPUID()] );
	vListInitialise( &xPendingReadyList[portCPUID()] );

	#if ( INCLUDE_vTaskDelete == 1 )
	{
		vListInitialise( &xTasksWaitingTermination );
	}
	#endif /* INCLUDE_vTaskDelete */

	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		vListInitialise( &xSuspendedTaskList[portCPUID()] );
	}
	#endif /* INCLUDE_vTaskSuspend */

	/* Start with pxDelayedTaskList[portCPUID()] using list1 and the pxOverflowDelayedTaskList
	using list2. */
	pxDelayedTaskList[portCPUID()] = &xDelayedTaskList1[portCPUID()];
	pxOverflowDelayedTaskList[portCPUID()] = &xDelayedTaskList2[portCPUID()];
}
/*-----------------------------------------------------------*/

static void prvCheckTasksWaitingTermination( void )
{

	/** THIS FUNCTION IS CALLED FROM THE RTOS IDLE TASK **/

	#if ( INCLUDE_vTaskDelete == 1 )
	{
		BaseType_t xListIsEmpty;

		/* ucTasksDeleted is used to prevent vTaskSuspendAll() being called
		too often in the idle task. */
		while( uxDeletedTasksWaitingCleanUp > ( UBaseType_t ) 0U )
		{
			vTaskSuspendAll();
			{
				xListIsEmpty = listLIST_IS_EMPTY( &xTasksWaitingTermination );
			}
			( void ) xTaskResumeAll();

			if( xListIsEmpty == pdFALSE )
			{
				TCB_t *pxTCB;

				taskENTER_CRITICAL();
				{
					pxTCB = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( ( &xTasksWaitingTermination ) );
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					--uxCurrentNumberOfTasks[portCPUID()];
					--uxDeletedTasksWaitingCleanUp;
				}
				taskEXIT_CRITICAL();

				prvDeleteTCB( pxTCB );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
	}
	#endif /* INCLUDE_vTaskDelete */
}
/*-----------------------------------------------------------*/

#if( configUSE_TRACE_FACILITY == 1 )

	void vTaskGetInfo( TaskHandle_t xTask, TaskStatus_t *pxTaskStatus, BaseType_t xGetFreeStackSpace, eTaskState eState )
	{
	TCB_t *pxTCB;

		/* xTask is NULL then get the state of the calling task. */
		pxTCB = prvGetTCBFromHandle( xTask );

		pxTaskStatus->xHandle = ( TaskHandle_t ) pxTCB;
		pxTaskStatus->pcTaskName = ( const char * ) &( pxTCB->pcTaskName [ 0 ] );
		pxTaskStatus->uxCurrentPriority = pxTCB->uxPriority;
		pxTaskStatus->pxStackBase = pxTCB->pxStack;
		pxTaskStatus->xTaskNumber = pxTCB->uxTCBNumber;

		#if ( INCLUDE_vTaskSuspend == 1 )
		{
			/* If the task is in the suspended list then there is a chance it is
			actually just blocked indefinitely - so really it should be reported as
			being in the Blocked state. */
			if( pxTaskStatus->eCurrentState == eSuspended )
			{
				vTaskSuspendAll();
				{
					if( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) != NULL )
					{
						pxTaskStatus->eCurrentState = eBlocked;
					}
				}
				xTaskResumeAll();
			}
		}
		#endif /* INCLUDE_vTaskSuspend */

		#if ( configUSE_MUTEXES == 1 )
		{
			pxTaskStatus->uxBasePriority = pxTCB->uxBasePriority;
		}
		#else
		{
			pxTaskStatus->uxBasePriority = 0;
		}
		#endif

		#if ( configGENERATE_RUN_TIME_STATS == 1 )
		{
			pxTaskStatus->ulRunTimeCounter = pxTCB->ulRunTimeCounter;
		}
		#else
		{
			pxTaskStatus->ulRunTimeCounter = 0;
		}
		#endif

		/* Obtaining the task state is a little fiddly, so is only done if the value
		of eState passed into this function is eInvalid - otherwise the state is
		just set to whatever is passed in. */
		if( eState != eInvalid )
		{
			pxTaskStatus->eCurrentState = eState;
		}
		else
		{
			pxTaskStatus->eCurrentState = eTaskGetState( xTask );
		}

		/* Obtaining the stack space takes some time, so the xGetFreeStackSpace
		parameter is provided to allow it to be skipped. */
		if( xGetFreeStackSpace != pdFALSE )
		{
			#if ( portSTACK_GROWTH > 0 )
			{
				pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxEndOfStack );
			}
			#else
			{
				pxTaskStatus->usStackHighWaterMark = prvTaskCheckFreeStackSpace( ( uint8_t * ) pxTCB->pxStack );
			}
			#endif
		}
		else
		{
			pxTaskStatus->usStackHighWaterMark = 0;
		}
	}

#endif /* configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

#if ( configUSE_TRACE_FACILITY == 1 )

	static UBaseType_t prvListTasksWithinSingleList( TaskStatus_t *pxTaskStatusArray, List_t *pxList, eTaskState eState )
	{
	volatile TCB_t *pxNextTCB, *pxFirstTCB;
	UBaseType_t uxTask = 0;

		if( listCURRENT_LIST_LENGTH( pxList ) > ( UBaseType_t ) 0 )
		{
			listGET_OWNER_OF_NEXT_ENTRY( pxFirstTCB, pxList );

			/* Populate an TaskStatus_t structure within the
			pxTaskStatusArray array for each task that is referenced from
			pxList.  See the definition of TaskStatus_t in task.h for the
			meaning of each TaskStatus_t structure member. */
			do
			{
				listGET_OWNER_OF_NEXT_ENTRY( pxNextTCB, pxList );
				vTaskGetInfo( ( TaskHandle_t ) pxNextTCB, &( pxTaskStatusArray[ uxTask ] ), pdTRUE, eState );
				uxTask++;
			} while( pxNextTCB != pxFirstTCB );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return uxTask;
	}

#endif /* configUSE_TRACE_FACILITY */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) )

	static uint16_t prvTaskCheckFreeStackSpace( const uint8_t * pucStackByte )
	{
	uint32_t ulCount = 0U;

		while( *pucStackByte == ( uint8_t ) tskSTACK_FILL_BYTE )
		{
			pucStackByte -= portSTACK_GROWTH;
			ulCount++;
		}

		ulCount /= ( uint32_t ) sizeof( StackType_t ); /*lint !e961 Casting is not redundant on smaller architectures. */

		return ( uint16_t ) ulCount;
	}

#endif /* ( ( configUSE_TRACE_FACILITY == 1 ) || ( INCLUDE_uxTaskGetStackHighWaterMark == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( INCLUDE_uxTaskGetStackHighWaterMark == 1 )

	UBaseType_t uxTaskGetStackHighWaterMark( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	uint8_t *pucEndOfStack;
	UBaseType_t uxReturn;

		pxTCB = prvGetTCBFromHandle( xTask );

		#if portSTACK_GROWTH < 0
		{
			pucEndOfStack = ( uint8_t * ) pxTCB->pxStack;
		}
		#else
		{
			pucEndOfStack = ( uint8_t * ) pxTCB->pxEndOfStack;
		}
		#endif

		uxReturn = ( UBaseType_t ) prvTaskCheckFreeStackSpace( pucEndOfStack );

		return uxReturn;
	}

#endif /* INCLUDE_uxTaskGetStackHighWaterMark */
/*-----------------------------------------------------------*/

#if ( INCLUDE_vTaskDelete == 1 )

	static void prvDeleteTCB( TCB_t *pxTCB )
	{
		/* This call is required specifically for the TriCore port.  It must be
		above the vPortFree() calls.  The call is also used by ports/demos that
		want to allocate and clean RAM statically. */
		portCLEAN_UP_TCB( pxTCB );

		/* Free up the memory allocated by the scheduler for the task.  It is up
		to the task to free any memory allocated at the application level. */
		#if ( configUSE_NEWLIB_REENTRANT == 1 )
		{
			_reclaim_reent( &( pxTCB->xNewLib_reent ) );
		}
		#endif /* configUSE_NEWLIB_REENTRANT */

		#if( ( configSUPPORT_DYNAMIC_ALLOCATION == 1 ) && ( configSUPPORT_STATIC_ALLOCATION == 0 ) && ( portUSING_MPU_WRAPPERS == 0 ) )
		{
			/* The task can only have been allocated dynamically - free both
			the stack and TCB. */
			vPortFree( pxTCB->pxStack );
			vPortFree( pxTCB );
		}
		#elif( tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE == 1 )
		{
			/* The task could have been allocated statically or dynamically, so
			check what was statically allocated before trying to free the
			memory. */
			if( pxTCB->ucStaticallyAllocated == tskDYNAMICALLY_ALLOCATED_STACK_AND_TCB )
			{
				/* Both the stack and TCB were allocated dynamically, so both
				must be freed. */
				vPortFree( pxTCB->pxStack );
				vPortFree( pxTCB );
			}
			else if( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_ONLY )
			{
				/* Only the stack was statically allocated, so the TCB is the
				only memory that must be freed. */
				vPortFree( pxTCB );
			}
			else
			{
				/* Neither the stack nor the TCB were allocated dynamically, so
				nothing needs to be freed. */
				configASSERT( pxTCB->ucStaticallyAllocated == tskSTATICALLY_ALLOCATED_STACK_AND_TCB	)
				mtCOVERAGE_TEST_MARKER();
			}
		}
		#endif /* configSUPPORT_DYNAMIC_ALLOCATION */
	}

#endif /* INCLUDE_vTaskDelete */
/*-----------------------------------------------------------*/

static void prvResetNextTaskUnblockTime( void )
{
TCB_t *pxTCB;

	if( listLIST_IS_EMPTY( pxDelayedTaskList[portCPUID()] ) != pdFALSE )
	{
		/* The new current delayed list is empty.  Set xNextTaskUnblockTime to
		the maximum possible value so it is	extremely unlikely that the
		if( xTickCount[portCPUID()] >= xNextTaskUnblockTime ) test will pass until
		there is an item in the delayed list. */
		xNextTaskUnblockTime[portCPUID()] = portMAX_DELAY;
	}
	else
	{
		/* The new current delayed list is not empty, get the value of
		the item at the head of the delayed list.  This is the time at
		which the task at the head of the delayed list should be removed
		from the Blocked state. */
		( pxTCB ) = ( TCB_t * ) listGET_OWNER_OF_HEAD_ENTRY( pxDelayedTaskList[portCPUID()] );
		xNextTaskUnblockTime[portCPUID()] = listGET_LIST_ITEM_VALUE( &( ( pxTCB )->xStateListItem ) );
	}
}
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) )

	TaskHandle_t xTaskGetCurrentTaskHandle( void )
	{
	TaskHandle_t xReturn;

		/* A critical section is not required as this is not called from
		an interrupt and the current TCB will always be the same for any
		individual execution thread. */
		xReturn = pxCurrentTCB[portCPUID()];

		return xReturn;
	}

#endif /* ( ( INCLUDE_xTaskGetCurrentTaskHandle == 1 ) || ( configUSE_MUTEXES == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) )

	BaseType_t xTaskGetSchedulerState( void )
	{
	BaseType_t xReturn;

		if( xSchedulerRunning[portCPUID()] == pdFALSE )
		{
			xReturn = taskSCHEDULER_NOT_STARTED;
		}
		else
		{
			if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
			{
				xReturn = taskSCHEDULER_RUNNING;
			}
			else
			{
				xReturn = taskSCHEDULER_SUSPENDED;
			}
		}

		return xReturn;
	}

#endif /* ( ( INCLUDE_xTaskGetSchedulerState == 1 ) || ( configUSE_TIMERS == 1 ) ) */
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

	void vTaskPriorityInherit( TaskHandle_t const pxMutexHolder )
	{
	TCB_t * const pxTCB = ( TCB_t * ) pxMutexHolder;

		/* If the mutex was given back by an interrupt while the queue was
		locked then the mutex holder might now be NULL. */
		if( pxMutexHolder != NULL )
		{
			/* If the holder of the mutex has a priority below the priority of
			the task attempting to obtain the mutex then it will temporarily
			inherit the priority of the task attempting to obtain the mutex. */
			if( pxTCB->uxPriority < pxCurrentTCB[portCPUID()]->uxPriority )
			{
				/* Adjust the mutex holder state to account for its new
				priority.  Only reset the event list item value if the value is
				not	being used for anything else. */
				if( ( listGET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ) ) & taskEVENT_LIST_ITEM_VALUE_IN_USE ) == 0UL )
				{
					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB[portCPUID()]->uxPriority ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}

				/* If the task being modified is in the ready state it will need
				to be moved into a new list. */
				if( listIS_CONTAINED_WITHIN( &( pxReadyTasksLists[portCPUID()][ pxTCB->uxPriority ] ), &( pxTCB->xStateListItem ) ) != pdFALSE )
				{
					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{
						taskRESET_READY_PRIORITY( pxTCB->uxPriority );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* Inherit the priority before being moved into the new list. */
					pxTCB->uxPriority = pxCurrentTCB[portCPUID()]->uxPriority;
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					/* Just inherit the priority. */
					pxTCB->uxPriority = pxCurrentTCB[portCPUID()]->uxPriority;
				}

				traceTASK_PRIORITY_INHERIT( pxTCB, pxCurrentTCB[portCPUID()]->uxPriority );
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

	BaseType_t xTaskPriorityDisinherit( TaskHandle_t const pxMutexHolder )
	{
	TCB_t * const pxTCB = ( TCB_t * ) pxMutexHolder;
	BaseType_t xReturn = pdFALSE;

		if( pxMutexHolder != NULL )
		{
			/* A task can only have an inherited priority if it holds the mutex.
			If the mutex is held by a task then it cannot be given from an
			interrupt, and if a mutex is given by the holding task then it must
			be the running state task. */
			configASSERT( pxTCB == pxCurrentTCB[portCPUID()] );

			configASSERT( pxTCB->uxMutexesHeld );
			( pxTCB->uxMutexesHeld )--;

			/* Has the holder of the mutex inherited the priority of another
			task? */
			if( pxTCB->uxPriority != pxTCB->uxBasePriority )
			{
				/* Only disinherit if no other mutexes are held. */
				if( pxTCB->uxMutexesHeld == ( UBaseType_t ) 0 )
				{
					/* A task can only have an inherited priority if it holds
					the mutex.  If the mutex is held by a task then it cannot be
					given from an interrupt, and if a mutex is given by the
					holding	task then it must be the running state task.  Remove
					the	holding task from the ready	list. */
					if( uxListRemove( &( pxTCB->xStateListItem ) ) == ( UBaseType_t ) 0 )
					{
						taskRESET_READY_PRIORITY( pxTCB->uxPriority );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* Disinherit the priority before adding the task into the
					new	ready list. */
					traceTASK_PRIORITY_DISINHERIT( pxTCB, pxTCB->uxBasePriority );
					pxTCB->uxPriority = pxTCB->uxBasePriority;

					/* Reset the event list item value.  It cannot be in use for
					any other purpose if this task is running, and it must be
					running to give back the mutex. */
					listSET_LIST_ITEM_VALUE( &( pxTCB->xEventListItem ), ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxTCB->uxPriority ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */
					prvAddTaskToReadyList( pxTCB );

					/* Return true to indicate that a context switch is required.
					This is only actually required in the corner case whereby
					multiple mutexes were held and the mutexes were given back
					in an order different to that in which they were taken.
					If a context switch did not occur when the first mutex was
					returned, even if a task was waiting on it, then a context
					switch should occur when the last mutex is returned whether
					a task is waiting on it or not. */
					xReturn = pdTRUE;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		return xReturn;
	}

#endif /* configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if ( portCRITICAL_NESTING_IN_TCB == 1 )

	void vTaskEnterCritical( void )
	{
		portDISABLE_INTERRUPTS();

		if( xSchedulerRunning[portCPUID()] != pdFALSE )
		{
			( pxCurrentTCB[portCPUID()]->uxCriticalNesting )++;

			/* This is not the interrupt safe version of the enter critical
			function so	assert() if it is being called from an interrupt
			context.  Only API functions that end in "FromISR" can be used in an
			interrupt.  Only assert if the critical nesting count is 1 to
			protect against recursive calls if the assert function also uses a
			critical section. */
			if( pxCurrentTCB[portCPUID()]->uxCriticalNesting == 1 )
			{
				portASSERT_IF_IN_ISR();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* portCRITICAL_NESTING_IN_TCB */
/*-----------------------------------------------------------*/

#if ( portCRITICAL_NESTING_IN_TCB == 1 )

	void vTaskExitCritical( void )
	{
		if( xSchedulerRunning[portCPUID()] != pdFALSE )
		{
			if( pxCurrentTCB[portCPUID()]->uxCriticalNesting > 0U )
			{
				( pxCurrentTCB[portCPUID()]->uxCriticalNesting )--;

				if( pxCurrentTCB[portCPUID()]->uxCriticalNesting == 0U )
				{
					portENABLE_INTERRUPTS();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* portCRITICAL_NESTING_IN_TCB */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	static char *prvWriteNameToBuffer( char *pcBuffer, const char *pcTaskName )
	{
	size_t x;

		/* Start by copying the entire string. */
		strcpy( pcBuffer, pcTaskName );

		/* Pad the end of the string with spaces to ensure columns line up when
		printed out. */
		for( x = strlen( pcBuffer ); x < ( size_t ) ( configMAX_TASK_NAME_LEN - 1 ); x++ )
		{
			pcBuffer[ x ] = ' ';
		}

		/* Terminate. */
		pcBuffer[ x ] = 0x00;

		/* Return the new end of string. */
		return &( pcBuffer[ x ] );
	}

#endif /* ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) */
/*-----------------------------------------------------------*/

#if ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	void vTaskList( char * pcWriteBuffer )
	{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	char cStatus;

		/*
		 * PLEASE NOTE:
		 *
		 * This function is provided for convenience only, and is used by many
		 * of the demo applications.  Do not consider it to be part of the
		 * scheduler.
		 *
		 * vTaskList() calls uxTaskGetSystemState(), then formats part of the
		 * uxTaskGetSystemState() output into a human readable table that
		 * displays task names, states and stack usage.
		 *
		 * vTaskList() has a dependency on the sprintf() C library function that
		 * might bloat the code size, use a lot of stack, and provide different
		 * results on different platforms.  An alternative, tiny, third party,
		 * and limited functionality implementation of sprintf() is provided in
		 * many of the FreeRTOS/Demo sub-directories in a file called
		 * printf-stdarg.c (note printf-stdarg.c does not provide a full
		 * snprintf() implementation!).
		 *
		 * It is recommended that production systems call uxTaskGetSystemState()
		 * directly to get access to raw stats data, rather than indirectly
		 * through a call to vTaskList().
		 */


		/* Make sure the write buffer does not contain a string. */
		*pcWriteBuffer = 0x00;

		/* Take a snapshot of the number of tasks in case it changes while this
		function is executing. */
		uxArraySize = uxCurrentNumberOfTasks[portCPUID()];

		/* Allocate an array index for each task.  NOTE!  if
		configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
		equate to NULL. */
		pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks[portCPUID()] * sizeof( TaskStatus_t ) );

		if( pxTaskStatusArray != NULL )
		{
			/* Generate the (binary) data. */
			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );

			/* Create a human readable table from the binary data. */
			for( x = 0; x < uxArraySize; x++ )
			{
				switch( pxTaskStatusArray[ x ].eCurrentState )
				{
					case eReady:		cStatus = tskREADY_CHAR;
										break;

					case eBlocked:		cStatus = tskBLOCKED_CHAR;
										break;

					case eSuspended:	cStatus = tskSUSPENDED_CHAR;
										break;

					case eDeleted:		cStatus = tskDELETED_CHAR;
										break;

					default:			/* Should not get here, but it is included
										to prevent static checking errors. */
										cStatus = 0x00;
										break;
				}

				/* Write the task name to the string, padding with spaces so it
				can be printed in tabular form more easily. */
				pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );

				/* Write the rest of the string. */
				sprintf( pcWriteBuffer, "\t%c\t%u\t%u\t%u\r\n", cStatus, ( unsigned int ) pxTaskStatusArray[ x ].uxCurrentPriority, ( unsigned int ) pxTaskStatusArray[ x ].usStackHighWaterMark, ( unsigned int ) pxTaskStatusArray[ x ].xTaskNumber );
				pcWriteBuffer += strlen( pcWriteBuffer );
			}

			/* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
			is 0 then vPortFree() will be #defined to nothing. */
			vPortFree( pxTaskStatusArray );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* ( ( configUSE_TRACE_FACILITY == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*----------------------------------------------------------*/

#if ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) )

	void vTaskGetRunTimeStats( char *pcWriteBuffer )
	{
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	uint32_t ulTotalTime, ulStatsAsPercentage;

		#if( configUSE_TRACE_FACILITY != 1 )
		{
			#error configUSE_TRACE_FACILITY must also be set to 1 in FreeRTOSConfig.h to use vTaskGetRunTimeStats().
		}
		#endif

		/*
		 * PLEASE NOTE:
		 *
		 * This function is provided for convenience only, and is used by many
		 * of the demo applications.  Do not consider it to be part of the
		 * scheduler.
		 *
		 * vTaskGetRunTimeStats() calls uxTaskGetSystemState(), then formats part
		 * of the uxTaskGetSystemState() output into a human readable table that
		 * displays the amount of time each task has spent in the Running state
		 * in both absolute and percentage terms.
		 *
		 * vTaskGetRunTimeStats() has a dependency on the sprintf() C library
		 * function that might bloat the code size, use a lot of stack, and
		 * provide different results on different platforms.  An alternative,
		 * tiny, third party, and limited functionality implementation of
		 * sprintf() is provided in many of the FreeRTOS/Demo sub-directories in
		 * a file called printf-stdarg.c (note printf-stdarg.c does not provide
		 * a full snprintf() implementation!).
		 *
		 * It is recommended that production systems call uxTaskGetSystemState()
		 * directly to get access to raw stats data, rather than indirectly
		 * through a call to vTaskGetRunTimeStats().
		 */

		/* Make sure the write buffer does not contain a string. */
		*pcWriteBuffer = 0x00;

		/* Take a snapshot of the number of tasks in case it changes while this
		function is executing. */
		uxArraySize = uxCurrentNumberOfTasks[portCPUID()];

		/* Allocate an array index for each task.  NOTE!  If
		configSUPPORT_DYNAMIC_ALLOCATION is set to 0 then pvPortMalloc() will
		equate to NULL. */
		pxTaskStatusArray = pvPortMalloc( uxCurrentNumberOfTasks[portCPUID()] * sizeof( TaskStatus_t ) );

		if( pxTaskStatusArray != NULL )
		{
			/* Generate the (binary) data. */
			uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalTime );

			/* For percentage calculations. */
			ulTotalTime /= 100UL;

			/* Avoid divide by zero errors. */
			if( ulTotalTime > 0 )
			{
				/* Create a human readable table from the binary data. */
				for( x = 0; x < uxArraySize; x++ )
				{
					/* What percentage of the total run time has the task used?
					This will always be rounded down to the nearest integer.
					ulTotalRunTimeDiv100 has already been divided by 100. */
					ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalTime;

					/* Write the task name to the string, padding with
					spaces so it can be printed in tabular form more
					easily. */
					pcWriteBuffer = prvWriteNameToBuffer( pcWriteBuffer, pxTaskStatusArray[ x ].pcTaskName );

					if( ulStatsAsPercentage > 0UL )
					{
						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcWriteBuffer, "\t%lu\t\t%lu%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter, ulStatsAsPercentage );
						}
						#else
						{
							/* sizeof( int ) == sizeof( long ) so a smaller
							printf() library can be used. */
							sprintf( pcWriteBuffer, "\t%u\t\t%u%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter, ( unsigned int ) ulStatsAsPercentage );
						}
						#endif
					}
					else
					{
						/* If the percentage is zero here then the task has
						consumed less than 1% of the total run time. */
						#ifdef portLU_PRINTF_SPECIFIER_REQUIRED
						{
							sprintf( pcWriteBuffer, "\t%lu\t\t<1%%\r\n", pxTaskStatusArray[ x ].ulRunTimeCounter );
						}
						#else
						{
							/* sizeof( int ) == sizeof( long ) so a smaller
							printf() library can be used. */
							sprintf( pcWriteBuffer, "\t%u\t\t<1%%\r\n", ( unsigned int ) pxTaskStatusArray[ x ].ulRunTimeCounter );
						}
						#endif
					}

					pcWriteBuffer += strlen( pcWriteBuffer );
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			/* Free the array again.  NOTE!  If configSUPPORT_DYNAMIC_ALLOCATION
			is 0 then vPortFree() will be #defined to nothing. */
			vPortFree( pxTaskStatusArray );
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}

#endif /* ( ( configGENERATE_RUN_TIME_STATS == 1 ) && ( configUSE_STATS_FORMATTING_FUNCTIONS > 0 ) ) */
/*-----------------------------------------------------------*/

TickType_t uxTaskResetEventItemValue( void )
{
TickType_t uxReturn;

	uxReturn = listGET_LIST_ITEM_VALUE( &( pxCurrentTCB[portCPUID()]->xEventListItem ) );

	/* Reset the event list item to its normal value - so it can be used with
	queues and semaphores. */
	listSET_LIST_ITEM_VALUE( &( pxCurrentTCB[portCPUID()]->xEventListItem ), ( ( TickType_t ) configMAX_PRIORITIES - ( TickType_t ) pxCurrentTCB[portCPUID()]->uxPriority ) ); /*lint !e961 MISRA exception as the casts are only redundant for some ports. */

	return uxReturn;
}
/*-----------------------------------------------------------*/

#if ( configUSE_MUTEXES == 1 )

	void *pvTaskIncrementMutexHeldCount( void )
	{
		/* If xSemaphoreCreateMutex() is called before any tasks have been created
		then pxCurrentTCB[portCPUID()] will be NULL. */
		if( pxCurrentTCB[portCPUID()] != NULL )
		{
			( pxCurrentTCB[portCPUID()]->uxMutexesHeld )++;
		}

		return pxCurrentTCB[portCPUID()];
	}

#endif /* configUSE_MUTEXES */
/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )
	{
	uint32_t ulReturn;

		taskENTER_CRITICAL();
		{
			/* Only block if the notification count is not already non-zero. */
			if( pxCurrentTCB[portCPUID()]->ulNotifiedValue == 0UL )
			{
				/* Mark this task as waiting for a notification. */
				pxCurrentTCB[portCPUID()]->ucNotifyState = taskWAITING_NOTIFICATION;

				if( xTicksToWait > ( TickType_t ) 0 )
				{
					prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
					traceTASK_NOTIFY_TAKE_BLOCK();

					/* All ports are written to allow a yield in a critical
					section (some will yield immediately, others wait until the
					critical section exits) - but it is not something that
					application code should ever do. */
					portYIELD_WITHIN_API();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		taskENTER_CRITICAL();
		{
			traceTASK_NOTIFY_TAKE();
			ulReturn = pxCurrentTCB[portCPUID()]->ulNotifiedValue;

			if( ulReturn != 0UL )
			{
				if( xClearCountOnExit != pdFALSE )
				{
					pxCurrentTCB[portCPUID()]->ulNotifiedValue = 0UL;
				}
				else
				{
					pxCurrentTCB[portCPUID()]->ulNotifiedValue = ulReturn - 1;
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			pxCurrentTCB[portCPUID()]->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
		}
		taskEXIT_CRITICAL();

		return ulReturn;
	}

#endif /* configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait )
	{
	BaseType_t xReturn;

		taskENTER_CRITICAL();
		{
			/* Only block if a notification is not already pending. */
			if( pxCurrentTCB[portCPUID()]->ucNotifyState != taskNOTIFICATION_RECEIVED )
			{
				/* Clear bits in the task's notification value as bits may get
				set	by the notifying task or interrupt.  This can be used to
				clear the value to zero. */
				pxCurrentTCB[portCPUID()]->ulNotifiedValue &= ~ulBitsToClearOnEntry;

				/* Mark this task as waiting for a notification. */
				pxCurrentTCB[portCPUID()]->ucNotifyState = taskWAITING_NOTIFICATION;

				if( xTicksToWait > ( TickType_t ) 0 )
				{
					prvAddCurrentTaskToDelayedList( xTicksToWait, pdTRUE );
					traceTASK_NOTIFY_WAIT_BLOCK();

					/* All ports are written to allow a yield in a critical
					section (some will yield immediately, others wait until the
					critical section exits) - but it is not something that
					application code should ever do. */
					portYIELD_WITHIN_API();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		taskENTER_CRITICAL();
		{
			traceTASK_NOTIFY_WAIT();

			if( pulNotificationValue != NULL )
			{
				/* Output the current notification value, which may or may not
				have changed. */
				*pulNotificationValue = pxCurrentTCB[portCPUID()]->ulNotifiedValue;
			}

			/* If ucNotifyValue is set then either the task never entered the
			blocked state (because a notification was already pending) or the
			task unblocked because of a notification.  Otherwise the task
			unblocked because of a timeout. */
			if( pxCurrentTCB[portCPUID()]->ucNotifyState == taskWAITING_NOTIFICATION )
			{
				/* A notification was not received. */
				xReturn = pdFALSE;
			}
			else
			{
				/* A notification was already pending or a notification was
				received while the task was waiting. */
				pxCurrentTCB[portCPUID()]->ulNotifiedValue &= ~ulBitsToClearOnExit;
				xReturn = pdTRUE;
			}

			pxCurrentTCB[portCPUID()]->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif /* configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskGenericNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue )
	{
	TCB_t * pxTCB;
	BaseType_t xReturn = pdPASS;
	uint8_t ucOriginalNotifyState;

		configASSERT( xTaskToNotify );
		pxTCB = ( TCB_t * ) xTaskToNotify;

		taskENTER_CRITICAL();
		{
			if( pulPreviousNotificationValue != NULL )
			{
				*pulPreviousNotificationValue = pxTCB->ulNotifiedValue;
			}

			ucOriginalNotifyState = pxTCB->ucNotifyState;

			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;

			switch( eAction )
			{
				case eSetBits	:
					pxTCB->ulNotifiedValue |= ulValue;
					break;

				case eIncrement	:
					( pxTCB->ulNotifiedValue )++;
					break;

				case eSetValueWithOverwrite	:
					pxTCB->ulNotifiedValue = ulValue;
					break;

				case eSetValueWithoutOverwrite :
					if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
					{
						pxTCB->ulNotifiedValue = ulValue;
					}
					else
					{
						/* The value could not be written to the task. */
						xReturn = pdFAIL;
					}
					break;

				case eNoAction:
					/* The task is being notified without its notify value being
					updated. */
					break;
			}

			traceTASK_NOTIFY();

			/* If the task is in the blocked state specifically to wait for a
			notification then unblock it now. */
			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{
				( void ) uxListRemove( &( pxTCB->xStateListItem ) );
				prvAddTaskToReadyList( pxTCB );

				/* The task should not have been on an event list. */
				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				#if( configUSE_TICKLESS_IDLE != 0 )
				{
					/* If a task is blocked waiting for a notification then
					xNextTaskUnblockTime might be set to the blocked task's time
					out time.  If the task is unblocked for a reason other than
					a timeout xNextTaskUnblockTime is normally left unchanged,
					because it will automatically get reset to a new value when
					the tick count equals xNextTaskUnblockTime.  However if
					tickless idling is used it might be more important to enter
					sleep mode at the earliest possible time - so reset
					xNextTaskUnblockTime here to ensure it is updated at the
					earliest possible time. */
					prvResetNextTaskUnblockTime();
				}
				#endif

				if( pxTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
				{
					/* The notified task has a priority above the currently
					executing task so a yield is required. */
					taskYIELD_IF_USING_PREEMPTION();
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif /* configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskGenericNotifyFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotificationValue, BaseType_t *pxHigherPriorityTaskWoken )
	{
	TCB_t * pxTCB;
	uint8_t ucOriginalNotifyState;
	BaseType_t xReturn = pdPASS;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToNotify );

		/* RTOS ports that support interrupt nesting have the concept of a
		maximum	system call (or maximum API call) interrupt priority.
		Interrupts that are	above the maximum system call priority are keep
		permanently enabled, even when the RTOS kernel is in a critical section,
		but cannot make any calls to FreeRTOS API functions.  If configASSERT()
		is defined in FreeRTOSConfig.h then
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
		failure if a FreeRTOS API function is called from an interrupt that has
		been assigned a priority above the configured maximum system call
		priority.  Only FreeRTOS functions that end in FromISR can be called
		from interrupts	that have been assigned a priority at or (logically)
		below the maximum system call interrupt priority.  FreeRTOS maintains a
		separate interrupt safe API to ensure interrupt entry is as fast and as
		simple as possible.  More information (albeit Cortex-M specific) is
		provided on the following link:
		http://www.freertos.org/RTOS-Cortex-M3-M4.html */
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		pxTCB = ( TCB_t * ) xTaskToNotify;

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			if( pulPreviousNotificationValue != NULL )
			{
				*pulPreviousNotificationValue = pxTCB->ulNotifiedValue;
			}

			ucOriginalNotifyState = pxTCB->ucNotifyState;
			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;

			switch( eAction )
			{
				case eSetBits	:
					pxTCB->ulNotifiedValue |= ulValue;
					break;

				case eIncrement	:
					( pxTCB->ulNotifiedValue )++;
					break;

				case eSetValueWithOverwrite	:
					pxTCB->ulNotifiedValue = ulValue;
					break;

				case eSetValueWithoutOverwrite :
					if( ucOriginalNotifyState != taskNOTIFICATION_RECEIVED )
					{
						pxTCB->ulNotifiedValue = ulValue;
					}
					else
					{
						/* The value could not be written to the task. */
						xReturn = pdFAIL;
					}
					break;

				case eNoAction :
					/* The task is being notified without its notify value being
					updated. */
					break;
			}

			traceTASK_NOTIFY_FROM_ISR();

			/* If the task is in the blocked state specifically to wait for a
			notification then unblock it now. */
			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{
				/* The task should not have been on an event list. */
				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
				{
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					/* The delayed and ready lists cannot be accessed, so hold
					this task pending until the scheduler is resumed. */
					vListInsertEnd( &( xPendingReadyList[portCPUID()] ), &( pxTCB->xEventListItem ) );
				}

				if( pxTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
				{
					/* The notified task has a priority above the currently
					executing task so a yield is required. */
					if( pxHigherPriorityTaskWoken != NULL )
					{
						*pxHigherPriorityTaskWoken = pdTRUE;
					}
					else
					{
						/* Mark that a yield is pending in case the user is not
						using the "xHigherPriorityTaskWoken" parameter to an ISR
						safe FreeRTOS function. */
						xYieldPending[portCPUID()] = pdTRUE;
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );

		return xReturn;
	}

#endif /* configUSE_TASK_NOTIFICATIONS */
/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken )
	{
	TCB_t * pxTCB;
	uint8_t ucOriginalNotifyState;
	UBaseType_t uxSavedInterruptStatus;

		configASSERT( xTaskToNotify );

		/* RTOS ports that support interrupt nesting have the concept of a
		maximum	system call (or maximum API call) interrupt priority.
		Interrupts that are	above the maximum system call priority are keep
		permanently enabled, even when the RTOS kernel is in a critical section,
		but cannot make any calls to FreeRTOS API functions.  If configASSERT()
		is defined in FreeRTOSConfig.h then
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID() will result in an assertion
		failure if a FreeRTOS API function is called from an interrupt that has
		been assigned a priority above the configured maximum system call
		priority.  Only FreeRTOS functions that end in FromISR can be called
		from interrupts	that have been assigned a priority at or (logically)
		below the maximum system call interrupt priority.  FreeRTOS maintains a
		separate interrupt safe API to ensure interrupt entry is as fast and as
		simple as possible.  More information (albeit Cortex-M specific) is
		provided on the following link:
		http://www.freertos.org/RTOS-Cortex-M3-M4.html */
		portASSERT_IF_INTERRUPT_PRIORITY_INVALID();

		pxTCB = ( TCB_t * ) xTaskToNotify;

		uxSavedInterruptStatus = portSET_INTERRUPT_MASK_FROM_ISR();
		{
			ucOriginalNotifyState = pxTCB->ucNotifyState;
			pxTCB->ucNotifyState = taskNOTIFICATION_RECEIVED;

			/* 'Giving' is equivalent to incrementing a count in a counting
			semaphore. */
			( pxTCB->ulNotifiedValue )++;

			traceTASK_NOTIFY_GIVE_FROM_ISR();

			/* If the task is in the blocked state specifically to wait for a
			notification then unblock it now. */
			if( ucOriginalNotifyState == taskWAITING_NOTIFICATION )
			{
				/* The task should not have been on an event list. */
				configASSERT( listLIST_ITEM_CONTAINER( &( pxTCB->xEventListItem ) ) == NULL );

				if( uxSchedulerSuspended[portCPUID()] == ( UBaseType_t ) pdFALSE )
				{
					( void ) uxListRemove( &( pxTCB->xStateListItem ) );
					prvAddTaskToReadyList( pxTCB );
				}
				else
				{
					/* The delayed and ready lists cannot be accessed, so hold
					this task pending until the scheduler is resumed. */
					vListInsertEnd( &( xPendingReadyList[portCPUID()] ), &( pxTCB->xEventListItem ) );
				}

				if( pxTCB->uxPriority > pxCurrentTCB[portCPUID()]->uxPriority )
				{
					/* The notified task has a priority above the currently
					executing task so a yield is required. */
					if( pxHigherPriorityTaskWoken != NULL )
					{
						*pxHigherPriorityTaskWoken = pdTRUE;
					}
					else
					{
						/* Mark that a yield is pending in case the user is not
						using the "xHigherPriorityTaskWoken" parameter in an ISR
						safe FreeRTOS function. */
						xYieldPending[portCPUID()] = pdTRUE;
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
		portCLEAR_INTERRUPT_MASK_FROM_ISR( uxSavedInterruptStatus );
	}

#endif /* configUSE_TASK_NOTIFICATIONS */

/*-----------------------------------------------------------*/

#if( configUSE_TASK_NOTIFICATIONS == 1 )

	BaseType_t xTaskNotifyStateClear( TaskHandle_t xTask )
	{
	TCB_t *pxTCB;
	BaseType_t xReturn;

		/* If null is passed in here then it is the calling task that is having
		its notification state cleared. */
		pxTCB = prvGetTCBFromHandle( xTask );

		taskENTER_CRITICAL();
		{
			if( pxTCB->ucNotifyState == taskNOTIFICATION_RECEIVED )
			{
				pxTCB->ucNotifyState = taskNOT_WAITING_NOTIFICATION;
				xReturn = pdPASS;
			}
			else
			{
				xReturn = pdFAIL;
			}
		}
		taskEXIT_CRITICAL();

		return xReturn;
	}

#endif /* configUSE_TASK_NOTIFICATIONS *//*-----------------------------------------------------------*/


static void prvAddCurrentTaskToDelayedList( TickType_t xTicksToWait, const BaseType_t xCanBlockIndefinitely )
{
TickType_t xTimeToWake;
const TickType_t xConstTickCount = xTickCount[portCPUID()];

	#if( INCLUDE_xTaskAbortDelay == 1 )
	{
		/* About to enter a delayed list, so ensure the ucDelayAborted flag is
		reset to pdFALSE so it can be detected as having been set to pdTRUE
		when the task leaves the Blocked state. */
		pxCurrentTCB[portCPUID()]->ucDelayAborted = pdFALSE;
	}
	#endif

	/* Remove the task from the ready list before adding it to the blocked list
	as the same list item is used for both lists. */
	if( uxListRemove( &( pxCurrentTCB[portCPUID()]->xStateListItem ) ) == ( UBaseType_t ) 0 )
	{
		/* The current task must be in a ready list, so there is no need to
		check, and the port reset macro can be called directly. */
		portRESET_READY_PRIORITY( pxCurrentTCB[portCPUID()]->uxPriority, uxTopReadyPriority[portCPUID()] );
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	#if ( INCLUDE_vTaskSuspend == 1 )
	{
		if( ( xTicksToWait == portMAX_DELAY ) && ( xCanBlockIndefinitely != pdFALSE ) )
		{
			/* Add the task to the suspended task list instead of a delayed task
			list to ensure it is not woken by a timing event.  It will block
			indefinitely. */
			vListInsertEnd( &xSuspendedTaskList[portCPUID()], &( pxCurrentTCB[portCPUID()]->xStateListItem ) );
		}
		else
		{
			/* Calculate the time at which the task should be woken if the event
			does not occur.  This may overflow but this doesn't matter, the
			kernel will manage it correctly. */
			xTimeToWake = xConstTickCount + xTicksToWait;

			/* The list item will be inserted in wake time order. */
			listSET_LIST_ITEM_VALUE( &( pxCurrentTCB[portCPUID()]->xStateListItem ), xTimeToWake );

			if( xTimeToWake < xConstTickCount )
			{
				/* Wake time has overflowed.  Place this item in the overflow
				list. */
				vListInsert( pxOverflowDelayedTaskList[portCPUID()], &( pxCurrentTCB[portCPUID()]->xStateListItem ) );
			}
			else
			{
				/* The wake time has not overflowed, so the current block list
				is used. */
				vListInsert( pxDelayedTaskList[portCPUID()], &( pxCurrentTCB[portCPUID()]->xStateListItem ) );

				/* If the task entering the blocked state was placed at the
				head of the list of blocked tasks then xNextTaskUnblockTime
				needs to be updated too. */
				if( xTimeToWake < xNextTaskUnblockTime[portCPUID()] )
				{
					xNextTaskUnblockTime[portCPUID()] = xTimeToWake;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
		}
	}
	#else /* INCLUDE_vTaskSuspend */
	{
		/* Calculate the time at which the task should be woken if the event
		does not occur.  This may overflow but this doesn't matter, the kernel
		will manage it correctly. */
		xTimeToWake = xConstTickCount + xTicksToWait;

		/* The list item will be inserted in wake time order. */
		listSET_LIST_ITEM_VALUE( &( pxCurrentTCB[portCPUID()]->xStateListItem ), xTimeToWake );

		if( xTimeToWake < xConstTickCount )
		{
			/* Wake time has overflowed.  Place this item in the overflow list. */
			vListInsert( pxOverflowDelayedTaskList[portCPUID()], &( pxCurrentTCB[portCPUID()]->xStateListItem ) );
		}
		else
		{
			/* The wake time has not overflowed, so the current block list is used. */
			vListInsert( pxDelayedTaskList[portCPUID()], &( pxCurrentTCB[portCPUID()]->xStateListItem ) );

			/* If the task entering the blocked state was placed at the head of the
			list of blocked tasks then xNextTaskUnblockTime needs to be updated
			too. */
			if( xTimeToWake < xNextTaskUnblockTime[portCPUID()] )
			{
				xNextTaskUnblockTime[portCPUID()] = xTimeToWake;
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}

		/* Avoid compiler warning when INCLUDE_vTaskSuspend is not 1. */
		( void ) xCanBlockIndefinitely;
	}
	#endif /* INCLUDE_vTaskSuspend */

#if( configUSE_GLOBAL_EDF == 1 )
    // Task finished execution within its period so we need to reset the state
    pxCurrentTCB[portCPUID()]->xTaskState = TASK_WAITING;
#endif /* configUSE_GLOBAL_EDF */    
}

/*-------------------------------------------------------------------------------------*/

#if ( configUSE_SCHEDULER_EDF == 1 && configUSE_SRP == 1 )

#ifndef MAX_SYS_CEIL_LEVELS
    #define MAX_SYS_CEIL_LEVELS 16
#endif

#ifndef MAX_RUNTIME_STACK_DEPTH
#define MAX_RUNTIME_STACK_DEPTH ( ( size_t ) ( 100 * 4096 ) ) // Number of stack variables that stack can hold
#endif

#ifndef TEMP_STACK_DEPTH
#define TEMP_STACK_DEPTH ( ( size_t ) ( 4096 ) )
#endif

BaseType_t vSRPInitSRP(void) {
    uint32_t xMaxSysCeilStackSize;
    StackType_t *pxTopOfStack;

    xMaxSysCeilStackSize = MAX_SYS_CEIL_LEVELS * sizeof( StackType_t );

    mSysCeilStack.xStackBottomLimit = (StackType_t *) pvPortMalloc( xMaxSysCeilStackSize );
    
#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    uint32_t xMaxRuntimeStackSize;
    uint32_t xMaxTempStackSize;
    
    if ( mSysCeilStack.xStackBottomLimit != NULL ) {
        xMaxRuntimeStackSize = MAX_RUNTIME_STACK_DEPTH * sizeof( StackType_t );

        mRuntimeStack.xStackBottomLimit = (StackType_t *) pvPortMalloc( xMaxRuntimeStackSize );

        if ( mRuntimeStack.xStackBottomLimit == NULL ) {
            vPortFree( mSysCeilStack.xStackBottomLimit );
            mSysCeilStack.xStackBottomLimit = NULL;
            return pdFALSE;
        }

        xMaxTempStackSize = TEMP_STACK_DEPTH * sizeof( StackType_t );
        mTempStack.xStackBottomLimit = (StackType_t *) pvPortMalloc( xMaxTempStackSize );
        
        if ( mTempStack.xStackBottomLimit == NULL ) {
            vPortFree( mSysCeilStack.xStackBottomLimit );
            vPortFree( mRuntimeStack.xStackBottomLimit );
            return pdFALSE;
        }
    }
#else /* configUSE_SHARED_RUNTIME_STACK */
    if ( mSysCeilStack.xStackBottomLimit == NULL ) {
        return pdFALSE;
    }
#endif /* configUSE_SHARED_RUNTIME_STACK */

    mSysCeilStack.xStackTopLimit = mSysCeilStack.xStackBottomLimit + xMaxSysCeilStackSize - 1;
    mSysCeilStack.xStackTop = mSysCeilStack.xStackTopLimit + 1;

#if( configUSE_SHARED_RUNTIME_STACK == 1 )    
    mRuntimeStack.xStackTopLimit = mRuntimeStack.xStackBottomLimit + xMaxRuntimeStackSize - 1;
    mRuntimeStack.xStackTop = mRuntimeStack.xStackTopLimit + 1;

    mTempStack.xStackTopLimit = mTempStack.xStackBottomLimit + xMaxTempStackSize - 1;
    mTempStack.xStackTop = mTempStack.xStackTopLimit + 1;
    
    printk("Initial mRuntimeStack.xStackTopLimit: 0x%x\r\n", mRuntimeStack.xStackTopLimit);
    printk("Initial mRuntimeStack.xStackBottomLimit: 0x%x\r\n", mRuntimeStack.xStackBottomLimit);
    printk("Initial mRuntimeStack.xStackTop: 0x%x\r\n", mRuntimeStack.xStackTop);
    printk("Initial mTempStack.xStackTopLimit: 0x%x\r\n", mTempStack.xStackTopLimit);
    printk("Initial mTempStack.xStackBottomLimit: 0x%x\r\n", mTempStack.xStackBottomLimit);

#endif /* configUSE_SHARED_RUNTIME_STACK */
    
    // Align the top of the stack
    pxTopOfStack = ( StackType_t *) mSysCeilStack.xStackTop;
    pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

    mSysCeilStack.xStackTop = pxTopOfStack;
    configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
    pxTopOfStack = (StackType_t * ) mRuntimeStack.xStackTop;
    pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );

    mRuntimeStack.xStackTop = pxTopOfStack;
    configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );

    pxTopOfStack = (StackType_t * ) mTempStack.xStackTop;
    pxTopOfStack = ( StackType_t * ) ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack ) & ( ~( ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) ) );    

    mTempStack.xStackTop = pxTopOfStack;
    configASSERT( ( ( ( portPOINTER_SIZE_TYPE ) pxTopOfStack & ( portPOINTER_SIZE_TYPE ) portBYTE_ALIGNMENT_MASK ) == 0UL ) );
#endif /* configUSE_SHARED_RUNTIME_STACK */
    
    /* Push NULL to indicate lowest system ceiling. */
    srpSysCeilStackPush( (StackType_t) NULL );

    vListInitialise(&pxResourceList);

    return pdTRUE;
}

ResourceHandle_t vSRPSemaphoreCreateBinary(void) {

    Resource_t *vResource = pvPortMalloc(sizeof(Resource_t));
    if (vResource != NULL)
    {
        vResource->xSemaphore = xSemaphoreCreateBinary();
        if (vResource->xSemaphore == NULL)
        {
            vPortFree(vResource);
            vResource = NULL;
        }
        else
        {
            vResource->xCeilPtr = NULL;
            xSemaphoreGive(vResource->xSemaphore);
        }
    }

    vResource->pxOwner = NULL;

    vListInitialiseItem(&(vResource->xResourceItem));
    listSET_LIST_ITEM_OWNER(&(vResource->xResourceItem), vResource);
    vListInsertEnd(&pxResourceList, &(vResource->xResourceItem));

    return (ResourceHandle_t) vResource;
}


/* This function is called from the task level */
BaseType_t vSRPSemaphoreTake(ResourceHandle_t vResourceHandle, TickType_t xBlockTime) {
    BaseType_t vVal;
    StackType_t *vTopPtr;
    Resource_t *vResource;

    vResource = (Resource_t *) vResourceHandle;

    portENTER_CRITICAL();
    vTopPtr = (StackType_t *) srpSysCeilStackPeak();
    configASSERT( vTopPtr == NULL || pxCurrentTCB[portCPUID()]->xPreemptionLevel > *vTopPtr );
    srpSysCeilStackPush( (StackType_t) vResource->xCeilPtr );
    portEXIT_CRITICAL();

    vVal = xSemaphoreTake( vResource->xSemaphore, xBlockTime );

    if ( vVal == pdFALSE ) {
        portENTER_CRITICAL();
        vSRPSysCeilStackPop();
        portEXIT_CRITICAL();
    }

    vResource->pxOwner = pxCurrentTCB[portCPUID()];

    return vVal;
}

/* This function is called from the task level */
BaseType_t vSRPSemaphoreGive(ResourceHandle_t vResourceHandle) {
    BaseType_t vVal;
    Resource_t *vResource;

    vResource = (Resource_t *) vResourceHandle;

    vVal = xSemaphoreGive( vResource->xSemaphore );

    if ( vVal == pdFALSE ) {
        return pdFALSE;
    }

    portENTER_CRITICAL();
    vSRPSysCeilStackPop();
    portEXIT_CRITICAL();

    vResource->pxOwner = NULL;

    

    return pdTRUE;
}

void vSRPTCBSemaphoreGive(TCB_t* tcb)
{

    ListItem_t const* xEndMarker = listGET_END_MARKER(&pxResourceList);
    ListItem_t* xCurrentItem = listGET_HEAD_ENTRY(&pxResourceList);
    while(xCurrentItem != xEndMarker)
    {
        Resource_t* xResource = listGET_LIST_ITEM_OWNER(xCurrentItem);
        if (xResource->pxOwner == tcb )
        {
             BaseType_t pdVal = xSemaphoreGive(xResource->xSemaphore);
             configASSERT(pdVal == pdTRUE);
             vSRPSysCeilStackPop();
        }
    }
}

void srpSetTaskBlockTime(TaskHandle_t xTaskHandle, TickType_t xBlockTime) {
    TCB_t *pxTCB;
    pxTCB = (TCB_t *) xTaskHandle;
    pxTCB->xBlockTime = xBlockTime;
}

static StackType_t srpSysCeilStackPeak(void) {
    StackType_t vStackVar;

    configASSERT(mSysCeilStack.xStackTop != NULL);

    vStackVar = *mSysCeilStack.xStackTop;

    return vStackVar;
}

static void vSRPSysCeilStackPop(void) {
    configASSERT( mSysCeilStack.xStackTop != NULL );
    configASSERT( mSysCeilStack.xStackTop < mSysCeilStack.xStackTopLimit );

    mSysCeilStack.xStackTop++;
}

static void srpSysCeilStackPush(StackType_t vStackVar) {

    srpStackPush( &mSysCeilStack, vStackVar );
    
}

static void srpStackPush(Stack_t *vStackT, StackType_t vStackVar) {
    configASSERT(vStackT != NULL);

    vStackT->xStackTop--;
    *vStackT->xStackTop = vStackVar;
}

void srpConfigToUseResource(ResourceHandle_t vResourceHandle, TaskHandle_t vTaskHandle) {
    TCB_t *pxTCB;
    Resource_t *pxResource;

    pxTCB = (TCB_t *) vTaskHandle;
    pxResource = (Resource_t *) vResourceHandle;
    
    if ( pxResource->xCeilPtr == NULL ||
         pxTCB->xPreemptionLevel > *pxResource->xCeilPtr ) {
        pxResource->xCeilPtr = &pxTCB->xPreemptionLevel;
    }
}

#if( configUSE_SHARED_RUNTIME_STACK == 1 )
static void srpRuntimeStackPopTask(void) {
    if( pxCurrentTCB[portCPUID()]->pxTaskCode != prvIdleTask ) {
        configASSERT( pxCurrentTCB[portCPUID()]->pxStack != NULL );
        // Set it back to empty, doesn't matter if other tasks are still running, because
        // if they get preempted then the stack top value will be set to the correct value
        // in vTaskSwitchContext
        mRuntimeStack.xStackTop = pxCurrentTCB[portCPUID()]->pxStack;

        pxCurrentTCB[portCPUID()]->pxStack = NULL;
    }
}

static void srpInitTaskRuntimeStack(TCB_t * vTCB) {
    if ( vTCB->pxTaskCode != prvIdleTask ) {
        if ( vTCB->pxStack == NULL ) {
            vTCB->pxStack = mRuntimeStack.xStackTop;
            
            vTCB->pxTopOfStack = pxPortInitialiseStack( mRuntimeStack.xStackTop - 1, vTCB->pxTaskCode, vTCB->pvParameters );
            configASSERT( vTCB->pxTopOfStack >= mRuntimeStack.xStackBottomLimit );
        }
    }        
}
#endif

#endif /* configUSE_SRP */

#if( configUSE_SCHEDULER_EDF == 1  && configUSE_CBS_SERVER == 1)
BaseType_t xServerCBSCreate( TaskFunction_t pxTaskCode,
                              const char * const pcName,
                              const uint16_t usStackDepth,
                              void * const pvParameters,
                              UBaseType_t uxPriority,
                              TickType_t xBudget,
                              TickType_t xPeriod,
                              TaskHandle_t * const pxCreatedTask )
{
    TCB_t* pxNewServerTCB = NULL;
    JobQueue_t* pxJobQueue = NULL;

    pxJobQueue  = (JobQueue_t *) pvPortMalloc( sizeof( JobQueue_t ) );
    
    if (pxJobQueue == NULL) {
        return pdFALSE;
    };

    pxJobQueue->pxJobItems = (JobItem_t *) pvPortMalloc( sizeof( JobItem_t ) * SERVER_JOB_QUEUE_SIZE );

    if (pxJobQueue->pxJobItems == NULL) {
        vPortFree(pxJobQueue);
        return pdFALSE;
    }

    pxJobQueue->sMaxSize = SERVER_JOB_QUEUE_SIZE;
    pxJobQueue->sHead = 0;
    pxJobQueue->sSize = 0;
    
    BaseType_t pdResult = xTaskCreate( pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority,
                                       xBudget, 0, xPeriod, (TaskHandle_t*) &pxNewServerTCB );
    configASSERT( pdResult == pdTRUE );

    pxNewServerTCB->pxJobQueue = (BaseType_t) pxJobQueue;
   
    pxNewServerTCB->xIsServer = pdTRUE;
    pxNewServerTCB->fUtilization = ( pxNewServerTCB->xWCET / pxNewServerTCB->xPeriod );
    
    *pxCreatedTask = ( TaskHandle_t ) pxNewServerTCB;
    
    vTaskSuspend( pxNewServerTCB );

    return pdResult;
}


void vServerCBSReplenish( TCB_t* pxServerTCB )
{
    pxServerTCB->xCurrentRunTime = 0;
    printk("Replen: Server xItemValue: %u\r\n", pxServerTCB->xStateListItem.xItemValue);
    pxServerTCB->xStateListItem.xItemValue += pxServerTCB->xPeriod;
    pxServerTCB->xRelativeDeadline = pxServerTCB->xStateListItem.xItemValue;
}

void vServerCBSRefresh( TCB_t* pxServerTCB )
{
    if ( !listIS_CONTAINED_WITHIN( &pxReadyTasksLists[portCPUID()][PRIORITY_EDF], &pxServerTCB->xStateListItem ) ) {
        return;
    }
    printk("Refreshing!\r\n");
    uxListRemove( &pxServerTCB->xStateListItem );
    prvAddTaskToReadyList( pxServerTCB );
}

/* TickType_t xServerCBSGetCurrentCapacity( TaskHandle_t pxServer ) */
/* { */
/*     TCB_t* pxServerTCB = ( TCB_t * ) pxServer; */
/*     configASSERT( pxServerTCB->xIsServer == pdTRUE ); */
/*     return pxServerTCB->xWCET - pxServerTCB->xCurrentRunTime; */
/* } */

void vServerCBSNotify( TaskHandle_t pxServer, TaskFunction_t pxJobCode, void * pvParameters)
{
    taskDISABLE_INTERRUPTS();
    
    TCB_t* pxServerTCB = ( TCB_t * ) pxServer;
    
    // If the server is already active. i.e. servicing a request. Do nothing.
    if ( listIS_CONTAINED_WITHIN( &pxReadyTasksLists[portCPUID()][PRIORITY_EDF], &pxServerTCB->xStateListItem ) )
        return;

    // If the server is idle and we dont have enough capacity then increase the deadline and
    // replenish else do nothing.
    // TODO: Handle tick count overflows
    TickType_t xRemainingCapacity = pxServerTCB->xWCET - pxServerTCB->xCurrentRunTime;
    if ( ( xTaskGetTickCount() + ( ( ( ( float ) xRemainingCapacity ) / pxServerTCB->xWCET) * pxServerTCB->xPeriod ) )  >=
         pxServerTCB->xStateListItem.xItemValue )
    {
        pxServerTCB->xStateListItem.xItemValue = 0;
        vServerCBSReplenish( pxServerTCB );
    }

    vServerCBSQueueJob(pxServerTCB, pxJobCode, pvParameters);    
    vTaskResume( pxServerTCB );

    taskENABLE_INTERRUPTS();    
}

static BaseType_t vServerCBSQueueJob( TCB_t *pxServerTCB, TaskFunction_t pxJobCode, void * pvParameters ) {
    JobQueue_t *pxJobQueue = (JobQueue_t *) pxServerTCB->pxJobQueue;

    if (pxJobQueue->sSize == pxJobQueue->sMaxSize) {
        printk("Server Job Queue is full, Server Name: %s, not registering job\r\n", pxServerTCB->pcTaskName);
        return pdFALSE;
    }

    size_t sTail = (pxJobQueue->sHead + pxJobQueue->sSize) % pxJobQueue->sMaxSize;
    pxJobQueue->pxJobItems[sTail].pxJobCode = pxJobCode;
    pxJobQueue->pxJobItems[sTail].pvParameters = pvParameters;
    pxJobQueue->sSize++;

    return pdTRUE;
}

static void vServerCBSDequeueJob( JobQueue_t * pxJobQueue ) {
    if (pxJobQueue->sSize == 0) {
        return;
    }
    pxJobQueue->sHead = (pxJobQueue->sHead + 1) % pxJobQueue->sMaxSize;
    pxJobQueue->sSize--;
}

#define vServerCBSPeakJob( pxJobQueue ) &pxJobQueue->pxJobItems[pxJobQueue->sHead];

void vServerCBSRunJob(void) {
    //printk("vServerCBSRunJob beginning\r\n");
    TCB_t *pxServerTCB = (TCB_t *) pxCurrentTCB[portCPUID()];
    configASSERT(pxServerTCB->xIsServer);
    
    int xLeftover;
    
    JobQueue_t *pxJobQueue = (JobQueue_t *) pxServerTCB->pxJobQueue;
    if (pxJobQueue->sSize == 0) {
        printk("No Job is Queued when calling vServerRunJob\r\n, Server Name: %s", pxServerTCB->pcTaskName);
        return;
    }

    while (1) {
        JobItem_t *xJobItem;
    
        xJobItem = vServerCBSPeakJob(pxJobQueue);
        configASSERT(xJobItem->pxJobCode != NULL);

        (* xJobItem->pxJobCode) (xJobItem->pvParameters);
    
        vServerCBSDequeueJob(pxJobQueue);
        
        xLeftover = pxServerTCB->xWCET - pxServerTCB->xCurrentRunTime;
        if (xLeftover <= 0) {
            break;
        }
        if (pxJobQueue->sSize == 0) {
#if( configUSE_CBS_CASH == 1 )
            portDISABLE_INTERRUPTS();        
            printk("CASH Leftover: %u, Server: %s\r\n", xLeftover, pxServerTCB->pcTaskName);

            size_t sCursor, sTail;
            CASHItem_t *xCASHItems = xCASHQueue.xCASHItems;
            
            // Try to find the correct position starting from head. This is because mod
            // in C is not the mathematical mod, it is the remainder and will return negative
            // results so we want to avoid decrementing.
            sCursor = xCASHQueue.sHead;
            sTail = ( xCASHQueue.sHead + xCASHQueue.sSize ) % xCASHQueue.sMaxSize;
            while (sCursor != sTail) {
                printk("CASH Item: xDeadline: %u, xBudget: %u\r\n", xCASHItems[sCursor].xDeadline, xCASHItems[sCursor].xBudget);   
                sCursor = (sCursor + 1) % xCASHQueue.sMaxSize;
            }

            portENABLE_INTERRUPTS();            

            vServerCBSQueueCASH(xLeftover, pxCurrentTCB[portCPUID()]->xStateListItem.xItemValue);
#endif /* configUSE_CBS_CASE */
            break;
        }
        portDISABLE_INTERRUPTS();        
        printk("Leftover budget for other tasks: %u\r\n", xLeftover);
        portENABLE_INTERRUPTS();                
    }
    portDISABLE_INTERRUPTS();
    printk("vServerCBSRunJob completed, suspending server: %s\r\n", pxServerTCB->pcTaskName);
    portENABLE_INTERRUPTS();
    vTaskSuspend(NULL);
}

#if( configUSE_CBS_CASH == 1 )

void vServerCBSDecrementCASHQueue(void) {
    if (xCASHQueue.sSize == 0) {
        // printk("Decrementing an empty CASH Queue, ignoring...\r\n");
        return;
    }
    CASHItem_t *xCASHItems = xCASHQueue.xCASHItems;
    xCASHItems[xCASHQueue.sHead].xBudget--;
    if (xCASHItems[xCASHQueue.sHead].xBudget == 0) {
        xCASHQueue.sHead = (xCASHQueue.sHead + 1) % xCASHQueue.sMaxSize;
        xCASHQueue.sSize--;
        printk("Removed item\r\n");
    }
}

void vServerCBSQueueCASH(TickType_t xLeftover, TickType_t xDeadline) {
    configASSERT(xLeftover > 0);
    portENTER_CRITICAL();
    
    if (xCASHQueue.sSize >= CASH_QUEUE_SIZE) {
        printk("CASH queue over run! panic...");
        configASSERT(pdFALSE);
    }
    size_t sCursor, sTail;
    CASHItem_t *xCASHItems = xCASHQueue.xCASHItems;

    // Try to find the correct position starting from head. This is because mod
    // in C is not the mathematical mod, it is the remainder and will return negative
    // results so we want to avoid decrementing.
    sCursor = xCASHQueue.sHead;
    sTail = ( xCASHQueue.sHead + xCASHQueue.sSize ) % xCASHQueue.sMaxSize;
    while (sCursor != sTail) {
        if (xCASHItems[sCursor].xDeadline > xDeadline) {
            // Found right position.
            break;
        }
        else if (xCASHItems[sCursor].xDeadline == xDeadline) {
            // Deadline is the same, so we don't need to add a new entry.
            xCASHItems[sCursor].xBudget += xLeftover;
            return;
        }
        sCursor = (sCursor + 1) % xCASHQueue.sMaxSize;
    }

    // Shift everything one element back, starting from sCursor to sTail
    size_t sLeft = sCursor;
    size_t sRight;
    CASHItem_t xItem = xCASHItems[sLeft];
    while (sLeft != sTail) {
        sRight = (sLeft + 1) % xCASHQueue.sMaxSize;
        SWAP (xCASHItems[sRight], xItem);
        sLeft = sRight;
    }
    
    xCASHItems[sCursor].xBudget = xLeftover;
    xCASHItems[sCursor].xDeadline = xDeadline;
    xCASHQueue.sSize++;

    portEXIT_CRITICAL();
}
#endif /* configUSE_CBS_CASH */

#endif /* configUSE_SCHEDULER_EDF && configUSE_CBS_SERVER */

#if( configUSE_GLOBAL_EDF == 1 )
/**
 * Used by the kernel core.
 */
TCB_t *xGetSecondaryCoreCurrentTask(uint32_t cpuid) {
    TCB_t *pxCurrentTask;

    portSPIN_LOCK_ACQUIRE( &mCoreInfo[cpuid].xLock );
    pxCurrentTask = mCoreInfo[cpuid].pxCurrentTask;
    portSPIN_LOCK_RELEASE( &mCoreInfo[cpuid].xLock );

    return pxCurrentTask;    
}

/**
 * Used by the kernel core.
 */
TCB_t *xGetSecondaryCorePendingTask(uint32_t cpuid) {
    TCB_t *pxPendingTask;

    portSPIN_LOCK_ACQUIRE( &mCoreInfo[cpuid].xLock );
    pxPendingTask = mCoreInfo[cpuid].pxPendingTask;
    portSPIN_LOCK_RELEASE( &mCoreInfo[cpuid].xLock );

    return pxPendingTask;    
}

/**
 * Used by the kernel core.
 */
void vSetSecondaryCorePendingTask(uint32_t cpuid, TCB_t *xTCB) {
    portSPIN_LOCK_ACQUIRE( &mCoreInfo[cpuid].xLock );
    mCoreInfo[cpuid].pxPendingTask = xTCB;
    portSPIN_LOCK_RELEASE( &mCoreInfo[cpuid].xLock );
}

#define xGetCoreIdleTask() mCoreInfo[portCPUID()].pxIdleTask
#define xGetCorePendingTask() xGetSecondaryCorePendingTask( portCPUID() )
#define vSetCorePendingTask( xTCB ) vSetSecondaryCorePendingTask( portCPUID(), xTCB )

void vSetCoreCurrentTask(TCB_t *xTask) {
    uint32_t id = portCPUID();
    
    portSPIN_LOCK_ACQUIRE( &mCoreInfo[id].xLock );
    mCoreInfo[id].pxCurrentTask = xTask;
    portSPIN_LOCK_RELEASE( &mCoreInfo[id].xLock );    
}

TCB_t *xGetCoreCurrentTask(void) {
    TCB_t * xTask = xGetSecondaryCoreCurrentTask( portCPUID() );        
    if ( portCPUID() != portKERNEL_CORE &&
         xTask == NULL) {
        xTask = mCoreInfo[portCPUID()].pxIdleTask;
    }
    //printk("xGetCoreCurrentTask: %s\r\n", xTask->pcTaskName);
    return xTask;
    
}

TCB_t *xPopCorePendingTask(void) {
    uint32_t id = portCPUID();
    TCB_t *pxPendingTask;
    //printk("Pop!\r\n");

    portSPIN_LOCK_ACQUIRE( &mCoreInfo[id].xLock );
    pxPendingTask = mCoreInfo[id].pxPendingTask;
    mCoreInfo[id].pxPendingTask = NULL;
    mCoreInfo[id].pxCurrentTask = pxPendingTask;

    if (pxPendingTask == NULL) {
        portSPIN_LOCK_RELEASE( &mCoreInfo[id].xLock );
        //printk("Core: %u, pending task NULL, panic...\r\n", portCPUID());
        configASSERT(pxPendingTask != NULL);
    }
    portSPIN_LOCK_RELEASE( &mCoreInfo[id].xLock );

    //printk("pxPendingTask->pcTaskName: %s\r\n", pxPendingTask->pcTaskName);
    return pxPendingTask;
}

/**
 * Determines whether or not secondary cores should switch task.
 *
 */
uint32_t ulSecondaryCoreIRQHandler(void) {
    uint32_t msg_type = portREAD_CORE_CHANNEL( portSECONDARY_CORE_INT_CHANNEL );
    //printk("My portCPUID: %u, msg: 0x%x\r\n", portCPUID(), msg_type);
    portCLEAR_CORE_INTERRUPT( portSECONDARY_CORE_INT_CHANNEL );

//    msg_type = portREAD_CORE_CHANNEL( portSECONDARY_CORE_INT_CHANNEL );
//    printk("msg after: %u\r\n", msg_type);

    return msg_type == MSG_SWITCH_TASK;
}

/**
 * Function that needs to be called after saving the context for a task on a
 * secondary core. This function updates the task state of the saved task to
 * ensure proper scheduling.
 */
void vSecondaryCoreAfterSaveContext(void) {
    //printk("vSecondaryCoreAfterSaveContext!\r\n");
    TCB_t *xCurrentTask = xGetCoreCurrentTask();
    //printk("xCurrentTask->pcTaskName: %s\r\n", xCurrentTask->pcTaskName);
    if (xCurrentTask != xGetCoreIdleTask()) {
        //printk("Mark xCurrentTask->xTaskState: %u\r\n", xCurrentTask->xTaskState);
        if (xCurrentTask->xTaskState == TASK_COMPLETED) {
            // Assuming a core has the same number of unique channels as the number of cores.
            portSEND_CORE_INTERRUPT( portKERNEL_CORE, portCPUID(), MSG_FINISHED_TASK );

            INC_NUM_IDLE_CORES();
        }
        else if (xCurrentTask->xTaskState == TASK_SET_PHASE) {
            portSEND_CORE_INTERRUPT( portKERNEL_CORE, portCPUID(), MSG_SET_TASK_PHASE );

            INC_NUM_IDLE_CORES();
        }
        else {
            xCurrentTask->xTaskState = TASK_WAITING;
        }
    }
    vSetCoreCurrentTask( NULL );
}

extern void vPortSecondaryRestoreTaskContext(void);
/**
 * Initializes the core before executing the idle task of the core.
 * This function must only be used by vInitCoreStructures to setup entry
 * point for secondary cores.
 */
static void vCoreWaitToStart(void) {
    //printk("vCoreWaitToStart Reached!\r\n");
    portENABLE_CORE_INTERRUPT_CHANNEL( portSECONDARY_CORE_INT_CHANNEL );
    portENABLE_INTERRUPTS();

    vPortSecondaryRestoreTaskContext();
    while(1);
}

void vInitCoreStructures(uint32_t numCoresUsed) {
    if (numCoresUsed <= 1) {
        return;
    }
    BaseType_t xResult = pdPASS;
    
    if (vSyncQueueInit(&xFinishedTaskQueue, MAX_FINISH_QUEUE_SIZE)) {
        printk("Failed to initialize finished task queue!\r\n");
        return;
    }

    uint32_t cpuid;    
    TaskHandle_t tskHandle = 0;
    TCB_t *tsk = NULL;    
    
    for (cpuid=1; cpuid<NUM_CORES; cpuid++) {
        mCoreInfo[cpuid].pxCurrentTask = NULL;
        mCoreInfo[cpuid].pxPendingTask = NULL;
        mCoreInfo[cpuid].xLock = 0;
        
        portENABLE_CORE_INTERRUPT_CHANNEL( cpuid );

        xResult = xTaskCreate( prvSecondaryIdleTask,
                                "Secondary IDLE", configMINIMAL_STACK_SIZE,
                                ( void * ) NULL,
                                ( tskIDLE_PRIORITY | portPRIVILEGE_BIT ),
                                0xFFFFFFFF,
                                0xFFFFFFFF,
                                0xFFFFFFFF,
                                &tskHandle);
        if (xResult != pdPASS) {
           // printk("Init core structures, out of memory, panic...\r\n");
            configASSERT(0);
        }
        tsk = (TCB_t *) tskHandle;
        // Idle Task are always seen as executing
        tsk->xTaskState = TASK_EXECUTING;
         
        // Cores not started running tasks yet so it's safe to access the core info of
        // other cores.            
        mCoreInfo[cpuid].pxIdleTask = tsk;
        mCoreInfo[cpuid].pxPendingTask = tsk;
        portCORE_SET_START_FUNCTION(cpuid, vCoreWaitToStart);
    }
    
    ulNumCoresUsed = numCoresUsed;
    ulNumIdleCores = numCoresUsed-1;
}

/**
 * Signal to the kernel core that the current running task
 * on secondary core has completed.
 */
void vCoreSignalTaskEnd(void) {
    portDISABLE_INTERRUPTS();

    TCB_t *xCurrentTask = xGetCoreCurrentTask();
    vSyncQueueEnqueue( &xFinishedTaskQueue, (void *) xCurrentTask );
    xCurrentTask->xTaskState = TASK_COMPLETED;

    TCB_t *xPendingTask = xGetCorePendingTask();
    if (xPendingTask == NULL) {
        TCB_t *idle = xGetCoreIdleTask();
        vSetCorePendingTask( idle );
    }

    portENABLE_INTERRUPTS();
}

void vCoreSignalTaskEndPhase( TickType_t ticks ) {
    portDISABLE_INTERRUPTS();

    TCB_t *xCurrentTask = xGetCoreCurrentTask();
    vSyncQueueEnqueue( &xFinishedTaskQueue, (void *) xCurrentTask );
    xCurrentTask->xTaskPhase = ticks;
    xCurrentTask->xTaskState = TASK_SET_PHASE;

    TCB_t *xPendingTask = xGetCorePendingTask();
    if (xPendingTask == NULL) {
        TCB_t *idle = xGetCoreIdleTask();
        vSetCorePendingTask( idle );
    }

    portENABLE_INTERRUPTS();
}

/**
 * This function should only be called by the kernel core.
 * @param msg: message send by other core.
 */
void vKernelCoreInterruptHandler(uint32_t msg) {
    //printk("Handling completed task\r\n");
    if (msg == MSG_FINISHED_TASK || msg == MSG_SET_TASK_PHASE) {

        TCB_t *xTask = (TCB_t *) vSyncQueueDequeue(&xFinishedTaskQueue);

        configASSERT(xTask != NULL);

        TCB_t *xSavedCurrentTCB = pxCurrentTCB[portCPUID()];
        pxCurrentTCB[portCPUID()] = xTask;

        if (msg == MSG_SET_TASK_PHASE) {
            vTaskDelayUntil2( &pxCurrentTCB[portCPUID()]->xLastWakeTime, pxCurrentTCB[portCPUID()]->xTaskPhase );
        }
        else {
            vTaskDelayUntil2( &pxCurrentTCB[portCPUID()]->xLastWakeTime, pxCurrentTCB[portCPUID()]->xPeriod );
        }

        pxCurrentTCB[portCPUID()] = xSavedCurrentTCB;
    }
}

void prvSecondaryIdleTask(void * param) {
    while(1);
}

#endif /* configUSE_GLOBAL_EDF */

#if ( configUSE_PARTITION_SCHEDULER == 1 )

UBaseType_t uxTaskGetNumberOfCoreTasks( uint32_t core )
{
	return uxCurrentNumberOfTasks[core];
}

TaskHandle_t pxCoreGetCurrentTask( uint32_t core )
{

    return ( TaskHandle_t ) pxCurrentTCB[core];
}

UBaseType_t xCoreGetCurrentTasklist( UBaseType_t xCore, TaskHandle_t* pxTasks, UBaseType_t xSize )
{
    portDISABLE_INTERRUPTS();
    if ( pxTasks == NULL || xSize <= 0 )
        return 0;

    List_t* pxTaskLists[] = { &pxReadyTasksLists[ xCore ][ PRIORITY_EDF ],
                              &xDelayedTaskList1[ xCore ], &xDelayedTaskList2[ xCore ],
                              &xPendingReadyList[ xCore ], NULL };

    List_t** pxTaskList = pxTaskLists;
    ListItem_t const* pxEndMarker = NULL;
    ListItem_t* pxCurrentItem = NULL;
    UBaseType_t xTaskIndex = 0;


    while ( *pxTaskList != NULL )
    {

        pxEndMarker = listGET_END_MARKER( *pxTaskList );
        pxCurrentItem = listGET_HEAD_ENTRY( *pxTaskList );

        while( pxCurrentItem != pxEndMarker && pxCurrentItem != NULL )
        {
            pxTasks[xTaskIndex++] = listGET_LIST_ITEM_OWNER( pxCurrentItem );
            if ( xTaskIndex >= xSize )
            {
                portENABLE_INTERRUPTS();
                return xTaskIndex;
            }
            pxCurrentItem = listGET_NEXT( pxCurrentItem );
        }

        pxTaskList++;
    }

    portENABLE_INTERRUPTS();
    return xTaskIndex;
}

float fTaskGetUtilization( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->fUtilization;
}

BaseType_t xTaskGetWCET( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->xWCET;
}

BaseType_t xTaskGetExecution( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->xCurrentRunTime;
}

BaseType_t xTaskGetPeriod( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->xPeriod;
}

BaseType_t xTaskGetDeadline( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->xRelativeDeadline;
}

BaseType_t xTaskGetRunningTime( TaskHandle_t xTask )
{
    TCB_t* xTaskTCB = ( TCB_t* ) xTask;
    return xTaskTCB->xRelativeDeadline;
}

#endif /* configUSE_PARITION_SCHEDULER */

#ifdef FREERTOS_MODULE_TEST
	#include "tasks_test_access_functions.h"
#endif
