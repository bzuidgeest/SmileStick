

typedef struct
{
    char message[10];
    int8_t length;
} stickData_t;

queue_t call_queue;
//queue_t results_queue;


void coreUSBMain();