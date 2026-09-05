/*
 * io_driver.c
 *
 *  Created on: Jan 28, 2026
 *      Author: yusuf
 */
#include "io_driver.h"

void IO_Initalization (IO_Info_t *ioInfo)
{
	/***************Input Initalization***************/
	ioInfo->inputInfo.userButton.GPIOx			=	GPIOA;
	ioInfo->inputInfo.userButton.GPIO_Pin		=	GPIO_PIN_0;
	ioInfo->inputInfo.userButton.numOfInput		=	0;
	ioInfo->inputInfo.userButton.currentTime	=	0;
	ioInfo->inputInfo.userButton.debounceTime	=	DEBOUNCE_TIME;
	ioInfo->inputInfo.userButton.Input_Status	=	Input_Status_Low;
	ioInfo->inputInfo.userButton.currentState	=	GPIO_PIN_RESET;
	ioInfo->inputInfo.userButton.lastState		=	GPIO_PIN_RESET;

	/***************Output Initalization***************/
	ioInfo->outputInfo.ledGreen.GPIOx			= 	GPIOD;
	ioInfo->outputInfo.ledGreen.GPIO_Pin		=	GPIO_PIN_12;
	ioInfo->outputInfo.ledGreen.pinState		= 	GPIO_PIN_RESET;

	ioInfo->outputInfo.ledOrange.GPIOx			= 	GPIOD;
	ioInfo->outputInfo.ledOrange.GPIO_Pin		=	GPIO_PIN_13;
	ioInfo->outputInfo.ledOrange.pinState		= 	GPIO_PIN_RESET;

	ioInfo->outputInfo.ledRed.GPIOx				= 	GPIOD;
	ioInfo->outputInfo.ledRed.GPIO_Pin			=	GPIO_PIN_14;
	ioInfo->outputInfo.ledRed.pinState			= 	GPIO_PIN_RESET;

	ioInfo->outputInfo.ledBlue.GPIOx			= 	GPIOD;
	ioInfo->outputInfo.ledBlue.GPIO_Pin			=	GPIO_PIN_15;
	ioInfo->outputInfo.ledBlue.pinState			= 	GPIO_PIN_RESET;
}

void IO_Status_Control(IO_Info_t *ioInfo)
{
	IO_Output_Control(&ioInfo->outputInfo.ledGreen);
	IO_Output_Control(&ioInfo->outputInfo.ledOrange);
	IO_Output_Control(&ioInfo->outputInfo.ledRed);
	IO_Output_Control(&ioInfo->outputInfo.ledBlue);

	IO_Input_Control_With_Debounce(&ioInfo->inputInfo.userButton);
}

void IO_Input_Control_With_Debounce (Input_State_t *inputState)
{
	inputState->currentState=HAL_GPIO_ReadPin(inputState->GPIOx, inputState->GPIO_Pin);

	if(inputState->currentState==GPIO_PIN_SET)
	{
		if(inputState->Input_Status==Input_Status_Low)
		{
			if(inputState->currentState!=inputState->lastState)
			{
				inputState->currentTime=HAL_GetTick();
				inputState->lastState=inputState->currentState;
			}
			if(HAL_GetTick()-inputState->currentTime >= inputState->debounceTime)
			{
				inputState->Input_Status=Input_Status_High;
			}
		}
		else
		{
				//Zaten High
		}
	}
	else
	{
		inputState->lastState=inputState->currentState;
		inputState->Input_Status=Input_Status_Low;
	}
}
void IO_Output_Control(Output_State_t *outputState)
{
HAL_GPIO_WritePin(outputState->GPIOx, outputState->GPIO_Pin, outputState->pinState);
}
