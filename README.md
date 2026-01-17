# Arduino-Step-Sequencer
64 step arduino midi sequencer

Sketch for a 64 step midi sequencer with ssd1306 oled display.  

Requires ATmega32U4  

For now uses MidiUSB library to send midi over usb.  

Can read external cv clock for synch on pin 7 if available.  

Up to 8 banks of 8 steps each.  

Steps selectable with buttons.  

Note selection with potentiometer.  


TODO:  

Add potentiometer for octave editing.  

Add logic to dynamically add banks to sequence.  

Add potentiometer for velocity editing.  

Add functionality to save sequence in eeprom.  

Add logic for using a 5pin midi socket connection.  

Add logic to send CV gate, pitch and velocity signals.  

