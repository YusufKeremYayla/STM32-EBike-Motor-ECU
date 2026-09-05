/*
 * io_drive.h
 *
 *  Created on: Jan 28, 2026
 *      Author: yusuf
 */
#ifndef INC_IO_DRIVER_H_
#define INC_IO_DRIVER_H_
#include "main.h"
#define DEBOUNCE_TIME		100
typedef enum
{
	 Input_Status_Low 		= 0,
	 Input_Status_High
}Input_Status_t;
typedef struct
{
	GPIO_TypeDef 			*GPIOx;
	uint16_t 				GPIO_Pin;

	uint16_t 				numOfInput;
	uint32_t 				currentTime;
	uint32_t 				debounceTime;

	Input_Status_t			Input_Status;
	GPIO_PinState 			currentState;
	GPIO_PinState 			lastState;
}Input_State_t;

typedef struct
{
	Input_State_t 				userButton;
}Inputs_Info_t;

typedef struct
{
	GPIO_TypeDef		    *GPIOx;
	uint16_t 				GPIO_Pin;
	GPIO_PinState			pinState;

}Output_State_t;

typedef struct
{
	Output_State_t			ledGreen;
	Output_State_t			ledOrange;
	Output_State_t			ledRed;
	Output_State_t			ledBlue;
}Outputs_Info_t;

typedef struct
{
	Inputs_Info_t			inputInfo;
	Outputs_Info_t			outputInfo;

}IO_Info_t;

void IO_Initalization (IO_Info_t *ioInfo);
void IO_Status_Control(IO_Info_t *ioInfo);
void IO_Input_Control_With_Debounce (Input_State_t *inputState);
void IO_Output_Control(Output_State_t *outputState);

#endif /* INC_IO_DRIVER_H_ */
