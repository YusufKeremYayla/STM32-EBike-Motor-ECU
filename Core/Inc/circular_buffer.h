/*
 * circularbuffer.h
 *
 *  Created on: Feb 24, 2026
 *      Author: yusuf
 */

#ifndef INC_CIRCULAR_BUFFER_H_
#define INC_CIRCULAR_BUFFER_H_

#include "stdbool.h"
#include "stdint.h"
#include "string.h"

#define CIRCULAR_BUFFER_SIZE 	512

typedef struct
{
	uint8_t  buffer[CIRCULAR_BUFFER_SIZE];
	uint16_t head;
	uint16_t tail;

}Circular_Buffer_t;

/*****		Buffer Initalization  	*****/
void 	 Circular_Buffer_Init 		 (Circular_Buffer_t *circularbuffer);

bool  	 Circular_Buffer_Dequeue     (Circular_Buffer_t *circularbuffer , uint8_t *data);
bool 	 Circular_Buffer_Enqueue     (Circular_Buffer_t *circularbuffer , uint8_t data);

//Is Buffer Empty ? | Is Buffer Fully ?
bool 	 Circular_Buffer_Is_Empty 	 (Circular_Buffer_t *circularbuffer);
bool 	 Circular_Buffer_Is_Fully	 (Circular_Buffer_t *circularbuffer);

uint16_t Circular_Buffer_Count	     (Circular_Buffer_t *circularbuffer);

#endif /* INC_CIRCULAR_BUFFER_H_ */
