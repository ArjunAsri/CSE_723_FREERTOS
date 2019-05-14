/*The University of Auckland
 * LCFR System - LCFR system is able to interface with the power network
 * and switch off or on loads based on frequency and rate of change of frequencty
 * thresholds
 *
 * Author: Arjun Kumar
 * Date : 19th April 2018
 * Email : akmu999@aucklanduni.ac.nz
 *
 * */
#include <stdio.h>
#include <stdbool.h>
#include "sys/alt_alarm.h"
#include "system.h"
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
#include "VGA.h"
#include "FreeRTOS/timers.h"
#include "convertToBinary.h"
#include "keyboardISR.h"


#define mainREG_TEST_1_PARAMETER    ( ( void * ) 0x12345678 )
#define mainREG_TEST_2_PARAMETER    ( ( void * ) 0x87654321 )
#define mainREG_TEST_PRIORITY       ( tskIDLE_PRIORITY + 1)
#define TASK_STACKSIZE       2048


//----------------FreeRTOS Task Priorities--------------------//
#define GETSEM_TASK1_PRIORITY             13
#define GET_PS2_ISR_Handler_PRIORITY      14
#define GET_BUTTON_ISR_HANDLER_PRIORITY   14
#define StartInterrupt_Task_PRIORITY 10
#define taskSwitchesPolling_PRIORITY 14
#define taskMonitoring_PRIORITY		(tskIDLE_PRIORITY)+5
#define taskPrint_PRIORITY 				9
#define Timer_Reset_Task_P      (tskIDLE_PRIORITY+1)

//----------------FreeRTOS Binary Semaphores--------------------//
SemaphoreHandle_t signal_from_button_isr;
SemaphoreHandle_t signal_from_Timer_ISR;
SemaphoreHandle_t signal_shed_next_load;
SemaphoreHandle_t signal_500_timer;
SemaphoreHandle_t newFrequencyInput;
SemaphoreHandle_t newROCInput;
SemaphoreHandle_t calculateTimeDeviation;

//----------------FreeRTOS Mutex Semaphores--------------------//
xSemaphoreHandle mutex_LED_values = 0;
xSemaphoreHandle mutex_ThresHold_Values = 0;


//----------------FreeRTOS Queues--------------------//
xQueueHandle Queue_Task_Monitoring = 0;
xQueueHandle Q_freq_data_Monitor_Task =0;
xQueueHandle Q_number_of_samples = 0;
xQueueHandle Q_New_inputed_Frequency = 0; //new ferquency
xQueueHandle Q_load_shedding_time = 0;
xQueueHandle Q_new_Roc = 0;

//----------------TIMERS--------------------//
TimerHandle_t timer;
TimerHandle_t screenRefreshRate;
TimerHandle_t  nextLoadShedTimer;


//----------------TASKS--------------------//
void Button_Interrupt_ISR_Handler(void *pvParameters);
void PRVGADraw_Task(void *pvParameters );
void taskMonitoring(void *pvParameters);
void vTimerCallback(xTimerHandle t_timer);
void vTimerCallbackSignalShedLoad(xTimerHandle t_timer);
void taskSwitchesPolling(void *pvParameters);
void button_interrupt(void* context, alt_u32 id);

//-----------------Global Variables and Flags---------------------------//
int arrayLoadShedTimerValue[5] = {0};
int a = 0; //iterator for the array above

static int previousState = 0;

static bool loadShed = false;
static bool toggleTimer = false;
bool loadShedOccured = false;
bool loadsManaged = false;


int numberOfLoadsOn = 0;
static int uiSwitchValue = 0;
static int PreviousSwitchValue = 0;
int greenLED = 0x00;

int counter = 0;

TickType_t initialTime = 0;
TickType_t finalTime = 0;


int sheddedLoadCount = 0;

static int sheddedLoads[8]={0};
double previousFrequency = 0;
float ThresFreq = 50.0;
float ThresROC = 10.0;//threshold roc
static int stateChangeCounter = 0;
static bool newValue = false;
static bool clearedToReSwitchLoads = false;


//---------------------------------------------------------------------------//


/*Funciton taskMonitoring : This function is used to monitor the status
 * of the relay. The function calculates the frequency and the Threshold
 * and based on that load Shedding occurs.
 *  The function is created using the FreeRTOS API xCreateTask()
 * Priority = 6
 * INPUTS = void
 * Output = void*/
void taskMonitoring(void *pvParameters){
	ThresFreq = 50.0;
	ThresROC = 10.0;
	while(1){
		if(xSemaphoreTake(mutex_LED_values,0)){
			double frequency,samples = 0;
			xQueueReceive(Q_number_of_samples,&samples,0);
			xQueueReceive(Q_freq_data_Monitor_Task,&frequency,0); //wait for 1 tick
			//new Frequency Threshold
			if(xSemaphoreTake(newFrequencyInput,0)){
				xQueueReceive(Q_New_inputed_Frequency,&ThresFreq,0);
			}
			//new ROC Threshold
			if(xSemaphoreTake(newROCInput,0)){
				xQueueReceive(Q_new_Roc,&ThresROC,0);
				printf("%f\n",ThresROC);
			}

			//Critical Section to protect the data from being corrupted
			taskENTER_CRITICAL();
				float ROC_frequency = ((frequency -previousFrequency)*16000)/samples;
			taskEXIT_CRITICAL();

			previousFrequency = frequency;

			if(ROC_frequency <0){
				ROC_frequency= ROC_frequency*-1;
				printf("Current ROC: %f\n", ROC_frequency);
			}


			//First decide the system state
			if((frequency < ThresFreq) || (ROC_frequency > ThresROC)){
				if(previousState == stable){
					initialTime = xTaskGetTickCount();
					int i = 0;
					for(i = 0; i < 8 ; i++){
						if(Loads[i]){
					//printf("Loads ON INDEX %d\n",i);
							numberOfLoadsOn++;//counting the number of loads that were connected at the time the system was disconnected
					//printf("number of loads %d\n", numberOfLoadsOn);
						}
					}
					sheddedLoadCount = 0;
					loadShed = false;
					if(ROC_frequency > ThresROC){
						systemstate = High_ROC;
					}else{
						systemstate = underFrequency;
					}
					previousState = underFrequency;
					xTimerReset(nextLoadShedTimer,0);
					stateChangeCounter = 2;
					clearedToReSwitchLoads = false;

				}
				loadsManaged = true; //once anomaly detected loads managed go to
			}else{
				if((previousState ==underFrequency)||(previousState== High_ROC)){
					xTimerReset(nextLoadShedTimer,0);
					previousState= stable;

					systemstate = stable;
				}
				stateChangeCounter = 1;

			}




			if(!maintenanceModeOn){
				if(!(systemstate == stable)){//if system is not stable

					if(!loadShed){//if cleared to shed the next load after 500ms timer has checked the clearedToReSwtichLoads status
						int x = 0;
						for(x = 0; x < 8;x++){
							if((Loads[x])&&(!sheddedLoads[x])){//check the load Shed
								sheddedLoads[x]= true; //to remember which load has been shed
								loadShed = true;
								Loads[x] = false;
								PreviousSwitchValue &= ~(1 <<x);
								greenLED |= (1<<x);
								IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,(0x00FF&PreviousSwitchValue));//switch off the RED Led
								IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,greenLED); //Green LED On
								if(sheddedLoadCount ==0){
									sheddedLoadCount = 1;
									//finalTime = xTaskGetTickCount();
									//finalTime = finalTime - initialTime;
									finalTime++;
									xQueueSend(Q_load_shedding_time,&finalTime,0);//send the calculated first time shed value over
									//finalTime = 0;
									printf("%d\n",finalTime);

								}

								xTimerReset(nextLoadShedTimer,0); //system just loops back immediately
								break;
							}
						}
					}
				}else{
					if(clearedToReSwitchLoads){
						int checkTheSwitch = PreviousSwitchValue;
						if(numberOfLoadsOn>0){//if the loads are still on
							numberOfLoadsOn--;//decrease the counter regardless

							if((sheddedLoads[7])){//check if the load is on or off //becuase switch was off load was already off = thats why
								sheddedLoads[7]=false; // change the flag
								//printf("This is where error Happens\n");
								int checkValue = uiSwitchValue;//make sure that the actual value is not being edited
								checkValue &= (1 << 7);
								greenLED &= ~(1<<7); //switch the green LED off
								clearedToReSwitchLoads = false;
									if(checkValue == 128){
										Loads[7] = true;
										PreviousSwitchValue |= (1 << 7); //switch the LED on
										//IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,PreviousSwitchValue);
									//xTimerStart(nextLoadShedTimer,0); //start the 500ms timer immediately
									}

								xTimerReset(nextLoadShedTimer,0);
												//once all the loads have been reconnected enable the switches, check the code for polling switches
							}else if((sheddedLoads[6])){//check if the load is on or off //becuase switch was off load was already off = thats why
								sheddedLoads[6]=false; // change the flag
								int checkValue = uiSwitchValue;//make sure that the actual value is not being edited
								checkValue &= (1 << 6);
								greenLED &= ~(1<<6); //switch the green LED off
								clearedToReSwitchLoads = false;
									if(checkValue == 64){
										Loads[6] = true;
										PreviousSwitchValue |= (1 << 6); //switch the LED on
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[5])){
								sheddedLoads[5]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 5);
								greenLED &= ~(1<<5);
								clearedToReSwitchLoads = false;
									if(checkValue == 32){
										Loads[5] = true;
										PreviousSwitchValue |= (1 << 5);
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[4])){
								sheddedLoads[4]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 4);
								greenLED &= ~(1<<4);
								clearedToReSwitchLoads = false;
									if(checkValue == 16){
										Loads[4] = true;
										PreviousSwitchValue |= (1 << 4);
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[3])){
								sheddedLoads[3]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 3);
								greenLED &= ~(1<<3);
								clearedToReSwitchLoads = false;
									if(checkValue == 8){
										Loads[3] = true;
										PreviousSwitchValue |= (1 << 3);
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[2])){
								sheddedLoads[2]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 2);
								greenLED &= ~(1<<2);
								clearedToReSwitchLoads = false;
									if(checkValue == 4){
										Loads[2] = true;
										PreviousSwitchValue |= (1 << 2);
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[1])){
								sheddedLoads[1]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 1);
								greenLED &= ~(1<<1);
								clearedToReSwitchLoads = false;
									if(checkValue == 2){
										Loads[1] = true;
										PreviousSwitchValue |= (1 << 1);
									}
								xTimerReset(nextLoadShedTimer,0);

							}else if((sheddedLoads[0])){
								sheddedLoads[0]=false;
								int checkValue = uiSwitchValue;
								checkValue &= (1 << 0);
								greenLED &= ~(1<<0);
								clearedToReSwitchLoads = false;
								IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,greenLED);
									if(checkValue == 1){
										Loads[0] = true;
										PreviousSwitchValue |= (1 << 0);
									}
								xTimerReset(nextLoadShedTimer,0);//start the 500ms timer immediately
								}
							}
							IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,greenLED);
							IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,(0x00FF&PreviousSwitchValue));
						}

						if(numberOfLoadsOn ==0){//reset the sheddedLoadsOn
							loadsManaged = false;
							int i = 0;
							for(i = 0; i < 8; i++){
								sheddedLoads[i] = false;
							}
							newValue = false;
							sheddedLoadCount = 0;
					}
				}
			}

			if(sheddedLoads[0]){//first LoadShedHas Occured
				if(!newValue){
					newValue = true;
				}
			}
			xSemaphoreGive(mutex_LED_values);
		}
	vTaskDelay(20); //Delay to allow the VGA tasks to work
	}
}

/*Button Interrupt ISR Handler : This function handles the button interrupt by reading
 * the state of the button
 * Input = void
 * Output = void
 * Task Priority = 14 (Highest on the scheduler)*/

void Button_Interrupt_ISR_Handler(void *pvParameters){
	while(1){
		if(xSemaphoreTake(signal_from_button_isr, 5)){
			printf("Button ISR\n");
			 IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
			 if(!maintenanceModeOn){
				 if(!loadsManaged){
					 maintenanceModeOn = true;
				 }
			 	printf("maintenanceModeOn\n");
			 }else{
			 	maintenanceModeOn = false;
			 	printf("maintenanceModeOFF\n");
			 }
		}
	}
}

/*ISR ps2 Keyboard : This function is triggered when the interrupt from the key press on the
 * PS2 keyboard occurs.
 * Inputs -
 * Outputs - */
void ps2_isr (void* context, alt_u32 id)
{
	//Think of declaring variables as global, interrupt should not be long
	char ascii;
	int status = 0;
	unsigned char key = 0;
	KB_CODE_TYPE decode_mode;
	//Saving the context of the key immediately
	status = decode_scancode (context, &decode_mode , &key , &ascii);
	//Sending Data over xQueue
	xQueueSendFromISR(Global_Queue_Handle, &status, NULL);
	xQueueSendFromISR(Global_Queue_Handle, &ascii, NULL);
	xQueueSendFromISR(Global_Queue_Handle, &key, NULL);
	xQueueSendFromISR(Global_Queue_Handle, &decode_mode, NULL);

	xSemaphoreGiveFromISR(shared_resource_sem, NULL);

}

/*Timer Call Back Function for Polling Switches */
void vTimerCallback(xTimerHandle t_timer){ //Timer for polling
	xSemaphoreGiveFromISR(signal_from_Timer_ISR,2);  		//semaphore signal to the Polling Task
}

/*Timer Call Back Function for signalShedLoad */
void vTimerCallbackSignalShedLoad(xTimerHandle nextLoadShedTimer){

	if(stateChangeCounter == 1){
		loadShed = true; //DO NOT SHED THE next load, flag changed
		clearedToReSwitchLoads = true;
		//printf("Timer 2\n");
		stateChangeCounter = 0;
	}else{
		clearedToReSwitchLoads = false;
		loadShed = false; //reset the loadShed for the next highest priority loadShed to occur
	}
}

/*Task SwitchesPolling: This task does polling for the switches and is called periodiclly using the timer
 * The funciton has been created using the FreeRTOS xCreateTask() function.
 * Input = void
 * Output = void
 * Priority = */
void taskSwitchesPolling(void *pvParameters){
	while(1){
		if(xSemaphoreTake(signal_from_Timer_ISR,portMAX_DELAY)){//wait for the signal else do not proceed
			if(xSemaphoreTake(mutex_LED_values,0)){ //mutex, if the semaphore from the taskMonitoring which manages the thresholds has released it as the variables for switch
			xTimerStop(timer,0); //stop the timer process the information and then restart it
			uiSwitchValue = IORD_ALTERA_AVALON_PIO_DATA(SLIDE_SWITCH_BASE); //reading switches every time timer resets
			int ones, tens;
			if(((systemstate == stable)&&(numberOfLoadsOn==0))||(maintenanceModeOn)){
				ones = ((0x000F)&uiSwitchValue);	// write the value of the switches to the red LEDs	 	 	 //and the switches to display the correct leds
				tens = ((0x00F0)&uiSwitchValue);
			//put this in critical section
				convertToBinary(tens,ones,&Loads);
				IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, ((0x00FF)&uiSwitchValue));
				PreviousSwitchValue = uiSwitchValue; //update the previous
			}else{
				/*Switch Operation during the Load Shedding Period*/
				/*THIS WORKS*/
				int i = 0;
				for(i = 0; i < 8; i++){
					int temp1 = uiSwitchValue; //do not alter the actual global variables
					int checkValue_1 = (temp1 &= (1<<i)); //check the fourth LED // value = 4
					int temp = PreviousSwitchValue;
					int checkValue_2 = (temp &= (1<<i));//value = 4
					if(checkValue_1<checkValue_2){ //if a load is removed
						PreviousSwitchValue &= ~(1<<i); //clear the bit
						Loads[i] = false;
					}
				}
			    IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE, ((0x00FF)&PreviousSwitchValue)); //write only the previous switch values

			}
			if(maintenanceModeOn){ //If maintenance mode is on then clear all the green LEDs
				IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,0x00); //clear all the green LEDs in maintenanceModeOn
				loadsManaged = false;
			}
		}
		xTimerStart(timer,0);
		xSemaphoreGive(mutex_LED_values);
	}
	}

}

void freq_relay(){
	#define SAMPLING_FREQ 16000.0
	double numberOfSamples = IORD(FREQUENCY_ANALYSER_BASE, 0);
	double temp = SAMPLING_FREQ/numberOfSamples;
	xQueueSendFromISR(Q_number_of_samples,&numberOfSamples,pdFALSE);
	xQueueSendFromISR(Q_freq_data_Monitor_Task,&temp,pdFALSE);
	xQueueSendToBackFromISR( Q_freq_data, &temp, pdFALSE );
	return;
}

void button_interrupt(void* context, alt_u32 id)
{
	int* temp = (int*) context;
	(*temp) = IORD_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE);
	// clears the edge capture register
	IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7);
	xSemaphoreGiveFromISR(signal_from_button_isr,NULL);

}

int main()
{
  IOWR_ALTERA_AVALON_PIO_EDGE_CAP(PUSH_BUTTON_BASE, 0x7); // clears the edge capture register. Writing 1 to bit clears pending interrupt for corresponding button.
  IOWR_ALTERA_AVALON_PIO_IRQ_MASK(PUSH_BUTTON_BASE, 0x1); // enable interrupts for all buttons only using the right most button beofre the reset
  alt_irq_register(PUSH_BUTTON_IRQ,NULL, button_interrupt);// register the ISR

	/*PS2 ISR Declaration - START*/
	alt_up_ps2_dev * ps2_device = alt_up_ps2_open_dev(PS2_NAME);
	if(ps2_device == NULL){
		printf("can't find PS/2 device\n");
		return 1;
	}
	alt_up_ps2_clear_fifo (ps2_device) ;
	alt_irq_register(PS2_IRQ, ps2_device, ps2_isr);
	IOWR_8DIRECT(PS2_BASE,4,1);// register the PS/2 interrupt

	/*PS2 ISR Declaration - END*/


	/*Frequency Analyser ISR Declaration - START*/
	alt_irq_register(FREQUENCY_ANALYSER_IRQ, 0, freq_relay);
	/**Frequency Analyser ISR Declaration - END*/



	/*CREATING BINARY SEMAPHORES*/
  	shared_resource_sem = xSemaphoreCreateBinary();
  	signal_from_button_isr = xSemaphoreCreateBinary();
  	signal_from_Timer_ISR = xSemaphoreCreateBinary();
  	signal_refresh_screen = xSemaphoreCreateBinary();
  	signal_shed_next_load = xSemaphoreCreateBinary();
  	signal_500_timer = xSemaphoreCreateBinary();
  	 newFrequencyInput = xSemaphoreCreateBinary();
  	 newROCInput = xSemaphoreCreateBinary();;

  	/*CREATING QUEUES*/
  	Global_Queue_Handle = xQueueCreate(4, sizeof(int));
  	Q_freq_data = xQueueCreate(100, sizeof(double));
  	Q_freq_data_Monitor_Task = xQueueCreate(100,sizeof(double)); //second queue to handle the data from the system
  	Queue_Task_Monitoring = xQueueCreate(1,sizeof(double));
  	Q_number_of_samples = xQueueCreate(1,sizeof(double));
  	Q_New_Frequency = xQueueCreate(7, sizeof(int)); //Example enterd value "100.00 enter key"
  	Q_new_Roc = xQueueCreate(1, sizeof(int)); //new ROC
  	Q_New_inputed_Frequency = xQueueCreate(1,sizeof(float));
  	Q_load_shedding_time = xQueueCreate(1,sizeof(TickType_t));


  	/*MUTEX*/
  	mutex_LED_values = xSemaphoreCreateMutex();
  	mutex_ThresHold_Values = xSemaphoreCreateMutex();

  	//RESETTING LEDS
  	IOWR_ALTERA_AVALON_PIO_DATA(GREEN_LEDS_BASE,greenLED);
  	IOWR_ALTERA_AVALON_PIO_DATA(RED_LEDS_BASE,0x00);



  	//TASKS
  	  /*Monitoring and VGA Task*/
	xTaskCreate(taskMonitoring, "Monitor Loads", TASK_STACKSIZE, NULL, taskMonitoring_PRIORITY, NULL);
	xTaskCreate( PRVGADraw_Task, "VGA Display", configMINIMAL_STACK_SIZE, NULL, PRVGADraw_Task_P, &PRVGADraw );

	/*Keyboard and Button ISR Handlers*/
	xTaskCreate(Button_Interrupt_ISR_Handler,"Button ISR Handler", TASK_STACKSIZE, NULL, GET_BUTTON_ISR_HANDLER_PRIORITY, NULL);
	xTaskCreate(ps2_ISR_Handler		,"PS2 ISR Handler",TASK_STACKSIZE, NULL,GET_PS2_ISR_Handler_PRIORITY, NULL);
	xTaskCreate(taskSwitchesPolling, "Polling Switches",TASK_STACKSIZE, NULL,taskSwitchesPolling_PRIORITY ,NULL);

	xTaskCreate(vTimerCallback,"timer switches polling",TASK_STACKSIZE, NULL, 2, NULL);
	/*TIMERS*/
	timer = xTimerCreate("Timer for Switches", 100, pdTRUE, NULL, vTimerCallback); //timers callback function is executed when timer has finished.
	nextLoadShedTimer = xTimerCreate("Load Shed Timer",500,pdTRUE, NULL, vTimerCallbackSignalShedLoad );

	/*Starting Timers*/
	xTimerStart(nextLoadShedTimer, 0); //start the loadShedTimer
	if (xTimerStart(timer, 0) != pdPASS){ //start the Polling Timer
		printf("Cannot start timer");
	}
  	/* Start the scheduler */
	vTaskStartScheduler();

  while(1){} //Keep the program alive
  return 0;
}
