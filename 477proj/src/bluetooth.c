#include "stm32f0xx.h"
#include <stddef.h>

#include <bluetooth.h>

void init_usart5(void) {

    /*
    Enable the RCC clocks to GPIOC and GPIOD.

    Do all the steps necessary to configure pin PC12 to be routed to USART5_TX.

    Do all the steps necessary to configure pin PD2 to be routed to USART5_RX.

    Enable the RCC clock to the USART5 peripheral.
    */

    RCC->AHBENR |= RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIODEN;

    /*
    RX : PC12, AF2 : DIO 0
    TX : PD2,  AF2 : DIO 1
     */

    //10 alternate function
    GPIOC->MODER |= 2 << (12 * 2);
    GPIOD->MODER |= 2 << (2 * 2);

    GPIOC->AFR[1] |= 2 << (4 * 4);
    GPIOD->AFR[0] |= 2 << (2 * 4);

    RCC->APB1ENR |= RCC_APB1ENR_USART5EN;

    /*
    Configure USART5 as follows:
    (First, disable it by turning off its UE bit.)

    Set a word size of 8 bits.

    Set it for one stop bit.

    Set it for no parity.

    Use 16x oversampling.

    Use a baud rate of 115200 (115.2 kbaud). Refer to table 96 of the Family Reference Manual,
    or simply divide the system clock rate by 115200.

    Enable the transmitter and the receiver by setting the TE and RE bits.

    Enable the USART.

    Finally, you should wait for the TE and RE bits to be acknowledged by checking that TEACK
    and REACK bits are both set in the ISR. This indicates that the USART is ready to transmit and receive.

    */

    USART5->CR1 &= ~(USART_CR1_UE);

    USART5->CR1 &= ~((1 << 28) | (1 << 12));

    USART5->CR2 &= ~(3 << 12);

    USART5->CR1 &= ~(USART_CR1_PCE);

    USART5->CR1 &= ~(USART_CR1_OVER8);

    // to set to 9600, brr == 1388
    USART5->BRR |= 0x1388;

    USART5->CR1 |= USART_CR1_TE | USART_CR1_RE;

    USART5->CR1 |= USART_CR1_UE;

    while ( USART_ISR_TEACK != (USART5->ISR & USART_ISR_TEACK) &&
            USART_ISR_REACK != (USART5->ISR & USART_ISR_REACK)
    );

    return;
}

int main_bluetooth(void) // bluetooth
{

    // PC12 HM19:RXD STM32:TX
    // PD2  HM19:TXD STM32:RX

    // F0:5E:CD:E2:21:B4

    init_usart5();

    char *c = "dorienhatesthis";
    // AT+ADDR? check mac addr, useful for pi but pretty useless without connection
    // AT+CONNL, try to connect to last succeeded device
    // AT+RENEW, factory reset

    int i;
    int f;
    i = 0;
    f = 0;

    while (f < 5000) {
        while(!(USART5->ISR & USART_ISR_TXE)) { }
        USART5->TDR = c[i];
        i++;
        if (i > 12) {
            i = 0;
        }
        f++;
    }

    return 0;
}
