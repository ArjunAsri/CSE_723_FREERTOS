
#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "keyboardISR.h"

xQueueHandle Global_Queue_Handle = 0;
xQueueHandle Q_New_Frequency =0;
 char inputBuffer[8] = {0};
 bool frequencyReadyToBeSend = false;
/*FLAGS Start*/
 bool flag = false; //static keeps it the same throughout the files
int indexCount = 0;
int isInputFrequency = true;

void ps2_ISR_Handler(void *pvParameters){
	while(1){
		if(xSemaphoreTake(shared_resource_sem,portMAX_DELAY)){
			//needed variables
			//key, ascii, status
			//status is received using the decode_scancode
			//decode_Scancode provides the required information
		char ascii;
		int status = 0;
		unsigned char key = 0;
		KB_CODE_TYPE decode_mode;
		xQueueReceive(Global_Queue_Handle,&status,NULL);
		xQueueReceive(Global_Queue_Handle,&ascii,NULL);
		xQueueReceive(Global_Queue_Handle,&key,NULL);
		xQueueReceive(Global_Queue_Handle,&decode_mode,NULL);

		xQueueReset( Global_Queue_Handle );
		if(status == 0 ){
			if (!flag) //success
			{
				flag = true;
				// print out the result
					switch ( decode_mode )
						{
						case KB_ASCII_MAKE_CODE :
							frequencyReadyToBeSend = false; //reset the flag so the new values can be entered
							printf ( "ASCII   : %c \n", ascii ) ;
							xQueueSend(Q_New_Frequency,&ascii,NULL);
							if(ascii == 42){ // 42 == *
								isInputFrequency = false;
							}else if(ascii == 43){//43 == +
								isInputFrequency = true;
							}
							if(((ascii>47) && (ascii <58))||(ascii == 46)){ //keep the numbers and the decimal
								if(!(ascii == 46 && indexCount == 0)){ //ignore if the first part is decimal
								inputBuffer[indexCount] = ascii;
								indexCount++;
								frequencyReadyToBeSend = false;
								}
							}
							break ;
						case KB_LONG_BINARY_MAKE_CODE :
						// do nothing
						case KB_BINARY_MAKE_CODE :
							if(ascii = 13){
								printf("Enter Key Pressed\n");
								indexCount = 0;
									frequencyReadyToBeSend = true;
							}else{
								printf ( "MAKE CODE : %x\n", key ) ;
							}
							break ;
						case KB_BREAK_CODE :
						// do nothing
						default :
							printf ( "DEFAULT   : %x\n", key ) ;
							break ;
					}
				//IOWR(SEVEN_SEG_BASE,0 ,key);
				}
			else {
				flag = false;
			}
			}
			}
	}

}
