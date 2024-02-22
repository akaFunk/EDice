#include "pdk/device.h"
#include "auto_sysclock.h"
#include "delay.h"

#include <stdint.h>
#include <stdlib.h>


#define BOARD_REV 2


#define BTN_CTRL  PBC
#define BTN_PULL  PBPH
#define BTN_DATA  PB
#define BTN_PIN   4

#define LED_CTRL PBC
#define LED_DATA PB
#define LED_MASK 0xef;

#if BOARD_REV == 1
    #define LED_PIN_D1 6
    #define LED_PIN_D2 1
    #define LED_PIN_D3 7
    #define LED_PIN_D4 3
    #define LED_PIN_D5 5
    #define LED_PIN_D6 0
    #define LED_PIN_D7 2

#elif BOARD_REV == 2
    #define LED_PIN_D1 7
    #define LED_PIN_D2 6
    #define LED_PIN_D3 1
    #define LED_PIN_D4 3
    #define LED_PIN_D5 0
    #define LED_PIN_D6 5
    #define LED_PIN_D7 2
#endif

#define LED_VALUE(pin) (1<<pin)
#define BUTTON_RELEASED (BTN_DATA & (1<<BTN_PIN))
#define BUTTON_PRESSED (!BUTTON_RELEASED)

#define S_SLEEP         0
#define S_QUICK_CYCLE   1
#define S_SLOWING_CYCLE 2
#define S_HOLD_VALUE    3


uint8_t led_values[] = {
    LED_VALUE(LED_PIN_D4), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D4) | LED_VALUE(LED_PIN_D7),
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D4) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D7), 
    LED_VALUE(LED_PIN_D1) | LED_VALUE(LED_PIN_D2) | LED_VALUE(LED_PIN_D3) | LED_VALUE(LED_PIN_D5) | LED_VALUE(LED_PIN_D6) | LED_VALUE(LED_PIN_D7)
};


uint8_t x8 = 1;
uint8_t last_x8 = 1;

uint8_t xorshift8() {
    
    last_x8 = x8 % 6;

    while (x8 % 6 == last_x8) {
        x8 ^= x8 << 5;
        x8 ^= x8 >> 3;
        x8 ^= x8 << 6;
    }

    return x8;
}

// Main program
void main() {

    // Initialize hardware
    LED_CTRL |= 0xef;
    BTN_CTRL &= ~(1<<BTN_PIN);
    BTN_PULL |= (1<<BTN_PIN);

    PAPH = 0xff;
    PBPH = 0xff;

    uint16_t counter = 0;

    loop:
    while(1)
    {
        // Sleep
        while (BUTTON_RELEASED) {
            LED_DATA = 0;
            __stopsys();
        }


        // Quick Cycle
        counter = 0;

        while (BUTTON_PRESSED) {
            LED_DATA = led_values[xorshift8() % 6];
            counter++;
        }


        // Sleep, if short button press
        if (counter <= 500) {
            continue;
        }


        //Slower Cycle
        counter = 0;
        while (counter < 40) {
            counter++;
            LED_DATA = led_values[xorshift8() % 6];

            for (uint16_t i = 0; i < counter * counter / 10; i++) {
                _delay_ms(2);

                // Reset to Quick Cycle, if button is pressed
                if (BUTTON_PRESSED) {
                    goto loop;
                }   
            }
        }


        /*
        // Hold Value
        counter = 0;
        while (counter < 1000) {
            counter++;
            _delay_ms(1);

            // Reset to Quick Cycle, if button is pressed
            if (BUTTON_PRESSED) {
                goto loop;
            }   
        }
        */


        // Blink a few times
        counter = 0;
        while (counter < 2 * 6) {
            
            LED_DATA = (counter % 2) * led_values[x8 % 6];
            counter++;
            
            for (uint16_t i = 0; i < 100; i++) {
                
                _delay_ms(1);
                
                // Reset to Quick Cycle, if button is pressed
                if (BUTTON_PRESSED) {
                    goto loop;
                }
            }
        }


        // Hold Value
        counter = 0;
        while (counter < 10000) {
            counter++;
            _delay_ms(1);

            // Reset to Quick Cycle, if button is pressed
            if (BUTTON_PRESSED) {
                goto loop;
            }   
        }
    }
}


unsigned char __sdcc_external_startup(void)
{
    AUTO_INIT_SYSCLOCK();
    AUTO_CALIBRATE_SYSCLOCK(TARGET_VDD_MV);
    return 0;
}
