/** 
 * Daschr's Oled example from 2021 github.com/daschr/pico_ssd1306
 * This is midicon the midi controller code I wrote in 2022 and stopped working wiht a new coputer in 2023
 * I am now recompiling and attempting to get it to work after updating the pico-sdk and tinyusb to version 0.51
 * Anil, Nov 1 2023
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "pico/binary_info.h"
#include "bsp/board.h"
#include "tusb.h"

#define S0 11
#define S1 12
#define S2 13
#define NUMBER_OF_POTS 6
#define NUMBER_OF_BUTTONS 2
#define STARTING_CONTROLLER_FOR_POTS 44
#define STARTING_CONTROLLER_FOR_BUTTONS 82


enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
const uint LED_PIN = PICO_DEFAULT_LED_PIN;

 int map(int s, int a1, int a2, int b1, int b2) {
   return b1 + (s - a1) * (b2 - b1) / (a2 - a1);
 }

//Utility to blink led
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

// set up the pots I will use
struct pot
{
	uint8_t mux_address; 
	uint8_t controller_number; 
	uint8_t previous_value; 
	uint8_t value; 
	bool changed; 
}; 

// create  pots
struct pot pots[NUMBER_OF_POTS];

void init_pots(struct pot pots[])
{
	uint8_t mux_address = 0; 
	uint8_t controller_number = STARTING_CONTROLLER_FOR_POTS;
	uint8_t default_value = 63;

	for (int i = 0; i < NUMBER_OF_POTS; i++)
	{
		pots[i].mux_address = mux_address++; 
		pots[i].controller_number = controller_number++; 
		pots[i].value = default_value;
		pots[i].previous_value = default_value; 
		pots[i].changed = false;
	} 

} 

void set_mux_address(uint8_t address)
{
	// Reset the address to zero to make sure; using a CD4051
	// mask shifted to pin 111 using gpio_clr_mask to drive each pin low
	uint32_t mask = 0b111 << S0;
	gpio_clr_mask(mask);

	
	// loop through mux pins
	for (int i = S0; i < S2 + 1; i++)
	{
		// which bit are we looking at? If i = 11, we are looking at lowest-value bit or bit 0
		int bit_location = i - S0;  
		// mask address with 1 shifted bit_location times then shfit back that many times to get the actual bit
		int set_gpio = (address & ( 1 << bit_location)) >> bit_location;

		// put the value to that gpio
		gpio_put(i, set_gpio); 
	} 

} 

int get_pot_value(struct pot* pot)
{
	// Change mux addresses and read value
	set_mux_address(pot->mux_address);

	// store the previous value and set change flag to false
	pot->previous_value = pot->value;
	
	// cycle through and read average of 1000 reads; to reduce the jitter in any one reading
	 int number_of_readings = 1000; 
	 int sum_of_readings = 0;
	 int average_reading = 0; 

	 for (int i = 0; i < number_of_readings ; i++)
	 {
		 sum_of_readings += adc_read();
	 } 

	 average_reading = (int) sum_of_readings / number_of_readings; 
		
	// read the new value and if it is more than 2 unit away from old one, set changed flag to true
	pot->value = map(average_reading, 0, 0xfff, 0, 0xff);
	if (abs(pot->value - pot->previous_value) > 1) pot->changed = true;

	return 0; 
}

int get_pot_values(struct pot* pots)
{ 
	for (int i = 0; i < NUMBER_OF_POTS; i ++ )
	{
		get_pot_value(&pots[i]);
	} 
	return 0; 
} 

struct button 
{
	uint8_t gpio_address; 
	uint8_t value; 
	uint8_t controller_number; 
	bool previous_closed; 
	bool closed; 
	bool changed; 
	bool toggle; // if true make switch act like a toggle 
}; 

// create buttongs
struct button buttons[NUMBER_OF_BUTTONS];

void init_buttons(struct button buttons[]) 
{
	// set up some starting values
	uint8_t	gpio_address = 2; 
	uint8_t controller_number = STARTING_CONTROLLER_FOR_BUTTONS; 

	// Loop through buttons
	for (int i = 0; i < NUMBER_OF_BUTTONS; i ++) 
	{
		buttons[i].gpio_address = gpio_address++; 
		buttons[i].controller_number = controller_number++; 
		buttons[i].previous_closed = true; 
		buttons[i].value = 0; 
		buttons[i].changed = false; 
		buttons[i].toggle = true; 
		buttons[i].closed = false; 

		// initialize the state of each gpio pin for each button
		gpio_init(buttons[i].gpio_address); 
		gpio_set_dir(buttons[i].gpio_address, GPIO_IN); 
		gpio_pull_down(buttons[i].gpio_address);  

	}
}

void get_button_value(struct button* button)
{
	button->previous_closed = button->closed;
	button->closed = gpio_get(button->gpio_address);
	button->changed = (button->closed != button->previous_closed);

	if (button->toggle)
	{
		if (button->closed)
		{
			if (!button->changed) 
			{
				button->value = button->value == 127 ? 0 : 127; 
			} 
		}
	} 
	else
	{
		if (button->closed)
		{
			button->value = (!button->changed) ? 127 : 0; 
		} 
		else
		{ 
			button->value = (!button->changed) ? 0 : button->value; 
		}
	}

}

void get_button_values(struct button* buttons)
{
	for (int i = 0 ; i < NUMBER_OF_BUTTONS; i++)
	{
		get_button_value(&buttons[i]); 
	}
}

// MIDI Stuff

void midi_task(uint8_t midi_value, uint32_t controller_number)
{
  // static uint32_t start_ms = 0;

  uint8_t const cable_num = 0; // MIDI jack associated with USB endpoint
  uint8_t const channel   = 0; // 0 for channel 1

  // The MIDI interface always creates input and output port/jack descriptors
  // regardless of these being used or not. Therefore incoming traffic should be read
  // (possibly just discarded) to avoid the sender blocking in IO
  uint8_t packet[4];
  while ( tud_midi_available() ) tud_midi_packet_read(packet);

  // send midi control value periodically
  // if (board_millis() - start_ms < 30) return; // not enough time
  // start_ms += 30;

  // Send midi_value on for current position controller 15 on channel 1.
  uint8_t controller_change[3] = { 0xB0 | channel, controller_number, midi_value};
  tud_midi_stream_write(cable_num, controller_change, 3);
}

int main() {

	// initialize all the standard requirements - io, board, usb, adc
    stdio_init_all();
	board_init();
	tusb_init();
	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);

	// initalize the I2c for the display module
	// get a display and initialize it

	// set up the mux addressing pins gpios
	gpio_init(S0); 
	gpio_set_dir(S0, GPIO_OUT); 
	gpio_init(S1); 
	gpio_set_dir(S1, GPIO_OUT); 
	gpio_init(S2); 
	gpio_set_dir(S2, GPIO_OUT); 

	// initalize midi surface controls
	init_pots(pots);
	init_buttons(buttons); 

	// forever loop
	while (1)
	{
		// run the usb task required to communicate with computer in usb midi

		get_pot_values(pots); 
		get_button_values(buttons); 
	
		// send the midi value  for the pots
		for (int i = 0; i < NUMBER_OF_POTS; i++)
		{
			// tinyusb device task
			tud_task(); 

			// if the pot has changed, send out a controller change and reset change to false
			if (pots[i].changed)
				{
					midi_task(pots[i].value, pots[i].controller_number);
					pots[i].changed = false;
				}
		}
		//
		// send the midi value  for the pots
		for (int i = 0; i < NUMBER_OF_BUTTONS; i++)
		{
			// tinyusb device task
			tud_task(); 

			// if the pot has changed, send out a controller change and reset change to false
			if (buttons[i].changed)
				{
					midi_task(buttons[i].value, buttons[i].controller_number);
					pots[i].changed = false;
				}
		}

		led_blinking_task();

	}

    return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}
