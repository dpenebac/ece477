#include "stm32f0xx.h"
#include "finger.h"
#include <stdio.h>

void init_usart_1(){
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; //Enable GPIO B
    GPIOB->MODER |= (2 << 12); //Set PB6 to alternate function mode for TX
    GPIOB->MODER |= (2 << 14); //Set PB7 to alternate function mode for RX
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; //Enable USART1 peripheral
    USART1->CR1 &= ~USART_CR1_UE; //Disable USART1
    USART1->CR1 &= ~((1 << 28) | (1 << 12)); //Set word length to 8 bits
    USART1->CR1 &= ~(USART_CR1_PCE); //Disable parity bit
    USART1->CR2 &= ~(USART_CR2_STOP); //Use 1 stop bit
    USART1->CR1 &= ~(USART_CR1_OVER8); //Use oversampling by 16
    USART1->BRR &= ~0xFFFF; //Clear baud rate
    USART1->BRR |= 0x9C4; //Set baud rate to 19200bps
    USART1->CR1 |= USART_CR1_TE; //Enable TX
    USART1->CR1 |= USART_CR1_RE; //Enable RX
    USART1->CR1 |= USART_CR1_UE; //Enable USART
    while((USART1->ISR & USART_ISR_TEACK) == 0 && (USART1->ISR & USART_ISR_REACK) == 0); //Wait for USART1 to be ready
}

uint8_t * finger_check_user(uint8_t UID){
    uint8_t check_command[8] = {0xF5, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xF5}; //
    uint8_t * response = finger_send_command(check_command);
    return response;
}

uint8_t * finger_send_command(uint8_t command[8]){
    static uint8_t response[8];
    for(int i = 0; i < 8; i++) {
        while(!(USART1->ISR & USART_ISR_TXE));
        USART1->TDR = command[i]; //Send next byte
    }
    for(int i = 0; i < 8; i++) {
        while(!(USART1->ISR & USART_ISR_RXNE));
        response[i] = USART1->RDR; //Recieve next byte
    }
    return response;
}

void finger_add_user(uint8_t UID){
    uint8_t add_user_command1[8] = {0xF5, 0x01, 0x00, UID, 0x01, 0x00, 0x01, 0xF5}; //Command 1 to add user
    uint8_t add_user_command2[8] = {0xF5, 0x02, 0x00, UID, 0x01, 0x00, 0x02, 0xF5}; //Command 2 to add user
    uint8_t add_user_command3[8] = {0xF5, 0x03, 0x00, UID, 0x01, 0x00, 0x03, 0xF5}; //Command 3 to add user
    while(!(GPIOB->IDR & (1 << 2))); //Wait for button press
    finger_send_command(add_user_command1); //Send command 1
    while(!(GPIOB->IDR & (1 << 2))); //Wait for button press
    finger_send_command(add_user_command2); //Send command 2
    while(!(GPIOB->IDR & (1 << 2))); //Wait for button press
    finger_send_command(add_user_command3); //Send command 3
}

void init_test(){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //Enable GPIO C
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; //Enable GPIO B
    GPIOC->MODER |= (1 << 12); //Set PC6 for general output
    GPIOC->MODER |= (1 << 14); //Set PC7 for general output
    GPIOB->PUPDR |= (1 << 5); //Pull down resister on PB2
    uint8_t clear_user_command[8] = {0xF5, 0x05, 0x00, 0x00, 0x00, 0x00, 0x05, 0xF5}; //Command to clear users
    finger_send_command(clear_user_command); //Send command to clear users
}

void finger_test(){
    finger_add_user(1);
    while(1){
        while(!(GPIOB->IDR & (1 << 2))); //Wait for button press
        uint8_t * response = finger_check_user(1); //
        GPIOC->ODR &= ~(3 << 6);
        if(response[3] == 1){
            GPIOC->ODR |= (1 << 7);
        }else{
            GPIOC->ODR |= (1 << 6);
        }
    }
}
