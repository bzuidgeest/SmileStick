#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"
#include "core2.h"


void coreUSBMain() {
    while (1) {
        // Controller data is passed to us via the stickData_t which contains 
        // the data and the length of the data.
        
        stickData_t stickData;

        queue_remove_blocking(&call_queue, &stickData);

        if (stickData.length == 1)
        {
            printfn("Received message of length %d, %d.", stickData.length, stickData.message[0]);
        }
        else if (stickData.length == 1)
        {
            printfn("Received message of length %d, %d, %d.", stickData.length, stickData.message[0], stickData.message[1]);
        }
        else
        {
            printfn("Received message of length %d.", stickData.length);
        }
        //int32_t (*func)() = (int32_t(*)())(entry.func);
        //int32_t result = (*func)(entry.data);

        //queue_add_blocking(&results_queue, &result);
    }
}