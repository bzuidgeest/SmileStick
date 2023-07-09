#include "shared.h"
#include "pico/util/queue.h"
#include "pico/multicore.h"


void printState();

extern queue_t call_queue;
extern queue_t outputQueue;


static bool redButton = false;
static bool yellowButton = false;
static bool blueButton = false;
static bool greenButton = false;
static bool enterButton = false;
static bool helpButton = false;
static bool exitButton = false;
static bool learningZoneButton = false;
static char x = 0;
static char y = 0;