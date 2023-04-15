#include "stm32f0xx.h"
#include <wakeup.h>

void init_wakeup(){
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN; //Enable GPIO C
    GPIOC->MODER |= (1 << 18); //Set PC9 for general output
    GPIOC->PUPDR |= (1 << 18); //Pull up resister on PC9
    GPIOC->ODR |= (1 << 9); //Set PC9 HIGH
}

void enter_low_power(){
    RCC->APB1ENR |= RCC_APB1ENR_PWREN; //Enable power clock
    PWR->CSR |= PWR_CSR_EWUP5; //Enable wakeup on wakeup pin 5, which is PC5
    PWR->CR |= PWR_CR_PDDS; //Enable standby mode on deepsleep
    PWR->CR |= PWR_CR_CWUF; //Clear the wake up flag
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;   //Enables deepsleep low power mode
    SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk; //Enter low power mode on sleep

    __WFI(); //Enter sleep mode
}
