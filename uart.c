#include "uart.h"

void print_voltage(uint16_t data);
void print_data1(uint16_t data);
void print_data2(int32_t data);
void uart_init(void);
void uart_GPIO_init(void);
void uart_print(uint32_t num);

void print_voltage(uint16_t data)
{
	uint16_t ones = (uint16_t) (4095 / 3.3);
	uart_print((uint16_t) (data / ones) + 0x30);
	uart_print(0x2E);
	uint16_t remainder1 = (uint16_t) ((data % ones) * 10);
	uart_print((uint16_t) (remainder1 / ones) + 0x30);
	uint16_t remainder2 = (uint16_t) ((remainder1 % ones) * 10);
	uart_print((uint16_t) (remainder2 / ones) + 0x30);
}

void print_data1(uint16_t data)
{
	int i;
	for (i = 0; i < 16; i++)
	{
		uint32_t num = data & (0x8000 >> i);
		if (num > 0)
		{
			uart_print(0x31);
		}
		else
		{
			uart_print(0x30);
		}
		//uart_print(num + 0x30);
		//uart_print(0x0A);
	}
	//uart_print(0x02);
	uart_print(0x0A);
	uart_print(0x0D);
}

void print_data2(int32_t data)
{
	int i;
	for (i = 0; i < 32; i++)
	{
		uint32_t num = data & (0x80000000 >> i);
		if (num > 0)
		{
			uart_print(0x31);
		}
		else
		{
			uart_print(0x30);
		}
		//uart_print(num + 0x30);
		//uart_print(0x0A);
	}
	//uart_print(0x02);
	uart_print(0x0A);
	uart_print(0x0D);
}

void uart_print(uint32_t num)
{
        USART2->TDR = num;
        while (!(USART2->ISR & USART_ISR_TC)) {};
}


void uart_init(void)
{
    uart_GPIO_init();
    // enable USART RCC
    RCC->APB1ENR1 |= (RCC_APB1ENR1_USART2EN);
    __enable_irq();
    NVIC->ISER[0] = (1 << (USART2_IRQn & 0x1F));
    // set baud rate to 115.2 kbps with flck of 4 Mhz
    // set prescaler to 20 for 4 Mhz fclk
    USART2->GTPR |= ((4 << USART_GTPR_PSC_Pos) | (2 << USART_GTPR_PSC_Pos));
    // USARTDIV = 4000000 / 115200 = 34.72 = 35d = 0x23
    USART2->BRR = 0x24;
    // enable RXNE and TX interrupts
    USART2->CR1 |= (USART_CR1_TXEIE | USART_CR1_RXNEIE);
    // enable RX and TX
    USART2->CR1 |= (USART_CR1_TE | USART_CR1_RE);
    // enable UART
    USART2->CR1 |= (USART_CR1_UE);
}


void uart_GPIO_init(void)
{
    // enable GPIOA pins
    RCC->AHB2ENR |= (RCC_AHB2ENR_GPIOAEN);
    // resets GPIO pin mode
    GPIOA->MODER &= ~(GPIO_MODER_MODE2 | GPIO_MODER_MODE3);
    // sets GPIO pins to alternate function
    GPIOA->MODER |=  (GPIO_MODER_MODE2_1 | GPIO_MODER_MODE3_1);
    // Sets GPIO pins to high speed
    GPIOA->OSPEEDR |=  ((3 << GPIO_OSPEEDR_OSPEED2_Pos) | ( 3 << GPIO_OSPEEDR_OSPEED3_Pos));
    // reset output type register
    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT2 | GPIO_OTYPER_OT3);
    // reset push-pull configuration
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD2 | GPIO_PUPDR_PUPD3);
    // Sets the alternate function of transmit pin to USART2
    GPIOA->AFR[0] |= ((7 << GPIO_AFRL_AFSEL2_Pos) | (7 << GPIO_AFRL_AFSEL3_Pos));
}
