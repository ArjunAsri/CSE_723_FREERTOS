#include "convertToBinary.h"

//Loads activeLoads;
int Loads[8] = {0};
void convertToBinary(int tens, int ones, int *ActiveLoads){


	switch(ones){

	case 0:
			Loads[0] = false;
			Loads[1] = false;
			Loads[2] = false;
			Loads[3] = false;

			break;
	case 1:
		Loads[0] = true;
		Loads[1] = false;
		Loads[2] = false;
		Loads[3] = false;
			break;
	case 0x0002:
		Loads[0] = false;
		Loads[1] = true;
		Loads[2]= false;
		Loads[3] = false;
			break;
	case 0x0003:
					Loads[0] = true;
					Loads[1] = true;
					Loads[2] = false;
					Loads[3] = false;
			break;
	case 0x0004:
					Loads[0] = false;
					Loads[1] = false;
					Loads[2] = true;
					Loads[3] = false;
			break;
	case 5:
					Loads[0] = true;
					Loads[1] = false;
					Loads[2] = true;
					Loads[3] = false;
			break;
	case (  0x0006):
					Loads[0] = false;
					Loads[1] = true;
					Loads[2] = true;
					Loads[3] = false;
			break;
	case (  0x0007):
					Loads[0] = true;
					Loads[1] = true;
					Loads[2] = true;
					Loads[3] = false;
			break;
	case (  0x0008):
					Loads[0] = false;
					Loads[1] = false;
					Loads[2] = false;
					Loads[3] = true;
			break;
	case (  0x0009):
					Loads[0] = true;
					Loads[1] = false;
					Loads[2] = false;
					Loads[3] = true;
			break;
	case (  0x000A):
					Loads[0] = false;
					Loads[1] = true;
					Loads[2] = false;
					Loads[3] = true;
			break;
	case (  0x000B):
					Loads[0] = true;
					Loads[1] = true;
					Loads[2] = false;
					Loads[3] = true;
			break;
	case (  0x000C):
					Loads[0] = false;
					Loads[1] = false;
					Loads[2] = true;
					Loads[3] = true;
			break;
	case (  0x000D):
					Loads[0] = true;
					Loads[1] = false;
					Loads[2] = true;
					Loads[3] = true;
			break;
	case (  0x000E):
					Loads[0] = false;
					Loads[1] = true;
					Loads[2] = true;
					Loads[3] = true;
			break;
	case (  0x000F):
					Loads[0] = true;
					Loads[1] = true;
					Loads[2] = true;
					Loads[3] = true;
			break;
	};
			switch(tens){
			case (  0x0000):
							Loads[4] = false;
							Loads[5] = false;
							Loads[6] = false;
							Loads[7] = false;
							break;
		case (  0x0010):
						Loads[4] = true;
						Loads[5] = false;
						Loads[6] = false;
						Loads[7] = false;
						break;
		case (  0x0020):
						Loads[4] = false;
						Loads[5] = true;
						Loads[6] = false;
						Loads[7] = false;
				break;
		case (  0x0030):
						Loads[4] = true;
						Loads[5] = true;
						Loads[6] = false;
						Loads[7] = false;
				break;
		case (  0x0040):
						Loads[4] = false;
						Loads[5] = false;
						Loads[6] = true;
						Loads[7] = false;
				break;
		case (  0x0050):
						Loads[4] = true;
						Loads[5] = false;
						Loads[6] = true;
						Loads[7] = false;
				break;
		case (  0x0060):
						Loads[4] = false;
						Loads[5] = true;
						Loads[6] = true;
						Loads[7] = false;
				break;
		case (  0x0070):
						Loads[4] = true;
						Loads[5] = true;
						Loads[6] = true;
						Loads[7] = false;
				break;
		case (  0x0080):
						Loads[4] = false;
						Loads[5] = false;
						Loads[6] = false;
						Loads[7] = true;
				break;
		case (  0x0090):
						Loads[4] = true;
						Loads[5] = false;
						Loads[6] = false;
						Loads[7] = true;
				break;
		case (  0x00A0):
						Loads[4] = false;
						Loads[5] = true;
						Loads[6] = false;
						Loads[7] = true;
				break;
		case (  0x00B0):
						Loads[4] = true;
						Loads[5] = true;
						Loads[6] = false;
						Loads[7] = true;
				break;
		case (  0x00C0):
						Loads[4] = false;
						Loads[5] = false;
						Loads[6] = true;
						Loads[7] = true;
				break;
		case (  0x00D0):
						Loads[4] = true;
						Loads[5] = false;
						Loads[6] = true;
						Loads[7] = true;
				break;
		case (0x00E0):
						Loads[4] = false;
						Loads[5] = true;
						Loads[6] = true;
						Loads[7] = true;
				break;
		case (0x00F0):
						Loads[4] = true;
						Loads[5] = true;
						Loads[6] = true;
						Loads[7] = true;

				break;

	};

}
