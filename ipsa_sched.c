/*
 * FreeRTOS V202107.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 * NOTE 1: The FreeRTOS demo threads will not be running continuously, so
 * do not expect to get real time behaviour from the FreeRTOS Linux port, or
 * this demo application.  Also, the timing information in the FreeRTOS+Trace
 * logs have no meaningful units.  See the documentation page for the Linux
 * port for further information:
 * https://freertos.org/FreeRTOS-simulator-for-Linux.html
 *
 * NOTE 2:  This project provides two demo applications.  A simple blinky style
 * project, and a more comprehensive test and demo application.  The
 * mainCREATE_SIMPLE_BLINKY_DEMO_ONLY setting in main.c is used to select
 * between the two.  See the notes on using mainCREATE_SIMPLE_BLINKY_DEMO_ONLY
 * in main.c.  This file implements the simply blinky version.  Console output
 * is used in place of the normal LED toggling.
 *
 * NOTE 3:  This file only contains the source code that is specific to the
 * basic demo.  Generic functions, such FreeRTOS hook functions, are defined
 * in main.c.
 ******************************************************************************
 *
 * main_blinky() creates one queue, one software timer, and two tasks.  It then
 * starts the scheduler.
 *
 * The Queue Send Task:
 * The queue send task is implemented by the prvQueueSendTask() function in
 * this file.  It uses vTaskDelayUntil() to create a periodic task that sends
 * the value 100 to the queue every 200 milliseconds (please read the notes
 * above regarding the accuracy of timing under Linux).
 *
 * The Queue Send Software Timer:
 * The timer is an auto-reload timer with a period of two seconds.  The timer's
 * callback function writes the value 200 to the queue.  The callback function
 * is implemented by prvQueueSendTimerCallback() within this file.
 *
 * The Queue Receive Task:
 * The queue receive task is implemented by the prvQueueReceiveTask() function
 * in this file.  prvQueueReceiveTask() waits for data to arrive on the queue.
 * When data is received, the task checks the value of the data, then outputs a
 * message to indicate if the data came from the queue send task or the queue
 * send software timer.
 *
 * Expected Behaviour:
 * - The queue send task writes to the queue every 200ms, so every 200ms the
 *   queue receive task will output a message indicating that data was received
 *   on the queue from the queue send task.
 * - The queue send software timer has a period of two seconds, and is reset
 *   each time a key is pressed.  So if two seconds expire without a key being
 *   pressed then the queue receive task will output a message indicating that
 *   data was received on the queue from the queue send software timer.
 *
 * NOTE:  Console input and output relies on Linux system calls, which can
 * interfere with the execution of the FreeRTOS Linux port. This demo only
 * uses Linux system call occasionally. Heavier use of Linux system calls
 * may crash the port.
 */

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>


/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "semphr.h"

/* Local includes. */
#include "console.h"

/* Priorities at which the tasks are created. */
#define YOUR_TASK1_PRIORITY                ( tskIDLE_PRIORITY + 1 )
#define YOUR_TASK2_PRIORITY                ( tskIDLE_PRIORITY + 2 )
#define YOUR_TASK3_PRIORITY                ( tskIDLE_PRIORITY + 3 )
#define YOUR_TASK4_PRIORITY                ( tskIDLE_PRIORITY + 4 )
/* The rate at which data is sent to the queue.  The times are converted from
 * milliseconds to ticks using the pdMS_TO_TICKS() macro. */

#define TASK1_FREQUENCY                    pdMS_TO_TICKS( 4000UL )
#define TASK2_FREQUENCY                    pdMS_TO_TICKS( 200UL )
#define TASK3_FREQUENCY                    pdMS_TO_TICKS( 3000UL )
#define TASK4_FREQUENCY                    pdMS_TO_TICKS( 800UL )


/* The number of items the queue can hold at once. */
#define mainQUEUE_LENGTH                   ( 4 )

/* The values sent to the queue receive task from the queue send task and the
 * queue send software timer respectively. */
#define mainVALUE_SENT_FROM_TASK           ( 100UL )
#define mainVALUE_SENT_FROM_TIMER          ( 200UL )

/*-----------------------------------------------------------*/

/*
 * The tasks as described in the comments at the top of this file.
 */
static void Task1( void * pvParameters );
static void Task2( void * pvParameters );
static void Task3( void * pvParameters );
static void Task4( void * pvParameters );


/*
 * The callback function executed when the software timer expires.
 */
static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle );

/*-----------------------------------------------------------*/

/* The queue used by both tasks. */
static QueueHandle_t xQueue = NULL;

/* A software timer that is started from the tick hook. */
static TimerHandle_t xTimer = NULL;

/*-----------------------------------------------------------*/

/*** SEE THE COMMENTS AT THE TOP OF THIS FILE ***/
void ipsa_sched(void)
{
    const TickType_t xTimerPeriod = 2000UL;

    /* Create the queue. */
    xQueue = xQueueCreate(mainQUEUE_LENGTH, sizeof(uint32_t));

    if (xQueue != NULL)
    {
        /* Start the two tasks as described in the comments at the top of this
         * file. */
      

        /* Create the software timer, but don't start it yet. */
        xTimer = xTimerCreate("Timer", xTimerPeriod, pdTRUE, NULL, prvQueueSendTimerCallback);

        if (xTimer != NULL)
        {
            xTimerStart(xTimer, 0);
        }

        
        xTaskCreate(Task1, "Task1", configMINIMAL_STACK_SIZE, NULL, YOUR_TASK1_PRIORITY, NULL);
        xTaskCreate(Task2, "Task2", configMINIMAL_STACK_SIZE, NULL, YOUR_TASK2_PRIORITY, NULL);
        xTaskCreate(Task3, "Task3", configMINIMAL_STACK_SIZE, NULL, YOUR_TASK3_PRIORITY, NULL);
        xTaskCreate(Task4, "Task4", configMINIMAL_STACK_SIZE, NULL, YOUR_TASK4_PRIORITY, NULL);

        /* Start the tasks and timer running. */
        vTaskStartScheduler();
    }

    /* If all is well, the scheduler will now be running, and the following
     * line will never be reached.  If the following line does execute, then
     * there was insufficient FreeRTOS heap memory available for the idle and/or
     * timer tasks	to be created.  See the memory management section on the
     * FreeRTOS web site for more details. */
    for (;;)
    {
    }
}
/*-----------------------------------------------------------*/
void Task1(void * pvParameters)
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = TASK1_FREQUENCY;

    xNextWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xNextWakeTime, xBlockTime);

        printf("Working ! :D\n");
    }
}

/*-----------------------------------------------------------*/

void Task2(void * pvParameters)
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = TASK2_FREQUENCY;
    double fahrenheit = 100;

    xNextWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xNextWakeTime, xBlockTime);

        double celsius = (5.0 / 9.0) * (fahrenheit - 32.0);
        printf("Temp: %f\n", celsius);
    }
}
/*-----------------------------------------------------------*/

void Task3(void * pvParameters)
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = TASK3_FREQUENCY;

    xNextWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xNextWakeTime, xBlockTime);

        long int num1 = 3287648234862934629;
        long int num2 = 2346723849729472340;
        long int result = num1 * num2;
        printf("Task 3 executed\n");
    }
}


/*-----------------------------------------------------------*/

int binarySearch(const int arr[], int size, int target) {
    int left = 0;
    int right = size - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (arr[mid] == target) {
            return 0;  
        } else if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
}

void Task4(void *pvParameters)
{
    TickType_t xNextWakeTime;
    const TickType_t xBlockTime = TASK4_FREQUENCY;

    xNextWakeTime = xTaskGetTickCount();

    int elements[50] = {2, 4, 7, 12, 15, 20, 22, 25, 28, 30,
                                32, 35, 40, 42, 45, 48, 50, 55, 60, 62,
                                65, 70, 75, 80, 82, 85, 88, 90, 92, 95,
                                100, 105, 110, 112, 115, 118, 120, 122, 125, 130,
                                135, 140, 145, 150, 155, 160, 165, 170, 175, 180};

    int targetElement = 15;

    for (;;)
    {
        vTaskDelayUntil(&xNextWakeTime, xBlockTime);

        binarySearch(elements, 50, targetElement);
       
        printf("Task 4 executed\n");
    }
}


/*-----------------------------------------------------------*/

static void prvQueueSendTimerCallback( TimerHandle_t xTimerHandle )
{
    const uint32_t ulValueToSend = mainVALUE_SENT_FROM_TIMER;

    /* This is the software timer callback function.  The software timer has a
     * period of two seconds and is reset each time a key is pressed.  This
     * callback function will execute if the timer expires, which will only happen
     * if a key is not pressed for two seconds. */

    /* Avoid compiler warnings resulting from the unused parameter. */
    ( void ) xTimerHandle;

    /* Send to the queue - causing the queue receive task to unblock and
     * write out a message.  This function is called from the timer/daemon task, so
     * must not block.  Hence the block time is set to 0. */
    xQueueSend( xQueue, &ulValueToSend, 0U );
}
/*-----------------------------------------------------------*/


