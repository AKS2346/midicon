# midicon
Midi controller in C for Raspberry Pi Pico

# CD4051 MUX
Using a simple MUX chip to attach 6 pots to a single ADC

# TinyUSB
Using example code from TinyUSB to create a midi-device seen by PC.  Learned a lot from https://github.com/infovore/pico-example-midi

# using ssd1306 OLED
Trying to use one and have played around with a few SSD1306 Libraries in C from github. Notably https://github.com/mytechnotalent/c_pico_ssd1306_driver  and https://github.com/daschr/pico-ssd1306 with the second one being more fruitful. 

However, I run into a problem whenever I am trying to use these SSD1306 libraries and it is that when they are running often the MIDI device fails and detachs from the PC.  So doing without that right now.   If you have an idea on this, please let me know what's happening.  

One thing I tried to do is run TUD_TASK() often enough to maintain MIDI contact with the computer; perhaps now I am running it too often, but it is working. 

# Making controls work to send messages only on changes
I would like not to flood my midi device with constant midi changes messages.  After some experimentation, I am now sampling the adc up to a 1000 time and averaging the result before setting the midi-out value.  Then check if midi-out value changed from previous value and if so, sending it.  That makes it send only when I have changed the pot.  But in practice there is still a delay and not too smooth.  1000 times may be too much and still playing with that. 
