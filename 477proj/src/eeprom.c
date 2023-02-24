#include "stm32f0xx.h"
#include <stddef.h>
#include "keypad_lcd.h"
#include <string.h>

#include <eeprom.h>
#define EEPROM_ADDR 0x50

// Structure to hold fingerprint and passcode information
int max_passcode_length = 20;

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
            GPIO_MODER_MODER8 |
            GPIO_MODER_MODER9
    );

    GPIOB->MODER |= (
            GPIO_MODER_MODER8_1 |
            GPIO_MODER_MODER9_1
    );

    GPIOB->AFR[1] &= ~(
            GPIO_AFRH_AFR8 |
            GPIO_AFRH_AFR9
    );

    GPIOB->AFR[1] |= (
            (1 << (4 * 0)) |
            (1 << (4 * 1))
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

    // loc >> 8 == 02
    // tmp[0] == 0x02 & 0x0f == 0x02
    // loc == 0x0234
    // tmp[1] == 0x0234 & 0xff == 0x34

    char tmp[34];
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
    TIM7->CR1 &= ~TIM_CR1_CEN; // Pause keypad scanning.

    char tmp[34];
    tmp[0] = (loc >> 8) & 0x0f;
    tmp[1] = loc & 0xff;

    i2c_senddata(EEPROM_ADDR,tmp,2);
    i2c_recvdata(EEPROM_ADDR,data,len);

    TIM7->CR1 |= TIM_CR1_CEN; // Resume keypad scanning.

    return;
}


void eeprom_blocking_write(uint16_t loc, const char* data, uint8_t len) {
    TIM7->CR1 &= ~TIM_CR1_CEN; // Pause keypad scanning.
    eeprom_write(loc, data, len);
    while (!eeprom_write_complete()); //never finishes
    TIM7->CR1 |= TIM_CR1_CEN; // Resume keypad scanning.
}

// Test function to set user_list of users all to zero to test functionality of i2c read
void zero_users(void) {
    int i;
    int j;

    j = 0;

    while (j < 5) {
        for(i = 0; i < max_passcode_length; i++) {
            user_list[j].passcode[i] = 0;
        }
        j += 1;
    }
}

// Test function to set user_list to some for loop logic based values to test functionality of i2c write
void reset_users(void) {

    int i;
    int j;

    j = 0;
    while (j < 2) { // only 2 passwords
        for(i = 0; i < max_passcode_length; i++) {
            user_list[j].passcode[i] = (30 - i) % 10;
            test_list[j].passcode[i] = (30 - i) % 10;
        }
        j += 1;
    }

    strcpy(user_list[0].passcode, "1234567890ABCD123456");
    strcpy(test_list[0].passcode, "1234567890ABCD123456");

    strcpy(user_list[1].passcode, "11111111111111111111");
    strcpy(test_list[1].passcode, "11111111111111111111");

    strcpy(user_list[2].passcode, "11111111111111111112");
    strcpy(test_list[2].passcode, "11111111111111111112");

    strcpy(user_list[3].passcode, "11111111111111111113");
    strcpy(test_list[3].passcode, "11111111111111111113");
}

// Updates EEPROM data
void update_users(void) {
    int address;
    char passcode[31];

    address = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < max_passcode_length; j++) {
            passcode[j] = user_list[i].passcode[j];
        }
        eeprom_blocking_write((uint16_t)address, passcode, max_passcode_length);
        address += 32;
    }
}

// Read data from EEPROM
void read_users(void) {

    //read_scores
    int address;
    char passcode[32];

    address = 0;

    for (int i = 0; i < 5; i++) {
        eeprom_read((uint16_t)address, passcode, max_passcode_length);
        for (int j = 0; j < max_passcode_length; j++) {
            user_list[i].passcode[j] = passcode[j];
        }
        address += 32;
    }

    return;
}

int main_eeprom(void) // eeprom
{

    //to write, call
    //eeprom_blocking_write(uint16_t loc, const char* data, uint8_t len)
    //loc = 0x32 location must be divisible by 32

    init_i2c(); // initiate i2c

    zero_users(); // clear user_list structure

    reset_users(); // set user_list structure to some for loop based logic value

    update_users(); // write user_list structure to EEPROM

    zero_users(); // clear user_list structure

    read_users(); // read back information from EEPROM and update user_list structure

    for (int i = 0; i < 5; i++) {
        if (strcmp(test_list[i].passcode, user_list[i].passcode) != 0) { // check each passcode
            for (;;) {
                spi1_dma_display2("EEPROM Broken");
            }
        }
    }

    return 0;
}
