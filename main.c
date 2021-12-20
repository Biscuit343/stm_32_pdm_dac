#include "main.h"
#include "keypad.h"
#include "uart.h"
#include <math.h>
#include <stdio.h>

#define	HI_RES 512
#define	HI_RES_D 512.0

// Prototypes
uint16_t get_dac_freq(void);
void SystemClock_Config(void);

uint16_t sin_lookup[HI_RES];

void lookup_init(void) {
    uint16_t dac_val;

    // populates a sin wave lookup table
    for(int i = 0; i < HI_RES; i++) {
        dac_val = DAC_volt_conv((int)(1500 * (sin(2.0 * M_PI * i / HI_RES_D)
                        + 1.0)));
        sin_lookup[i] = dac_val;
    }
}

/*
 * Updates the FIFO buffer with the given input
 * params = buffer, length, data
 * returns a 0 for failure and a 1 for pass
 */
int32_t buffer_update(int32_t buffer[], uint16_t length, int32_t data)
{
	if (buffer == NULL)
	{
		return 0;
	}
	int i;
	for (i = 0; i < (length - 1); i++)
	{
		buffer[i] = buffer[i + 1];
	}
	buffer[length - 1] = data;
	return 1;
}

/*
 * Gets the idx of the given buffer. idx 0 is the oldest value of the buffer
 * params = *buffer, length, idx
 * returns a data point from the buffer at its given idx
 */
int32_t buffer_get(int32_t buffer[], uint16_t length, uint32_t idx)
{
	int32_t data = 0;
	if (buffer == NULL)
	{
		return data;
	}
	if ((idx >= 0) && (idx < length))
	{
		data = buffer[idx];
	}
	return data;
}

/*
 * Gets the average of the given buffer.
 * params = *buffer, length
 * returns the average of the buffer size
 */
int32_t buffer_get_avg(int32_t buffer[], uint16_t length)
{
	int i;
	int32_t avg = 0;
	if (buffer == NULL)
	{
		return avg;
	}
	for (i = 0; i < length; i++)
	{
		avg = avg + buffer[i];
	}
	avg = avg / length;
	return avg;
}

/*
 * Prints the parameter number into the terminal through UART
 * params = idx
 */
void index_print(uint32_t idx)
{
	idx = idx % 10;
	uart_print(idx + 0x30);
}

/*
 * Scales an unsigned 16 bit sample to a signed 32 bit sample
 * params = sample
 * returns a signed 32 bit sample
 */
int32_t convert_data(uint16_t sample)
{
	// shift factor = (31 - 16) / 2 = 7.5 bits (if integer, use left shift instead)
	// scaling factor = 2 ^ (shift factor) = 181.02 (round down to prevent overflow)
	// overflow occurs if sample uses the entire bit depth
	int32_t data = ((uint32_t)sample) * 181;
	return data;
}


// Global variables
uint16_t sample = 0;
uint32_t idx = 0;
int32_t data = 0;
uint8_t is_zero_padded0 = 0;
uint8_t is_zero_padded1 = 0;
uint8_t is_zero_padded2 = 0;
uint32_t counter = 0;
uint16_t peak = 0;

int main(void)
{
	// initialize sine table, uart terminal, and keypad pins
	uart_init();
	lookup_init();
	keypad_init();

	// initialize GPIO pin as 1 bit output signal
	RCC->AHB2ENR   |=  (RCC_AHB2ENR_GPIOCEN);
	GPIOC->MODER   &= ~(GPIO_MODER_MODE9);
	GPIOC->MODER   |=  (GPIO_MODER_MODE9_0);
	GPIOC->OTYPER  &= ~(GPIO_OTYPER_OT9);
	GPIOC->PUPDR   &= ~(GPIO_PUPDR_PUPD9);
	GPIOC->OSPEEDR |=  (3 << GPIO_OSPEEDR_OSPEED9_Pos);

	// Make an 8x Interpolation filter. This can be done using 6 moving average filters
	int32_t buffer0[125] = {0};		// Holds the current list of samples that has been zero padded (sample rate x2)
	int32_t buffer1[125] = {0};		// Holds the first 125 tap moving average filter
	int32_t buffer2[125] = {0};		// Holds the second 125 tap moving average filter
	int32_t buffer3[25] = {0};		// Holds the first moving average output that has been zero padded (sample rate x2)x2
	int32_t buffer4[25] = {0};		// Holds the first 25 tap moving average filter
	int32_t buffer5[25] = {0};		// Holds the second 25 tap moving average filter
	int32_t buffer6[4] = {0};		// Holds the second moving average output that has been zero padded ((sample rate x2)x2)x2
	int32_t buffer7[4] = {0};		// Holds the first 4 tap moving average filter
	int32_t buffer8[4] = {0};		// Holds the second 4 tap moving average filter

	// Delay block buffers for a first order delta sigma modulator with a noise-coupled structure
	int32_t buffer9 = 0;		// 1 sample delay for first block system
	int32_t buffer10 = 0;		// 1 sample delay for second block system
	int32_t buffer11 = 0;		// 1 sample delay for quantized output signal

	uint8_t num;        		// declares a variable to track button presses
	uint8_t freq = 1;			// declares and initializes a variable to track current frequency
	uint8_t harmonics = 1;		// declares and initializes a variable to track # of harmonics

	while(1)
	{
		// get current keypad input
		num = button_press();
		// check if keypad input is valid
		if(num < 14 && num > 0)
		{
			// if input is a number
			if (num > 0 && num < 10)
			{
				// change frequency factor to input
				freq = num;
			}
			else // change harmonics
			{
				harmonics = num - 9;
			}
			// reinitialize all buffers and variables
			int i;
			for (i = 0; i < (sizeof(buffer0) / sizeof(int32_t)); i++)
			{
				buffer0[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer1) / sizeof(int32_t)); i++)
			{
				buffer1[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer2) / sizeof(int32_t)); i++)
			{
				buffer2[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer3) / sizeof(int32_t)); i++)
			{
				buffer3[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer4) / sizeof(int32_t)); i++)
			{
				buffer4[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer5) / sizeof(int32_t)); i++)
			{
				buffer5[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer6) / sizeof(int32_t)); i++)
			{
				buffer6[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer7) / sizeof(int32_t)); i++)
			{
				buffer7[i] = 0;
			}
			for (i = 0; i < (sizeof(buffer8) / sizeof(int32_t)); i++)
			{
				buffer8[i] = 0;
			}
			buffer9 = 0;
			buffer10 = 0;
			sample = 0;
			idx = 0;
			data = 0;
			is_zero_padded0 = 0;
			is_zero_padded1 = 0;
			is_zero_padded2 = 0;
			counter = 0;
			peak = 0;
		}

		// execute only if samples from both second and third interpolation filter aren't zero-padded
		if (is_zero_padded1 == 0 && is_zero_padded2 == 0)
		{
			// 125 tap IF 2x => 25 tap IF 2x => 4 tap IF 2x
			// list of sampled data with zero padding
			if (is_zero_padded0 == 0)
			{
				// Obtain the latest point (16-bit sample) from the function generator
				// Scale from 16-bit to 32-bit (31-bit to account for negative numbers) for higher resolution during quantization
				int j;
				data = 0;
				// for each harmonic order
				for (j = 0; j < harmonics; j++)
				{
					// add sine wave with appropriate harmonic frequency to data
					uint16_t new_sample = sample * (j + 1);
					if (new_sample >= HI_RES)
					{
						new_sample = new_sample - HI_RES;
					}
					data = data + convert_data(sin_lookup[new_sample]);
				}
				// divide my harmonics to avoid overflow
				data = data / harmonics;
				// update sample index
				sample = sample + (1 * freq);
				if (sample >= HI_RES)
				{
					sample = sample - HI_RES;
				}
				// update array with sample from function generator
				buffer_update(buffer0, 125, data);
				// next sample must be zero-padded
				is_zero_padded0 = 1;
				// increment index
				idx++;
			}
			else
			{
				// zero pad the current sample index
				buffer_update(buffer0, 125, 0);
				// next sample must not zero padded
				is_zero_padded0 = 0;
				// increment index
				idx++;
			}

		}// otherwise, don't sample and update

		// sampled list is full for moving average computation (sampling frequency has doubled)
		// Perform 125 tap moving average twice (if applicable) for sampled data

		// first moving average filter
		if (idx >= 125)
		{
			int32_t if1 = buffer_get_avg(buffer0, 125);
			// update 2nd buffer with average of 1st buffer
			buffer_update(buffer1, 125, if1);
		}
		// second moving average filter using first moving average samples
		if (idx >= (125 + 125))
		{
			int32_t if2 = buffer_get_avg(buffer1, 125);
			// update 3rd buffer with average of 2nd buffer
			buffer_update(buffer2, 125, if2);
		}

		// continue if both buffers are full
		if (idx >= 250)
		{
			// execute if third interpolation filter isn't zero padded
			if (is_zero_padded2 == 0)
			{
				// list of sampled data with zero padding
				if (is_zero_padded1 == 0)
				{
					// update 4th buffer with oldest data from 3rd buffer
					buffer_update(buffer3, 25, buffer_get(buffer2, 125, 0));
					// next sample must be zero-padded
					is_zero_padded1 = 1;
					// increment index
					idx++;
				}
				else
				{
					// update 4th buffer with zero
					buffer_update(buffer3, 25, 0);
					// next sample must not be zero-padded
					is_zero_padded1 = 0;
					// increment index
					idx++;
				}
			} // otherwise, don't sample and update

			// first moving average list is full for second moving average computation (sampling frequency has doubled)
			// Perform 25 tap moving average twice for sampled data
			if (idx >= (250 + 25))
			{
				int32_t if3 = buffer_get_avg(buffer3, 25);
				// update 5th buffer with average of 4th buffer
				buffer_update(buffer4, 25, if3);
			}
			if (idx >= (250 + 25 + 25))
			{
				int32_t if4 = buffer_get_avg(buffer4, 25);
				// update 6th buffer with average of 5th buffer
				buffer_update(buffer5, 25, if4);
			}

			// continue if both 125 tap moving average buffers are full
			if (idx >= 300)
			{
				// list of sampled data with zero padding
				if (is_zero_padded2 == 0)
				{
					// update 7th buffer with oldest data from 6th buffer
					buffer_update(buffer6, 4, buffer_get(buffer5, 25, 0));
					// next sample must be zero-padded
					is_zero_padded2 = 1;
					// increment index
					idx++;
				}
				else
				{
					// update 7th buffer with zero
					buffer_update(buffer6, 4, 0);
					// next sample must not be zero-padded
					is_zero_padded2 = 0;
					// increment index
					idx++;
				}
				// second moving average list is full for third moving average computation (sampling frequency has doubled)
				// Perform 4 tap moving average twice for sampled data
				if (idx >= (125 + 125 + 25 + 25 + 4))
				{
					int32_t if5 = buffer_get_avg(buffer6, 4);
					// update 8th buffer with average of 7th buffer
					buffer_update(buffer7, 4, if5);
				}
				if (idx >= (125 + 125 + 25 + 25 + 4 + 4))
				{
					int32_t if6 = buffer_get_avg(buffer7, 4);
					// update 9th buffer with average of 8th buffer
					buffer_update(buffer8, 4, if6);
				}
			}
		}
		// else, break and get next sample
		else
		{
			continue;
		}

		// At this point, buffer8 should contain the final interpolated data points for minimum distortion
		// Use FFT if you want to predict the frequency response of the sampled data (don't need to)

		// Loop Filter design 	=> z^-1 			=> Y[n] = X[n-1]
		// 						=> 1 - z^-1			=> Y[n] = X[n] - X[n-1]
		// 						=> 1 / (1 - z^-1) 	=> Y[n] = X[n] + X[n-1]

		int32_t v1 = buffer_get(buffer8, 4, 0) - buffer11;		// input signal from integrator - quantized output signal
		int32_t v2 = v1 + buffer9;								// v1 + first delayed signal
		int32_t v3 = v2 - buffer10;								// v2 - second delayed signal
		// no need to use dithering for v3

		// 16-bit Quantizer (Takes the 32-bit data point from integrator (v3) and quantize to nearest 16-bit depth)
		// shift factor = 7.5 bits, scaling factor = 181.02

		// if 32 bit output is a negative number, default to zero
		if (v3 < 0)
		{
			v3 = 0;
		}
		uint16_t v4 = (uint16_t)(v3 / 181);				// quantized output (scaled for 16 bits)
		int32_t v5 = (int32_t)(v4 * 181);				// quantized output (scaled for 31 bits)

		// update comparator value with highest 16-bit quantized output
		if (v4 > peak)
		{
			peak = v4;
		}

		// quantization error
		int32_t q_error = v5 - v3;

		// update delay buffers
		buffer9 = v1;
		buffer10 = q_error;
		buffer11 = v5;

		// Create 1-bit pdm signal for v4 (quantized output)
		// Add sample to the counter.
		// if sample is greater than threshold (reference voltage), output 1 and subtract sample with reference
		// else, output a zero
		counter = counter + v4;
		if (counter >= (peak))
		{
			counter = counter - (peak);
			// write 1 to the output
			GPIOC->BSRR = (GPIO_PIN_9);
			// print 1 to the UART terminal
			index_print(1);
		}
		else
		{
			// write 0 to the output
			GPIOC->BRR = (GPIO_PIN_9);
			// print 0 to the UART terminal
			index_print(0);
		}
	}
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}
/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
