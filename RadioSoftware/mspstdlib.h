/*
 * mspstdlib.h
 *
 *  Created on: Dec 26, 2023
 *      Author: merri
 */

/* USCI info:
 * -2x USCIA
 * -2x USCIB
 *
 * USCIA supports UART and SPI
 * USCIB supports I2C and SPI
 */

#ifndef MSPSTDLIB_H_
#define MSPSTDLIB_H_

enum Serial_Channel {
    UCA0,
    UCA1,
    UCB0,
    UCB1
};

void software_trim(); //TI clock trim code
void init_clock(); //TI clock init code

void init_UART(enum Serial_Channel channel, unsigned long baud); //configure serial channel for UART
void UART_putchar(enum Serial_Channel channel, char c); //transmit char over UART channel
void UART_putchars(enum Serial_Channel channel, char* msg); //transmit string over UART

void print_hex(enum Serial_Channel channel, char h);

void init_SPI(enum Serial_Channel channel); //configure serial channel for SPI

void init_I2C(enum Serial_Channel channel); //configure serial channel for I2C
unsigned char I2C_read(enum Serial_Channel channel, unsigned char device_addr, unsigned char register_addr); //read register data over I2C
unsigned char I2C_write(enum Serial_Channel channel, unsigned char device_addr, unsigned char register_addr); //write register data over I2C

#endif /* MSPSTDLIB_H_ */
