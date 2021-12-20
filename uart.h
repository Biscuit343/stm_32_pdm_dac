/*
 * uart.c
 *
 *         Authors:         Tyler Herzog and Daniel Lee
 *
 *         Description:    Header file for uart.c that defines prototypes
 */
#ifndef INC_UART_H_
#define INC_UART_H_

#include "main.h"

void uart_init(void);
void uart_GPIO_init(void);
void uart_print(uint32_t num);


#endif /* INC_UART_H_ */
