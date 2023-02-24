#include <keypad_lcd.h>
#include "stm32f0xx.h"
#include <string.h> // for memmove()
#include <stdlib.h> // for srandom() and random()

const char font[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, // 32: space
    0x86, // 33: exclamation
    0x22, // 34: double quote
    0x76, // 35: octothorpe
    0x00, // dollar
    0x00, // percent
    0x00, // ampersand
    0x20, // 39: single quote
    0x39, // 40: open paren
    0x0f, // 41: close paren
    0x49, // 42: asterisk
    0x00, // plus
    0x10, // 44: comma
    0x40, // 45: minus
    0x80, // 46: period
    0x00, // slash
    // digits
    0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x67,
    // seven unknown
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    // Uppercase
    0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71, 0x6f, 0x76, 0x30, 0x1e, 0x00, 0x38, 0x00,
    0x37, 0x3f, 0x73, 0x7b, 0x31, 0x6d, 0x78, 0x3e, 0x00, 0x00, 0x00, 0x6e, 0x00,
    0x39, // 91: open square bracket
    0x00, // backslash
    0x0f, // 93: close square bracket
    0x00, // circumflex
    0x08, // 95: underscore
    0x20, // 96: backquote
    // Lowercase
    0x5f, 0x7c, 0x58, 0x5e, 0x79, 0x71, 0x6f, 0x74, 0x10, 0x0e, 0x00, 0x30, 0x00,
    0x54, 0x5c, 0x73, 0x7b, 0x50, 0x6d, 0x78, 0x1c, 0x00, 0x00, 0x00, 0x6e, 0x00
};

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

void set_digit_segments(int digit, char val) {
    msg[digit] = (digit << 8) | val;
}

void print(const char str[])
{
    const char *p = str;
    for(int i=0; i<8; i++) {
        if (*p == '\0') {
            msg[i] = (i<<8);
        } else {
            msg[i] = (i<<8) | font[*p & 0x7f] | (*p & 0x80);
            p++;
        }
    }
}

void printfloat(float f)
{
    char buf[10];
    //snprintf(buf, 10, "%f", f);
    for(int i=1; i<10; i++) {
        if (buf[i] == '.') {
            // Combine the decimal point into the previous digit.
            buf[i-1] |= 0x80;
            memcpy(&buf[i], &buf[i+1], 10-i-1);
        }
    }
    print(buf);
}

void append_segments(char val) {
    for (int i = 0; i < 7; i++) {
        set_digit_segments(i, msg[i+1] & 0xff);
    }
    set_digit_segments(7, val);
}

void clear_display(void) {
    for (int i = 0; i < 8; i++) {
        msg[i] = msg[i] & 0xff00;
    }
}

// 16 history bytes.  Each byte represents the last 8 samples of a button.
uint8_t hist[16];
char queue[2];  // A two-entry queue of button press/release events.
int qin;        // Which queue entry is next for input
int qout;       // Which queue entry is next for output

const char keymap[] = "DCBA#9630852*741";

void push_queue(int n) {
    queue[qin] = n;
    qin ^= 1;
}

char pop_queue() {
    char tmp = queue[qout];
    queue[qout] = 0;
    qout ^= 1;
    return tmp;
}

void update_history(int c, int rows)
{
    // We used to make students do this in assembly language.
    for(int i = 0; i < 4; i++) {
        hist[4*c+i] = (hist[4*c+i]<<1) + ((rows>>i)&1);
        if (hist[4*c+i] == 0x01)
            push_queue(0x80 | keymap[4*c+i]);
        if (hist[4*c+i] == 0xfe)
            push_queue(keymap[4*c+i]);
    }
}

void drive_column(int c)
{
    GPIOC->BSRR = 0xf00000 | ~(1 << (c + 4));
}

int read_rows()
{
    return (~GPIOC->IDR) & 0xf;
}

char get_key_event(void) {
    for(;;) {
        asm volatile ("wfi" : :);   // wait for an interrupt
        if (queue[qout] != 0)
            break;
    }
    return pop_queue();
}

char get_keypress() {
    char event;
    for(;;) {
        // Wait for every button event...
        event = get_key_event();
        // ...but ignore if it's a release.
        if (event & 0x80)
            break;
    }
    return event & 0x7f;
}

void show_keys(void)
{
    char buf[] = "        ";
    for(;;) {
        char event = get_key_event();
        memmove(buf, &buf[1], 7);
        buf[7] = event;
        print(buf);
    }
}

// Turn on the dot of the rightmost display element.
void dot()
{
    msg[7] |= 0x80;
}

extern uint16_t display[34];
void spi1_dma_display1(const char *str)
{
    for(int i=0; i<16; i++) {
        if (str[i])
            display[i+1] = 0x200 + str[i];
        else {
            // End of string.  Pad with spaces.
            for(int j=i; j<16; j++)
                display[j+1] = 0x200 + ' ';
            break;
        }
    }
}

void spi1_dma_display2(char *str)
{
    for(int i=0; i<16; i++) {
        if (str[i])
            display[i+18] = 0x200 + str[i];
        else {
            // End of string.  Pad with spaces.
            for(int j=i; j<16; j++)
                display[j+18] = 0x200 + ' ';
            break;
        }
    }
}

int score = 0;
char disp1[17] = "                ";
char disp2[17] = "                ";
volatile int pos = 0;
void TIM17_IRQHandler(void)
{
    TIM17->SR &= ~TIM_SR_UIF;
    memmove(disp1, &disp1[1], 16);
    memmove(disp2, &disp2[1], 16);
    if (pos == 0) {
        if (disp1[0] != ' ')
            score += 1;
        if (disp2[0] != ' ')
            score -= 1;
        disp1[0] = '>';
    } else {
        if (disp2[0] != ' ')
            score += 1;
        if (disp1[0] != ' ')
            score -= 1;
        disp2[0] = '>';
    }
    int create = random() & 3;
    if (create == 0) { // one in four chance
        int line = random() & 1;
        if (line == 0) { // pick a line
            disp1[15] = 'x';
            disp2[15] = ' ';
        } else {
            disp1[15] = ' ';
            disp2[15] = 'x';
        }
    } else {
        disp1[15] = ' ';
        disp2[15] = ' ';
    }
    if (pos == 0)
        disp1[0] = '>';
    else
        disp2[0] = '>';
    if (score >= 100) {
        print("Score100");
        spi1_dma_display1("Game over");
        spi1_dma_display2("You win");
        NVIC->ICER[0] = 1<<TIM17_IRQn;
        return;
    }
    char buf[9];
    // snprintf(buf, 9, "Score% 3d", score);
    print(buf);
    spi1_dma_display1(disp1);
    spi1_dma_display2(disp2);
    TIM17->ARR = 250 - 1 - 2*score;
}

void init_tim17(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM17EN;
    TIM17->PSC = 48000 - 1;
    TIM17->ARR = 250 - 1;
    TIM17->CR1 |= TIM_CR1_ARPE;
    TIM17->DIER |= TIM_DIER_UIE;
    TIM17->CR1 |= TIM_CR1_CEN;
}

/*
void game(void)
{
    init_spi2();
    spi2_setup_dma();
    spi2_enable_dma();
    spi1_dma_display1("Type in password");
    spi1_dma_display2("                ");
    init_spi1();
    spi1_init_oled();
    spi1_setup_dma();
    spi1_enable_dma();
    //init_tim17(); // start timer
    get_keypress(); // Wait for key to start
    //spi1_dma_display1("Type in password");
    //spi1_dma_display2("                ");
    // Use the timer counter as random seed...
    //srandom(TIM17->CNT);
    // Then enable interrupt...
    // NVIC->ISER[0] = 1<<TIM17_IRQn;
    char buffer[32] = {0};
    int i = 0;
    for(;;) {
        char key = get_keypress();
        buffer[i] = key;
        i += 1;
        if (key == 'A'){
        spi1_dma_display1("Your password is ...");
        spi1_dma_display2(&buffer);
        //asm("cpsie i");
        }
    }
}
*/

#include "stm32f0xx.h"

// Be sure to change this to your login...
const char login[] = "mmanzhos";

void set_char_msg(int, char);
void nano_wait(unsigned int);


//===========================================================================
// Configure GPIOC
//===========================================================================
void enable_ports(void) {
    // Only enable port C for the keypad
    RCC->AHBENR |= RCC_AHBENR_GPIOCEN;
    GPIOC->MODER &= ~0xffff;
    GPIOC->MODER |= 0x55 << (4*2);
    GPIOC->OTYPER &= ~0xff;
    GPIOC->OTYPER |= 0xf0;
    GPIOC->PUPDR &= ~0xff;
    GPIOC->PUPDR |= 0x55;
}

uint8_t col; // the column being scanned

void drive_column(int);   // energize one of the column outputs
int  read_rows();         // read the four row inputs
void update_history(int col, int rows); // record the buttons of the driven column
char get_key_event(void); // wait for a button event (press or release)
char get_keypress(void);  // wait for only a button press event.
float getfloat(void);     // read a floating-point number from keypad
void show_keys(void);     // demonstrate get_key_event()

//===========================================================================
// Configure timer 7 to invoke the update interrupt at 1kHz
// Copy from lab 8 or 9.
//===========================================================================
void init_tim7(void) {
    // 1. Enable the RCC clock for TIM7.
    RCC -> APB1ENR |= RCC_APB1ENR_TIM7EN;

    // 2. Set the Prescaler and Auto-Reload Register to trigger a DMA request at a rate of 1 kHz (1000 per second).
    TIM7 -> PSC = 24000 - 1;
    TIM7 -> ARR = 2 - 1;

    // 3. Enable the UDE bit in the DIER.
    TIM7 -> DIER |= TIM_DIER_UIE;

    // 4. Set the CEN bit in TIM7_CR1
    // Enables Timer 15 to start counting by setting the CEN bit.
    TIM7 -> CR1 |= TIM_CR1_CEN;

    NVIC -> ISER[0] |= 1 << 18;
}

//===========================================================================
// Copy the Timer 7 ISR from lab 9
//===========================================================================
void TIM7_IRQHandler() {
  // 1. Acknowledge the Interrupt
  // To acknowledge a basic timer interrupt, you must write a zero to the UIF bit
  // of the status register (SR) of the timer.
  TIM7 -> SR = ~TIM_SR_UIF;

  int rows = read_rows();
  /*
  In the middle of the code is a call to update_history() which keeps track of the
  past eight samples of each button. If any button’s history has the bit pattern
  00000001, it means that the button has just now been recognized as pressed for the
  first time. If any button’s history has the bit pattern 11111110, it means that
  the button has just now been recognized as released for the first time.
  */

  update_history(col, rows);
  col = (col + 1) & 3;
  drive_column(col);
}




//===========================================================================
// 4.1 Bit Bang SPI LED Array
// The SPI interface is simple enough that it can be driven by setting individual
// GPIO pins high and low — a process known as bit-banging. This is a common method
// for using SPI with most microcontrollers because no specialized hardware is needed to do so.

// For our first implementation, we will bit bang the SPI protocol for the shift registers
// connected to the 7 segment LED array.

//===========================================================================
int msg_index = 0;
uint16_t msg[8] = { 0x0000,0x0100,0x0200,0x0300,0x0400,0x0500,0x0600,0x0700 };

//===========================================================================
// Configure PB12 (NSS), PB13 (SCK), and PB15 (MOSI) for outputs
//===========================================================================
void setup_bb(void) {

   // Configures GPIO Port B for bit-banging the 7 segment LED displays.
   // To do so, set pins PB12 (NSS), PB13 (SCK), and PB15 (MOSI) for general purpose output
   // (not an alternate function).

    // Enable GPIOB bit in the AHBENR register.
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN;

    // Set pins PB12 (NSS), PB13 (SCK), and PB15 (MOSI) for general purpose output
    // First we clear their MODER bits before setting them
    GPIOB->MODER &= ~(0xCF000000);

    // Then we set them to General Purpose Output mode (bits 01)
    GPIOB->MODER |= 0x45000000;

    // Initialize the ODR so that NSS (PB12) is high and SCK (PB13) is low.
    // It does not matter what MOSI is set to.

    // For each ‘1’ bit written to the lower 16 bits of BSRR, turn ON the
    // corresponding ODR bit and "1" bit written to the upper 16 bits, turn OFF the
    // corresponding ODR bit

    // Binary: 100000000000000001000000000000
    GPIOB->BSRR |= (1 << 12 | 1 << (16 + 13));

}

void small_delay(void) {
    // Once the circuitry and software is working you can comment out the nano_wait() call in small delay.
    nano_wait(50000);
}

//===========================================================================
// Set the MOSI bit, then set the clock high and low.
// Pause between doing these steps with small_delay().
//===========================================================================
void bb_write_bit(int val) {
    // val: either 0 or non-zero
    // NSS (PB12)
    // SCK (PB13)
    // MOSI (PB15)

    // Set the MOSI pin to low if the value of the parameter is zero.
    if (val == 0) {
        GPIOB->BSRR |= (1 << (16 + 15));
    } else {  // Otherwise, set it to high.
        GPIOB->BSRR |= (1 << 15);
    }

    small_delay();

    // Set the SCK pin to high.
    GPIOB->BSRR |= (1 << 13);

    small_delay();

    // Set the SCK pin to low.
    GPIOB->BRR |= (1 << 13);

    // For each ‘1’ bit written to the lower 16 bits of BRR, turn OFF the
    // corresponding ODR bit.
}

//===========================================================================
// Set NSS (PB12) low,
// write 16 bits using bb_write_bit,
// then set NSS high.
//===========================================================================
void bb_write_halfword(int halfword) {

// 1. Deassert the NSS pin
GPIOB -> BRR |= (1 << 12);

// 2. Call bb_write_bit() with bit 15 through 0
// Use the & operator to AND the result with a 1 to isolate one bit.
for (int i = 15; i >= 0; i--)
    bb_write_bit((halfword >> i) & 1);

//5. Assert the NSS pin
GPIOB->BSRR |= (1 << 12);
}

//===========================================================================
// Continually bitbang the msg[] array.
//===========================================================================
void drive_bb(void) {
    for(;;)
        for(int d=0; d<8; d++) {
            bb_write_halfword(msg[d]);
            nano_wait(1000000); // wait 1 ms between digits
        }
}




//============================================================================
// 4.2 Using the hardware SPI channel
// The STM32 has two SPI channels that do the work of the subroutines you just wrote
// in hardware instead of software. Implement the following subroutines to
// initialize and use the SPI2 interface.

// In labs 8 and 9, you used DMA to copy 16-bit words into an 11-bit GPIO ODR to drive the 7-segment displays.
// Now you’ve moved the 7-segment driver to a synchronous serial interface.
// Since there is a built-in peripheral that serializes bits, you can now do the same thing, with the same software.
//
// The only things you had to change were the initialization code to set up the SPI peripheral
// instead of parallel GPIO lines, and the output target of the DMA channel.
//============================================================================

//============================================================================
// setup_dma()
// Set up a circular DMA transfer to the SPI2->DR.
// The trigger will be triggered whenever TIM15 has an update event. As soon as
// the timer reaches the ARR value, the DMA channel will be triggered to write
// a new value into the data register.

// Write to SPI2->DR instead of GPIOB->ODR.
//============================================================================
void setup_dma(void) {
    // 1. Enable the RCC clock to the DMA controller (Family Reference 7.4.6)
    RCC->AHBENR |= RCC_AHBENR_DMAEN;

    // 2. Turn off the enable bit for channel 5.
    DMA1_Channel5 -> CCR &= ~DMA_CCR_EN;

    // 3. Set CPAR to the address of the SPI2->DR register.
    DMA1_Channel5 -> CPAR = (uint32_t) (&(SPI2 -> DR));

    // 4. Set CMAR to the msg array base address
    DMA1_Channel5 -> CMAR = (uint32_t) msg;

    // 5. Set CNDTR to 8
    DMA1_Channel5 -> CNDTR = 8;

    // 6. Set the DIRection for copying from-memory-to-peripheral.
    DMA1_Channel5 -> CCR |= DMA_CCR_DIR;

    // 7. Set the MINC to increment the CMAR for every transfer.
    DMA1_Channel5 -> CCR |= DMA_CCR_MINC;

    // 7.1 Clear the PINC bit to not increment the CPAR after each copy
    DMA1_Channel5 -> CCR &= ~DMA_CCR_PINC;

    // 8. Set the memory datum size to 16-bit.
    DMA1_Channel5 -> CCR |= DMA_CCR_MSIZE_0;

    // 9. Set the peripheral datum size to 16-bit.
    DMA1_Channel5 -> CCR |= DMA_CCR_PSIZE_0;

    // 10. Set the channel for CIRCular operation.
    DMA1_Channel5 -> CCR |= DMA_CCR_CIRC;
}


//============================================================================
// enable_dma()
// Copy this from lab 8 or lab 9.
//============================================================================
void enable_dma(void) {
    DMA1_Channel5 -> CCR |= DMA_CCR_EN;
}

//============================================================================
// Configure Timer 15 for an update rate of 1 kHz.
// Trigger the DMA channel on each update.
// Copy this from lab 8 or lab 9.
//============================================================================
void init_tim15(void) {
    // Enable TIM15’s clock in RCC and trigger a DMA request at a rate of 1 kHz.
    // Do that by setting the UDE bit in the DIER. Do not set the UIE bit in the DIER this time.
    RCC -> APB2ENR |= RCC_APB2ENR_TIM15EN;

    // 2. Set the Prescaler and Auto-Reload Register to trigger a DMA request at a rate of 1 kHz (1000 per second).
    TIM15 -> PSC = 24000 - 1;
    TIM15 -> ARR = 2 - 1;

    // 3. Enable the UDE bit in the DIER.
    TIM15 -> DIER |= TIM_DIER_UDE;

    // 4. Set the CEN bit in TIM7_CR1
    // Enables Timer 15 to start counting by setting the CEN bit.
    TIM15 -> CR1 |= TIM_CR1_CEN;

}


//===========================================================================
// Initialize the SPI2 peripheral.
//===========================================================================
void init_spi2(void) {
    // Enable the RCC clock for GPIO Port B. (remember to enable the GPIOB in RCC).
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN;

    // Connect NSS, SCK, and MOSI signals to pins PB12, PB13, and PB15, respectively.
    // Set these pins to use the alternate functions to do this!

    // First we clear their MODER bits before setting them
    GPIOB->MODER &= ~(0x0CF000000);

    // Then we set them to Alternate Function mode (bits 10)
    GPIOB->MODER |= 0x08A000000;

    // Connect NSS, SCK, and MOSI signals to pins PB12, PB13, and PB15, respectively.
    // by assigning them to AF0 = (0000) (TABLE 14)

    // Clears and sets the bits for PB12, PB13, and PB15 (they are on AFR[1], 9.4.10)
    GPIOB->AFR[1] &= ~(0xF0FF0000);

    /*
    The alternate function register is actually an array of two registers
    (GPIOA->AFR[0] and GPIOA->AFR[1]).

    AFR[0] -> AFRL (alternate function low register)  (PB(0-7) are on AFR[0])
    AFR[1] -> AFRH (alternate function high register) (PB(8-15) are on AFR[1])
    */

    /*
    Then, configure the SPI2 channel as follows:
    */

    // 0. Enable the clock for SPI2
    RCC -> APB1ENR |= RCC_APB1ENR_SPI2EN;

    // 1. Ensure that the CR1_SPE bit is clear
    // Many of the bits set in the control registers require that the SPI channel is not enabled
    SPI2 -> CR1 &= ~(SPI_CR1_SPE);

    // 2. Set the baud rate as low as possible (maximum divisor for baud rate = 256)
    // when baud rate is ‘111’, fSCK = 48 MHz / 256 = ~ 187 kHz (slowest the STM32 can clock SPI)
    SPI2 -> CR1 |= SPI_CR1_BR;

    // 3. Configure the interface for a 16-bit word size.
    SPI2 -> CR2 = SPI_CR2_DS; // 1111 -> 16-bit word size

    // 4. Configure the SPI channel to be in “master mode”
    SPI2 -> CR1 |= SPI_CR1_MSTR;

    // 5. Set the SS Output enable bit and enable NSSP
    SPI2 -> CR2 |= (SPI_CR2_SSOE | SPI_CR2_NSSP);

    // 6. Set the TXDMAEN bit (Tx buffer DMA enable) to enable DMA transfers on transmit buffer empty
    SPI2 -> CR2 |= SPI_CR2_TXDMAEN;

    // 7. Enable the SPI channel
    SPI2 -> CR1 |= SPI_CR1_SPE;

}


//============================================================================
// 4.3 Trigger DMA with SPI_TX
// An SPI peripheral can be configured to trigger the DMA channel all by itself.
// No timer is needed! It just so happens that the SPI2 transmitter triggers the
// same DMA channel that Timer 15 triggers. (See the entry in Table 32 for SPI2_TX.)

// You now have a system that can automatically transfer 16-bit chunks from an 8-entry array
// to the LED displays. Instead of needing a timer to trigger the DMA channel, the SPI peripheral
// does that automatically. As soon as one word is transferred, it requests the next word.
// The SPI system can run fast enough that words can be transferred 1500000 times per second.
// That would make the digits change faster than the 74HC138 could settle, and the letters
// would blur together.

// By setting the SPI clock rate as low as possible, 187.5 kHz, it means that each 16-bit word
// is delivered 187500/16 = 11719 times per second. You may notice that this makes the digits
// a little dimmer than they were when driven by the timer.

//============================================================================

//===========================================================================
// Configure the SPI2 peripheral to trigger the DMA channel when the
// transmitter is empty.
//===========================================================================
void spi2_setup_dma(void) {
    setup_dma();
    SPI2->CR2 |= SPI_CR2_TXDMAEN; // Transfer register empty DMA enable
}

//===========================================================================
// Enable the DMA channel.
//===========================================================================
void spi2_enable_dma(void) {
    enable_dma();
}



//===========================================================================
// 4.4 SPI OLED Display
// We will now use the SPI1 peripheral to drive the LCD OLED display.
// This will not interfere with the operation of the SPI2 peripheral.
// You can use them both, simultaneously, for different devices.

// Configures the SPI1 peripheral.
// Configure NSS, SCK, MISO and MOSI signals of SPI1 to pins PA15, PA5, PA6, and PA7, respectively.
//===========================================================================
void init_spi1() {
    // PA5  SPI1_SCK
    // PA6  SPI1_MISO
    // PA7  SPI1_MOSI
    // PA15 SPI1_NSS

    // Enable the RCC clock for GPIO Port A. (remember to enable the GPIOA in RCC).
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN;

    // Connect NSS, SCK, MISO and MOSI signals to pins PA15, PA5, PA6, and PA7, respectively.
    // Set these pins to use the alternate functions to do this!

    // First we clear their MODER bits before setting them
    GPIOA->MODER &= ~(0x0C000FC00);

    // Then we set them to Alternate Function mode (bits 10)
    GPIOA->MODER |= 0x08000A800;


    // Connect NSS, SCK, and MOSI signals to pins PA15, PA5, PA6, and PA7, respectively.
    // by assigning them to AF0 = (0000) (TABLE 14)

    // Clears and sets the bits for PA15, PA5, PA6, and PA7
    GPIOA->AFR[0] &= ~(0xFFF00000); // PA5, PA6, and PA7 (Section 9.4.9)
    GPIOA->AFR[1] &= ~(0xF0000000); // PA15 (Section 9.4.10)

    /*
    The alternate function register is actually an array of two registers
    (GPIOA->AFR[0] and GPIOA->AFR[1]).

    AFR[0] -> AFRL (alternate function low register)  (PA(0-7) are on AFR[0])
    AFR[1] -> AFRH (alternate function high register) (PA(8-15) are on AFR[1])
    */

    /*
    Then, configure the SPI1 channel as follows:
    */

    // 0. Enable the clock for SPI1
    RCC -> APB2ENR |= RCC_APB2ENR_SPI1EN;

    // 1. Ensure that the CR1_SPE bit is clear
    // Many of the bits set in the control registers require that the SPI channel is not enabled
    SPI1 -> CR1 &= ~(SPI_CR1_SPE);

    // 2. Set the baud rate as low as possible (maximum divisor for baud rate = 256)
    // when baud rate is ‘111’, fSCK = 48 MHz / 256 = ~ 187 kHz (slowest the STM32 can clock SPI)
    SPI1 -> CR1 |= SPI_CR1_BR;

    // 3. Configure the interface for a **** NOW 10-bit word size ****
    // To set the DS field to a 10-bit word, it is necessary to write the bit pattern
    // directly without clearing the DS field first. Thereafter, other bits can be ‘OR’ed to CR2.
    SPI1 -> CR2 = (SPI_CR2_DS_3 | SPI_CR2_DS_0); // 1001 -> 10-bit word size

    // 4. Configure the SPI channel to be in “master mode”
    SPI1 -> CR1 |= SPI_CR1_MSTR;

    // 5. Set the SS Output enable bit and enable NSSP
    SPI1 -> CR2 |= (SPI_CR2_SSOE | SPI_CR2_NSSP);

    // 6. Set the TXDMAEN bit (Tx buffer DMA enable) to enable DMA transfers on transmit buffer empty
    // When this bit is set, a DMA request is generated whenever the TXE flag is set.
    SPI1 -> CR2 |= SPI_CR2_TXDMAEN;

    // 7. Enable the SPI channel
    SPI1 -> CR1 |= SPI_CR1_SPE;


}

void spi_cmd(unsigned int data) {
    // Waits until the SPI_SR_TXE bit is set. (Transmit buffer Empty)
    while(!(SPI1->SR & SPI_SR_TXE)) {} // waits for the transmit buffer to be empty

    // Copies the parameter to the SPI_DR.
    SPI1->DR = data;
    // That’s all you need to do to write 10 bits of data to the SPI channel. The hardware does all the rest.
}

void spi_data(unsigned int data) {
    /*
    It accepts a single integer parameter, and does the same thing as spi_cmd(),
    except that it ORs the value 0x200 with the parameter before it copies it to the SPI_DR.
    This will set the RS bit to 1 for the 10-bit word sent.

    For instance, if you call the subroutine with the argument 0x41,
    it should send the 10-bit value 0x241 to the SPI_DR. This will perform a character write to the OLED LCD.
    */
    spi_cmd(data | 0x200);
}

// Performs each operation of the OLED LCD initialization sequence:
void spi1_init_oled() {
    nano_wait(1000000); // use to wait 1 ms for the display to power up and stabilize.
    spi_cmd(0x38);  // set for 8-bit operation
    spi_cmd(0x08);  // turn display off
    spi_cmd(0x01);  // clear display
    nano_wait(2000000); // use to wait 2 ms for the display to clear.
    spi_cmd(0x06);  // set the display to scroll
    spi_cmd(0x02); // move the cursor to the home position
    spi_cmd(0x0c); // turn the display on

// After these initialization steps are complete, the LCD is ready to display characters
// starting in the upper left corner. Data can be sent with a 10-bit SPI transfer where the
// first bit is a 1.

// For instance, the 10-bit word 10 0100 0001 (0x241) would tell the LCD to display the
// character ‘A’ at the current cursor position. In the C programming language, a character
// is treated as an 8-bit integer. In general, any character can be sent to the display
// by adding the character to 0x200 to produce a 16-bit result that can be sent to the
// SPI transmitter.
}


// It accepts a const char * parameter (also known as a string)
void spi1_display1(const char *string) {
    spi_cmd(0x02); // move the cursor to the home position

    // Call data() for each non-NUL character of the string.
    while(*string != '\0') {
        spi_data(*string);
        string++;
    }
}

// It accepts a const char * parameter (also known as a string)
void spi1_display2(const char *string) {
    spi_cmd(0xc0); // move the cursor to the lower row (offset 0x40)

    // Call data() for each non-NUL character of the string.
    while(*string != '\0') {
        spi_data(*string);
        string++;
    }
}


//===========================================================================
// 4.5 Trigger SPI1 DMA
//
// After going through the initialization process for the OLED display, it is able
// to receive new character data and cursor placement commands at the speed of SPI.

// In this step, we will configure SPI1 for DMA. Instead of using the spi1_display1()
// and spi1_display2() subroutines to place the cursor and write characters,
// they will be continually copied from a circular buffer named display[].
// It consists of 34 16-bit entries.
//===========================================================================

/*
Most SPI devices use a 4-, 8-, or 16-bit word size.
The SOC1602A uses a 2+8-bit word size to send two bits of configuration information plus an
8-bit character on each transfer.

The first two bits are, respectively, the register selection and read/write.
Since we will always be writing data to the display and never reading it,
we will always make the second bit a ‘0’.

The register selection determines whether a write is intended as a command or
a character to write to the display. Commands are needed to, for instance, initialize the display,
configure the communication format, clear the screen, move the cursor to a new position on the screen.

When the register select bit is zero, the transmission issues an 8-bit command to the display.
When the register select bit is one, the transmission represents a character to write to the display.
(Remember that each character must be sent with the 0x200 prefix --> this will perform a character write to the OLED LCD)
*/

//===========================================================================
// This is the 34-entry buffer to be copied into SPI1.
// Each element is a 16-bit value that is either character data or a command.
// Element 0 is the command to set the cursor to the first position of line 1. (“home command” for the display)
// The next 16 elements are 16 characters that are copied into the first line of the display.
// Element 17 is the command to set the cursor to the first position of line 2
// followed by 16 characters that are copied into the second line of the display.
//===========================================================================
uint16_t display[34] = {
        0x002, // Command to set the cursor at the first position line 1
        0x200+'E', 0x200+'C', 0x200+'E', 0x200+'3', 0x200+'6', + 0x200+'2', 0x200+' ', 0x200+'i',
        0x200+'s', 0x200+' ', 0x200+'t', 0x200+'h', + 0x200+'e', 0x200+' ', 0x200+' ', 0x200+' ',
        0x0c0, // Command to set the cursor at the first position line 2
        0x200+'c', 0x200+'l', 0x200+'a', 0x200+'s', 0x200+'s', + 0x200+' ', 0x200+'f', 0x200+'o',
        0x200+'r', 0x200+' ', 0x200+'y', 0x200+'o', + 0x200+'u', 0x200+'!', 0x200+' ', 0x200+' ',
};

//===========================================================================
// Configure the proper DMA channel to be triggered by SPI1_TX.
// Set the SPI1 peripheral to trigger a DMA when the transmitter is empty.
//===========================================================================
void spi1_setup_dma(void) {

    // DMA channel triggered by SPI1_TX --> DMA Channel 3 (Table 30)

    // It should circularly copy the 16-bit entries from the display[] array to
    // the SPI1->DR address. You should also configure the SPI1 device to trigger
    // a DMA operation when the transmitter is empty. ***

    // 1. Enable the RCC clock to the DMA controller (Family Reference 7.4.6)
    RCC->AHBENR |= RCC_AHBENR_DMAEN;

    // 2. Turn off the enable bit for channel 3.
    DMA1_Channel3 -> CCR &= ~DMA_CCR_EN;

    // 3. Set CPAR (The peripheral destination of data transfer) to SPI1->DR
    DMA1_Channel3 -> CPAR = (uint32_t) (&(SPI1->DR));

    // 4. Set CMAR (The memory source of data transfer) to the display array.
    DMA1_Channel3 -> CMAR = (uint32_t) display;

    // 5. Set CNDTR (The count of data elements to transfer) to the
    // number of elements in the array
    DMA1_Channel3 -> CNDTR = 34;

    // 6. Set the DIRection for copying from-memory-to-peripheral.
    DMA1_Channel3 -> CCR |= DMA_CCR_DIR;

    // 7. Set the MINC to increment the CMAR after every transfer.
    // The memory address should be incremented after every transfer.
    DMA1_Channel3 -> CCR |= DMA_CCR_MINC;

    // 7.1 Clear the PINC bit to not increment the CPAR after every transfer
    DMA1_Channel3 -> CCR &= ~DMA_CCR_PINC;

    /*
    PSIZE: Peripheral Size (00: 8-bit, 01: 16-bit, 10: 32-bit)
    MSIZE: Memory Size (00: 8-bit, 01: 16-bit, 10: 32-bit)
    */

    // 8. Set the memory datum size to 16-bit. !!!!! (each element in the array is uint16_t)
    DMA1_Channel3 -> CCR |= DMA_CCR_MSIZE_0;

    // 9. Set the peripheral datum size to 16-bit !!!!
    DMA1_Channel3 -> CCR |= DMA_CCR_PSIZE_0;

    // 10. Set the channel for CIRCular operation.
    // The array will be transferred repeatedly, so the channel should be set for circular operation.
    DMA1_Channel3 -> CCR |= DMA_CCR_CIRC;
}
/*

The array is set up, and circular DMA transfer is triggered by SPI1_TX to write it
to the SPI1->DR address. Thereafter, the display is effectively “memory-mapped”.
As characters are written to the array values, they are quickly copied to the display.
This allows you to easily create very complicated patterns and animations on the display.
*/

//===========================================================================
// Enable the DMA channel triggered by SPI1_TX.
//===========================================================================
void spi1_enable_dma(void) {
    // Enable the DMA channel (Channel 3) triggered by SPI1_TX.
    DMA1_Channel3 -> CCR |= DMA_CCR_EN;
}

//===========================================================================
// Main function
//===========================================================================

int main_keypad_lcd(void) {
    msg[0] |= font['E'];
    msg[1] |= font['C'];
    msg[2] |= font['E'];
    msg[3] |= font[' '];
    msg[4] |= font['3'];
    msg[5] |= font['6'];
    msg[6] |= font['2'];
    msg[7] |= font[' '];

    // GPIO enable
    enable_ports();
    // setup keyboard
    init_tim7();

    // Game on!  The goal is to score 100 points.
    game();
    return 0;
}
