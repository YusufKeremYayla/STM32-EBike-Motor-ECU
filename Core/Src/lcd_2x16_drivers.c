/*
 * lcd_2x16_drivers.c
 *
 *  Created on: May 2, 2026
 *      Author: yusuf
 */

#include "lcd_2x16_drivers.h"

void LCD_Initalization	(LCD_t *lcd)
{
	HAL_Delay(50);
	LCD_Send_Init_Nibble(lcd, 0x30);
	HAL_Delay(5);
	LCD_Send_Init_Nibble(lcd, 0x30);
	HAL_Delay(1);
	LCD_Send_Init_Nibble(lcd, 0x30);
	HAL_Delay(1);

	LCD_Send_Init_Nibble(lcd, 0x20);
	HAL_Delay(1);

	LCD_Send_Command(lcd, LCD_Cmd_FunctionSet | LCD_4BIT_MODE | LCD_2_LINE | LCD_5x8_DOTS);
	HAL_Delay(1);

	lcd->display_control = LCD_Display_On;
	LCD_Send_Command(lcd, LCD_Cmd_DisplayOnOff | lcd->display_control);
	HAL_Delay(1);

	LCD_Send_Command(lcd, LCD_Cmd_ClearDisplay);
	HAL_Delay(1);

	LCD_Send_Command(lcd, LCD_Cmd_EntryMode | DISPLAY_SHIFT_OFF | CURSOR_DIR_INC);
	HAL_Delay(1);

	if(lcd->backLight)
		LCD_Backlight_On(lcd);

	else
		LCD_Backlight_Off(lcd);
}

void LCD_Send_Command  (LCD_t *lcd, uint8_t cmd)
{
uint8_t data_u = cmd & 0xF0;
uint8_t data_l = (cmd << 4) & 0xF0;

uint8_t data_t[4];

data_t[0] = (data_u) | (lcd->backLight ? 0x08 : 0x00) | 0x04; //EN =1
data_t[1] = (data_u) | (lcd->backLight ? 0x08 : 0x00) | 0x00; //EN =0
data_t[2] = (data_l) | (lcd->backLight ? 0x08 : 0x00) | 0x04; //EN =1
data_t[3] = (data_l) | (lcd->backLight ? 0x08 : 0x00) | 0x00; //EN =0

HAL_I2C_Master_Transmit(lcd->hi2c, lcd->i2c_addr, data_t, 4, 100);
HAL_Delay(1);
}

void LCD_Clear	(LCD_t *lcd)
{
	LCD_Send_Command(lcd, LCD_Cmd_ClearDisplay);
	HAL_Delay(2);
}

void LCD_Home				(LCD_t *lcd)
{
	LCD_Send_Command(lcd, LCD_Cmd_ReturnHome);
	HAL_Delay(2);
}

void LCD_Send_Char			(LCD_t *lcd, char ch)
{
	LCD_Send_Data(lcd, (uint8_t)ch);
}

void LCD_Show_Cursor		(LCD_t *lcd)
{
	lcd->display_control |= LCD_Cursor_Blink_On;
	LCD_Send_Command(lcd, LCD_Cmd_DisplayOnOff | lcd->display_control);
}

void LCD_Hide_Cursor		(LCD_t *lcd)
{
	lcd->display_control &= ~LCD_Cursor_On;
	LCD_Send_Command(lcd, LCD_Cmd_DisplayOnOff | lcd->display_control);
}

void LCD_Set_Cursor	 (LCD_t *lcd, uint8_t rows, uint8_t col)
{
	const uint8_t rows_offset[] = {0x00, 0x40};

	if(rows >= lcd->rows)
		rows = 2;

	if(col>=lcd->columns)
		col = 15;

	LCD_Send_Command(lcd, LCD_Cmd_Set_DDRAM_Address | (rows_offset[rows] + col));
}

void LCD_Send_String   (LCD_t *lcd, const char *str)
{
	while(*str)
	{
		LCD_Send_Data(lcd, (uint8_t)*str++);
	}
}

void LCD_Send_Data	(LCD_t *lcd, uint8_t data)
{
	uint8_t data_u =  data & 0xF0;
	uint8_t data_l = (data<<4) & 0xF0;

	uint8_t data_t[4];

	data_t[0] = data_u | (lcd->backLight ? 0x08 : 0x00) | 0x05;
	data_t[1] = data_u | (lcd->backLight ? 0x08 : 0x00) | 0x01;

	data_t[2] = data_l | (lcd->backLight ? 0x08 : 0x00) | 0x05;
	data_t[3] = data_l | (lcd->backLight ? 0x08 : 0x00) | 0x01;

	HAL_I2C_Master_Transmit(lcd->hi2c, lcd->i2c_addr, data_t, 4, 100);
	HAL_Delay(1);
}

void LCD_Backlight_On	(LCD_t *lcd)
{
	lcd->backLight = true;
	LCD_Send_Command(lcd, 0x00);
}

void LCD_Backlight_Off	(LCD_t *lcd)
{
	lcd->backLight = false;
	LCD_Send_Command(lcd, 0x00);
}

void LCD_Printf	  (LCD_t *lcd, const char *format ,...)
{
	char buffer[64];

	va_list args;
	va_start(args, format);

	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	LCD_Send_String(lcd, buffer);
}

void LCD_Scroll_Text  (LCD_t *lcd, const char *text, uint8_t rows, uint16_t delayMs)
{
	char buffer[17];
	uint8_t len = strlen(text);
	if(len <= lcd->columns)
	{
		LCD_Set_Cursor(lcd, rows, 0);
		LCD_Send_String(lcd, text);
		return;
	}

		for(uint8_t i = 0; i <= len - lcd->columns; i++)
		{
			strncpy(buffer, &text[i], lcd->columns);
			buffer[lcd->columns] = '\0';

			LCD_Set_Cursor(lcd, rows, 0);
			LCD_Send_String(lcd, buffer);
			HAL_Delay(delayMs);
		}
}

void LCD_Send_Init_Nibble	(LCD_t *lcd, uint8_t Nibble)
{
uint8_t data_t[2];
uint8_t data_u = Nibble & 0xF0;

data_t[0] = data_u | (lcd->backLight ? 0x08 : 0x00) | 0x04 ;
data_t[1] = data_u | (lcd->backLight ? 0x08 : 0x00) ;

if(HAL_I2C_Master_Transmit(lcd->hi2c, lcd->i2c_addr, data_t, 2, 100)==HAL_OK)
	{
		//RETURN OK
	}
}
