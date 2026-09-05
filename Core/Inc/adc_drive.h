/*
 * adc_drive.h
 *
 *  Created on: Feb 6, 2026
 *      Author: yusuf
 */
#ifndef INC_ADC_DRIVE_H_
#define INC_ADC_DRIVE_H_
#include "main.h"

#define NUM_OF_ADC_CHANNEL 		  1			//ADC kanal sayısı önceden 5 ti test amacıyla 1 yapıldı
#define VDDA 					  3300 		//mV

#define DIGITAL_SCALE_6BITS 	  0x3F 		//63
#define DIGITAL_SCALE_8BITS 	  0xFF 		//255
#define DIGITAL_SCALE_10BITS 	  0x3FF 	//1023
#define DIGITAL_SCALE_12BITS	  0xFFF 	//4095

#define DIGITAL_SCALE 			  DIGITAL_SCALE_12BITS

typedef enum
{
	ADC_Channel_2 				 	  =	0,
	/*ADC_Channel_3 				 	  = 1,
	Temperature_Sensor_Channel 		  = 2,
	VREF_Int_Channel 				  = 3,
	VBat_Channel					  = 4
	 */  // test amacıyla 4 kanal kapatıldı
}ADC_Channel_Name;

typedef enum
{
	DMA_Transfer_Not_Completed	  =  0,
	DMA_Transfer_Completed 		  =	 1,
	DMA_Transfer_Not_Yet_Start	  =	 2

}DMA_Transfer_Status_t;

typedef enum
{
	ADC_No_Error 				  =	 0,
	ADC_Init_Start_Error

}ADC_Error_t;

typedef struct
{
	ADC_HandleTypeDef 			  *hadc;

	uint16_t 					  adcConvertedData   [NUM_OF_ADC_CHANNEL];
	float  	 					  adcVoltageData     [NUM_OF_ADC_CHANNEL];
	uint8_t 					  pot1Percentage;

	DMA_Transfer_Status_t 		  dmaTransferStatus;
	ADC_Error_t 		  		  adcError;

	float 	     				  Temperature;
	float 	     				  Vref;
	float 	     				  Vbat;

}ADC_Info_t;

void ADC_Initialization   (ADC_Info_t *adcinfo , ADC_HandleTypeDef *hadc);
void ADC_DMA_Conversion   (ADC_Info_t *adcinfo);

uint8_t MAP_Voltage_To_Persantage (float voltage, float inMin, float inMax, uint8_t outMin, uint8_t outMax); //Linear Mapping

#endif /* INC_ADC_DRIVE_H_ */
