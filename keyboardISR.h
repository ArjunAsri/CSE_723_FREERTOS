#include <stdio.h>
#include <stdbool.h>
#include "sys/alt_alarm.h"
#include "io.h"
#include "altera_up_avalon_ps2.h"
#include "altera_up_ps2_keyboard.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "alt_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"



//------------Including semaphores and queue declarations  ---------/
SemaphoreHandle_t shared_resource_sem;
extern xQueueHandle Global_Queue_Handle;
extern xQueueHandle Q_New_Frequency;

//Global Variables
extern  char inputBuffer[8];
extern  bool frequencyReadyToBeSend;
extern  bool flag; //static keeps it the same throughout the files
extern int indexCount;

//--------------PS2 ISR HANDLER FUNCTION --------------/
void ps2_ISR_Handler(void *pvParameters);
