#ifndef INC_KEYPAD_H_
#define INC_KEYPAD_H_

#include "main.h"

uint8_t display_led(uint8_t count);
uint8_t button_press(void);
uint8_t check_inputs(uint8_t row);
uint8_t keypad_poll(void);
void keypad_init(void);

// define macros for keypad indexing
#define ROW_0 		GPIO_PIN_10
#define ROW_1 		GPIO_PIN_8
#define ROW_2 		GPIO_PIN_9
#define ROW_3 		GPIO_PIN_6
#define COL_0 		GPIO_PIN_6
#define COL_1 		GPIO_PIN_10
#define COL_2 		GPIO_PIN_4
#define COL_3 		GPIO_PIN_5
#define NUM_ROWS 	4
#define NUM_COLS 	4

#endif /* INC_KEYPAD_H_ */
