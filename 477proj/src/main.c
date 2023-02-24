#include <keypad_lcd.h>
#include "stm32f0xx.h"
#include <stddef.h>
#include <string.h>

#include "main.h"
#include "finger.h"
#include "eeprom.h"
#include "bluetooth.h"
#include "keypad_lcd.h"

/*
States
    0 : Initialization/Sleep Mode (may need to seperate this)
    1 : Keypad/Fingerprint Scanning
    2 : Facial Recognition Scanning
    3 : Reject Passcode
    4 : Accept Passcode
    5 : Unlocked
*/

int state;
int password_limit = 20;

int main(void) {

    // System Initialization
    state = 0;
    // read_eeprom()
    main_eeprom(); // used to initialize default passwords, switch to just read command when finished testing

    // LCD/Keypad initialization
    enable_ports(); // enable portc
    init_tim7(); // timer for keypad input
    init_spi2();
    spi2_setup_dma();
    spi2_enable_dma();
    spi1_dma_display1("Initialization");
    spi1_dma_display2("                ");
    init_spi1();
    spi1_init_oled();
    spi1_setup_dma();
    spi1_enable_dma();

    // Init Fingerprint
    init_usart1();


    init_usart5();
    main_bluetooth(); // Init Bluetooth Uart

    spi1_dma_display1("Fin Init");

    // connect to bluetooth

    while (get_keypress() != '#'); // keypress to wake up

    int uid;
    int rp_resp;

    for (;;) {
        if ((uid = keypad_fingerprint()) != -1) {
            if ((rp_resp = facial_recognition(uid))) {
                if (unlock(uid)) {
                    /*
                        ask if want to change passcode :
                            change_passcode()
                    */
                }
            }
        }

        // press # to continue
    }

    return 0;
}

int keypad_fingerprint(void) {

    // Keypad Scanning
    state = 1;
    spi1_dma_display1("Type in password");
    spi1_dma_display2("                ");

    char password[20]; // user input password
    char compare[20]; // data structure because strcpy doesn't work with structs or something
    int i = 0;
    char key;

    for (;;) { // continually read password until max digits have been read, or the user presses the '#' key
        key = get_keypress();
        if (key == '#') {
            break;
        }
        password[i] = key;
        i += 1;
        spi1_dma_display1("Type # to finish");

        if (i == password_limit) {
            break;
        }
    }

    // Used for debugging
    /*
    spi1_dma_display2(&password);
    */

    read_users(); // eeprom reading of users

    uint8_t * response = finger_check_user(); // response[3] is the UID

    // Checking for failure responses and printing to the spi display for debugging purposes
    if(response[3] == ACK_NOUSER) {
        spi1_dma_display1("Fp ACK_NOUSER");
        return -1;
    }
    else if (response[3] == ACK_TIMEOUT) {
        spi1_dma_display1("Fp ACK_TIMEOUT");
        return -1;
    }

    int UID = response[3]; // extract uid from fingerprint reader response

    if (UID > 4) { // checking for invalid uid
        spi1_dma_display1("UID Invalid");
        return -1;
    }

    for (i = 0; i < 20; i++) {
        compare[i] = user_list[UID].passcode[i];
    }

    // strcpy(compare, user_list[UID].passcode); // might have to change to for loop for each variable because of the
                                              // the way strcpy works with data structures

    // Compare input and stored password

    for(int i = 0; i < 20; i++) {
        if (compare[i] != password[i]) {
            spi1_dma_display1("KP_EEPROM_MM"); // mismatch
            return -1;
        }
    }

    spi1_dma_display1("Successful KP_FP");

    return UID;
}

int facial_recognition(char UID) {

    spi1_dma_display1("Press #"); // press pound when ready to take picture
    while (get_keypress() != '#'); // keypress to wake up

    // send uid to raspberry pi
    // '\001'

    if (UID == '\001') {
        UID = 0x31;
    }
    UID = 0x31;

    char c[5] = {'u', 's', 'e', 'r', UID};
    char *send = c;

    for (int f = 0; f < 5; f++) {
        while(!(USART5->ISR & USART_ISR_TXE)) { }
        USART5->TDR = send[f];
    }

    char *r;

    // recieve yes/no (1/0) from raspberry pi

    // could change this to recieve only 1 byte of code is probably enough but we will see
    for(int i = 0; i < 8; i++) {
        while(!(USART1->ISR & USART_ISR_RXNE)); // what if timeout
        r[i] = USART1->RDR;
    }

    if (!strcmp(c, "yes")) {
        return 1; // yes recieved meaning that the UID matched up to the user at the door
    }
    else {
        return 0; // no
    }
}

int unlock(char UID) {
    // UID is used for authorized/unauthorized users

    return 1;
}

int lock() {
    return 1;
}
