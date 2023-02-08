/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "stm32f0xx.h"
#include "finger.h"

void init_all(){
    init_usart_1();
}

void enter_low_power(){

}

void exit_low_power(){

}

void EXTI2_3_IRQHandler(){
    EXTI->PR = (1 << 2);
    uint8_t * response = finger_check_user(1);
    if(response[4] == 0x00){
        GPIOC->ODR |= (1 << 6);
    }
}

int main(void)
{
	init_usart_1();
	init_test();
	finger_test();
}
