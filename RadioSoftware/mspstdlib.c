/*
 * mspstdlib.c
 *
 *  Created on: Dec 26, 2023
 *      Author: merri
 */

#include <mspstdlib.h>

#include <msp430.h>

#define MCLK_FREQ_MHZ 8

/* USCI A1 vector */
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR(void) {
    switch(__even_in_range(UCA1IV,USCI_UART_UCTXCPTIFG)) {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:break;
        case USCI_UART_UCTXIFG: break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

/* USCI A0 vector */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG)) {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:break;
        case USCI_UART_UCTXIFG: break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

/* This function is from the TI MSP430 example code. */
void software_trim() {
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do
    {
        CSCTL0 = 0x100;                         // DCO Tap = 256
        do
        {
            CSCTL7 &= ~DCOFFG;                  // Clear DCO fault flag
        }while (CSCTL7 & DCOFFG);               // Test DCO fault flag

        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ);// Wait FLL lock status (FLLUNLOCK) to be stable
                                                           // Suggest to wait 24 cycles of divided FLL reference clock
        while((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0));

        csCtl0Read = CSCTL0;                   // Read CSCTL0
        csCtl1Read = CSCTL1;                   // Read CSCTL1

        oldDcoTap = newDcoTap;                 // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;       // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070)>>4;// Get DCOFTRIM value

        if(newDcoTap < 256)                    // DCOTAP < 256
        {
            newDcoDelta = 256 - newDcoTap;     // Delta value between DCPTAP and 256
            if((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }
        else                                   // DCOTAP >= 256
        {
            newDcoDelta = newDcoTap - 256;     // Delta value between DCPTAP and 256
            if(oldDcoTap < 256)                // DCOTAP cross 256
                endLoop = 1;                   // Stop while loop
            else
            {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim<<4);
            }
        }

        if(newDcoDelta < bestDcoDelta)         // Record DCOTAP closest to 256
        {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    }while(endLoop == 0);                      // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy;                       // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy;                       // Reload locked DCOFTRIM
    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked
}

/* This function is from the TI MSP430 example code. */
void init_clock() {
    __bis_SR_register(SCG0);                 // disable FLL
    CSCTL3 |= SELREF__REFOCLK;               // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3;// DCOFTRIM=3, DCO Range = 8MHz
    CSCTL2 = FLLD_0 + 243;                  // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0);                // enable FLL
    software_trim();                        // Software Trim to get the best DCOFTRIM value

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                             // default DCODIV as MCLK and SMCLK source
}

void init_UART(enum Serial_Channel channel, unsigned long baud) {
    /* pointers to control registers */
    volatile unsigned int* x_CTLW0;
    volatile unsigned char* x_BR0;
    volatile unsigned char* x_BR1;
    volatile unsigned int* x_MCTLW;
    volatile unsigned int* x_IE;

    switch (channel) {
    case UCA0:
        P1SEL0 |= BIT6 | BIT7; //Configure UART pins

        x_CTLW0 = &UCA0CTLW0;
        x_BR0 = &UCA0BR0;
        x_BR1 = &UCA0BR1;
        x_MCTLW = &UCA0MCTLW;
        x_IE = &UCA0IE;
        break;
    case UCA1:
        P4SEL0 |= BIT2 | BIT3; //Configure UART pins

        x_CTLW0 = &UCA1CTLW0;
        x_BR0 = &UCA1BR0;
        x_BR1 = &UCA1BR1;
        x_MCTLW = &UCA1MCTLW;
        x_IE = &UCA1IE;
        break;
    default:
        return; //only UCAx supports UART
        break;
    }

    //configure clock source
    *x_CTLW0 |= UCSWRST | UCSSEL__SMCLK;

    switch (baud) {
    case 115200:
        *x_BR0 = 4;
        *x_BR1 = 0x00;
        *x_MCTLW = 0x5500 | UCOS16 | UCBRF_5;
        break;
    default: //default to 9600 baud
        *x_BR0 = 52; //8000000/16/9600
        *x_BR1 = 0x00;
        *x_MCTLW = 0x4900 | UCOS16 | UCBRF_1;
        break;
    }

    *x_CTLW0 &= ~UCSWRST; //initialize eUSCI
    *x_IE |= UCRXIE; //Enable USCI_A0 RX interrupt
}

void UART_putchar(enum Serial_Channel channel, char c) {
    /* pointers to UART registers */
    volatile unsigned int* x_TXBUF;
    volatile unsigned int* x_IFG;

    switch (channel) {
    case UCA0:
        x_TXBUF = &UCA0TXBUF;
        x_IFG = &UCA0IFG;
        break;
    case UCA1:
        x_TXBUF = &UCA1TXBUF;
        x_IFG = &UCA1IFG;
        break;
    default:
        return;
        break;
    }

    while ((*x_IFG & UCTXIFG) == 0); //wait until buffer is clear
    *x_TXBUF = c; //transmit data
}

void UART_putchars(enum Serial_Channel channel, char* msg) {
    unsigned int i = 0;

    while (msg[i] != '\0') {
        UART_putchar(channel, msg[i]);
        i++;
    }
}

void print_hex(enum Serial_Channel channel, char h) {
    int i;
    char nibble;
    for (i = 1; i >= 0; i--) {
        nibble = ((h >> (4 * i)) & 0x0F);
        if (nibble < 10) { //decimal number
            UART_putchar(channel, nibble + 48);
        }
        else { //letter
            UART_putchar(channel, nibble + 55);
        }
    }
}

void init_I2C(enum Serial_Channel channel) {
    volatile unsigned int* x_CTLW0;
    volatile unsigned char* x_BR0;
    volatile unsigned char* x_BR1;

    /* select registers based on channel */
    switch (channel) {
    case UCB0:
        P1SEL0 |= (BIT2 | BIT3);
        P1SEL1 |= BIT2 | BIT3; //secondary pin function select

        x_CTLW0 = &UCB0CTLW0;
        x_BR0 = &UCB0BR0;
        x_BR1 = &UCB0BR1;
        break;
    case UCB1:
        P4SEL0 |= (BIT6 | BIT7);
        P4SEL1 |= BIT6 | BIT7;

        x_CTLW0 = &UCB1CTLW0;
        x_BR0 = &UCB1BR0;
        x_BR1 = &UCB1BR1;
        break;
    default:
        return;
        break;
    }

    *x_CTLW0 = UCSWRST; //put into software reset mode to allow programming
    *x_CTLW0 = UCMST | UCMODE_3 | UCSYNC | UCSSEL_2; //master mode, I2C mode, SMCLK source

    //I2C clock rate
    *x_BR0 = 80; //(8MHz / 80 = 100 kHz)
    *x_BR1 = 0x00;

    *x_CTLW0 &= ~UCSWRST; //take out of reset mode and enable channel
}

unsigned char I2C_read(enum Serial_Channel channel, unsigned char device_addr, unsigned char register_addr) {
    volatile unsigned int* x_I2CSA;
    volatile unsigned int* x_CTLW0;
    volatile unsigned int* x_IFG;
    volatile unsigned int* x_TXBUF;
    volatile unsigned int* x_RXBUF;

    unsigned char r;

    switch (channel) {
    case UCB0:
        x_I2CSA = &UCB0I2CSA;
        x_CTLW0 = &UCB0CTLW0;
        x_IFG = &UCB0IFG;
        x_TXBUF = &UCB0TXBUF;
        x_RXBUF = &UCB0RXBUF;
        break;
    case UCB1:
        x_I2CSA = &UCB1I2CSA;
        x_CTLW0 = &UCB1CTLW0;
        x_IFG = &UCB1IFG;
        x_TXBUF = &UCB1TXBUF;
        x_RXBUF = &UCB1RXBUF;
        break;
    default:
        return 0x00;
        break;
    }

    *x_I2CSA = device_addr; //set slave address

    /* address write */
    *x_CTLW0 |= UCTR | UCTXSTT; //set to I2C transmit mode and send start flag
    while ((*x_CTLW0 & UCTXSTT) == 1 && (*x_IFG & UCTXIFG0 ) == 0); //wait for transmit to be over

    *x_TXBUF = register_addr; //transmit register address
    while ((*x_IFG & UCTXIFG0) == 0); //wait for end of transmit

    *x_CTLW0 |= UCTR | UCTXSTP; //send stop flag in transmit mode

    /* data read */
    *x_CTLW0 &= ~UCTR; //set to read mode
    *x_CTLW0 |= UCTXSTT; //send start flag
    while ((*x_CTLW0 & UCTXSTT) == 1); //wait for start condition to be over

    while ((*x_IFG & UCRXIFG0) == 0); //wait for RX
    r = *x_RXBUF;
    *x_CTLW0 |= UCTXSTP; //send stop flag

    return r;
}

unsigned char I2C_write(enum Serial_Channel channel, unsigned char device_addr, unsigned char register_addr) {
    return 0;
}
