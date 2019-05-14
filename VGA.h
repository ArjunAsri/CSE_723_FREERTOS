#include <stdio.h>
#include <stdlib.h>
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t signal_refresh_screen;

//For frequency plot
#define FREQPLT_ORI_X 101		//x axis pixel position at the plot origin
#define FREQPLT_GRID_SIZE_X 5	//pixel separation in the x axis between two data points
#define FREQPLT_ORI_Y 199.0		//y axis pixel position at the plot origin
#define FREQPLT_FREQ_RES 20.0	//number of pixels per Hz (y axis scale)

#define ROCPLT_ORI_X 101
#define ROCPLT_GRID_SIZE_X 5
#define ROCPLT_ORI_Y 259.0
#define ROCPLT_ROC_RES 0.5		//number of pixels per Hz/s (y axis scale)

#define MIN_FREQ 45.0 //minimum frequency to draw

#define PRVGADraw_Task_P    (tskIDLE_PRIORITY)+3
TaskHandle_t PRVGADraw;


extern QueueHandle_t Q_freq_data;
typedef enum {stable, underFrequency, High_ROC} systemState;

typedef struct{
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
}Line;
extern systemState systemstate;

extern Line line_freq, line_roc;


extern double freq[100], dfreq[100];
extern int i, j;

extern bool maintenanceModeOn;
/****** VGA display ******/

void PRVGADraw_Task(void *pvParameters );

//void HelperTaskDraw(Loads *activeLoads, bool maintenanceModeOn, bool frequencyReadyToBeSend);

void freq_relay();
