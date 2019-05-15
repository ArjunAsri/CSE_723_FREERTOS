Compsys723 Assignment 1 FreeRTOS

Author : Arjun Kumar

If the Leds on the system are not responding then please reprogram the board. 
Setting Up the Project
-Connect the VGA to the DE2-115 board, connect the PS2 keyboard, connect the serial usb cable to the board to program it,
-Connect the power cable to the board

Open Nios 13.0 IDE
- Create a new empty project
- Import the test and BSP test files into the project
- Build the project
- Go to run conifgurations on the task bar, click select refresh connections and then run the application.
- Switch to the VGA output on the screen and then run it. 

Operating the relay 
- To input a new frequency and new ROC Threshold please press the "+" button first and then type in the numbers in the following manner
Example - 
	1.5
	1) Enter + , then 0, then 1, then ".", then 5 . At the end of the input press enter this will update the system
	Frequency 
	IT IS IMPORTANT TO TYPE IN THE REQUIRED FREQUENCY AND ROC THRESHOLD WHEN THE SYSTEM IS ACTIVATED FOR THE FIRST TIME

	15.0
	2) Enter +, then 1, then 5, then ".", then 0. At the end of the input press enter.

The Maintenance mode can be acitvated only when the load shedding has been completed. The maintenance mode when activated
in the stable mode and can only be activated in the stable mode will remain active regardless the read frequency and ROC values

In the maintenance mode any of the Loads can be activated or disconnected. 
User is not allowed to activate a load while the system is managing Load.
