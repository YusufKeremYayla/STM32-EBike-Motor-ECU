/*
 * uart_ex.c
 *
 *  Created on: Feb 27, 2026
 *      Author: yusuf
 */
#include "uart_ex.h"
#include "circular_buffer.h"

void UARTx_Initalization (UART_Ex_t *uart , UART_HandleTypeDef *huart , Circular_Buffer_t *cbIn , Circular_Buffer_t *cbOut)
{
	uart->huart		=	 huart;
	uart->cbIn		=	 cbIn;
	uart->cbOut		=	 cbOut;

	Circular_Buffer_Init(cbOut);
	Circular_Buffer_Init(cbIn);

	__HAL_UART_ENABLE_IT(uart->huart , UART_IT_RXNE);
	__HAL_UART_ENABLE_IT(uart->huart , UART_IT_TXE);
}
void UARTx_Write_Char    (UART_Ex_t *uart, char ch)
{
	if(Circular_Buffer_Enqueue(uart->cbOut, ch))
	{
		if(!(uart->huart->Instance->CR1 & USART_CR1_TXEIE))
		{
			uint8_t ch;

			if(Circular_Buffer_Dequeue(uart->cbOut, &ch))
			{
				uart->huart->Instance->DR = ch; //Prime the Pump
				__HAL_UART_ENABLE_IT(uart->huart , UART_IT_TXE);
			}
		}
	}
}
void UARTx_Put_String    (UART_Ex_t *uart, char *str)
{
	while(*str)
	{
		UARTx_Write_Char(uart, *str);
		str++;
	}
}
int  UARTx_Printf        (UART_Ex_t *uart ,const char *format, ...)
{
	char tx_buffer[256];
	va_list args;

	va_start(args , format);
	int length = vsnprintf(tx_buffer, sizeof(tx_buffer), format, args);
	va_end(args);

	UARTx_Put_String(uart, tx_buffer);

	return length;
}

bool UARTx_ReadLine(UART_Ex_t *uart, char *lineBuffer , uint16_t maxLen)
{
	static bool 	messageReady 	= false;
	static bool 	lastCr		  	= false;
	static uint16_t index			= 0;
	uint8_t  ch 					= 0;

	if(messageReady)
		{
		   messageReady = false;
		   lineBuffer[index] = '\0';
		   index = 0;

		   return true;
		}
		while (Circular_Buffer_Dequeue(uart->cbIn, &ch))
		{
			if(lastCr && ch == '\n')
			  {
			   messageReady = true;
			   lastCr	=	false;
			   break ;
			  }

			if(ch == '\r')
			{
				lastCr	=	true;
			}
			else
			{
				if(lastCr)
				{
					if(index < maxLen - 1)
					{
						lineBuffer[index++] = '\r';
						lastCr = false;
					}
				}

				if(index < maxLen - 1)
				{
				lineBuffer[index++] = ch;
				}
			}
		}
		return false;
	}
