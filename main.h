
#include <stdio.h>
#include <string.h>
#include "shared.h"
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"




queue_t call_queue;

queue_t outputQueue;

bool sendByte(char data);