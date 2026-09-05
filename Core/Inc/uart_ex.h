/*
 * uart_ex.h
 *
 *  Created on: Feb 27, 2026
 *      Author: yusuf
 */

#ifndef INC_UART_EX_H_
#define INC_UART_EX_H_

#include "stdarg.h"
#include "stdio.h"
#include "stdint.h"
#include "main.h"
#include "circular_buffer.h"

typedef struct
{

	Circular_Buffer_t	 *cbIn ;
	Circular_Buffer_t 	 *cbOut;
	UART_HandleTypeDef 	 *huart;

}UART_Ex_t;

void UARTx_Initalization (UART_Ex_t *uart , UART_HandleTypeDef *huart , Circular_Buffer_t *cbIn , Circular_Buffer_t *cbOut);
void UARTx_Write_Char    (UART_Ex_t *uart, char ch);
void UARTx_Put_String    (UART_Ex_t *uart, char *str);
int  UARTx_Printf        (UART_Ex_t *uart ,const char *format, ...);
bool UARTx_ReadLine		 (UART_Ex_t *uart, char *lineBuffer , uint16_t maxLen);

#endif /* INC_UART_EX_H_ */
