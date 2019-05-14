#include <stdio.h>
#include <stdbool.h>
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_ps2.h"
#include "altera_up_ps2_keyboard.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"

/*typedef struct{
	bool L1;
	bool L2;
	bool L3;
	bool L4;
	bool L5;
	bool L6;
	bool L7;
	bool L8;
}Loads;*/


int Loads[8];
//extern Loads activeLoads;
typedef struct{
	bool shedL1,shedL2,shedL3,shedL4,shedL5,shedL6,shedL7,shedL8;

}shedLoads;

void convertToBinary(int tens, int ones, int *ActiveLoads);


//Loads getLoadsValue(int *load);
//Loads setLoadsValue(int *load);
