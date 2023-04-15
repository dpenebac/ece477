#include "stm32f0xx.h"

int main_keypad_lcd(void);

extern uint16_t msg[8];

// helper
void nano_wait(unsigned int n);

// game
void game();
void init_spi2();
void spi2_setup_dma();
void spi2_enable_dma();
void init_spi1();
void spi1_init_oled();
void spi1_setup_dma();
void spi1_enable_dma();
char get_keypress();

// main
void enable_ports();
void init_tim7();
void spi1_dma_display1(const char *);
void spi1_dma_display2(char *);
