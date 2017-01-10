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

/*
 * main() creates all the demo application tasks, then starts the scheduler.
 * The web documentation provides more details of the standard demo application
 * tasks, which provide no particular functionality but do provide a good
 * example of how to use the FreeRTOS API.
 *
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Check" task - This only executes every five seconds but has a high priority
 * to ensure it gets processor time.  Its main function is to check that all the
 * standard demo tasks are still operational.  While no errors have been
 * discovered the check task will print out "OK" and the current simulated tick
 * time.  If an error is discovered in the execution of a task then the check
 * task will print out an appropriate error message.
 *
 */


/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#define mainTHEORETICAL_VALIDATION_RUN              ( 0 )
#define mainTHEORETICAL_MAX_TASKS                   ( 50 )

#define mainAPERIODIC_TASK_PROBABILITY              ( 80 )
#define mainAPERIODIC_SHORT_TASK_PROBABILITY_UPPER  ( 80 )
#define mainAPERIODIC_LONG_TASK_PROBABILITY_LOWER   ( 99 )

/*
 * Support for theoretical validation of schedulability.
 */
typedef struct tskPeriodic
{
    int32_t lPeriodMS;
    int32_t lDurationMS;
} tskPeriodic;

typedef struct tskAperiodic
{
    int32_t lArrivalMS;
    int32_t lDurationMS;
} tskAperiodic;

typedef struct theoreticalTasks
{
    tskPeriodic xPeriodicTasks[ mainTHEORETICAL_MAX_TASKS ];
    int16_t iPeriodicCount;
    tskAperiodic xAperiodicTasks[ mainTHEORETICAL_MAX_TASKS ];
    int16_t iAperiodicCount;
} theoreticalTasks;

void prvTheoreticalAddPeriodic( int32_t, int32_t, theoreticalTasks *pxTasks );
void prvTheoreticalAddAperiodic( int32_t, int32_t, theoreticalTasks *pxTasks );
bool prvTheoreticalTestSchedulability( theoreticalTasks *pxTasks );

/* Creates some theoretical tasks for testing. */
void prvCreateTheoreticalTasks( theoreticalTasks *pxTasks );

/*
 * Real tasks.
 */
void vShortTaskFunction( void *pvParameters );
void vMediumTaskFunction( void *pvParameters );
void vLongTaskFunction( void *pvParameters );

/* Special tasks that randomly creates aperiodic tasks. For testing. */
void vAperiodicSeederTaskFunction( void *pvParameters );

/*-----------------------------------------------------------*/

/*
 * Helpers.
 */
void prvPrintString( char *pcMessage );
char * prvAperiodicRandName( size_t length );

/*-----------------------------------------------------------*/

/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.
 */
void vApplicationMallocFailedHook( void );

/*-----------------------------------------------------------*/

int main ( void )
{
    if ( mainTHEORETICAL_VALIDATION_RUN )
    {
        theoreticalTasks *xTheoreticalTasks = malloc( sizeof( theoreticalTasks ) );
        xTheoreticalTasks->iPeriodicCount = 0;
        xTheoreticalTasks->iAperiodicCount = 0;
        prvCreateTheoreticalTasks( xTheoreticalTasks );
        if ( prvTheoreticalTestSchedulability( xTheoreticalTasks ) )
        {
            printf( "TRUE - possible to schedule all tasks" );
            fflush( stdout );
        }
        else
        {
            printf( "FALSE - impossible to schedule all tasks" );
            fflush( stdout );
        }
        return 0;
    }
    srand( time( 0 ) );

    xPeriodicTaskCreate( vMediumTaskFunction,
                         "p-mt-5",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 5,
                         1000 );
    xPeriodicTaskCreate( vShortTaskFunction,
                         "p-st-5",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 5,
                         5000 );
    xPeriodicTaskCreate( vMediumTaskFunction,
                         "p-mt-4",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 4,
                         3000 );
    /*xPeriodicTaskCreate( vLongTaskFunction,
                         "p-lt-3",
                         configMINIMAL_STACK_SIZE,
                         NULL,
                         tskIDLE_PRIORITY + 3,
                         5000 );*/

    if ( configSEED_APERIODIC_TASKS )
    {
        xPeriodicTaskCreate( vAperiodicSeederTaskFunction,
                             "p-seedert-2",
                             configMINIMAL_STACK_SIZE,
                             NULL,
                             tskIDLE_PRIORITY + 6,
                             1000 );
    }

	/* Start the scheduler itself. */
	vTaskStartScheduler();

	/* Should never get here unless there was not enough heap space to create
	the idle and other system tasks. */
	return 0;
}
/*-----------------------------------------------------------*/

void prvTheoreticalAddPeriodic( int32_t lPeriod, int32_t lDuration, theoreticalTasks *pxTasks )
{
    pxTasks->xPeriodicTasks[pxTasks->iPeriodicCount].lPeriodMS = lPeriod;
    pxTasks->xPeriodicTasks[pxTasks->iPeriodicCount].lDurationMS = lDuration;
    pxTasks->iPeriodicCount++;
}

void prvTheoreticalAddAperiodic( int32_t lArrival, int32_t lDuration, theoreticalTasks *pxTasks )
{
    pxTasks->xAperiodicTasks[pxTasks->iAperiodicCount].lArrivalMS = lArrival;
    pxTasks->xAperiodicTasks[pxTasks->iAperiodicCount].lDurationMS = lDuration;
    pxTasks->iAperiodicCount++;
}

bool prvTheoreticalTestSchedulability( theoreticalTasks *pxTasks )
{
    // TODO
    return false;
}

void prvCreateTheoreticalTasks( theoreticalTasks *pxTasks )
{
    // TODO
}
/*-----------------------------------------------------------*/

void vShortTaskFunction( void *pvParameters )
{
    /* Variables can be declared just as per a normal function. Each instance of a task
    created using this example function will have its own copy of the lVariableExample
    variable. This would not be true if the variable was declared static – in which case
    only one copy of the variable would exist, and this copy would be shared by each
    created instance of the task. (The prefixes added to variable names are described in
    section 1.5, Data Types and Coding Style Guide.) */
    int32_t li;
    for ( li = 0; li < 50000000; li++ );
    /* Should the task implementation ever break out of the above loop, then the task
    must be deleted before reaching the end of its implementing function. The NULL
    parameter passed to the vTaskDelete() API function indicates that the task to be
    deleted is the calling (this) task. The convention used to name API functions is
    described in section 1.5, Data Types and Coding Style Guide. */
    vTaskDelete( NULL );
}

void vMediumTaskFunction( void *pvParameters )
{
    int32_t li;
    for ( li = 0; li < 100000000; li++ );
    vTaskDelete( NULL );
}

void vLongTaskFunction( void *pvParameters )
{
    int32_t li;
    for ( li = 0; li < 1000000000; li++ );
    vTaskDelete( NULL );
}

void vAperiodicSeederTaskFunction( void *pvParameters )
{
    int32_t iTaskCreated;
    bool xCreateAperiodicTask = ( rand() % 100 ) < mainAPERIODIC_TASK_PROBABILITY;
    if ( xCreateAperiodicTask )
    {
        iTaskCreated = rand() % 100;
        prvPrintString("Aperiodic task created.");
        if (iTaskCreated < mainAPERIODIC_SHORT_TASK_PROBABILITY_UPPER)
        {
            xAperiodicTaskCreate( vShortTaskFunction,
                                  prvAperiodicRandName(10),
                                  configMINIMAL_STACK_SIZE,
                                  NULL );
        }
        else if (iTaskCreated > mainAPERIODIC_LONG_TASK_PROBABILITY_LOWER)
        {
            xAperiodicTaskCreate( vLongTaskFunction,
                                  prvAperiodicRandName(10),
                                  configMINIMAL_STACK_SIZE,
                                  NULL );
        }
        else
        {
            xAperiodicTaskCreate( vMediumTaskFunction,
                                  prvAperiodicRandName(10),
                                  configMINIMAL_STACK_SIZE,
                                  NULL );
        }
    }
    vTaskDelete( NULL );
}
/*-----------------------------------------------------------*/

void prvPrintString( char *pcMessage )
{
    printf( "%s\n", pcMessage );
    fflush( stdout );
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    vAssertCalled( __LINE__, __FILE__ );
}

void vAssertCalled( unsigned long ulLine, const char * const pcFileName )
{
 	taskENTER_CRITICAL();
	{
        printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
        fflush(stdout);
	}
	taskEXIT_CRITICAL();
	exit(-1);
}
/*-----------------------------------------------------------*/

char * prvAperiodicRandName(size_t length) {

    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static int ncharset = 62;
    char *randomString = NULL;
    int n;

    if (length) {
        randomString = malloc(sizeof(char) * (length + 1));

        if (randomString) {
            randomString[0] = 'a';
            randomString[1] = '-';
            for (n = 2; n < length; n++) {
                int key = rand() % ncharset;
                randomString[n] = charset[key];
            }

            randomString[length] = '\0';
        }
    }

    return randomString;
}
/*-----------------------------------------------------------*/
