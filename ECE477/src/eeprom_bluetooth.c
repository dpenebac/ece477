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
#include <stddef.h>
#include "stm32f0xx.h"

#define EEPROM_ADDR 0x50

//===========================================================================
// 2.2 I2C helpers
//===========================================================================

void init_i2c(void) {

    /*
    Write a C subroutine named init_i2c() that configures PB6 and PB7 to be, respectively,
    SCL and SDA of the I2C1 peripheral.
    Enable the RCC clock to GPIO Port B and the I2C1 channel,
    set the MODER fields for PB6 and PB7,
    and set the alternate function register entries.
    */

    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    GPIOB->MODER &= ~(
            GPIO_MODER_MODER6 |
            GPIO_MODER_MODER7
    );

    GPIOB->MODER |= (
            GPIO_MODER_MODER6_1 |
            GPIO_MODER_MODER7_1
    );

    GPIOB->AFR[0] &= ~(
            GPIO_AFRL_AFR6 |
            GPIO_AFRL_AFR7
    );

    GPIOB->AFR[0] |= (
            (1 << (4 * 6)) |
            (1 << (4 * 7))
    ); //setting 6 and 7 to be af1

    /*
    Then configure the I2C1 channel as follows:
    First, disable the PE bit in CR1 before making the following configuration changes.
    Turn off the ANFOFF pubit (turn on the analog noise filter).
    Disable the error interrupt.
    Turn off the NOSTRETCH bit in CR1 to enable clock stretching.
    Set the TIMINGR register as follows: (Note that these are configurations found on Table 83 of the Family Reference Manual. Why is I2C1's clock 8 MHz? See Figure 209 of the Reference Manual for a hint).
    Set the prescaler to 0.
    Set the SCLDEL field to 3.
    Set the SDADEL field to 1.
    Set the SCLH field to 3.
    Set the SCLL field to 9.
    Disable both of the "own addresses", OAR1 and OAR2.
    Configure the ADD10 field of CR2 for 7-bit mode.
    Turn on the AUTOEND setting to enable automatic end.
    Enable the channel by setting the PE bit in CR1.
     */

    I2C1->CR1 &= ~I2C_CR1_PE;
    I2C1->CR1 &= ~I2C_CR1_ANFOFF;
    I2C1->CR1 &= ~I2C_CR1_ERRIE;
    I2C1->CR1 &= ~I2C_CR1_NOSTRETCH;

    I2C1->TIMINGR = 0;
    I2C1->TIMINGR &= ~I2C_TIMINGR_PRESC; //clear prescaler
    I2C1->TIMINGR |= 0 << 28; //set prescaler to 0
    I2C1->TIMINGR |= 3 << 20;
    I2C1->TIMINGR |= 1 << 16;
    I2C1->TIMINGR |= 3 << 8;
    I2C1->TIMINGR |= 9 << 0;

    I2C1->OAR1 &= ~I2C_OAR1_OA1EN;
    I2C1->OAR2 &= ~I2C_OAR2_OA2EN;

    //I2C1->OAR1 = I2C_OAR1_OA1EN | 0x2;

    I2C1->CR2 &= ~I2C_CR2_ADD10; //set to 0 for 7 bit mode
    I2C1->CR2 |= I2C_CR2_AUTOEND;
    I2C1->CR1 |= I2C_CR1_PE;

    return;
}

void i2c_waitidle(void) {
    while ((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY);
    return;
}

void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir) {
    uint32_t tmpreg = I2C1->CR2;

    tmpreg &= ~(
            I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD |
            I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START |
            I2C_CR2_STOP
    );

    if (dir == 1)
        tmpreg |= I2C_CR2_RD_WRN; //read from slave
    else
        tmpreg &= ~(I2C_CR2_RD_WRN); //write to slave

    tmpreg |= ((devaddr << 1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES);
    tmpreg |= I2C_CR2_START;
    I2C1->CR2 = tmpreg;

    return;
}

void i2c_stop(void) {
    if (I2C1->ISR & I2C_ISR_STOPF) {
        return;
    }

    I2C1->CR2 |= I2C_CR2_STOP;

    while ((I2C1->ISR & I2C_ISR_STOPF) == 0);

    I2C1->ICR |= I2C_ICR_STOPCF;

    return;
}

int i2c_checknack(void) {
    if ((I2C1->ISR & I2C_ISR_NACKF) == I2C_ISR_NACKF){
        return 1;
    }

    return 0;
}

void i2c_clearnack(void) {
    I2C1->ICR |= I2C_ICR_NACKCF;
}

int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size) {
    int i;

    if (size <= 0 || data == 0) {
        return -1;
    }

    uint8_t * udata = (uint8_t*) data;
    i2c_waitidle();
    i2c_start(devaddr, size, 0);

    for (i = 0; i < size; i++) {

        int count = 0;
        while ((I2C1->ISR & I2C_ISR_TXIS) == 0) {
            count += 1;
            if (count > 1000000) return -1;
            if (i2c_checknack()) { i2c_clearnack(); i2c_stop(); return -1; }
        }

        I2C1->TXDR = udata[i] & I2C_TXDR_TXDATA;
    }

    while ((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);

    if ((I2C1->ISR & I2C_ISR_NACKF) != 0)
        return -1;

    i2c_stop();

    return 0;
}

int i2c_recvdata(uint8_t devaddr, void *data, uint8_t size) {

    int i;
    if (size <= 0 || data == 0) return -1;

    uint8_t *udata = (uint8_t*) data;

    i2c_waitidle();

    i2c_start(devaddr, size, 1);

    for (i = 0; i < size; i++) {
        int count = 0;

        while ((I2C1->ISR & I2C_ISR_RXNE) == 0) {
            count += 1;
            if (count > 1000000) return -1;
            if (i2c_checknack()) { i2c_clearnack(); i2c_stop(); return -1; }
        }

        udata[i] = I2C1->RXDR;
    }

    while ((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);

    if ((I2C1->ISR & I2C_ISR_NACKF) != 0)
        return -1;

    i2c_stop();

    return 0;

}

//===========================================================================
// 2.4 EEPROM functions
//===========================================================================
void eeprom_write(uint16_t loc, const char* data, uint8_t len) {
    //int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size)
    //i2c_senddata(EEPROM_ADDR, data, len);

    /*
    Since writes will be no longer than 32 bytes, we recommend you create an array of 34 bytes in this subroutine.
    In the first two bytes, put the decomposed 12-bit address.
    Copy the rest of the data into bytes 2 through 33.
    Invoke i2c_senddata() with a length that is the data size plus 2.
     */

    //loc == 0x0234
    //tmp[0] = 02
    //tmp[1] = 34

    char tmp[32];
    tmp[0] = (loc >> 8) & 0x0f;
    tmp[1] = loc & 0xff;

    int i;
    for (i = 2; i < 31; i++) {
        tmp[i] = data[i - 2];
    }

    i2c_senddata(EEPROM_ADDR, tmp, len + 2);

    return;

}

/*
Wait for the I2C channel to be idle.
Initiate an i2c_start() with the correct I2C EEPROM device ID, zero length, and write-intent.
Wait until either the TC flag or NACKF flag is set.
If the NACKF flag is set, clear it, invoke i2c_stop() and return 0.
If the NACKF flag is not set, invoke i2c_stop() and return 1.
 */

int eeprom_write_complete(void) {
    i2c_waitidle();

    i2c_start(EEPROM_ADDR,0,0); //0 is to write

    while ((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);

    if (i2c_checknack()) {
        i2c_clearnack();
        i2c_stop();
        return 0;
    }
    else {
        i2c_stop();
        return 1;
    }
}

void eeprom_read(uint16_t loc, char data[], uint8_t len) {

    char tmp[32];
    tmp[0] = (loc >> 8) & 0x0f;
    tmp[1] = loc & 0xff;

    i2c_senddata(EEPROM_ADDR,tmp,2);
    i2c_recvdata(EEPROM_ADDR,data,len);

    return;
}


void eeprom_blocking_write(uint16_t loc, const char* data, uint8_t len) {
    eeprom_write(loc, data, len);
    while (!eeprom_write_complete());
}

void update_users(void);
void format_users(char []);
void read_users();
void unformat_users(char []);
void reset_users(void);
void zero_users(void);


// Structure to hold fingerprint and passcode information
struct user {
    uint8_t fingerprint_id;
    uint8_t passcode[5];
};

struct user user_list[5];

// Test function to set user_list of users all to zero to test functionality of i2c read
void zero_users(void) {
    int i;
    int j;

    for(i = 0; i < 5; i++) {
        user_list[i].fingerprint_id = 0;
        for (j = 0; j < 5; j++) {
            user_list[i].passcode[j] = 0;
        }
    }
}

// Test function to set user_list to some for loop logic based values to test functionality of i2c write
void reset_users(void) {

    int i;
    int j;

    for(i = 0; i < 5; i++) {
        user_list[i].fingerprint_id = i;
        for (j = 0; j < 5; j++) {
            user_list[i].passcode[j] = i + j;
        }
    }
}

// Updates EEPROM data
void update_users(void) {
    char users[30];

    format_users(users);

    eeprom_blocking_write((uint16_t)0x00, users, 30);
}

// Formats user_list structure into a single array to write to EEPROM
void format_users(char users[]) {

    int i;
    int j;
    int c;

    c = 0;

    for (i = 0; i < 30; i += 6) {
        users[i] = user_list[c].fingerprint_id;
        for (j = 1; j < 5; j++) {
            users[i + j] = user_list[c].passcode[j - 1];
        }
        c += 1;
    }

    return;
}

// Converts array data into user_list structure
void unformat_users(char users[]) { //writes into global struct
    int i;
    int j;
    int c = 0;

    int k, l;

    for (i = 0; i < 30; i += 6) {
        user_list[c].fingerprint_id = users[i];
        for (j = 1; j < 5; j++) {
            user_list[c].passcode[j - 1] = users[i + j];
        }
        c += 1;
    }
}

// Read data from EEPROM
void read_users(void) {

    //read_scores

    char users[30];

    eeprom_read((uint16_t)0x00, users, 30);

    unformat_users(users);

    return;
}

int eeprom(void)
{

    //to write, call
    //eeprom_blocking_write(uint16_t loc, const char* data, uint8_t len)
    //loc = 0x32 location must be divisible by 32

    init_i2c(); // initiate i2c

    reset_users(); // set user_list structure to some for loop based logic value

    update_users(); // write user_list structure to EEPROM

    zero_users(); // clear user_list structure

    read_users(); // read back information from EEPROM and update user_list structure

    return(0);
}

			
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

int main2(void)
{

    //to write, call
    //eeprom_blocking_write(uint16_t loc, const char* data, uint8_t len)
    //loc = 0x32 location must be divisible by 32

    init_i2c(); // initiate i2c

    reset_users(); // set user_list structure to some for loop based logic value

    update_users(); // write user_list structure to EEPROM

    zero_users(); // clear user_list structure

    read_users(); // read back information from EEPROM and update user_list structure

    return(0);
}

int main(void)
{

    // PC12 HM19:RXD STM32:TX
    // PD2  HM19:TXD STM32:RX

    // F0:5E:CD:E2:21:B4

    init_usart5();

    char *c = "batman";
    // AT+ADDR? check mac addr, useful for pi but pretty useless without connection
    // AT+CONNL, try to connect to last succeeded device
    // AT+RENEW, factory reset

    int f;
    f = 0;

    for (;;) {
        while(!(USART5->ISR & USART_ISR_TXE)) { }
        USART5->TDR = c[f];
        f++;

        if (c[f] == 0) {
            break;
        }
    }

}
