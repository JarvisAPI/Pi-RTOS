//
//  edfexact.c
//  
//
//  Created by Leo Zhang on 2018-10-30.
//

#include <stdio.h>

#if ( configUSE_EDF_EXACT_ANALYSIS == 1 )
/* If configUSE_EDF_EXACT_ANALYSIS is 1 then exact analysis is used instead of LL bound
 */
{
    
}

/*
 precondition: put new task into ready list
 returns boolean indicating if current system plus new task is schedulable
 through exact edf analysis
 */
bool edfexact(void)
{
    List_t* readyList = &pxReadyTasksLists[PRIORITY_EDF];
    float sum = 0;
    float P = 0;
    float D = 0;
    float U = 0;
    float L = 0;
    float dTotalUtilization = 0;
    float lStar = 0;
    
    uint32_t ulNumTasks = listCURRENT_LIST_LENGTH( readyList );
    
    //find lStar
    ListItem_t* currentItem = listGET_HEAD_ENTRY(readyList);
    ListItem_t const* endMarker = listGET_END_MARKER(readyList);
    while( currentItem != endMarker )
    {
        TCB_t* tcb = listGET_LIST_ITEM_OWNER( currentItem );
        P = (float) tcb->xPeriod;
        D = (float) tcb->xRelativeDeadline;
        U = (float) tcb->xWCET / tcb->xRelativeDeadline;
        dTotalUtilization += U;
        lStar += (P - D) * U;
        currentItem = listGET_NEXT( currentItem );
    }
    if( dTotalUtilization > 1 ) {
        return false;
    }
    lStar /= ( 1 - dTotalUtilization );
    
    //check all absolute deadlines by iterating through all periods of each task
    currentItem = listGET_HEAD_ENTRY(readyList);
    while( currentItem != endMarker )
    {
        //get task parameters
        TCB_t* tcb = listGET_LIST_ITEM_OWNER( currentItem );
        P = (float) tcb->xPeriod;
        D = (float) tcb->xRelativeDeadline;
        U = (float) tcb->xWCET / tcb->xRelativeDeadline;
        dTotalUtilization += U;
        lStar += (P - D) * U;
        currentItem = listGET_NEXT( currentItem );

        //verify that demand at time L is less than L, where L is equal to
        //the task's absolute deadlines up to lStar
        for( float L = P; L <= lStar; L += P )
        {
            float totalDemand = 0;

            ListItem_t* currentItem2 = listGET_HEAD_ENTRY(readyList);
            ListItem_t const* endMarker2 = listGET_END_MARKER(readyList);

            while( currentItem2 != endMarker2 )
            {
                TCB_t* tcb2 = listGET_LIST_ITEM_OWNER( currentItem2 );
                float P2 = tcb2->xPeriod;
                float D2 = tcb2->xRelativeDeadline;
                float C2 = tcb2->xWCET;
                int temp = ( L + P2 - D2 ) / P2;
                totalDemand += temp * C2;

                currentItem = listGET_NEXT( currentItem );
            }
            if( totalDemand > L ) {
                while(1);
                return false;
            }
        }
    }
    
    return true;
}
