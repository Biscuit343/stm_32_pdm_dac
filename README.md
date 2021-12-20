# stm_32_pdm_dac

This is my current implementation of a DAC using an STM32L476RG microcontroller board.
It uses a function generator to produce a sine wave as an input signal which will go through multiple digital filters to output a 1-bit stream out of one GPIO pin.
This is a passion project of mine and as such, does not follow any industry standards that I know of.

## General Behavior Description

This PDM DAC is a device that locally generates a sine wave, converts it to a high fidelity output using a delta-sigma modulator which converts 16 bit samples to a high frequency 1 bit stream. This method was chosen to reduce using multiple discrete components as seen with other external discrete DACs. It functions through a keypad where the number 1-9 plays a sine wave tone at various step frequencies, and the letters A-D change the number of harmonics that are played. As an example, pressing ‘2’ and ‘B’ will play a 2KHz sine wave layering the first and second harmonics together. This STM32 does all of the audio processing with the exception of a 2nd order low pass filter (cutoff frequency is 24,000 Hz) at its output. This output is then fed into a 0.5 - 0.75W speaker for audio playback.

## Complications

After testing and debugging, I found that the STM32 does not have a high enough processing power to properly output our high fidelity audio even running at clock speed of 32 MHz. Desired output speed needs to be roughly 12,000 times faster in order to properly play any recognizable audio. The custom DAC is outputting at a bitrate of 220 bits per second. To output a distinguishable tone, the DAC should output at a bitrate of 2.8224 megabits per second. This is due to how the audio encoding format Digital Stream Digital (DSD) works. The compact disc (CD) uses an encoding format called pulse code modulation (PCM) to record audio at 16-bit 44.1KHz. DSD uses 64 times the oversampling method of PCM to record audio. This is done in part due to quantization errors caused by using only a 1 bit depth. To minimize distortion, the output stream needs to play at a very high sampling frequency, pass through a noise-shaping filter, and go through a low pass filter to remove the high-frequency noise.

To learn more: https://en.wikipedia.org/wiki/Direct_Stream_Digital

It is entirely possible that the low bitrate output is caused by time complexity issues. Some ways I could remedy this issue is:

1. Remove uncessary functions compiled in runtime.
2. Minimize the number of digital filters used. (This is not desired since multiple interpolation filters are used to oversample the input signal)
3. Use a smaller bit depth when sampling. (It's possible that the STM32 is reading data serially which uses more instructions per second (IPS). Reducing the bit depth may or may not increase computational overhead)
4. Eliminate negative quantization errors into the feedback loop.
5. Use an RTOS framework for better time optimizations.

Despite ways to fix the underlying issue, I don't have much faith that the microcontroller will be fast enough to output at the desired rate. This is why many high performance DACs use specialized digital signal processors to handle such computations. Same reasoning can be applyed to why ASIC machines are used for bitcoin mining. I will be redoing this project with an FPGA board that shows a better low level hardware realization to see if this project is feasible or not.

## Required Materials

1. STM32L476RG (https://www.st.com/en/microcontrollers-microprocessors/stm32l476rg.html#overview)
2. 4x4 Matrix Membrane Keypad (https://www.jameco.com/z/27899-Parallax-4x4-Matrix-Membrane-Keypad_2161473.html)
3. Jumper Wires (M-M and M-F)
4. SPEAKER 8OHM 250MW TOP PORT 89DB (https://www.digikey.be/en/products/detail/mallory-sonalert-products-inc/PSR-36N08A-JQ/2071445)
6. IC CMOS 2 CIRCUIT 8DIP (https://www.digikey.com/en/products/detail/texas-instruments/OPA2340PA/266130)
7. CAP CER 0.03UF 1KV Z5V RADIAL (https://www.digikey.com/en/products/detail/nte-electronics-inc/90330/11652214)
8. Carbon Film Resistor 220 Ohm 1/4 Watt 5% (https://www.jameco.com/z/CF1-4W221JRC-Jameco-Valuepro-Carbon-Film-Resistor-220-Ohm-1-4-Watt-5-_690700.html)

Last three materials are used for an active 2nd-order low pass filter with a gain factor of 2 and a cut-off frequency of 24,000 Hz. For volume control, a MOSFET and a potentiometer can be used to feed the desired voltage into the supply voltage pin in the op-amp.
