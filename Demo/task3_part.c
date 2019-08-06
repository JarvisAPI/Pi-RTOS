#ifdef TASK3_PART
#include <FreeRTOS.h>
#include <task.h>
#include <printk.h>
#include <string.h>

#include "Drivers/rpi_gpio.h"
#include "Drivers/rpi_irq.h"
#include "Drivers/rpi_aux.h"
#include "Drivers/rpi_core_timer.h"
#include "Drivers/rpi_core.h"
#include "Drivers/rpi_memory.h"
#include "Drivers/rpi_core_timer.h"
#include "Drivers/rpi_mmu.h"
#include "Drivers/rpi_synch.h"
#include "util.h"

extern uint32_t CPUID(void);


typedef struct TaskInfo_s
{
    int iTaskNumber;
    TickType_t xWCET;
    TickType_t xPeriod;
    TickType_t xRelativeDeadline;
    const char* name;
    const char* color;
} TaskInfo_t;


///////////////////////////////////////////////////////////////////////////////////////////////////
// Task Info
///////////////////////////////////////////////////////////////////////////////////////////////////
#define TASK_NONE_NAME "N/A"

TaskInfo_t pxPartDemoTasks[][2] =
{
    {
        {1, 300, 2000, 2000, "Task 1", "[32;1m"},
        {2, 1900, 4000, 4000, "Task 2", "[33;1m"},
    },
    {
        {1, 500, 2000, 2000, "Task 1", "[32;1m"},
        {2, 500, 4000, 4000, "Task 2", "[33;1m"},
    },
    {
        {1, 1200, 2000, 2000, "Task 1", "[32;1m"},
        {2, 1000, 4000, 4000, "Task 2", "[33;1m"},
    },
    {
        {1, 1000, 2000, 2000, "Task 1", "[32;1m"},
        {1, 1200, 4000, 4000, "Task 2", "[32;1m"},
    },

};
const int iNumTasks[] = {1, 1, 1, 2};


///////////////////////////////////////////////////////////////////////////////////////////////////
// Gloabl Vars
///////////////////////////////////////////////////////////////////////////////////////////////////
static Mutex_t mLock = portMUTEX_INIT_VAL;
volatile bool bOver = true;
volatile bool bTaskPrint = true;
volatile int cores_rdy = 0;


void vTimingTestTask( void *pParam )
{
    while( true )
    {
        TaskInfo_t* xTaskInfo = ( TaskInfo_t* ) pParam;

        if ( bTaskPrint )
            printk("\033%s [ Task %d ] Started at %u\r\n\033[0m",xTaskInfo->color,
                                                                 xTaskInfo->iTaskNumber,
                                                                 xTaskGetTickCount());

        vBusyWait(xTaskInfo->xWCET);


        if ( !bOver )
        {
            // TODO Fix racey condition here
            bOver = true;
            vBusyWait( 100*xTaskInfo->xWCET );
            printk( "THIS SHOULD NEVER PRINT%d\r\n", xTaskInfo->iTaskNumber);
            configASSERT( false );
        }

        if ( bTaskPrint )
            printk("\033%s [ Task %d ] Ended at %u\r\n\033[0m",xTaskInfo->color,
                                                               xTaskInfo->iTaskNumber,
                                                               xTaskGetTickCount());

        vEndTaskPeriod();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Serial Handler
///////////////////////////////////////////////////////////////////////////////////////////////////
#define SERIAL_BUFFER_SIZE 256
#define SERIAL_TERM_CHAR 0x04
#define SERIAL_CMD_TOP "top"
#define SERIAL_CMD_TOP_QUIT "top -t"

char pcSerialBuffer[SERIAL_BUFFER_SIZE] = {0};
uint8_t ucSerialBufferLength = 0;

void vHandleKeyboard( int iIRQ,  void *pParam )
{
    char character = rpi_aux_getc();
    if ( character == SERIAL_TERM_CHAR )
    {
        // NULL terminate the buffer
        pcSerialBuffer[ucSerialBufferLength] = 0;

        // Determine command
        bool top = strcmp( SERIAL_CMD_TOP, pcSerialBuffer ) == 0;
        bool top_quit = strcmp( SERIAL_CMD_TOP_QUIT, pcSerialBuffer ) == 0;
        if ( top )
        {
            if ( !bTaskPrint )
            {
                printk("Error: TOP already started!\r\n");
            }
            else
            {
                bTaskPrint = false;
                vSetCorePrint( false );
                printk("Starting top...\r\n");
                vPrintClear();
            }
        }
        else if ( top_quit )
            if ( bTaskPrint )
            {
                printk("Error: TOP not started!\r\n");
            }
            else
            {
                bTaskPrint = true;
                vSetCorePrint( true );
                printk("Exiting  top...\r\n");
            }
        else
            printk("ERROR: Command not recognized: %s\r\n", pcSerialBuffer);

        ucSerialBufferLength = 0;
        return;
    }


    pcSerialBuffer[ucSerialBufferLength++] = character;

    // Since we need to null the buffer before strcmp we need to make sure we have a byte at the end
    if ( ucSerialBufferLength >= ( SERIAL_BUFFER_SIZE - 1 ) )
    {
        ucSerialBufferLength = 0;
        printk("Error: command too large!\r\n");
    }
}

typedef struct TableHeader_s
{
    const char* pcName;
    BaseType_t xSize;

} TableHeader_t;

void vPrintTableHeaders( const TableHeader_t* pxHeaders, BaseType_t xSize )
{

    BaseType_t xOfft = 0;

    printk("||");
    vPrintSaveCursor();

    for ( BaseType_t i = 0; i< xSize; i++ )
    {
        vPrintRestoreCursor();

        const TableHeader_t* pxCurrHeader = &pxHeaders[i];

        // Compute size and padding
        BaseType_t xNameSize = strlen( pxCurrHeader->pcName );
        configASSERT( xNameSize <= pxCurrHeader->xSize );
        BaseType_t xNamePad = (pxCurrHeader->xSize - xNameSize) / 2;

        if ( xOfft + xNamePad )
            vPrintCursorMoveRight( xOfft + xNamePad );

        printk( "%s", pxCurrHeader->pcName );

        if ( xNamePad )
            vPrintCursorMoveRight( xNamePad );

        printk("||");

        xOfft += xNameSize + ( 2 * xNamePad ) + 2;
    }   

}

void vPrintTableRowCol( const TableHeader_t* pxHeaders, BaseType_t xRow, BaseType_t xCol )
{

    BaseType_t xOfft = 0;

    for ( BaseType_t i = 0; i < xCol - 1; i++ )
    {

        // Compute size and padding
        BaseType_t xNameSize = strlen( pxHeaders[i].pcName );
        configASSERT( xNameSize <= pxHeaders[i].xSize );
        BaseType_t xNamePad = (pxHeaders[i].xSize - xNameSize) / 2;


        xOfft += xNameSize + ( 2 * xNamePad ) + 2;
    }


    vPrintSaveCursor();
    if ( xOfft )
        vPrintCursorMoveRight( xOfft );
    printk("%u", xRow);
    vPrintRestoreCursor();
    vPrintCursorMoveRight( xOfft + pxHeaders[xCol - 1].xSize );
    printk("||");
    vPrintRestoreCursor();
}

void vTopTask( void* pParam )
{
    while ( true )
    {
        if ( !bTaskPrint )
        {
            vPrintClear();
            vNPrint( '-', 86 );
            printk("\n\r");

            // Print core summaries
            for ( unsigned int core = 0; core < CORES; core++ )
            {
                // Determine Utilization and color
                float utill = fGetCoreTotalUtilization( core );
                char* utill_color = COLOR_GREEN;
                if ( utill > 0.8 )
                    utill_color = COLOR_RED;
                else if ( utill > 0.4 )
                    utill_color = COLOR_YELLOW;


                printk( "||   Core %u   ||\033[s", core );
                // Print Progress bar
                printk("\033%s", utill_color);
                vNPrint( '|', utill * 30 );
                printk("\033[m\033[u\033[30C|| ");

                // Percentage column
                vPrintSaveCursor();
                printk(" \033%s%u%%%s ", utill_color, ( uint32_t ) ( utill * 100 ), COLOR_END);
                vPrintRestoreCursor();
                vPrintCursorMoveRight( 5 );

                // Print Percentage
                printk("|| ");
                vPrintSaveCursor();
                printk(" %u Tasks ", uxTaskGetNumberOfCoreTasks( core ) );
                vPrintRestoreCursor();
                vPrintCursorMoveRight( 11 );
                printk(" || ");

                // Print the current task summaries
                vPrintSaveCursor();
                TaskHandle_t xTask = pxCoreGetCurrentTask( core );
                char* pcTaskName = xTask != NULL ? pcTaskGetName( xTask ) : TASK_NONE_NAME;
                printk(" %s ", pcTaskName );
                vPrintRestoreCursor();
                vPrintCursorMoveRight( 11 );
                printk(" ||");

                // Next Core
                printk("\n\r");
            }
            vNPrint( '-', 86 );
            printk("\r\n\r\n");

            // Draw the job table headers
            TableHeader_t x[] = { { "Name", 10 }, { "ID", 4 }, { "Core", 6 }, { "Util", 6 },
                                  { "WCET", 8 }, { "Period", 8 }, { "Deadline", 10 },
                                  { "Time", 8} };
            vPrintTableHeaders( x, 8 );

            printk("\r\n");
            // Draw the job table values
            TaskHandle_t CoreTasks[10];
            for ( unsigned int core = 0; core < CORES; core++ )
            {
                BaseType_t xCoreNumTasks = xCoreGetCurrentTasklist( 3, CoreTasks, 10 );
                for ( BaseType_t i = 0; i < xCoreNumTasks; i++ )
                {
                    printk("||");
                    vPrintSaveCursor();
                    printk( "%s", pcTaskGetName(CoreTasks[ i ] ) );
                    vPrintRestoreCursor();
                    vPrintCursorMoveRight( 10 );
                    printk("||");
                    vPrintRestoreCursor();
                    vPrintTableRowCol( x, uxTaskGetTaskNumber(CoreTasks[i]), 2 );
                    vPrintTableRowCol( x, core, 3 );
                    vPrintTableRowCol( x, 100, 4 );
                    vPrintTableRowCol( x, xTaskGetWCET(CoreTasks[i]), 5 );
                    vPrintTableRowCol( x, xTaskGetPeriod(CoreTasks[i]), 6 );
                    vPrintTableRowCol( x, xTaskGetDeadline(CoreTasks[i]), 7 );
                    vPrintTableRowCol( x, xTaskGetExecution(CoreTasks[i]), 8 );
                    printk("\r\n");
                }
            }


            printk("\r\n");

        }

        vEndTaskPeriod();
    }
}



///////////////////////////////////////////////////////////////////////////////////////////////////
// Core Setup
///////////////////////////////////////////////////////////////////////////////////////////////////
void wake(void) {
    rpi_cpu_irq_disable();
    rpi_core_timer_init();

    printk("Starting task creation\r\n");
    for (int iTaskNum = 0; iTaskNum < iNumTasks[CPUID()]; iTaskNum++)
    {
        TaskInfo_t* tasks = pxPartDemoTasks[CPUID()];
        xTaskCreate(vTimingTestTask, tasks[iTaskNum].name, 256, (void *) &tasks[iTaskNum],
                    PRIORITY_EDF, tasks[iTaskNum].xWCET, tasks[iTaskNum].xRelativeDeadline,
                    tasks[iTaskNum].xPeriod, NULL);
    }

    // Start top task for CPU 0
    if ( CPUID() == 0 )
    {
        xTaskCreate(vTopTask, "TOP TASK!", 256, NULL,
                    PRIORITY_EDF, 50, 800,
                    800, NULL);
    }
    printk("Finish creating tasks\r\n");


    portMUTEX_ACQUIRE( &mLock );
    cores_rdy++;
    
    vPrintSchedule();
    vVerifyEDFExactBound();
    vVerifyLLBound();
    portMUTEX_RELEASE( &mLock );

    while(cores_rdy < 4);

    vTaskStartScheduler();

    /*
     *	We should never get here, but just in case something goes wrong,
     *	we'll place the CPU into a safe loop.
     */
    while(1);
}

int main(void)
{
    rpi_aux_mu_init();
    mmu_init_page_table();
    mmu_init();
    rpi_cpu_irq_disable();
    vSetCorePrint(true);

    // Setup IRQs
    rpi_irq_register_handler( RPI_IRQ_ID_UART, vHandleKeyboard, NULL );
    rpi_irq_register_handler( RPI_IRQ_ID_AUX, vHandleKeyboard, NULL );
    rpi_irq_enable( RPI_IRQ_ID_UART );
    rpi_irq_enable( RPI_IRQ_ID_AUX );

    core_set_start_fn( 1, wake );
    core_start(1);
    core_set_start_fn( 2, wake );    
    core_start(2);
    core_set_start_fn( 3, wake );    
    core_start(3);

    wake();
    while(1);
}
#endif

