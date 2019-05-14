#include <stdio.h>
#include <stdbool.h>
#include "sys/alt_alarm.h"
#include "system.h"
#include "io.h"
#include "altera_up_avalon_ps2.h"
#include "altera_up_ps2_keyboard.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "linkedList.h"



/*Helper Functions*/
typedef struct{
int L1,L2,L3,L4,L5,L6,L7,L8;
}priorityList;

//void helperTaskShedLoad(Loads *activeLoads, linkedlist *listOfShedLoads);

//The funciton expects a pointer to the priorityList
//void helperTaskDefineLoadPriority(priorityList *List, Loads *activeLoads);

