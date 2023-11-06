#include "pdk/device.h"
#include "auto_sysclock.h"
#include "delay.h"

#define BTN_CTRL  PBC
#define BTN_PULL  PBPH
#define BTN_DATA  PB
#define BTN_PIN   4

#define LED_CTRL PBC
#define LED_DATA PB
#define LED_MASK 0xef;

uint8_t led_values[] = {0x08, 0x44, 0x40+0x08+0x04, 0x40+0x20+0x04+0x02, 0x40+0x20+0x04+0x02+0x08, 0x40+0x20+0x04+0x02+0x80+0x01};

// Main program
void main() {

    // Initialize hardware
    LED_CTRL |= 0xef;
    BTN_CTRL &= ~(1<<BTN_PIN);
    BTN_PULL |= (1<<BTN_PIN);

    PAPH = 0xff;
    PBPH = 0xff;

    uint32_t counter = 0;

    while(1)
    {
        BEGIN:
        // Wait for button press
        uint32_t last_counter = counter;
        while(BTN_DATA & (1<<BTN_PIN))
        {
            counter++;
            if(counter - last_counter > 1000000)
                goto SLEEP;
        }

        last_counter = counter;

        // Wait for button relase and print some random values
        while(!(BTN_DATA & (1<<BTN_PIN)))
        {
            counter++;
            LED_DATA = led_values[(counter>>4)%6];
        }

        if(counter - last_counter < 200)
        {
            goto SLEEP;
        }
        else
        {
            // Print some more values and get slower and slower
            for(uint16_t i = 0; i < 40; i++)
            {
                counter++;
                LED_DATA = led_values[counter%6];
                for(uint16_t k = 0; k < i*i/10; k++)
                     _delay_ms(2);
                // If button is pressed again, cancel this loop
                if(!(BTN_DATA & (1<<BTN_PIN)))
                    break;
            }
            // Output the current value
            LED_DATA = led_values[counter%6];
        }
        goto BEGIN;

        SLEEP:
        LED_DATA = 0;
        __stopsys();
    }
}

unsigned char __sdcc_external_startup(void)
{
    AUTO_INIT_SYSCLOCK();
    AUTO_CALIBRATE_SYSCLOCK(TARGET_VDD_MV);
    return 0;
}
