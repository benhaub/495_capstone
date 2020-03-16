/*
(C) Kelsey Kosheluk, January 2020

This script will read data values entered by a user through a console to the launchpad via a USB port configured with a baud rate of 9600 and no parity bit set.
The led will change colour and the pwm will change duty cycle on PB6 based on the user input of 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 or .

Keep in mind if changing pins to run pwm, PB6 is shorted to PD0 and PB7 is shorted to Pd1 on the development board.

Same thing as main_old.c, but uses serial through PB0 and PB1` instead of PA0 and PA1.

PD6 is the receive pin
PD7 is the transmit pin

All page numbers are with reference to the Intel TM4C123GH6PM data sheet
*/

# include "tm4c123gh6pm.h"
#include <string.h>
#include <stdlib.h>


void init_gpio();
char readChar(void);
void delay_us(unsigned int us);
void printChar(char c);
void printString(char * string);
char* readString(char delimiter);
char c;
char d;
unsigned int uart_sel = 0; // if this is one, debug through usb, otherwise

// PWM initialization is on page 1239 of the datasheet
void init_gpio() {
    // Activate the two PWM modules
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R0;
    while ((SYSCTL_PRPWM_R & SYSCTL_PRPWM_R0) == 0) {}; // ensure there is a proper delay
    SYSCTL_RCGCPWM_R |= SYSCTL_RCGCPWM_R1;
    while ((SYSCTL_PRPWM_R & SYSCTL_PRPWM_R1) == 0) {}; // ensure there is a proper delay

    // Activate clock for ports F, E, C, & B
    SYSCTL_RCGCGPIO_R |= 0x36; // p. 340: 11_0110 --> FE_DCBA
    volatile unsigned long delay_clk; // Delay for clock, must have 3 sys clock delay.
    delay_clk = SYSCTL_RCGC0_R; // just a delay for the clock to settle. This requires no-operation.

    // Port B Stuff
    GPIO_PORTB_DEN_R |= 0x70; // Enable digital ports for PB6, PB5, & PB4
    GPIO_PORTB_AFSEL_R |= 0x70; // p. 672: Enables alternate functions for PB6, PB5, & PB4
    GPIO_PORTB_DIR_R |= 0x70; // p. 673: Set pins as outputs
    GPIO_PORTB_ODR_R &= ~0x70; // p. 676: disable open drain on PB6, PB5, & PB4
    GPIO_PORTB_AMSEL_R &= ~0x70; // p. 687: disable analog function on pins

    // Port C Stuff
    GPIO_PORTC_DEN_R |= 0x30; // Enable digital ports for PC5 & PC4
    GPIO_PORTC_AFSEL_R |= 0x30; // p. 672: Enables alternate functions for PC5 & PC4
    GPIO_PORTC_DIR_R |= 0x30; // p. 673: Set pins as outputs
    GPIO_PORTC_ODR_R &= ~0x30; // p. 676: disable open drain on PC5 & PC4
    GPIO_PORTC_AMSEL_R &= ~0x30; // p. 687: disable analog function on pins

    // Port E Stuff
    GPIO_PORTE_DEN_R |= 0x30; // Enable digital ports for PE5 & PE4
//    GPIO_PORTE_DEN_R &= 0x0F; // Disable digital ports for PE3-PE0
    GPIO_PORTE_AFSEL_R |= 0x30; // p. 672: Enables alternate functions for PE5 & PE4
//    GPIO_PORTE_AFSEL_R &= ~0x0F; // p.672: Disable alternate functions for PE0 - PE3 --> used for analog inputs
    GPIO_PORTE_DIR_R |= 0x30; // p. 673: Set pins as outputs
//    GPIO_PORTE_DIR_R &= ~0x0F; // p. 673 : Set pins as inputs for PE0 - PE3 --> used for analog inputs
    GPIO_PORTE_ODR_R &= ~0x30; // p. 676: disable open drain on PE5 & PE4
    GPIO_PORTE_AMSEL_R &= ~0x30; // p. 687: disable analog function on pins
//    GPIO_PORTE_AMSEL_R |= 0x0F; // p.687: enable analog functionality on pins PE0 - PE3

    // Port F Stuff
    // the following two lines are needed to unlock SW2
//    GPIO_PORTF_LOCK_R  |= 0x4C4F434B; // p.684: Enable write access to GPIO_PORTF_CR.  Needed to enable SW2
//    GPIO_PORTF_CR_R |= 0x01; // p. 685: Commit register. Determines which bits of the GPIOAFSEL, GPIOPUR, GPIOPDR, and GPIODEN registers are committed when a write to these registers is performed. Needed to enable SW2
    GPIO_PORTF_DEN_R |= 0x0E; // Enable digital ports for PF3, PF2, & PF1
    GPIO_PORTF_AFSEL_R |= 0x0E; // p. 672: Enables alternate functions for PF3, PF2, & PF1
    GPIO_PORTF_DIR_R |= 0x0E; // p. 673: Set pins as outputs
    GPIO_PORTF_ODR_R &= ~0x0E; // p. 676: disable open drain on PF3, PF2, & PF1
    GPIO_PORTF_AMSEL_R &= ~0x0E; // p. 687: disable analog function on pins

    // Enable PWM peripheral functions
    GPIO_PORTB_PCTL_R |= 0x04440000; // Enable PWM peripheral functions on PB6, PB5, & PB4 and UART functions on PB1 & PB0
    GPIO_PORTC_PCTL_R |= 0x00440000; // Enable PWM peripheral functions on PC5 & PC4
    GPIO_PORTE_PCTL_R |= 0x00550000; // Enable PWM peripheral functions on PE5 & PE4
    GPIO_PORTF_PCTL_R |= 0x00005550; // Enable PWM peripheral functions on PF3, PF2, & PF1
    GPIO_PORTE_PCTL_R &= ~0x0000FFFF; // Clear peripheral functions on PE3-PE0


    // Set PWM clock dividers here ********************************************************************
    SYSCTL_RCC_R |= 0x00100000; // p.254: Use PWMDIV as pwm clock divider
    delay_clk = SYSCTL_RCGC0_R; // another delay

    SYSCTL_RCC_R |= 0x000E0000; // p. 254: Divide PWM clock by 64 for 312.5 KHz clock
    delay_clk = SYSCTL_RCGC0_R; // another delay

    // Set up PWM registers here
    PWM0_0_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM0CTL
    PWM0_1_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM1CTL
    PWM0_3_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM3CTL

    PWM1_2_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM0CTL
    PWM1_1_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM1CTL
    PWM1_3_CTL_R &= ~0x0007FFFF; // p. 1266: Clear the registers in PWM3CTL

    // PWM Module 0 Generators
    PWM0_0_GENA_R &= ~0x00000FFF; // p. 1282
    PWM0_0_GENA_R |= 0x8C; // p.1282: Use CMPA to create duty cycle
    PWM0_1_GENA_R &= ~0x00000FFF; // p. 1282
    PWM0_1_GENA_R |= 0x8C; // p.1282: Use CMPA to create duty cycle
    PWM0_1_GENB_R &= ~0x00000FFF; // p. 1285
    PWM0_1_GENB_R |= 0x80C; // p.1285: Use CMPB to create duty cycle
    PWM0_3_GENA_R &= ~0x00000FFF; // p. 1282
    PWM0_3_GENA_R |= 0x8C; // p.1282: Use CMPA to create duty cycle
    PWM0_3_GENB_R &= ~0x00000FFF; // p. 1285
    PWM0_3_GENB_R |= 0x80C; // p.1285: Use CMPB to create duty cycle

    // PWM Module 1 Generators
    PWM1_2_GENB_R &= ~0x00000FFF; // p. 1285
    PWM1_2_GENB_R |= 0x80C; // p.1285: Use CMPB to create duty cycle
    PWM1_1_GENA_R &= ~0x00000FFF; // p. 1282
    PWM1_1_GENA_R |= 0x8C; // p.1282: Use CMPA to create duty cycle
    PWM1_1_GENB_R &= ~0x00000FFF; // p. 1285
    PWM1_1_GENB_R |= 0x80C; // p.1285: Use CMPB to create duty cycle
    PWM1_3_GENA_R &= ~0x00000FFF; // p. 1282
    PWM1_3_GENA_R |= 0x8C; // p.1282: Use CMPA to create duty cycle
    PWM1_3_GENB_R &= ~0x00000FFF; // p. 1285
    PWM1_3_GENB_R |= 0x80C; // p.1285: Use CMPB to create duty cycle

    // PWM Module 0 Period
    PWM0_0_LOAD_R &= ~0xFFFF; // p. 1278
    PWM0_0_LOAD_R |= 0x186A; // p. 1278: Set the PWM to have a period of 20 milliseconds for a 50Hz signal
    PWM0_1_LOAD_R &= ~0xFFFF; // p. 1278
    PWM0_1_LOAD_R |= 0x186A; // p. 1278: Set the PWM to have a period of 20 milliseconds for a 50Hz signal
    PWM0_3_LOAD_R &= ~0xFFFF; // p. 1278
    PWM0_3_LOAD_R |= 0x186A; // p. 1278: Set the PWM to have a period of 20 milliseconds for a 50Hz signal

    // PWM Module 1 Period
    PWM1_2_LOAD_R &= ~0xFFFF; // p. 1278
    PWM1_2_LOAD_R |= 0x7D; // p. 1278: Set the PWM to have a period of 400 microseconds for a 25KHz signal
    PWM1_1_LOAD_R &= ~0xFFFF; // p. 1278
    PWM1_1_LOAD_R |= 0x7D; // p. 1278: Set the PWM to have a period of 400 microseconds for a 25KHz signal
    PWM1_3_LOAD_R &= ~0xFFFF; // p. 1278
    PWM1_3_LOAD_R |= 0x7D; // p. 1278: Set the PWM to have a period of 400 microseconds for a 25KHz signal

    // PWM Module 0: Set some initial comparator values to set the duty cycle on startup
    PWM0_0_CMPA_R &= ~0xFFFF; // p. 1280
    PWM0_0_CMPA_R |= 0x16A5; // p.1280:
    PWM0_1_CMPA_R &= ~0xFFFF; // p. 1280
    PWM0_1_CMPA_R |= 0x16A5; // p.1280:
    PWM0_1_CMPB_R &= ~0xFFFF; // p. 1281
    PWM0_1_CMPB_R |= 0x16A5; // p. 1281:
    PWM0_3_CMPA_R &= ~0xFFFF; // p. 1280
    PWM0_3_CMPA_R |= 0x16A5; // p.1280:
    PWM0_3_CMPB_R &= ~0xFFFF; // p. 1281
    PWM0_3_CMPB_R |= 0x16A5; // p. 1281:

    // PWM Module 1: Set some initial comparator values to set the duty cycle on startup
    PWM1_2_CMPB_R &= ~0xFFFF; // p. 1281
//    PWM1_2_CMPB_R |= 0x7C; // p. 1281:
    PWM1_1_CMPA_R &= ~0xFFFF; // p. 1280
//    PWM1_1_CMPA_R |= 0x7C; // p.1280:
    PWM1_1_CMPB_R &= ~0xFFFF; // p. 1281
//    PWM1_1_CMPB_R |= 0x7C; // p. 1281:
    PWM1_3_CMPA_R &= ~0xFFFF; // p. 1280
//    PWM1_3_CMPA_R |= 0x7C; // p.1280:
    PWM1_3_CMPB_R &= ~0xFFFF; // p. 1281
//    PWM1_3_CMPB_R |= 0x7C; // p. 1281:

    // Start the PWM Generators
    PWM0_0_CTL_R |= 0x1; // p. 1266: start the timers for the pwm0 generator 0
    PWM0_1_CTL_R |= 0x1; // p. 1266: start the timers for the pwm0 generator 1
    PWM0_3_CTL_R |= 0x1; // p. 1266: start the timers for the pwm0 generator 3
    PWM1_1_CTL_R |= 0x1; // p. 1266: start the timers for the pwm1 generator 1
    PWM1_2_CTL_R |= 0x1; // p. 1266: start the timers for the pwm1 generator 2
    PWM1_3_CTL_R |= 0x1; // p. 1266: start the timers for the pwm1 generator 3

    // Enable PWM Modules
    PWM0_ENABLE_R |= 0xCD; // p. 1247: enable the PWM outputs
    PWM1_ENABLE_R |= 0xEC;//0xCE; // p. 1247: enable the PWM outputs


    if (uart_sel == 0) {
        // 1. Enable the UART module using the RCGCUART register (see page 344).
        SYSCTL_RCGCUART_R |= 0x4;

        // 2. Enable the clock to the appropriate GPIO module via the RCGCGPIO register (see page 340).  To find out which GPIO port to enable, refer to Table 23-5 on page 1351.
        SYSCTL_RCGCGPIO_R |= 0x8;

        // 3. Set the GPIO AFSEL bits for the appropriate pins (see page 671). To determine which GPIOs to configure, see Table 23-4 on page 1344
        GPIO_PORTD_AFSEL_R |= 0xC0;

        // 4. Configure the GPIO current level and/or slew rate as specified for the mode selected.  See page 673 and page 681

        // 5. Configure the PMCn fields in the GPIOPCTL register to assign the UART signals to the appropriate pins (see page 688 and Table 23-5 on page 1351).
        GPIO_PORTD_PCTL_R &= ~0xFFFFFFFF;
        GPIO_PORTD_PCTL_R |= 0x11000000;

        GPIO_PORTD_DEN_R |= 0xC0;

        // Find  the Baud-Rate Divisor
        // BRD = 16,000,000 / (16 * 9600) = 104.1666666666666666
        // UART_FBRD[DIVFRAC] = integer(0.166667 * 64 + 0.5) = 11

        // BRD = 16,000,000 / (16 * 115200) = 8
        // UART_FBRD[DIVFRAC] = integer(0.6805555555556 * 64 + 0.5) = 44


        // With the BRD values in hand, the UART configuration is written to the module in the following order

        // 1. Disable the UART by clearing the UARTEN bit in the UARTCTL register
        UART2_CTL_R &= ~(1<<0);

        // 2. Write the integer portion of the BRD to the UARTIBRD register
        UART2_IBRD_R = 104;//11;//104;

        // 3. Write the fractional portion of the BRD to the UARTFBRD register.
        UART2_FBRD_R = 11;//44;//11;

        // 4. p.916:  Write the desired serial parameters to the UARTLCRH register (in this case, a value of 0x0000.0060)
        UART2_LCRH_R = 0x70;//(0x3<<5)|(1<<4); // 8-bit, no parity, 1-stop bit
    //    UART2_LCRH_R = 0x76; // 8-bit, parity enabled, even parity checking, 1-stop bit

        // 5. Configure the UART clock source by writing to the UARTCC register
        UART2_CC_R = 0x0;

        // 6. Optionally, configure the µDMA channel (see “Micro Direct Memory Access (µDMA)” on page 585) and enable the DMA option(s) in the UARTDMACTL register

        // 7. Enable the UART by setting the UARTEN bit in the UARTCTL register.
        UART2_CTL_R = 0x301; //(1<<0)|(1<<8)|(1<<9);
    }
    else if (uart_sel == 1) {
        // 1. Enable the UART module using the RCGCUART register (see page 344).
        SYSCTL_RCGCUART_R |= (1<<0);

        // 2. Enable the clock to the appropriate GPIO module via the RCGCGPIO register (see page 340).  To find out which GPIO port to enable, refer to Table 23-5 on page 1351.
        SYSCTL_RCGCGPIO_R |= (1<<0);

        // 3. Set the GPIO AFSEL bits for the appropriate pins (see page 671). To determine which GPIOs to configure, see Table 23-4 on page 1344
        GPIO_PORTA_AFSEL_R = (1<<1)|(1<<0);

        // 4. Configure the GPIO current level and/or slew rate as specified for the mode selected.  See page 673 and page 681

        // 5. Configure the PMCn fields in the GPIOPCTL register to assign the UART signals to the appropriate pins (see page 688 and Table 23-5 on page 1351).
        GPIO_PORTA_PCTL_R = (1<<0)|(1<<4);

        GPIO_PORTA_DEN_R = (1<<0)|(1<<1);

        // Find  the Baud-Rate Divisor
        // BRD = 16,000,000 / (16 * 9600) = 104.1666666666666666
        // UART_FBRD[DIVFRAC] = integer(0.166667 * 64 + 0.5) = 11


        // With the BRD values in hand, the UART configuration is written to the module in the following order

        // 1. Disable the UART by clearing the UARTEN bit in the UARTCTL register
        UART0_CTL_R &= ~(1<<0);

        // 2. Write the integer portion of the BRD to the UARTIBRD register
        UART0_IBRD_R = 104;

        // 3. Write the fractional portion of the BRD to the UARTFBRD register.
        UART0_FBRD_R = 11;

        // 4. Write the desired serial parameters to the UARTLCRH register (in this case, a value of 0x0000.0060)
        UART0_LCRH_R = 0x70;//(0x3<<5)|(1<<4); // 8-bit, no parity, 1-stop bit

        // 5. Configure the UART clock source by writing to the UARTCC register
        UART0_CC_R = 0x0;

        // 6. Optionally, configure the µDMA channel (see “Micro Direct Memory Access (µDMA)” on page 585) and enable the DMA option(s) in the UARTDMACTL register

        // 7. Enable the UART by setting the UARTEN bit in the UARTCTL register.
        UART0_CTL_R = (1<<0)|(1<<8)|(1<<9);
    }
}


void init_ADC (void) {

    volatile unsigned long delay_clk_2;
    SYSCTL_RCGCADC_R |= 0x01; // p. 352: ADC run mode clock gating control. Enables and provides clock for ADC Module 0 in RUN mode.
    delay_clk_2 = SYSCTL_RCGCADC_R; // delay for settling.

//    SYSCTL_RCGCGPIO_R |= 0x10; // p. 340: GPIO clock gating control. Enable clock gating for port E.
//    SYSCTL_RCGCGPIO_R &= ~0x2F; // p. 340: GPIO clock gating control. Disable clock gating for other ports.

    ADC0_ACTSS_R &= ~0x0F; // p. 821: ADC Active Sample Sequencer. Deactivate ADC SS3, ADC SS2, ADC SS1, & ADC SS0 --> &= ~0000_1111 during initialisation.

    ADC0_EMUX_R |= 0xFFFF; // p. 833: ADC Event Multiplexer. Sets bits [11:8] high to enable continuous sampling of EM3-EM0.
//    ADC0_EMUX_R &= ~0xFFFF; // p. 833: ADC Event Multiplexer.

    ADC0_SSMUX1_R &= ~0xFFFF; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Disables all of the available bits of SSMUX2.
    ADC0_SSMUX1_R |= 0x8; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Enables bit 0 of SSMUX2 to use inputs PE1 & PE0 on second sample of sequence.

    ADC0_SSMUX2_R &= ~0xFFFF; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Disables all of the available bits of SSMUX2.
    ADC0_SSMUX2_R |= 0x7; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Enables bit 0 of SSMUX2 to use inputs PE1 & PE0 on second sample of sequence.

    ADC0_SSMUX3_R &= ~0xFFFF; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Disables all of the available bits of SSMUX2.
    ADC0_SSMUX3_R |= 0x6; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Enables bit 0 of SSMUX2 to use inputs PE1 & PE0 on second sample of sequence.

    ADC0_SSMUX0_R &= ~0xFFFF; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Disables all of the available bits of SSMUX2.
    ADC0_SSMUX0_R |= 0x9; // p. 867 (details on p. 851): ADC Sample Sequencer Input Multiplex. Enables bit 0 of SSMUX2 to use inputs PE1 & PE0 on second sample of sequence.

    ADC0_SSCTL1_R |= 0x2; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.
    ADC0_SSCTL1_R &= ~0xD; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.

    ADC0_SSCTL2_R |= 0x2; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.
    ADC0_SSCTL2_R &= ~0xD; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.

    ADC0_SSCTL3_R |= 0x2; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.
    ADC0_SSCTL3_R &= ~0xD; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.

    ADC0_SSCTL0_R |= 0x2; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.
    ADC0_SSCTL0_R &= ~0xD; // p. 871: ADC Sample Sequence Control 2. Enables bit 0, meaning analog input is differentially sampled and enabling bit 1 on the second nibble, ending the sampling sequencing after the second sample.

    ADC0_SSPRI_R &= ~0x3333; // p. 841: ADC Sample Sequencer Priority. Sets all related bits high in SSPRI.
    ADC0_SSPRI_R |= 0x3201; // p. 841: ADC Sample Sequencer Priority. Ensures that SS3, then SS1, and then SS0 are set at lower priority than SS2, ie. SS2>SS3>SS1>SS0 for priority.

    ADC0_PC_R |= 0x7; // p. 891: ADC Peripheral Configuration. 0x7 selects 1 Msps as sample speed.  Alternatively, 0x1 will set sample speed at 125 ksps.
    ADC0_PC_R &= ~0x8; // p. 891: ADC Peripheral Configuration. Ensures that there is no other sampling speed selected.

    ADC0_SAC_R |= 0x6; // p. 847: ADC Sample Averaging Control. Average 64 samples in fifo. Samples AVG = 2^ADC0_SAC times per read. 6 is max, 0 is min.
    ADC0_SAC_R &= ~0xF; // p. 847: ADC Sample Averaging Control. Average 64 samples in fifo. Samples AVG = 2^ADC0_SAC times per read. 6 is max, 0 is min.

    ADC0_PSSI_R |= 0x0; // p. 845: ADC Sample Sequence Initiate. Sets bit 2&1 of PSSI high, (SS3 is trigger).
    ADC0_PSSI_R &= ~0xF; // p. 845: ADC Sample Sequence Initiate. Ensures that bits other than bit 2&1 of PSSI are set low in the first nibble.
//    ADC0_PSSI_R |= 0x1; // p. 845: ADC Sample Sequence Initiate. Sets bit 2&1 of PSSI high, (SS3 is trigger).

//    ADC0_ACTSS_R |= 0xF; // p. 821: ADC Active Sample Sequencer. Activate ADC sample sequencers (bit 2).
}


char readChar(void)
{
    char c;
    if (uart_sel == 0) {
        while((UART2_FR_R & (1<<4)) != 0);
        c = UART2_DR_R;
    }
    else if (uart_sel == 1) {
        while((UART0_FR_R & (1<<4)) != 0);
        c = UART0_DR_R;
    }
    return c;
}


void printChar(char c)
{
    if (uart_sel == 0) {
        while((UART2_FR_R & (1<<5)) != 0);
        UART2_DR_R = c;
    }
    else if (uart_sel == 1) {
        while((UART0_FR_R & (1<<5)) != 0);
        UART0_DR_R = c;
    }
}


void printString(char * string)
{
  while(*string)
  {
    printChar(*(string++));
  }
}


char* readString(char delimiter)
{

  int stringSize = 0;
  char* string = (char*)calloc(10,sizeof(char));
  char c = readChar();
  printChar(c);

  while(c!=delimiter)
  {

    *(string+stringSize) = c; // put the new character at the end of the array
    stringSize++;

    if((stringSize%10) == 0) // string length has reached multiple of 10
    {
      string = (char*)realloc(string,(stringSize+10)*sizeof(char)); // adjust string size by increasing size by another 10
    }

    c = readChar();
    printChar(c); // display the character the user typed
  }

  if(stringSize == 0)
  {

    return '\0'; // null car
  }
  return string;
}



void delay_us(unsigned int us) { // a blocking delay that is set with an  input of microseconds
    long unsigned i;
    unsigned int iterations;
    iterations = us * 2;
    for (i = 0; i < iterations; i++);
}





unsigned int delay_thing = 0;
unsigned int fan_data = 0x0000, servo_data = 0x0000;
unsigned int UART_error_flag = 0x0;
unsigned int ADC_read_0 = 0x000, ADC_read_1 = 0x000, ADC_read_2 = 0x000, ADC_read_3 = 0x000; // data read by the enabled ADC pins
int main(void) {
    init_gpio();
//    init_ADC();
    while(1) {
//        d = 'z';
//        c = 'Z';

//        printString("Enter 0-4(Servo) or 5-9(Fan) and 0-A(Position/Speed) :");
//        UART_error_flag = 0x0;
        d = readChar();
        c = readChar();
//        if (error_flag == 1) {
//            UART_error_flag = 0x1;
//        }
//        c = readChar();
//        if (error_flag == 1) {
//            UART_error_flag = 0x1;
//        }
//
//        if (UART_error_flag == 0x1) {
//            printChar('Z')
//        }


//        PWM0_ENABLE_R &= ~0xCD; // p. 1247: enable the PWM outputs
//        PWM1_ENABLE_R &= ~0xEC;//0xCE; // p. 1247: enable the PWM outputs
//        delay_us(100);

//        ADC0_ACTSS_R |= 0x01;
//        ADC0_RIS_R = 0x1;
////        ADC0_PSSI_R |= 0x1;
//        ADC_read_0 = ADC0_SSFIFO0_R; // bottom analog input
//        ADC0_ISC_R = 0x1;
//        ADC0_ACTSS_R &= ~0x1;
//
//        delay_us(100);
//
//        ADC0_ACTSS_R |= 0x02;
//        ADC0_RIS_R = 0x2;
////        ADC0_PSSI_R |= 0x2;
//        ADC_read_1 = ADC0_SSFIFO1_R; // bottom analog input
//        ADC0_ISC_R = 0x2;
//        ADC0_ACTSS_R &= ~0x2;
//
//        delay_us(100);

//        ADC0_ACTSS_R |= 0x04;
//        ADC0_RIS_R = 0x4;
////        ADC0_PSSI_R |= 0x4;
//        ADC_read_2 = ADC0_SSFIFO2_R; // bottom analog input
//        ADC0_ISC_R = 0x4;
//        ADC0_ACTSS_R &= ~0x4;
//
//        delay_us(100);
//
//        ADC0_ACTSS_R |= 0x08;
//        ADC0_RIS_R = 0x8;
////        ADC0_PSSI_R |= 0x8;
//        ADC_read_3 = ADC0_SSFIFO3_R; // bottom analog input
//        ADC0_ISC_R = 0x8;
//        ADC0_ACTSS_R &= ~0x8;
//
//        delay_us(100);


//        PWM0_ENABLE_R |= 0xCD; // p. 1247: enable the PWM outputs
//        PWM1_ENABLE_R |= 0xEC;//0xCE; // p. 1247: enable the PWM outputs
//        delay_us(100);

//        PWM0_0_CMPA_R = 0x17E0 - (ADC_read_0 >> 2);
//        delay_us(100);
//
//        PWM0_1_CMPA_R = 0x17E0 - (ADC_read_1 >> 2);
//        delay_us(100);
//
//        PWM0_1_CMPB_R = 0x17E0 - (ADC_read_2 >> 2);
//        delay_us(100);
//
//        PWM0_3_CMPA_R = 0x17E0 - (ADC_read_3 >> 2);
//        delay_us(100);

// if no error, send lowercase z
//        if ((UART2_RIS_R & 0x78) == 0x00) {
//            printChar('z');
//            UART2_ICR_R |= 0x780;
//        }

//        printChar(d);
//        printChar(c);
        //        volatile long i;
        //        for (i = 1; i < 16000;i++){}

//        printChar(c);
//        printString("\n\r");

        // Set fan_data & servo_data values based on
        switch(c) {
        case '0':
            servo_data = 0x1620; //maximum duty cycle
            fan_data = 0x00;
            break;
        case '1':
            servo_data = 0x163F; // 11.05%
            fan_data = 0x0C;
            break;
        case '2':
            servo_data = 0x166E; //10.1%
            fan_data = 0x19;
            break;
        case '3':
            servo_data = 0x169D; // 9.15%
            fan_data = 0x25;
            break;
        case '4':
            servo_data = 0x16CB; // 8.2%
            fan_data = 0x32;
            break;
        case '5':
            servo_data = 0x16F9; // 7.25%
            fan_data = 0x3E;
            break;
        case '6':
            servo_data = 0x1727; // 6.3%
            fan_data = 0x4B;
            break;
        case '7':
            servo_data = 0x1755; // 5.35%
            fan_data = 0x56;
            break;
        case '8':
            servo_data = 0x1783; // 4.4%
            fan_data = 0x63;
            break;
        case '9':
            servo_data = 0x17B1; // 3.45%
            fan_data = 0x6F;
            break;
        case 'A':
            servo_data = 0x17E0; // minimum duty cycle
            fan_data = 0x7C;
            break;
        }

        // Update PWM values here based on which piece of hardware needs to update
        switch(d) {
        case '0': // servo 0
            PWM0_0_CMPA_R = servo_data;
            break;
        case '1': // servo 1
            PWM0_1_CMPA_R = servo_data;
            break;
        case '2': // servo 2
            PWM0_1_CMPB_R = servo_data;
            break;
        case '3': // servo 3
            PWM0_3_CMPA_R = servo_data;
            break;
        case '4': // servo 4
            PWM0_3_CMPB_R = servo_data;
            break;
        case '5': // fan 0
            PWM1_1_CMPA_R = fan_data;
            break;
        case '6': // fan 1
            PWM1_1_CMPB_R = fan_data;
            break;
        case '7': // fan 2
            PWM1_2_CMPB_R = fan_data;
            break;
        case '8': // fan 3
            PWM1_3_CMPA_R = fan_data;
            break;
        case '9': // fan 4
            PWM1_3_CMPB_R = fan_data;
            break;

        }

    }
}
