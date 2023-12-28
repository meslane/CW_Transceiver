/**
 * main.c
 */
#include <msp430.h>

#include <mspstdlib.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   //stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    software_trim();
    init_clock();

    init_UART(UCA1, 9600); //this is the USB UART
    init_I2C(UCB0);

    while (1) {
        //UART_putchars(UCA1, "WHAT HATH GOD WROUGHT?\n\r");
        UART_putchars(UCA1, "\n\r");
        print_hex(UCA1, I2C_read(UCB0, 0x60, 0xB7));
        UART_putchars(UCA1, "\n\r");
    }

    return 0;
}
