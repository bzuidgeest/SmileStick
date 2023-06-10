# SmileStick

* Cleanup branch is work in progress to make some nicer code.

This is my repository for my experiments in using a pi pico to control a VTech v.SMILE controller.


Console pinout, using default numbering for ps/2 style female connector. Perspective is from the front of console looking in. Meaning that the
controller connection is flipped.
<pre>
6			5
4			3
	2	1

1 VCC 
2 CTS --> GPIO7
3 Tx (from console) (connect to GPIO4 UART1 TX on pico )
4 GND 
5 Tx (from controller) (connect to GPIO5 UART1 RX on pico )
6 RTS (from controller) has pulse to low when pushing buttons
</pre>
Notes:

* After a while of idling the joysting stops sending data. 
I suspect this had something to do with de 0xBX values the console seems to send every 20 seconds. Its also the reply to the 0x78
at the startup. The 0x78 needs be followed with a 0xB reply before the controller wakes up. I have a trace where a 0xBX is send
every 20 seconds. But how the X in that is determined I have no clue.

* The third byte in the idle reply from the controller matches the code for leds all off. I suspect this can be any valid values for
the leds. So on sending an idle reponse adjust the third by to the correct value for the led state.

* Sometimes need an extra reset after programming?

