#include "pdk/device.h"
#include "auto_sysclock.h"
#include "delay.h"

#include <stdint.h>
#include <stdlib.h>


/****************************************************
 *                                                  *
 *  board-specific and software-specific settings   *
 *                                                  *
 ****************************************************/

#define DEBUG                       0

#define SHORT_BUTTON_PRESS_MS       250
#define SLOWING_CYCLE_ITERATIONS    40
#define BLINKING_ITERATIONS         6
#define HOLD_VALUE_DURATION_MS      10000

#define BOARD_REV 2

#if BOARD_REV == 1
    #define LED_PIN_D1  6
    #define LED_PIN_D2  5
    #define LED_PIN_D3  0
    #define LED_PIN_D4  3
    #define LED_PIN_D5  1
    #define LED_PIN_D6  7
    #define LED_PIN_D7  2
    #define BTN_PIN     4

#elif BOARD_REV == 2
    #define LED_PIN_D1  7
    #define LED_PIN_D2  6
    #define LED_PIN_D3  1
    #define LED_PIN_D4  3
    #define LED_PIN_D5  0
    #define LED_PIN_D6  5
    #define LED_PIN_D7  2
    #define BTN_PIN     4
#endif

/****************************************************/


#define BTN_CTRL  PBC
#define BTN_PULL  PBPH
#define BTN_DATA  PB

#define LED_CTRL PBC
#define LED_DATA PB
#define LED_MASK 0xef;

#define LED_VALUE(pin) (1<<pin)
#define BUTTON_RELEASED (BTN_DATA & (1<<BTN_PIN))
#define BUTTON_PRESSED (!BUTTON_RELEASED)

#define UINT8_TO_DIE_VALUE(val) (val % 6)
// #define UINT8_TO_DIE_VALUE(val) (3U * val / 128U)


uint8_t xorshift8(void);
void main(void);
void initializeHardware(void);
void sleep();
uint16_t quickCycle();
uint16_t slowingCycle();
uint16_t blink();
uint16_t holdValue();
void debug();
unsigned char __sdcc_external_startup(void);


uint8_t led_values[] = {
    LED_VALUE(LED_PIN_D4), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D4) | LED_VALUE(LED_PIN_D7),
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D4) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D3) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D6) | LED_VALUE(LED_PIN_D7)
};


uint8_t x8 = 1;

uint8_t xorshift8() {
    
    uint8_t last_x8 = UINT8_TO_DIE_VALUE(x8);

    while (UINT8_TO_DIE_VALUE(x8) == last_x8) {
        x8 ^= x8 << 5;
        x8 ^= x8 >> 3;
        x8 ^= x8 << 6;
    }

    return x8;
}


void main() {

    initializeHardware();
        
    #if DEBUG != 0

        while(1)
        {
            debug();
        }

    #else

        while(1)
        {
            sleep();

            // Sleep, if short button press
            if (quickCycle() <= SHORT_BUTTON_PRESS_MS) {
                continue;
            }

            if (slowingCycle()) {
                continue;
            }

            if (blink()) {
                continue;
            }

            if (holdValue()) {
                continue;
            }
        }

    #endif
}


void initializeHardware() {

    LED_CTRL |= 0xef;
    BTN_CTRL &= ~(1<<BTN_PIN);
    BTN_PULL |= (1<<BTN_PIN);

    PAPH = 0xff;
    PBPH = 0xff;
}


void sleep() {
    
    while (BUTTON_RELEASED) {
        LED_DATA = 0;
        __stopsys();
    }
}


uint16_t quickCycle() {

    uint16_t counter = 0;

    while (BUTTON_PRESSED) {
        LED_DATA = led_values[UINT8_TO_DIE_VALUE(xorshift8())];
        counter++;
        _delay_ms(1);
    }

    return counter;
}


uint16_t slowingCycle() {

    uint16_t counter = 0;

    while (counter < SLOWING_CYCLE_ITERATIONS) {

        counter++;
        LED_DATA = led_values[UINT8_TO_DIE_VALUE(xorshift8())];

        for (uint16_t i = 0; i < counter * counter / 10; i++) {
            _delay_ms(2);

            // Reset to Quick Cycle, if button is pressed
            if (BUTTON_PRESSED) {
                return 1;
            }   
        }
    }

    return 0;
}


uint16_t blink() {

    uint16_t counter = 0;

    while (counter < 2 * BLINKING_ITERATIONS) {
        
        LED_DATA = (counter % 2) * led_values[UINT8_TO_DIE_VALUE(x8)];
        counter++;
        
        for (uint16_t i = 0; i < 100; i++) {
            
            _delay_ms(1);
            
            // Reset to Quick Cycle, if button is pressed
            if (BUTTON_PRESSED) {
                return 1;
            }
        }
    }

    return 0;
}


uint16_t holdValue() {

    uint16_t counter = 0;

    while (counter < HOLD_VALUE_DURATION_MS) {

        counter++;
        _delay_ms(1);

        // Reset to Quick Cycle, if button is pressed
        if (BUTTON_PRESSED) {
            return 1;
        }
    }

    return 0;
}


void debug() {

    // Test die numbers in ascending order
    for (uint16_t i = 0; i < 6; i++) {
        
        LED_DATA = led_values[i];
        _delay_ms(1000);
    }


    // Test individual LEDs in ascending order (by name)
    LED_DATA = LED_VALUE(LED_PIN_D1);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D2);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D3);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D4);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D5);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D6);
    _delay_ms(1000);
    LED_DATA = LED_VALUE(LED_PIN_D7);
    _delay_ms(1000);


    // Test button functionality by activating all LEDs, if button is pressed
    uint16_t counter = 0;
    while(counter < 10000) {
        LED_DATA = (0xFF ^ LED_VALUE(BTN_PIN)) * BUTTON_PRESSED;
        _delay_ms(1);
        counter++;
    }
}


unsigned char __sdcc_external_startup(void)
{
    AUTO_INIT_SYSCLOCK();
    AUTO_CALIBRATE_SYSCLOCK(TARGET_VDD_MV);
    return 0;
}
