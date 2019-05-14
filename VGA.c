#include <stdio.h>
#include <stdlib.h>
#include "sys/alt_irq.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "stdbool.h"
#include "FreeRTOS/FreeRTOS.h"
#include "FreeRTOS/task.h"
#include "FreeRTOS/queue.h"
#include "freertos/semphr.h"
#include "VGA.h"
#include "convertToBinary.h"

//-------Global Queues --------//
QueueHandle_t Q_freq_data;
QueueHandle_t Q_New_inputed_Frequency;
QueueHandle_t Q_load_shedding_time;
QueueHandle_t Q_new_Roc;
/****** VGA display ******/

SemaphoreHandle_t signal_refresh_screen;
SemaphoreHandle_t newFrequencyInput;
SemaphoreHandle_t newROCInput;
systemState systemstate = stable;

TickType_t loadSheddingTime[5] = {0};


xSemaphoreHandle mutex_ThresHold_Values;
//Global Variables from other files Start
//extern Loads activeLoads;
extern int isInputFrequency; //global Variable
extern float ThresFreq, ThresROC;
extern int Loads[8];
extern bool frequencyReadyToBeSend;
extern char inputBuffer[8];
extern int indexCount;
extern bool loadsManaged;
float minValue, maxValue = 0.0;
float convertedNumber = 0.0;//global
//Gloabl Variables form other files End
Line line_freq, line_roc;
systemState systemstate;
double freq[100], dfreq[100];
int i = 99, j = 0;

bool maintenanceModeOn = false;
void PRVGADraw_Task(void *pvParameters ){


	char frequencyBuffer[10]={0};
	char frequencyDataFromQueue[2]={0};
	//initialize VGA controllers
	alt_up_pixel_buffer_dma_dev *pixel_buf;
	pixel_buf = alt_up_pixel_buffer_dma_open_dev(VIDEO_PIXEL_BUFFER_DMA_NAME);
	if(pixel_buf == NULL){
		printf("can't find pixel buffer device\n");
	}
	alt_up_pixel_buffer_dma_clear_screen(pixel_buf, 0);

	alt_up_char_buffer_dev *char_buf;
	char_buf = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma");
	if(char_buf == NULL){
		printf("can't find char buffer device\n");
	}
	alt_up_char_buffer_clear(char_buf);



	//Set up plot axes   /*Setting up colour*/
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 590, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 50, 200, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);
	alt_up_pixel_buffer_dma_draw_vline(pixel_buf, 100, 220, 300, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);


	alt_up_char_buffer_string(char_buf, "System Active Time:", 50, 1);
	alt_up_char_buffer_string(char_buf, "Threshold Frequency(Hz):", 2, 40); //x,y pixel positions
	alt_up_char_buffer_string(char_buf, "Threshold ROC (Hz/s):", 45, 40); //x,y pixel positions
	alt_up_char_buffer_string(char_buf, "Loads Currently ON:", 2, 42);
	alt_up_char_buffer_string(char_buf, "Maintenance State Status:", 2, 44);
	alt_up_char_buffer_string(char_buf, "5 Immediate Readings:", 2, 46);
	alt_up_char_buffer_string(char_buf, "Max Time:", 50, 42);
	alt_up_char_buffer_string(char_buf, "Min Time:", 50, 44);
	alt_up_char_buffer_string(char_buf, "Average Time:", 50, 48);
	alt_up_char_buffer_string(char_buf, "System State:", 2, 48);
	alt_up_char_buffer_string(char_buf, "50", 30, 40); //set the frequency as 50 initially

	int InitialValues[4] = {0};

	sprintf(InitialValues,"%f", ThresFreq);
	alt_up_char_buffer_string(char_buf,InitialValues, 30, 40);

	sprintf(InitialValues,"%f", ThresROC);
	alt_up_char_buffer_string(char_buf,InitialValues, 66, 40);


	alt_up_pixel_buffer_dma_draw_hline(pixel_buf, 100, 1, 20, ((0x3ff << 20) + (0x3ff << 10) + (0x3ff)), 0);


	alt_up_char_buffer_string(char_buf, "Frequency(Hz)", 4, 4);
	alt_up_char_buffer_string(char_buf, "52", 10, 7); //x,y coordinates in the code
	alt_up_char_buffer_string(char_buf, "50", 10, 12);
	alt_up_char_buffer_string(char_buf, "48", 10, 17);
	alt_up_char_buffer_string(char_buf, "46", 10, 22);

	alt_up_char_buffer_string(char_buf, "df/dt(Hz/s)", 4, 26);
	alt_up_char_buffer_string(char_buf, "60", 10, 28);
	alt_up_char_buffer_string(char_buf, "30", 10, 30);
	alt_up_char_buffer_string(char_buf, "0", 10, 32);
	alt_up_char_buffer_string(char_buf, "-30", 9, 34);
	alt_up_char_buffer_string(char_buf, "-60", 9, 36);




	while(1){
		//if(xSemaphoreTake(signal_refresh_screen,portMAX_DELAY)){

		//char Feq_ROC_Buffer[5];
		//sprintf(Feq_ROC_Buffer, " %f", );
		if(loadsManaged && (!maintenanceModeOn)){
			alt_up_char_buffer_string(char_buf, "Managing Loads", 17, 50);
		}else{
			alt_up_char_buffer_string(char_buf, "              ", 17, 50);
		}
		if(maintenanceModeOn){
			alt_up_char_buffer_string(char_buf, "ON ", 28, 44);
		}else{
			alt_up_char_buffer_string(char_buf, "OFF", 28, 44);
		}

		 if(systemstate == stable){
			 alt_up_char_buffer_string(char_buf, "Stable          ", 17, 48);
		 }else if(systemstate == underFrequency){
			 alt_up_char_buffer_string(char_buf, "Under Frequency", 17, 48);
		 }else if(systemstate == High_ROC){
			 alt_up_char_buffer_string(char_buf, "ROC            ", 17, 48);
		 }


		 TickType_t temp = 0;


		 int TimeValueBuffer[15] ={0};
		 if(xQueueReceive(Q_load_shedding_time,&temp,0)){
			 loadSheddingTime[4] = loadSheddingTime[3];
			 loadSheddingTime[3] = loadSheddingTime[2];
			 loadSheddingTime[2] = loadSheddingTime[1];
			 loadSheddingTime[1] = loadSheddingTime[0];
			 loadSheddingTime[0] = temp;
			 sprintf(TimeValueBuffer,"%d ms, %d ms, %d ms, %d ms, %d ms",loadSheddingTime[0],loadSheddingTime[1],loadSheddingTime[2],loadSheddingTime[3],loadSheddingTime[4]);
			 alt_up_char_buffer_string(char_buf, TimeValueBuffer, 25, 46);

			 float averageTime = (loadSheddingTime[0]+loadSheddingTime[1]+loadSheddingTime[2]+loadSheddingTime[3]+loadSheddingTime[4])*0.2;//divided by 5
			 sprintf(TimeValueBuffer,"%f ",averageTime);
			 alt_up_char_buffer_string(char_buf, TimeValueBuffer, 65, 48);

			 if(temp < minValue){
				 minValue = temp;
			 }

			 if (temp > maxValue ){
				maxValue = temp;
			 }
			 sprintf(TimeValueBuffer,"%f ",minValue);
			 alt_up_char_buffer_string(char_buf, TimeValueBuffer, 60, 44); //Min Value display coordinates
			 sprintf(TimeValueBuffer,"%f ",maxValue);
			 alt_up_char_buffer_string(char_buf, TimeValueBuffer, 60, 42); //Max Value display coordinates
		 }


		 if(frequencyReadyToBeSend){
				if(inputBuffer[2] == 46){
				convertedNumber = ((inputBuffer[0]-48)*10) + ((inputBuffer[1]-48)*1)+ ((inputBuffer[3]-48)*0.1);
			 }
				 if(isInputFrequency){
				 xQueueSend(Q_New_inputed_Frequency, &convertedNumber, NULL);//send the new frequency
				 xSemaphoreGiveFromISR(newFrequencyInput,NULL);
				 }else{
					 xQueueSend(Q_new_Roc, &convertedNumber, NULL);//send the new frequency
					 xSemaphoreGiveFromISR(newROCInput,NULL);
				 }

			 if((convertedNumber>0 && convertedNumber <100)){ //just to ensure the correct number has been entered
				//sprintf( frequencyBuffer,"%c",frequencyDataFromQueue ); //using xQueue

				 if(isInputFrequency){
				sprintf(frequencyBuffer,"%c", inputBuffer[0]);
				alt_up_char_buffer_string(char_buf,frequencyBuffer, 30, 40);
				//printf("xQueue received %d \n",frequencyBuffer[0]);
				sprintf(frequencyBuffer,"%c", inputBuffer[1]);
				alt_up_char_buffer_string(char_buf,frequencyBuffer, 31, 40);
				//printf("xQueue received %d \n",frequencyBuffer[0]);
				sprintf(frequencyBuffer,"%c", inputBuffer[2]);
				alt_up_char_buffer_string(char_buf,frequencyBuffer, 32, 40);
				sprintf(frequencyBuffer,"%c", inputBuffer[3]);
				alt_up_char_buffer_string(char_buf,frequencyBuffer, 33, 40);
				 } else{
					sprintf(frequencyBuffer,"%c", inputBuffer[0]);
					alt_up_char_buffer_string(char_buf,frequencyBuffer, 66, 40);
					sprintf(frequencyBuffer,"%c", inputBuffer[1]);
					alt_up_char_buffer_string(char_buf,frequencyBuffer, 67, 40);
					sprintf(frequencyBuffer,"%c", inputBuffer[2]);
					alt_up_char_buffer_string(char_buf,frequencyBuffer, 68, 40);
					sprintf(frequencyBuffer,"%c", inputBuffer[3]);
					alt_up_char_buffer_string(char_buf,frequencyBuffer, 69, 40);
				 }
				indexCount = 0;
			}
		 }
		 //Display the first Load if index 0 is true on the Loads array
		 if(Loads[0]==true){
				alt_up_char_buffer_string(char_buf, "L1", 22, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 22, 42);
		 }
		 //Display the Load 2 if index 1 is true
		 if(Loads[1]==true){
				alt_up_char_buffer_string(char_buf, "L2", 25, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 25, 42);
		 }
		 //Display the Load 3 if index 2 is true
		 if(Loads[2]==true){
				alt_up_char_buffer_string(char_buf, "L3", 28, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 28, 42);
		 }
		 //Display the Load 4 if index 3 is true
		 if(Loads[3]==true){
				alt_up_char_buffer_string(char_buf, "L4", 31, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 31, 42);
		 }
		 //Display the Load 5 if index 4 is true
		 if(Loads[4]==true){
				alt_up_char_buffer_string(char_buf, "L5", 34, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 34, 42);
		 }
		 //Display the Load 6 if index 5 is true
		 if(Loads[5]==true){
				alt_up_char_buffer_string(char_buf, "L6", 37, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 37, 42);
		 }
		 //Display the Load 7 if index 6 is true
		 if(Loads[6]==true){
				alt_up_char_buffer_string(char_buf, "L7", 40, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 40, 42);
		 }
		 //Display the Load 8 if index 7 is true
		 if(Loads[7]==true){
				alt_up_char_buffer_string(char_buf, "L8", 43, 42);
		 }else{
			 alt_up_char_buffer_string(char_buf, "  ", 43, 42);
		 }

		//receive frequency data from queue
		while(uxQueueMessagesWaiting( Q_freq_data ) != 0){
			xQueueReceive( Q_freq_data, freq+i, 0 );
			//calculate frequency RoC
			if(i==0){
				dfreq[0] = (freq[0]-freq[99]) * 2.0 * freq[0] * freq[99] / (freq[0]+freq[99]);
			}
			else{
				dfreq[i] = (freq[i]-freq[i-1]) * 2.0 * freq[i]* freq[i-1] / (freq[i]+freq[i-1]);
			}
			if (dfreq[i] > 100.0){
				dfreq[i] = 100.0;
			}

			i =	++i%100; //point to the next data (oldest) to be overwritten

		}
		int time = xTaskGetTickCount()/1000;

		/*int remainder = time %60;
		if(remainder == 0){
			time = time/60;
		}*/

		char TimeBuff[4] = {0};
		sprintf(TimeBuff,"%d s",time);
		alt_up_char_buffer_string(char_buf, TimeBuff, 72, 1);

		//clear old graph to draw new graph
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 0, 639, 199, 0, 0);
		alt_up_pixel_buffer_dma_draw_box(pixel_buf, 101, 201, 639, 299, 0, 0);

			for(j=0;j<99;++j){ //i here points to the oldest data, j loops through all the data to be drawn on VGA
				if (((int)(freq[(i+j)%100]) > MIN_FREQ) && ((int)(freq[(i+j+1)%100]) > MIN_FREQ)){
					//Calculate coordinates of the two data points to draw a line in between
					//Frequency plot
					line_freq.x1 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * j;
					line_freq.y1 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j)%100] - MIN_FREQ));

					line_freq.x2 = FREQPLT_ORI_X + FREQPLT_GRID_SIZE_X * (j + 1);
					line_freq.y2 = (int)(FREQPLT_ORI_Y - FREQPLT_FREQ_RES * (freq[(i+j+1)%100] - MIN_FREQ));

					//Frequency RoC plot
					line_roc.x1 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * j;
					line_roc.y1 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j)%100]);

					line_roc.x2 = ROCPLT_ORI_X + ROCPLT_GRID_SIZE_X * (j + 1);
					line_roc.y2 = (int)(ROCPLT_ORI_Y - ROCPLT_ROC_RES * dfreq[(i+j+1)%100]);

					//Draw
					alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_freq.x1, line_freq.y1, line_freq.x2, line_freq.y2, 0x3ff << 0, 0);
					alt_up_pixel_buffer_dma_draw_line(pixel_buf, line_roc.x1, line_roc.y1, line_roc.x2, line_roc.y2, 0x3ff << 0, 0);
				}
			}
		}
}
