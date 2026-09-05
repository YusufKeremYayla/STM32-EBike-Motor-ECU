/*
 * adc_drive.c
 *
 *  Created on: Feb 6, 2026
 *      Author: yusuf
 */
#include "adc_drive.h"

DMA_Transfer_Status_t globalDmaTransferStatus = DMA_Transfer_Not_Yet_Start;

void ADC_Initialization (ADC_Info_t *adcinfo , ADC_HandleTypeDef *hadc)
{
adcinfo->hadc = hadc;
adcinfo->adcError	=  ADC_No_Error;

for(int i=0 ;i<NUM_OF_ADC_CHANNEL ;i++)
{
	adcinfo->adcConvertedData[i]  = 0; //RAM temizleme
	adcinfo->adcVoltageData  [i]  = 0; // ''    ''
}
adcinfo->dmaTransferStatus = globalDmaTransferStatus;

if(HAL_ADC_Start_DMA(adcinfo->hadc, (uint32_t*)adcinfo->adcConvertedData, NUM_OF_ADC_CHANNEL)!=HAL_OK)
	{
	adcinfo->adcError = ADC_Init_Start_Error;
	}
}
void ADC_DMA_Conversion (ADC_Info_t *adcinfo)
{
	adcinfo->dmaTransferStatus = globalDmaTransferStatus;

	if(adcinfo->dmaTransferStatus == DMA_Transfer_Completed)
	{
		globalDmaTransferStatus = DMA_Transfer_Not_Completed;

		for(int i=0 ;i<NUM_OF_ADC_CHANNEL ;i++)
		{
			adcinfo->adcVoltageData[i] = (__LL_ADC_CALC_DATA_TO_VOLTAGE(VDDA,
															            adcinfo->adcConvertedData[i],
																        DIGITAL_SCALE))/1000.0;
		}
		adcinfo->pot1Percentage = MAP_Voltage_To_Persantage(adcinfo->adcVoltageData[ADC_Channel_2],
			        						0.0,
											3.3,
											0,
											100);

		/*adcinfo->Vref        =__LL_ADC_CALC_VREFANALOG_VOLTAGE(adcinfo->adcConvertedData[VREF_Int_Channel] , DIGITAL_SCALE);
		adcinfo->Temperature=__LL_ADC_CALC_TEMPERATURE(adcinfo->Vref ,adcinfo->adcConvertedData[Temperature_Sensor_Channel],
																											DIGITAL_SCALE);
		adcinfo->Vbat 		 = adcinfo->adcVoltageData[VBat_Channel];*/
	}
}
uint8_t MAP_Voltage_To_Persantage (float voltage, float inMin, float inMax, uint8_t outMin, uint8_t outMax) //Linear Mapping
{
	uint8_t percentage = (voltage - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
	return percentage;
}
/**
  * @brief  Regular conversion complete callback in non blocking mode
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
*/
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	//DMA transfer status completed
	globalDmaTransferStatus = DMA_Transfer_Completed;
}
/**
  * @brief  Regular conversion half DMA transfer callback in non blocking mode
  * @param  hadc pointer to a ADC_HandleTypeDef structure that contains
  *         the configuration information for the specified ADC.
  * @retval None
*/
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	//DMA transfer status didn't complete
	globalDmaTransferStatus = DMA_Transfer_Not_Completed;
}
