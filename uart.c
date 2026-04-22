/*
 *
 *   uart-interrupt.c
 *
 *
 *
 *   @author
 *   @date
 */

#include <inc/tm4c123gh6pm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "uart.h"

// These variables are declared as examples for your use in the interrupt handler.
volatile char command_byte = -1; // byte value for special character used as a command
volatile int command_flag = 0; // flag to tell the main program a special command was received

void uart_init(void)
{
    //TODO
    //enable clock to GPIO port B
    SYSCTL_RCGCGPIO_R |= 0b000010;

    //enable clock to UART1
    SYSCTL_RCGCUART_R |= 0b000010;

    //wait for GPIOB and UART1 peripherals to be ready
    while ((SYSCTL_PRGPIO_R & 0b000010) == 0)
    {
    };
    while ((SYSCTL_PRUART_R & 0b000010) == 0)
    {
    };

    //enable digital functionality on port B pins
    GPIO_PORTB_DEN_R |= 0b000011;

    //enable alternate functions on port B pins
    GPIO_PORTB_AFSEL_R |= 0b000011;

    //enable UART1 Rx and Tx on port B pins
    GPIO_PORTB_PCTL_R = (GPIO_PORTB_PCTL_R & ~0xee) | 0x11;

    //calculate baud rate
#define BAUD_RATE 115200.0
#define CLOCK_DIV 16.0
#define BRD (16000000.0 / (CLOCK_DIV * BAUD_RATE))

    uint16_t iBRD = (uint16_t) BRD; //use equations
    uint16_t fBRD = (uint16_t) ((BRD - iBRD) * 64 + 0.5); //use equations

    //turn off UART1 while setting it up
    UART1_CTL_R &= ~0b01;

    //set baud rate
    //note: to take effect, there must be a write to LCRH after these assignments
    UART1_IBRD_R = iBRD;
    UART1_FBRD_R = fBRD;

    //set frame, 8 data bits, 1 stop bit, no parity, no FIFO
    //note: this write to LCRH must be after the BRD assignments
    UART1_LCRH_R = 0b01100000;

    //use system clock as source
    //note from the datasheet UARTCCC register description:
    //field is 0 (system clock) by default on reset
    //Good to be explicit in your code
    UART1_CC_R = 0x0;

    //////Enable interrupts

    //first clear RX interrupt flag (clear by writing 1 to ICR)
    UART1_ICR_R |= 0x10;

    //enable RX raw interrupts in interrupt mask register
    UART1_IM_R |= 0x10;

    //NVIC setup: set priority of UART1 interrupt to 1 in bits 21-23
    NVIC_PRI1_R = (NVIC_PRI1_R & 0xFF0FFFFF) | 0x00200000;

    //NVIC setup: enable interrupt for UART1, IRQ #6, set bit 6
    NVIC_EN0_R |= 0b01000000;

    //tell CPU to use ISR handler for UART1 (see interrupt.h file)
    //from system header file: #define INT_UART1 22
    IntRegister(INT_UART1, UART1_Handler);

    //globally allow CPU to service interrupts (see interrupt.h file)
    IntMasterEnable();

    //re-enable UART1 and also enable RX, TX (three bits)
    //note from the datasheet UARTCTL register description:
    //RX and TX are enabled by default on reset
    //Good to be explicit in your code
    //Be careful to not clear RX and TX enable bits
    //(either preserve if already set or set them)
    UART1_CTL_R |= 0b1100000001;

}

void uart_sendChar(char data)
{
    if (data == '\n')
    {
        while (UART1_FR_R & 0x08)
            ;
        UART1_DR_R = '\r';
    }
//    else if (data == '\r')
//    {
//        while (UART1_FR_R & 0x08)
//            ;
//        UART1_DR_R = '\n';
//    }
    while (UART1_FR_R & 0x08)
        ;
    UART1_DR_R = data;
}

//char uart_receive(void)
//{
//    while (UART1_FR_R & 0x10)
//        ;
//
//    return UART1_DR_R;
//}
//
//char uart_recieve_nonblocking(void)
//{
//    if (UART1_FR_R & 0x10)
//        return UART1_DR_R;
//    else
//        return 0;
//}

//char uart_

void uart_sendStr(const char *str)
{
    int i;
    for (i = 0; str[i] != 0; ++i)
    {
        uart_sendChar(str[i]);
    }
    // don't forget null byte
    uart_sendChar(0);
}

int uart_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int o = uart_vprintf(format, args);
    va_end(args);
    return o;
}

int uart_vprintf(const char *format, va_list args)
{
    char *str;
    vasprintf(&str, format, args);
    if (!str)
    {
        // allocation failure
        return -1;
    }
    uart_sendStr(str);
    free(str);
    return 0;
}

// Interrupt handler for receive interrupts
void UART1_Handler(void)
{
    //check if handler called due to RX event
    if (UART1_MIS_R & 0x10)
    {
        //byte was received in the UART data register
        //clear the RX trigger flag (clear by writing 1 to ICR)
        UART1_ICR_R |= 0x10;

        //read the byte received from UART1_DR_R and echo it back to PuTTY
        //ignore the error bits in UART1_DR_R
        char byte_received = UART1_DR_R & 0xff;
        uart_sendChar(byte_received);

        switch (byte_received)
        {
        case '\r':
            //send a newline character back to PuTTY
            uart_sendChar('\n');
            break;
        case CMD_SWEEP_IR:
        case CMD_SERVO_CAL:
        case CMD_ADC_RAW:
        case CMD_ADC_DIST:
        case CMD_CANCEL:
            command_byte = byte_received;
            command_flag = 1;
            break;
        default:
            break;
        }
        //if byte received is a carriage return
//        if (byte_received == '\r')
//        {
//            //send a newline character back to PuTTY
//            uart_sendChar('\n');
//        }
//        else
//        {
//            //AS NEEDED
//            //code to handle any other special characters
//            //code to update global shared variables
//            //DO NOT PUT TIME-CONSUMING CODE IN AN ISR
//
//            if (byte_received == command_byte)
//            {
//                command_flag = 1;
//            }
//        }
    }
}
