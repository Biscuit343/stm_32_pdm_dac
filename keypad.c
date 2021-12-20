#include "keypad.h"
#include "main.h"
#include <math.h>

uint8_t display_led(uint8_t count);
uint8_t button_press(void);
uint8_t check_inputs(uint8_t row);
uint8_t keypad_poll(void);

uint16_t ROWS[4] = {ROW_0, ROW_1, ROW_2, ROW_3};
uint16_t COLS[4] = {COL_0, COL_1, COL_2, COL_3};

uint8_t  NUM_ARR[4][4] = {
		{15, 0, 14, 13} ,
		{ 7, 8,  9, 12} ,
		{ 4, 5,  6, 11} ,
		{ 1, 2,  3, 10}
};

void keypad_init(void)
{
	RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN
			| RCC_AHB2ENR_GPIOCEN);

	// enable GPIOA pins for keypad rows as outputs
	GPIOA->MODER   &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE9 | GPIO_MODER_MODE8
			| GPIO_MODER_MODE10);
	GPIOA->MODER   |=  (GPIO_MODER_MODE6_0 | GPIO_MODER_MODE9_0
			| GPIO_MODER_MODE8_0 | GPIO_MODER_MODE10_0);
	GPIOA->OTYPER  &= ~(GPIO_OTYPER_OT6 | GPIO_OTYPER_OT9 | GPIO_OTYPER_OT8
			| GPIO_OTYPER_OT10);
	GPIOA->PUPDR   &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD9 | GPIO_PUPDR_PUPD8
			| GPIO_PUPDR_PUPD10);
	GPIOA->OSPEEDR |=  ((3 << GPIO_OSPEEDR_OSPEED6_Pos)
			| (3 << GPIO_OSPEEDR_OSPEED9_Pos) | (3 << GPIO_OSPEEDR_OSPEED8_Pos)
			| (3 << GPIO_OSPEEDR_OSPEED10_Pos));

	// enable GPIOB pins for keypad columns as inputs
	GPIOB->MODER   &= ~(GPIO_MODER_MODE4 | GPIO_MODER_MODE5 | GPIO_MODER_MODE6
			| GPIO_MODER_MODE10);
	GPIOB->OTYPER  &= ~(GPIO_OTYPER_OT4 | GPIO_OTYPER_OT5 | GPIO_OTYPER_OT6
			| GPIO_OTYPER_OT10);
	GPIOB->PUPDR   &= ~(GPIO_PUPDR_PUPD4 | GPIO_PUPDR_PUPD5 | GPIO_PUPDR_PUPD6
			| GPIO_PUPDR_PUPD10);
	GPIOB->PUPDR   |=  (GPIO_PUPDR_PUPD4_1 | GPIO_PUPDR_PUPD5_1
			| GPIO_PUPDR_PUPD6_1 | GPIO_PUPDR_PUPD10_1);
	GPIOB->OSPEEDR |=  ((3 << GPIO_OSPEEDR_OSPEED4_Pos)
			| (3 << GPIO_OSPEEDR_OSPEED5_Pos) | (3 << GPIO_OSPEEDR_OSPEED6_Pos)
			| (3 << GPIO_OSPEEDR_OSPEED10_Pos));
}


uint8_t keypad_poll(void) {
	uint8_t num = 16;
	while(num > 16)
		num = button_press();
	return num;
}


uint8_t button_press(void) {
	uint8_t num = 16;
	for(uint8_t row = 0; row < NUM_ROWS; row++) {
		GPIOA->BSRR = (ROWS[row]);
		for(uint8_t col = 0; col < NUM_COLS; col++) {
			if((GPIOB->IDR) & COLS[col])
				num = NUM_ARR[row][col];
		}
		GPIOA->BRR  = (ROWS[row]);
	}
	return num;
}
