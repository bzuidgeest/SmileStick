#include <stdio.h>
#include "pico/stdlib.h"
#include "core2.h"


void coreUSBMain() {
    while (1) {
        // Controller data is passed to us via the stickData_t which contains 
        // the data and the length of the data.
        
        stickData_t stickData;

        queue_remove_blocking(&call_queue, &stickData);

        // if (stickData.length == 1)
        // {
        //     printf("Received message of length %d, 0x%x.\r\n", stickData.length, stickData.message[0]);
        // }
        // else if (stickData.length == 2)
        // {
        //     printf("Received message of length %d, 0x%x, 0x%x.\r\n", stickData.length, stickData.message[0], stickData.message[1]);
        // }
        // else
        // {
        //     printf("Received message of length %d.\r\n", stickData.length);
        // }

        for (int i = 0; i < stickData.length; i++)
        {
            if ((stickData.message[i] & 0xF0) == 0xC0)
            {
                x = (stickData.message[i] & 0x0F);
            }
            if ((stickData.message[i] & 0xF0) == 0x80)
            {
                y = (stickData.message[i] & 0x0F);
            }
            if ((stickData.message[i] & 0xF0) == 0x90)
            {
                greenButton = (stickData.message[i] & 0x01) ? true : false;
                blueButton = (stickData.message[i] & 0x02) ? true : false;
                yellowButton = (stickData.message[i] & 0x04) ? true : false;
                redButton = (stickData.message[i] & 0x08) ? true : false;
            }
            if ((stickData.message[i] & 0xF0) == 0xA0)
            {
                if ((stickData.message[i] & 0x0F) == 0x3)
                {
                    helpButton = true;
                    enterButton = false;
                    exitButton = false;
                    learningZoneButton = false;
                }
                else if ((stickData.message[i] & 0x0F) == 0x1)
                {
                    helpButton = false;
                    enterButton = true;
                    exitButton = false;
                    learningZoneButton = false;
                }
                else if ((stickData.message[i] & 0x0F) == 0x2)
                {
                    helpButton = false;
                    enterButton = false;
                    exitButton = true;
                    learningZoneButton = false;
                }
                else if ((stickData.message[i] & 0x0F) == 0x4)
                {
                    helpButton = false;
                    enterButton = false;
                    exitButton = false;
                    learningZoneButton = true;
                }
                else 
                { 
                    helpButton = false;
                    enterButton = false;
                    exitButton = false;
                    learningZoneButton = false;                 
                }
            }
        
        }
        printState();
        //int32_t (*func)() = (int32_t(*)())(entry.func);
        //int32_t result = (*func)(entry.data);

        //queue_add_blocking(&results_queue, &result);
    }
}

void printState() {
	printf(redButton ? "R1 " : "R0 ");
    printf(yellowButton ? "Y1 " : "Y0 ");
    printf(blueButton ? "B1 " : "B0 ");
    printf(greenButton ? "G1 " : "G0 ");
    printf(enterButton ? "e1 " : "e0 ");
    printf(helpButton ? "h1 " : "h0 ");
    printf(exitButton ? "x1 " : "x0 ");
    printf(learningZoneButton ? "l1 " : "l0 ");
    printf(" | ");
    printf("%d", x);
    printf(", ");
    printf("%d\r\n", y);
}